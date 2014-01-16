/*
 * descripten - ECMAScript to native compiler
 * Copyright (C) 2011-2014 Christian Kindahl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <cstring>
#include <limits>
#include <set>
#include <sstream>
#include <gc_cpp.h>     // NOTE: 3rd party.
#include "common/cast.hh"
#include "common/exception.hh"
#include "common/lexical.hh"
#include "common/unicode.hh"
#include "parser/parser.hh"
#include "algorithm.hh"
#include "context.hh"
#include "conversion.hh"
#include "date.hh"
#include "debug.hh"
#include "error.hh"
#include "eval.hh"
#include "frame.hh"
#include "global.hh"
#include "messages.hh"
#include "native.hh"
#include "object.hh"
#include "operation.h"
#include "platform.hh"
#include "property.hh"
#include "prototype.hh"
#include "standard.hh"
#include "utility.hh"
#include "unique.hh"

using parser::Lexer;
using parser::ParseException;
using parser::Parser;
using parser::StringStream;

/**
 * Macro for defining own properties referring to built-in functions.
 */
#ifndef ES_DEFINE_PROP_FUN
#define ES_DEFINE_PROP_FUN(name, fun_ptr, fun_len) \
    define_new_own_property(name, EsPropertyDescriptor(false, true, true, EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(), fun_ptr, fun_len))));
#endif

/**
 * Macro for defining own properties referring to objects.
 */
#ifndef ES_DEFINE_PROP_OBJ
#define ES_DEFINE_PROP_OBJ(name, obj) \
    define_new_own_property(name, EsPropertyDescriptor(false, true, true, EsValue::from_obj((obj))));
#endif

EsFunction *EsObject::default_constr_ = NULL;

EsObject::EsObject()
    : prototype_(NULL)
    , extensible_(true)
    , map_(this)
{
}

EsObject::~EsObject()
{
}

void EsObject::make_inst()
{
    prototype_ = es_proto_obj();
    class_ = _USTR("Object");
    extensible_ = true;
}

void EsObject::make_proto()
{
    prototype_ = NULL;
    class_ = _USTR("Object");
    extensible_ = true;

    // 15.2.4
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_obj_proto_to_str, 0);
    ES_DEFINE_PROP_FUN(property_keys.to_locale_string, es_std_obj_proto_to_loc_str, 0);
    ES_DEFINE_PROP_FUN(property_keys.value_of, es_std_obj_proto_val_of, 0);
    ES_DEFINE_PROP_FUN(property_keys.has_own_property, es_std_obj_proto_has_own_prop, 1);
    ES_DEFINE_PROP_FUN(property_keys.is_prototype_of, es_std_obj_proto_is_proto_of, 1);
    ES_DEFINE_PROP_FUN(property_keys.property_is_enumerable, es_std_obj_proto_prop_is_enum, 1);
}

EsObject *EsObject::create_raw()
{
    return new (GC)EsObject();
}

EsObject *EsObject::create_inst()
{
    EsObject *o = new (GC)EsObject();
    o->make_inst();
    return o;
}

EsObject *EsObject::create_inst_with_class(const String &class_name)
{
    EsObject *o = new (GC)EsObject();
    o->make_inst();
    o->class_ = class_name;
    return o;
}

EsObject *EsObject::create_inst_with_prototype(EsObject *prototype)
{
    EsObject *o = new (GC)EsObject();
    o->make_inst();
    o->prototype_ = prototype;
    return o;
}

EsFunction *EsObject::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsObjectConstructor::create_inst();

    return default_constr_;
}

std::vector<EsPropertyKey> EsObject::own_properties() const
{
    std::vector<EsPropertyKey> all_keys;
    all_keys.reserve(indexed_properties_.count() + map_.size());
    for (const std::pair<uint32_t, EsProperty> &entry : indexed_properties_)
        all_keys.push_back(EsPropertyKey::from_u32(entry.first));

    for (EsPropertyKey &key : map_.keys())
        all_keys.push_back(key);

    return all_keys;
}

std::vector<EsPropertyKey> EsObject::properties() const
{
    // FIXME: Cache this vector.
    std::vector<EsPropertyKey> prop_keys = own_properties();

    if (prototype_)
    {
        std::vector<EsPropertyKey> tmp = prototype_->properties();
        prop_keys.insert(prop_keys.end(), tmp.begin(), tmp.end());
    }

    return prop_keys;
}

EsObject::Iterator EsObject::begin()
{
    return Iterator(this, own_properties(), true);
}

EsObject::Iterator EsObject::end()
{
    // FIXME: Calling this repeatedly is expensive.
    return Iterator(this, own_properties(), false);
}

EsObject::Iterator EsObject::begin_recursive()
{
    return Iterator(this, properties(), true);
}

EsObject::Iterator EsObject::end_recursive()
{
    // FIXME: Calling this repeatedly is expensive.
    return Iterator(this, properties(), false);
}

EsPropertyReference EsObject::get_own_property(EsPropertyKey p)
{
    if (p.is_index())
    {
        EsProperty *prop = indexed_properties_.get(p.as_index());
        if (prop)
            return EsPropertyReference(this, prop);

        return EsPropertyReference();
    }

    return map_.lookup(p);
}

EsPropertyReference EsObject::get_property(EsPropertyKey p)
{
    EsPropertyReference prop = get_own_property(p);
    if (prop)
        return prop;
 
    return prototype_ ? prototype_->get_property(p) : EsPropertyReference();
}

bool EsObject::getT(EsPropertyKey p, EsPropertyReference &prop)
{
    prop = get_property(p);
    return true;
}

bool EsObject::get_resolveT(const EsPropertyReference &prop, EsValue &v)
{
    if (!prop)
    {
        v = EsValue::undefined;
        return true;
    }
    
    if (prop->is_data())
    {
        v = prop->value_or_undefined();
        return true;
    }

    assert(prop->is_accessor());

    // Property must be an accessor.
    EsValue getter = prop->getter_or_undefined();
    if (getter.is_undefined())
    {
        v = getter;
        return true;
    }

    assert(getter.is_callable());

    EsCallFrame frame = EsCallFrame::push_function(
        0, getter.as_function(), EsValue::from_obj(this));
    if (!getter.as_function()->callT(frame))
        return false;

    v = frame.result();
    return true;
}

bool EsObject::can_put(EsPropertyKey p, EsPropertyReference &prop)
{
    prop = get_own_property(p);
    if (prop)
    {
        if (prop->is_accessor())
            return !prop->setter_or_undefined().is_undefined(); // FIXME: has_setter()?
        else
            return prop->is_writable();
    }
    
    if (!prototype_)
        return extensible_;
    
    prop = prototype_->get_property(p);
    if (!prop)
        return extensible_;
    
    if (prop->is_accessor())
        return !prop->setter_or_undefined().is_undefined();     // FIXME: has_setter()?
    
    if (!extensible_)
        return false;
    
    return prop->is_writable();
}

bool EsObject::can_put_own(const EsPropertyReference &current)
{
    assert(current);

    if (current->is_accessor())
        return !current->setter_or_undefined().is_undefined();  // FIXME: has_setter()?
    else
        return current->is_writable();
}

bool EsObject::putT(EsPropertyKey p, const EsValue &v, bool throws)
{
    EsPropertyReference prop;
    if (!can_put(p, prop))
    {
        if (throws)
        {
            ES_THROW(EsTypeError, es_fmt_msg(
                    ES_MSG_TYPE_PROP_PUT, p.to_string()->utf8().c_str()));
            return false;
        }
        return true;
    }

    if (prop && prop->is_data() && prop.base() == this)
        return update_own_propertyT(p, prop, v, throws);

    if (prop && prop->is_accessor())
    {
        EsValue setter = prop->setter_or_undefined();
        if (setter.is_undefined())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_PUT_NO_SETTER,
                        p.to_string()->utf8().c_str()));
                return false;
            }
            return true;
        }

        assert(setter.is_callable());

        EsCallFrame frame = EsCallFrame::push_function(
            1, setter.as_function(), EsValue::from_obj(this));
        frame.fp()[0] = v;

        return setter.as_function()->callT(frame);
    }
    else
    {
        return define_own_propertyT(p, EsPropertyDescriptor(true, true, true, v), throws);
    }
}

bool EsObject::put_ownT(EsPropertyKey p, EsPropertyReference &current,
                        const EsValue &v, bool throws)
{
    assert(current);

    if (!can_put_own(current))
    {
        if (throws)
        {
            ES_THROW(EsTypeError, es_fmt_msg(
                    ES_MSG_TYPE_PROP_PUT,
                    p.to_string()->utf8().c_str()));
            return false;
        }
        return true;
    }

    if (current->is_data())
        return update_own_propertyT(p, current, v, throws);

    if (current->is_accessor())
    {
        EsValue setter = current->setter_or_undefined();
        if (setter.is_undefined())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_PUT_NO_SETTER,
                        p.to_string()->utf8().c_str()));
                return false;
            }
            return true;
        }

        assert(setter.is_callable());

        EsCallFrame frame = EsCallFrame::push_function(
            1, setter.as_function(), EsValue::from_obj(this));
        frame.fp()[0] = v;

        return setter.as_function()->callT(frame);
    }

    assert(false);
    return true;
}

bool EsObject::has_property(EsPropertyKey p)
{
    return get_property(p);
}

bool EsObject::removeT(EsPropertyKey p, bool throws, bool &removed)
{
    EsPropertyReference prop = get_own_property(p);
    if (!prop)
    {
        removed = true;
        return true;
    }
    
    if (prop->is_configurable())
    {
        if (p.is_index())
            indexed_properties_.remove(p.as_index());
        else
            map_.remove(p);

        removed = true;
        return true;
    }

    if (throws)
    {
        ES_THROW(EsTypeError, es_fmt_msg(
                ES_MSG_TYPE_PROP_DELETE,
                p.to_string()->utf8().c_str()));
        return false;
    }

    removed = false;
    return true;
}

bool EsObject::default_valueT(EsTypeHint hint, EsValue &result)
{
    if (hint == ES_HINT_STRING)
    {
        EsValue to_str;
        if (!getT(property_keys.to_string, to_str))
            return false;

        if (to_str.is_callable())
        {
            EsCallFrame frame = EsCallFrame::push_function(
                0, to_str.as_function(), EsValue::from_obj(this));
            if (!to_str.as_function()->callT(frame))
                return false;

            EsValue str = frame.result();
            if (str.is_primitive())
            {
                result = str;
                return true;
            }
        }
        
        EsValue val_of;
        if (!getT(property_keys.value_of, val_of))
            return false;

        if (val_of.is_callable())
        {
            EsCallFrame frame = EsCallFrame::push_function(
                0, val_of.as_function(), EsValue::from_obj(this));
            if (!val_of.as_function()->callT(frame))
                return false;

            EsValue val = frame.result();
            if (val.is_primitive())
            {
                result = val;
                return true;
            }
        }
    }
    else    // Default is hinting a number.
    {
        EsValue val_of;
        if (!getT(property_keys.value_of, val_of))
            return false;

        if (val_of.is_callable())
        {
            EsCallFrame frame = EsCallFrame::push_function(
                0, val_of.as_function(), EsValue::from_obj(this));
            if (!val_of.as_function()->callT(frame))
                return false;

            EsValue val = frame.result();
            if (val.is_primitive())
            {
                result = val;
                return true;
            }
        }

        EsValue to_str;
        if (!getT(property_keys.to_string, to_str))
            return false;

        if (to_str.is_callable())
        {
            EsCallFrame frame = EsCallFrame::push_function(
                0, to_str.as_function(), EsValue::from_obj(this));
            if (!to_str.as_function()->callT(frame))
                return false;

            EsValue str = frame.result();
            if (str.is_primitive())
            {
                result = str;
                return true;
            }
        }
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_VAL_DEFAULT));
    return false;
}

bool EsObject::define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                    bool throws, bool &defined)
{
    // 8.12.9
    EsPropertyReference current = get_own_property(p);
    if (!current)
    {
        if (!extensible_)
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF_NO_EXT, p.to_string()->utf8().c_str()));
                return false;
            }
            defined = false;
            return true;
        }

        EsProperty prop = desc.is_generic() || desc.is_data() ?
            desc.create_data() : desc.create_accessor();

        if (p.is_index())
        {
            indexed_properties_.set(p.as_index(), prop);
        }
        else
        {
            assert(!map_.lookup(p));
            map_.add(p, prop);
        }

        defined = true;
        return true;
    }

    if (desc.empty() || current->described_by(desc))
    {
        defined = true;
        return true;
    }

    if (!current->is_configurable())
    {
        if (desc.is_configurable() || (desc.has_enumerable() &&
                                       desc.is_enumerable() != current->is_enumerable()))
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF, p.to_string()->utf8().c_str()));
                return false;
            }
            defined = false;
            return true;
        }
    }

    if (desc.is_generic())
    {
    }
    else if (current->is_data() != desc.is_data())
    {
        if (!current->is_configurable())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF, p.to_string()->utf8().c_str()));
                return false;
            }
            defined = false;
            return true;
        }

        if (current->is_data())
            current->convert_to_accessor();
        else
            current->convert_to_data();
    }
    else if (current->is_data() && desc.is_data())
    {
        if (!current->is_configurable())
        {
            if (!current->is_writable())
            {
                if (desc.is_writable() ||
                    (desc.value() &&
                     !algorithm::same_value(*desc.value(), current->value_or_undefined()))) // FIXME: value_or_undefined()?
                {
                    if (throws)
                    {
                        ES_THROW(EsTypeError, es_fmt_msg(
                                ES_MSG_TYPE_PROP_DEF,
                                p.to_string()->utf8().c_str()));
                        return false;
                    }
                    defined = false;
                    return true;
                }
            }
        }
    }
    else
    {
        assert(current->is_accessor());
        assert(desc.is_accessor());

        if (!current->is_configurable())
        {
            if ((desc.setter() && !algorithm::same_value(*desc.setter(), current->setter_or_undefined())) ||
                (desc.getter() && !algorithm::same_value(*desc.getter(), current->getter_or_undefined())))
            {
                if (throws)
                {
                    ES_THROW(EsTypeError, es_fmt_msg(
                            ES_MSG_TYPE_PROP_DEF,
                            p.to_string()->utf8().c_str()));
                    return false;
                }
                defined = false;
                return true;
            }
        }
    }

    current->copy_from(desc);
    defined = true;
    return true;
}

bool EsObject::update_own_propertyT(EsPropertyKey p, EsPropertyReference &current,
                                    const EsValue &v, bool throws)
{
    assert(current);

    if (!current->is_data())
    {
        assert(current->is_accessor());

        if (!current->is_configurable())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF,
                        p.to_string()->utf8().c_str()));
                return false;
            }

            return true;
        }

        current->convert_to_data();
    }
    else
    {
        if (!current->is_configurable() && !current->is_writable())
        {
            if (!algorithm::same_value(v, current->value_or_undefined()))
            {
                if (throws)
                {
                    ES_THROW(EsTypeError, es_fmt_msg(
                            ES_MSG_TYPE_PROP_DEF,
                            p.to_string()->utf8().c_str()));
                    return false;
                }

                return true;
            }
        }
    }

    current->set_value(v);
    return true;
}

void EsObject::define_new_own_property(EsPropertyKey p, const EsPropertyDescriptor &desc)
{
    assert(extensible_);

    EsProperty prop = desc.is_generic() || desc.is_data() ?
        desc.create_data() : desc.create_accessor();

    if (p.is_index())
    {
        indexed_properties_.set(p.as_index(), prop);
    }
    else
    {
        assert(!map_.lookup(p));
        map_.add(p, prop);
    }
}

EsArguments::EsArguments()
    : param_map_(NULL)
{
}

EsArguments::~EsArguments()
{
}

ES_API_FUN(es_arg_getter)
{
    THROW(InternalException, "internal error: es_arg_getter is not implemented.");
}

ES_API_FUN(es_arg_setter)
{
    THROW(InternalException, "internal error: es_arg_setter is not implemented.");
}

EsFunction *EsArguments::make_arg_getter(EsValue *val)
{
    return EsArgumentGetter::create_inst(val);
}

EsFunction *EsArguments::make_arg_setter(EsValue *val)
{
    return EsArgumentSetter::create_inst(val);
}

EsArguments *EsArguments::create_inst(EsFunction *callee,
                                      uint32_t argc, const EsValue argv[])
{
    EsArguments *a = new (GC)EsArguments();

    a->prototype_ = es_proto_obj();
    a->class_ = _USTR("Arguments");
    a->extensible_ = true;

    a->param_map_ = EsObject::create_inst();

    a->define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, true, true,
            EsValue::from_num(static_cast<double>(argc)))); // VERIFIED: 10.6.

    for (uint32_t i = argc; i-- > 0;)
    {
        const EsValue &val = argv[i];
        a->define_new_own_property(EsPropertyKey::from_u32(i),
                                   EsPropertyDescriptor(true, true, true, val));
    }

    if (!callee->is_strict())
    {
        a->define_new_own_property(property_keys.callee,
                                   EsPropertyDescriptor(false, true, true,
                                                        EsValue::from_obj(callee)));
    }
    else
    {
        EsFunction *thrower = es_throw_type_err(); // [[ThrowTypeError]]

        a->define_new_own_property(property_keys.caller,
            EsPropertyDescriptor(false, false,
                                 EsValue::from_obj(thrower),
                                 EsValue::from_obj(thrower)));

        a->define_new_own_property(property_keys.callee,
            EsPropertyDescriptor(false, false,
                                 EsValue::from_obj(thrower),
                                 EsValue::from_obj(thrower)));
    }

    return a;
}

EsArguments *EsArguments::create_inst(EsFunction *callee,
                                      uint32_t argc, EsValue argv[],
                                      uint32_t prmc, String prmv[])
{
    EsArguments *a = new (GC)EsArguments();
    
    a->prototype_ = es_proto_obj();
    a->class_ = _USTR("Arguments");
    a->extensible_ = true;

    a->param_map_ = EsObject::create_inst();
    
    a->define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, true, true,
            EsValue::from_num(static_cast<double>(argc)))); // VERIFIED: 10.6.

    StringSet mapped_names;

    for (uint32_t i = argc; i-- > 0;)
    {
        const EsValue &val = argv[i];
        a->define_new_own_property(EsPropertyKey::from_u32(i),
                                   EsPropertyDescriptor(true, true, true, val));

        if (i < prmc)
        {
            String name = prmv[i];
            if (!callee->is_strict() && mapped_names.count(name) == 0)
            {
                mapped_names.insert(name);

                EsFunction *g = make_arg_getter(&argv[i]);
                EsFunction *p = make_arg_setter(&argv[i]);

                a->param_map_->define_new_own_property(EsPropertyKey::from_u32(i),
                    EsPropertyDescriptor(Maybe<bool>(), true,
                                         EsValue::from_obj(g),
                                         EsValue::from_obj(p)));
            }
        }
    }

    if (!callee->is_strict())
    {
        a->define_new_own_property(property_keys.callee,
                                   EsPropertyDescriptor(false, true, true,
                                                        EsValue::from_obj(callee)));
    }
    else
    {
        EsFunction *thrower = es_throw_type_err(); // [[ThrowTypeError]]
        
        a->define_new_own_property(property_keys.caller,
            EsPropertyDescriptor(false, false,
                                 EsValue::from_obj(thrower),
                                 EsValue::from_obj(thrower)));
        
        a->define_new_own_property(property_keys.callee,
            EsPropertyDescriptor(false, false,
                                 EsValue::from_obj(thrower),
                                 EsValue::from_obj(thrower)));
    }
    
    return a;
}

void EsArguments::link_parameter(uint32_t i, EsValue *val)
{
    EsFunction *g = make_arg_getter(val);
    EsFunction *p = make_arg_setter(val);

    param_map_->define_new_own_property(EsPropertyKey::from_u32(i),
        EsPropertyDescriptor(Maybe<bool>(), true,
                             EsValue::from_obj(g),
                             EsValue::from_obj(p)));
}

EsPropertyReference EsArguments::get_own_property(EsPropertyKey p)
{
    EsPropertyReference prop = EsObject::get_own_property(p);
    if (!prop)
        return prop;

    EsPropertyReference map_prop = param_map_->get_own_property(p);
    if (map_prop)
    {
        EsValue v;
        param_map_->get_resolveT(map_prop, v); // This should never throw, no need to catch anything.

        prop->set_value(v);
    }
    
    return prop;
}

bool EsArguments::getT(EsPropertyKey p, EsPropertyReference &prop)
{
    prop = param_map_->get_own_property(p);
    if (!prop)
    {
        if (!EsObject::getT(p, prop))
            return false;
        
        EsValue v;
        if (!EsObject::get_resolveT(prop, v))
            return false;

        if (v.is_callable())
        {
            EsFunction *fun = v.as_function();
            if (p == property_keys.caller && fun->is_strict())
            {
                ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_PROP_CALLER));
                return false;
            }
        }

        return true;
    }

    return true;
}

bool EsArguments::removeT(EsPropertyKey p, bool throws, bool &removed)
{
    EsPropertyReference is_mapped = param_map_->get_own_property(p);

    if (!EsObject::removeT(p, throws, removed))
        return false;

    if (removed && is_mapped)
    {
        if (!param_map_->removeT(p, throws))
            return false;
    }
    
    return true;
}

bool EsArguments::define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                       bool throws, bool &defined)
{
    // 10.6
    bool allowed = false;
    if (!EsObject::define_own_propertyT(p, desc, false, allowed))
        return false;

    if (!allowed)
    {
        if (throws)
        {
            ES_THROW(EsTypeError, es_fmt_msg(
                    ES_MSG_TYPE_PROP_DEF, p.to_string()->utf8().c_str()));
            return false;
        }
        defined = false;
        return true;
    }
    
    if (param_map_->get_own_property(p))
    {
        if (desc.is_accessor())
        {
            if (!param_map_->removeT(p, false))
                return false;
        }
        else
        {
            if (desc.value())
            {
                if (!param_map_->putT(p, *desc.value(), throws))
                    return false;
            }

            if (!desc.is_writable())
            {
                if (!param_map_->removeT(p, false))
                    return false;
            }
        }
    }

    defined = true;
    return true;
}

bool EsArguments::update_own_propertyT(EsPropertyKey p, EsPropertyReference &current,
                                       const EsValue &v, bool throws)
{
    // 10.6
    if (!EsObject::update_own_propertyT(p, current, v, throws))
        return false;

    if (param_map_->get_own_property(p))
    {
        if (!param_map_->putT(p, v, throws))
            return false;
    }

    return true;
}

EsFunction *EsArray::default_constr_ = NULL;

EsArray::EsArray()
{
}

EsArray::~EsArray()
{
}

void EsArray::make_inst()
{
    prototype_ = es_proto_arr();
    class_ = _USTR("Array");
    extensible_ = true;
}

void EsArray::make_proto()
{
    prototype_ = es_proto_obj();
    class_ = _USTR("Array");
    extensible_ = true;

    // 15.4.4
    define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, false, true, EsValue::from_u32(0)));                // VERIFIED: 15.4.4
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());                    // VERIFIED: 15.4.4.1
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_arr_proto_to_str, 0);            // VERIFIED: 15.4.4.2
    ES_DEFINE_PROP_FUN(property_keys.to_locale_string, es_std_arr_proto_to_locale_str, 0);  // VERIFIED: 15.4.4.3
    ES_DEFINE_PROP_FUN(property_keys.concat, es_std_arr_proto_concat, 1);               // VERIFIED: 15.4.4.4
    ES_DEFINE_PROP_FUN(property_keys.join, es_std_arr_proto_join, 1);                   // VERIFIED: 15.4.4.5
    ES_DEFINE_PROP_FUN(property_keys.pop, es_std_arr_proto_pop, 0);                     // VERIFIED: 15.4.4.6
    ES_DEFINE_PROP_FUN(property_keys.push, es_std_arr_proto_push, 1);                   // VERIFIED: 15.4.4.7
    ES_DEFINE_PROP_FUN(property_keys.reverse, es_std_arr_proto_reverse, 0);             // VERIFIED: 15.4.4.8
    ES_DEFINE_PROP_FUN(property_keys.shift, es_std_arr_proto_shift, 0);                 // VERIFIED: 15.4.4.9
    ES_DEFINE_PROP_FUN(property_keys.slice, es_std_arr_proto_slice, 2);                 // VERIFIED: 15.4.4.10
    ES_DEFINE_PROP_FUN(property_keys.sort, es_std_arr_proto_sort, 1);                   // VERIFIED: 15.4.4.11
    ES_DEFINE_PROP_FUN(property_keys.splice, es_std_arr_proto_splice, 2);               // VERIFIED: 15.4.4.12
    ES_DEFINE_PROP_FUN(property_keys.unshift, es_std_arr_proto_unshift, 1);             // VERIFIED: 15.4.4.13
    ES_DEFINE_PROP_FUN(property_keys.index_of, es_std_arr_proto_index_of, 1);           // VERIFIED: 15.4.4.14
    ES_DEFINE_PROP_FUN(property_keys.last_index_of, es_std_arr_proto_last_index_of, 1); // VERIFIED: 15.4.4.15
    ES_DEFINE_PROP_FUN(property_keys.every, es_std_arr_proto_every, 1);                 // VERIFIED: 15.4.4.16
    ES_DEFINE_PROP_FUN(property_keys.some, es_std_arr_proto_some, 1);                   // VERIFIED: 15.4.4.17
    ES_DEFINE_PROP_FUN(property_keys.for_each, es_std_arr_proto_for_each, 1);           // VERIFIED: 15.4.4.18
    ES_DEFINE_PROP_FUN(property_keys.map, es_std_arr_proto_map, 1);                     // VERIFIED: 15.4.4.19
    ES_DEFINE_PROP_FUN(property_keys.filter, es_std_arr_proto_filter, 1);               // VERIFIED: 15.4.4.20
    ES_DEFINE_PROP_FUN(property_keys.reduce, es_std_arr_proto_reduce, 1);               // VERIFIED: 15.4.4.21
    ES_DEFINE_PROP_FUN(property_keys.reduce_right, es_std_arr_proto_reduce_right, 1);   // VERIFIED: 15.4.4.22
}

EsArray *EsArray::create_raw()
{
    return new (GC)EsArray();
}

EsArray *EsArray::create_inst(uint32_t len)
{
    EsArray *a = new (GC)EsArray();
    a->make_inst();
    
    // 15.4.5
    a->define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, false, true, EsValue::from_u32(len)));      // VERIFIED: 15.4.5.2

    a->indexed_properties_.reserve_compact_storage(len);
    
    return a;
}

EsArray *EsArray::create_inst_from_lit(uint32_t count, EsValue items[])
{
    EsArray *a = new (GC)EsArray();
    a->make_inst();

    a->define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, false, true,
                             EsValue::from_num(static_cast<double>(count))));   // VERIFIED: 15.4.5.2

    EsValueVector::const_iterator it;
    for (uint32_t i = 0; i < count; i++)
    {
        if (items[i].is_nothing())
            continue;

        a->define_new_own_property(EsPropertyKey::from_u32(i),
                                   EsPropertyDescriptor(true, true, true, items[i]));
    }

    return a;
}

EsFunction *EsArray::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsArrayConstructor::create_inst();
    
    return default_constr_;
}

bool EsArray::define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                   bool throws, bool &defined)
{
    EsPropertyReference old_len_prop = get_own_property(property_keys.length);
    uint32_t old_len = old_len_prop ?
        (old_len_prop->value_or_undefined()).primitive_to_uint32() : 0;

    if (p == property_keys.length)
    {
        if (!desc.value())
            return EsObject::define_own_propertyT(p, desc, throws, defined);

        EsValue len;
        if (!(*desc.value()).to_primitiveT(ES_HINT_NUMBER, len))
            return false;

        uint32_t new_len = 0;
        if (!es_num_to_index(len.primitive_to_number(), new_len))
        {
            // NOTE: This should always be thrown, no matter if the throws
            //       parameter is false.
            ES_THROW(EsRangeError, es_fmt_msg(
                    ES_MSG_RANGE_INVALID_ARRAY,
                    len.primitive_to_string()->utf8().c_str()));
            return false;
        }

        EsPropertyDescriptor new_len_desc(desc);
        new_len_desc.set_value(EsValue::from_u32(new_len));
        if (new_len >= old_len)
            return EsObject::define_own_propertyT(p, new_len_desc, throws, defined);    // FIXME: update_own_property?
        
        if (!old_len_prop->is_writable())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF, p.to_string()->utf8().c_str()));
                return false;
            }
            defined = false;
            return true;
        }
        
        bool new_writable = false;  // Default: Non-semantic.
        if (!new_len_desc.has_writable() || new_len_desc.is_writable())
        {
            new_writable = true;
        }
        else
        {
            new_writable = false;
            new_len_desc.set_writable(true);
        }
        
        bool succeeded = false;
        if (!EsObject::define_own_propertyT(p, new_len_desc, throws, succeeded))
            return false;

        if (!succeeded)
        {
            defined = false;
            return true;
        }
        
        // FIXME: Iterate storage rather than indexes.
        while (new_len < old_len)
        {
            old_len--;
            
            bool delete_succeeded = false;
            if (!removeT(EsPropertyKey::from_u32(old_len), false, delete_succeeded))
                return false;

            if (!delete_succeeded)
            {
                new_len_desc.set_value(EsValue::from_u32(old_len + 1));
                if (!new_writable)
                    new_len_desc.set_writable(false);
                if (!EsObject::define_own_propertyT(p, new_len_desc, throws))
                    return false;
                
                if (throws)
                {
                    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_PROP_DELETE, lexical_cast<const char *>(old_len)));
                    return false;
                }

                defined = false;
                return true;
            }
        }
        
        if (!new_writable)
        {
            if (!EsObject::define_own_propertyT(p,
                EsPropertyDescriptor(Maybe<bool>(), Maybe<bool>(), false,
                                     Maybe<EsValue>()), false, defined))
            {
                return false;
            }
        }
        
        defined = true;
        return true;
    }
    
    // 15.4
    if (p.is_index())
    {
        if (p.as_index() >= old_len && !old_len_prop->is_writable())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF, p.to_string()->utf8().c_str()));
                return false;
            }

            defined = false;
            return true;
        }
        
        bool succeeded = false;
        if (!EsObject::define_own_propertyT(p, desc, false, succeeded))
            return false;

        if (!succeeded)
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF, p.to_string()->utf8().c_str()));
                return false;
            }

            defined = false;
            return true;
        }
        
        if (p.as_index() >= old_len)
            update_own_propertyT(property_keys.length, old_len_prop,
                EsValue::from_u64(static_cast<uint64_t>(p.as_index()) + 1), throws);
        
        defined = true;
        return true;
    }

    return EsObject::define_own_propertyT(p, desc, throws, defined);
}

bool EsArray::update_own_propertyT(EsPropertyKey p, EsPropertyReference &current,
                                   const EsValue &v, bool throws)
{
    assert(current);

    EsPropertyReference old_len_prop = get_own_property(property_keys.length);
    uint32_t old_len = old_len_prop ?
        old_len_prop->value_or_undefined().primitive_to_uint32() : 0;

    if (current == old_len_prop)
    {
        EsValue len;
        if (!v.to_primitiveT(ES_HINT_NUMBER, len))
            return false;

        uint32_t new_len = 0;
        if (!es_num_to_index(len.primitive_to_number(), new_len))
        {
            // NOTE: This should always be thrown, no matter if the throws
            //       parameter is false.
            ES_THROW(EsRangeError, es_fmt_msg(
                    ES_MSG_RANGE_INVALID_ARRAY,
                    len.primitive_to_string()->utf8().c_str()));
            return false;
        }

        if (new_len >= old_len)
            return EsObject::update_own_propertyT(p, current, EsValue::from_u32(new_len), throws);

        if (!old_len_prop->is_writable())
        {
            if (throws)
            {
                ES_THROW(EsTypeError, es_fmt_msg(
                        ES_MSG_TYPE_PROP_DEF,
                        p.to_string()->utf8().c_str()));
                return false;
            }
            return true;
        }

        if (!EsObject::update_own_propertyT(p, current, EsValue::from_u32(new_len), throws))
            return false;

        // FIXME: Iterate storage rather than indexes.
        while (new_len < old_len)
        {
            old_len--;

            bool delete_succeeded = false;
            if (!removeT(EsPropertyKey::from_u32(old_len), false, delete_succeeded))
                return false;

            if (!delete_succeeded)
            {
                if (!EsObject::update_own_propertyT(p, current, EsValue::from_u32(old_len + 1), throws))
                    return false;

                if (throws)
                {
                    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_PROP_DELETE,
                                                     lexical_cast<const char *>(old_len)));
                    return false;
                }

                return true;
            }
        }

        return true;
    }

    return EsObject::update_own_propertyT(p, current, v, throws);
}

EsFunction *EsBooleanObject::default_constr_ = NULL;

EsBooleanObject::EsBooleanObject()
    : primitive_value_(false)
{
}

EsBooleanObject::~EsBooleanObject()
{
}

void EsBooleanObject::make_inst()
{
    prototype_ = es_proto_bool();
    class_ = _USTR("Boolean");
    extensible_ = true;
}

void EsBooleanObject::make_proto()
{
    prototype_ = es_proto_obj();    // VERIFIED: 15.6.4
    class_ = _USTR("Boolean");      // VERIFIED: 15.6.4
    extensible_ = true;
    primitive_value_ = false;       // VERIFIED: 15.6.4
    
    // 15.6.4
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());            // VERIFIED: 15.6.4.1
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_bool_proto_to_str, 0);   // VERIFIED: 15.6.4.2
    ES_DEFINE_PROP_FUN(property_keys.value_of, es_std_bool_proto_val_of, 0);    // VERIFIED: 15.6.4.3
}

EsBooleanObject *EsBooleanObject::create_raw()
{
    return new (GC)EsBooleanObject();
}

EsBooleanObject *EsBooleanObject::create_inst(bool primitive_value)
{
    EsBooleanObject *b = new (GC)EsBooleanObject();
    b->make_inst();
    b->primitive_value_ = primitive_value;
    return b;
}

bool EsBooleanObject::primitive_value()
{
    return primitive_value_;
}

EsFunction *EsBooleanObject::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsBooleanConstructor::create_inst();
    
    return default_constr_;
}

EsFunction *EsDate::default_constr_ = NULL;

EsDate::EsDate()
    : primitive_value_(std::numeric_limits<double>::quiet_NaN())
{
}

EsDate::~EsDate()
{
}

void EsDate::make_inst()
{
    prototype_ = es_proto_date();   // VERIFIED: 15.9.3.3
    class_ = _USTR("Date");         // VERIFIED: 15.9.3.3
    extensible_ = true;             // VERIFIED: 15.9.3.3
}

void EsDate::make_proto()
{
    prototype_ = es_proto_obj();    // VERIFIED: 15.9.5
    class_ = _USTR("Date");         // VERIFIED: 15.9.5
    extensible_ = true;
    primitive_value_ = 0;

    // 15.9.5
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());                    // VERIFIED: 15.9.5.1
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_date_proto_to_str, 0);           // VERIFIED: 15.9.5.2
    ES_DEFINE_PROP_FUN(property_keys.to_date_string, es_std_date_proto_to_date_str, 0); // VERIFIED: 15.9.5.3
    ES_DEFINE_PROP_FUN(property_keys.to_time_string, es_std_date_proto_to_time_str, 0); // VERIFIED: 15.9.5.4
    ES_DEFINE_PROP_FUN(property_keys.to_locale_string, es_std_date_proto_to_locale_str, 0);             // VERIFIED: 15.9.5.5
    ES_DEFINE_PROP_FUN(property_keys.to_locale_date_string, es_std_date_proto_to_locale_date_str, 0);   // VERIFIED: 15.9.5.6
    ES_DEFINE_PROP_FUN(property_keys.to_locale_time_string, es_std_date_proto_to_locale_time_str, 0);   // VERIFIED: 15.9.5.7
    ES_DEFINE_PROP_FUN(property_keys.value_of, es_std_date_proto_val_of, 0);            // VERIFIED: 15.9.5.8
    ES_DEFINE_PROP_FUN(property_keys.get_time, es_std_date_proto_get_time, 0);          // VERIFIED: 15.9.5.9
    ES_DEFINE_PROP_FUN(property_keys.get_full_year, es_std_date_proto_get_full_year, 0);                // VERIFIED: 15.9.5.10
    ES_DEFINE_PROP_FUN(property_keys.get_utc_full_year, es_std_date_proto_get_utc_full_year, 0);        // VERIFIED: 15.9.5.11
    ES_DEFINE_PROP_FUN(property_keys.get_month, es_std_date_proto_get_month, 0);        // VERIFIED: 15.9.5.12
    ES_DEFINE_PROP_FUN(property_keys.get_utc_month, es_std_date_proto_get_utc_month, 0);                // VERIFIED: 15.9.5.13
    ES_DEFINE_PROP_FUN(property_keys.get_date, es_std_date_proto_get_date, 0);          // VERIFIED: 15.9.5.14
    ES_DEFINE_PROP_FUN(property_keys.get_utc_date, es_std_date_proto_get_utc_date, 0);  // VERIFIED: 15.9.5.15
    ES_DEFINE_PROP_FUN(property_keys.get_day, es_std_date_proto_get_day, 0);            // VERIFIED: 15.9.5.16
    ES_DEFINE_PROP_FUN(property_keys.get_utc_day, es_std_date_proto_get_utc_day, 0);    // VERIFIED: 15.9.5.17
    ES_DEFINE_PROP_FUN(property_keys.get_hours, es_std_date_proto_get_hours, 0);        // VERIFIED: 15.9.5.18
    ES_DEFINE_PROP_FUN(property_keys.get_utc_hours, es_std_date_proto_get_utc_hours, 0);                // VERIFIED: 15.9.5.19
    ES_DEFINE_PROP_FUN(property_keys.get_minutes, es_std_date_proto_get_minutes, 0);    // VERIFIED: 15.9.5.20
    ES_DEFINE_PROP_FUN(property_keys.get_utc_minutes, es_std_date_proto_get_utc_minutes, 0);            // VERIFIED: 15.9.5.21
    ES_DEFINE_PROP_FUN(property_keys.get_seconds, es_std_date_proto_get_seconds, 0);    // VERIFIED: 15.9.5.22
    ES_DEFINE_PROP_FUN(property_keys.get_utc_seconds, es_std_date_proto_get_utc_seconds, 0);            // VERIFIED: 15.9.5.23
    ES_DEFINE_PROP_FUN(property_keys.get_milliseconds, es_std_date_proto_get_milliseconds, 0);          // VERIFIED: 15.9.5.24
    ES_DEFINE_PROP_FUN(property_keys.get_utc_milliseconds, es_std_date_proto_get_utc_milliseconds, 0);  // VERIFIED: 15.9.5.25
    ES_DEFINE_PROP_FUN(property_keys.get_timezone_offset, es_std_date_proto_get_time_zone_off, 0);      // VERIFIED: 15.9.5.26
    ES_DEFINE_PROP_FUN(property_keys.set_time, es_std_date_proto_set_time, 1);          // VERIFIED: 15.9.5.27
    ES_DEFINE_PROP_FUN(property_keys.set_milliseconds, es_std_date_proto_set_milliseconds, 1);          // VERIFIED: 15.9.5.28
    ES_DEFINE_PROP_FUN(property_keys.set_utc_milliseconds, es_std_date_proto_set_utc_milliseconds, 1);  // VERIFIED: 15.9.5.29
    ES_DEFINE_PROP_FUN(property_keys.set_seconds, es_std_date_proto_set_seconds, 2);    // VERIFIED: 15.9.5.30
    ES_DEFINE_PROP_FUN(property_keys.set_utc_seconds, es_std_date_proto_set_utc_seconds, 2);            // VERIFIED: 15.9.5.31
    ES_DEFINE_PROP_FUN(property_keys.set_minutes, es_std_date_proto_set_minutes, 3);    // VERIFIED: 15.9.5.32
    ES_DEFINE_PROP_FUN(property_keys.set_utc_minutes, es_std_date_proto_set_utc_minutes, 3);            // VERIFIED: 15.9.5.33
    ES_DEFINE_PROP_FUN(property_keys.set_hours, es_std_date_proto_set_hours, 4);        // VERIFIED: 15.9.5.34
    ES_DEFINE_PROP_FUN(property_keys.set_utc_hours, es_std_date_proto_set_utc_hours, 4);// VERIFIED: 15.9.5.35
    ES_DEFINE_PROP_FUN(property_keys.set_date, es_std_date_proto_set_date, 1);          // VERIFIED: 15.9.5.36
    ES_DEFINE_PROP_FUN(property_keys.set_utc_date, es_std_date_proto_set_utc_date, 1);  // VERIFIED: 15.9.5.37
    ES_DEFINE_PROP_FUN(property_keys.set_month, es_std_date_proto_set_month, 2);        // VERIFIED: 15.9.5.38
    ES_DEFINE_PROP_FUN(property_keys.set_utc_month, es_std_date_proto_set_utc_month, 2);                // VERIFIED: 15.9.5.39
    ES_DEFINE_PROP_FUN(property_keys.set_full_year, es_std_date_proto_set_full_year, 3);                // VERIFIED: 15.9.5.40
    ES_DEFINE_PROP_FUN(property_keys.set_utc_full_year, es_std_date_proto_set_utc_full_year, 3);        // VERIFIED: 15.9.5.41
    ES_DEFINE_PROP_FUN(property_keys.to_utc_string, es_std_date_proto_to_utc_str, 0);   // VERIFIED: 15.9.5.42
    ES_DEFINE_PROP_FUN(property_keys.to_iso_string, es_std_date_proto_to_iso_str, 0);   // VERIFIED: 15.9.5.43
    ES_DEFINE_PROP_FUN(property_keys.to_json, es_std_date_proto_to_json, 1);            // VERIFIED: 15.9.5.44
}

EsDate *EsDate::create_raw()
{
    return new (GC)EsDate();
}

bool EsDate::create_inst(EsValue &result)
{
    EsDate *d = new (GC)EsDate();
    d->make_inst();
    d->primitive_value_ = time_now();

    result = EsValue::from_obj(d);
    return true;
}

bool EsDate::create_inst(const EsValue &value, EsValue &result)
{
    EsDate *d = new (GC)EsDate();
    d->make_inst();
    
    // 15.9.3.2
    EsValue v;
    if (!value.to_primitiveT(ES_HINT_NONE, v))
        return false;

    double num = 0.0;
    if (v.is_string())
        num = es_date_parse(v.as_string());
    else
        num = v.primitive_to_number();
    
    d->primitive_value_ = es_time_clip(num);
    
    result = EsValue::from_obj(d);
    return true;
}

bool EsDate::create_inst(const EsValue &year, const EsValue &month,
                         const EsValue *date, const EsValue *hours,
                         const EsValue *min, const EsValue *sec,
                         const EsValue *ms, EsValue &result)
{
    EsDate *d = new (GC)EsDate();
    d->make_inst();

    // 15.9.3.1
    double n_year = 0.0;
    if (!year.to_numberT(n_year))
        return false;
    double n_month = 0.0;
    if (!month.to_numberT(n_month))
        return false;

    double n_date = 1.0;
    if (date && !date->to_numberT(n_date))
        return false;
    double n_hours = 0.0;
    if (hours && !hours->to_numberT(n_hours))
        return false;
    double n_min = 0.0;
    if (min && !min->to_numberT(n_min))
        return false;
    double n_sec = 0.0;
    if (sec && !sec->to_numberT(n_sec))
        return false;
    double n_ms = 0.0;
    if (ms && !ms->to_numberT(n_ms))
        return false;

    double yr = n_year;

    int64_t i_year = static_cast<int64_t>(n_year);
    if (!std::isnan(n_year) && i_year >= 0 && i_year <= 99)
        yr = 1900 + i_year;

    double final_date = es_make_date(es_make_day(yr, n_month, n_date), es_make_time(n_hours, n_min, n_sec, n_ms));

    d->primitive_value_ = es_time_clip(es_utc(final_date));

    result = EsValue::from_obj(d);
    return true;
}

double EsDate::primitive_value() const
{
    return primitive_value_;
}

time_t EsDate::date_value() const
{
    return static_cast<time_t>(primitive_value_ / 1000.0);
}

EsFunction *EsDate::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsDateConstructor::create_inst();
    
    return default_constr_;
}

bool EsDate::default_valueT(EsTypeHint hint, EsValue &result)
{
    // According to 8.12.8 the date class defaults to string.
    return EsObject::default_valueT(hint == ES_HINT_NONE ? ES_HINT_STRING : hint, result);
}

EsFunction *EsNumberObject::default_constr_ = NULL;

EsNumberObject::EsNumberObject()
    : primitive_value_(0.0)
{
}

EsNumberObject::EsNumberObject(double primitive_value)
    : primitive_value_(primitive_value)
{
}

EsNumberObject::~EsNumberObject()
{
}

void EsNumberObject::make_inst()
{
    prototype_ = es_proto_num();
    class_ = _USTR("Number");
    extensible_ = true;
}

void EsNumberObject::make_proto()
{
    prototype_ = es_proto_obj();    // VERIFIED: 15.7.4
    class_ = _USTR("Number");       // VERIFIED: 15.7.4
    extensible_ = true;
    primitive_value_ = 0.0;         // VERIFIED: 15.7.4

    // 15.7.4
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());                        // VERIFIED: 15.7.4.1
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_num_proto_to_str, 1);                // VERIFIED: 15.7.4.2
    ES_DEFINE_PROP_FUN(property_keys.to_locale_string, es_std_num_proto_to_locale_str, 0);  // VERIFIED: 15.7.4.3
    ES_DEFINE_PROP_FUN(property_keys.value_of, es_std_num_proto_val_of, 0);                 // VERIFIED: 15.7.4.4
    ES_DEFINE_PROP_FUN(property_keys.to_fixed, es_std_num_proto_to_fixed, 1);               // VERIFIED: 15.7.4.5
    ES_DEFINE_PROP_FUN(property_keys.to_exponential, es_std_num_proto_to_exp, 1);           // VERIFIED: 15.7.4.6
    ES_DEFINE_PROP_FUN(property_keys.to_precision, es_std_num_proto_to_prec, 1);            // VERIFIED: 15.7.4.7
}

EsNumberObject *EsNumberObject::create_raw()
{
    return new (GC)EsNumberObject(0.0);
}

EsNumberObject *EsNumberObject::create_inst(double primitive_value)
{
    EsNumberObject *n = new (GC)EsNumberObject(primitive_value);
    n->make_inst();
    return n;
}

double EsNumberObject::primitive_value()
{
    return primitive_value_;
}

EsFunction *EsNumberObject::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsNumberConstructor::create_inst();
    
    return default_constr_;
}

EsFunction *EsStringObject::default_constr_ = NULL;

EsStringObject::EsStringObject()
    : primitive_value_(EsString::create())
{
}

EsStringObject::EsStringObject(const EsString *primitive_value)
    : primitive_value_(primitive_value)
{
}

EsStringObject::~EsStringObject()
{
}

void EsStringObject::make_inst()
{
    prototype_ = es_proto_str();
    class_ = _USTR("String");
    extensible_ = true;
}

void EsStringObject::make_proto()
{
    prototype_ = es_proto_obj();
    class_ = _USTR("String");
    extensible_ = true;

    // 15.5.4
    define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, false, false, EsValue::from_u32(0)));   // ASSUMED: CK 2012-02-11
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_str_proto_to_str, 0);
    ES_DEFINE_PROP_FUN(property_keys.value_of, es_std_str_proto_val_of, 0);
    ES_DEFINE_PROP_FUN(property_keys.char_at, es_std_str_proto_char_at, 1);
    ES_DEFINE_PROP_FUN(property_keys.char_code_at, es_std_str_proto_char_code_at, 1);
    ES_DEFINE_PROP_FUN(property_keys.concat, es_std_str_proto_concat, 1);
    ES_DEFINE_PROP_FUN(property_keys.index_of, es_std_str_proto_index_of, 1);
    ES_DEFINE_PROP_FUN(property_keys.last_index_of, es_std_str_proto_last_index_of, 1);
    ES_DEFINE_PROP_FUN(property_keys.locale_compare, es_std_str_proto_locale_compare, 1);
    ES_DEFINE_PROP_FUN(property_keys.match, es_std_str_proto_match, 1);
    ES_DEFINE_PROP_FUN(property_keys.replace, es_std_str_proto_replace, 2);
    ES_DEFINE_PROP_FUN(property_keys.search, es_std_str_proto_search, 1);
    ES_DEFINE_PROP_FUN(property_keys.slice, es_std_str_proto_slice, 2);
    ES_DEFINE_PROP_FUN(property_keys.split, es_std_str_proto_split, 2);
    ES_DEFINE_PROP_FUN(property_keys.substr, es_std_str_proto_substr, 2);
    ES_DEFINE_PROP_FUN(property_keys.substring, es_std_str_proto_substring, 2);
    ES_DEFINE_PROP_FUN(property_keys.to_lower_case, es_std_str_proto_to_lower_case, 0);
    ES_DEFINE_PROP_FUN(property_keys.to_locale_lower_case, es_std_str_proto_to_locale_lower_case, 0);
    ES_DEFINE_PROP_FUN(property_keys.to_upper_case, es_std_str_proto_to_upper_case, 0);
    ES_DEFINE_PROP_FUN(property_keys.to_locale_upper_case, es_std_str_proto_to_locale_upper_case, 0);
    ES_DEFINE_PROP_FUN(property_keys.trim, es_std_str_proto_trim, 0);
}

EsStringObject *EsStringObject::create_raw()
{
    return new (GC)EsStringObject();
}

EsStringObject *EsStringObject::create_inst(const EsString *primitive_value)
{
    EsStringObject *s = new (GC)EsStringObject(primitive_value);
    s->make_inst();
    
    // 15.5.5.1
    s->define_new_own_property(property_keys.length,
            EsPropertyDescriptor(false, false, false,
                    EsValue::from_num(static_cast<double>(
                            primitive_value->length()))));      // VERIFIED: 15.5.5.1
    
    return s;
}

const EsString *EsStringObject::primitive_value() const
{
    return primitive_value_;
}

EsFunction *EsStringObject::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_  = EsStringConstructor::create_inst();
    
    return default_constr_;
}

EsPropertyReference EsStringObject::get_own_property(EsPropertyKey p)
{
    EsPropertyReference prop = EsObject::get_own_property(p);
    if (prop)
        return prop;
        
    if (!p.is_index())
        return EsPropertyReference();
    
    // Make sure the indexer is within range.
    if (p.as_index() >= primitive_value_->length())
        return EsPropertyReference();
    
    // Create property.
    return EsPropertyReference(this,
            new (GC)EsProperty(true, false, false,
                    EsValue::from_str(EsString::create(
                            primitive_value_->at(p.as_index())))));
}

EsFunction *EsFunction::default_constr_ = NULL;

EsFunction::EsFunction(EsLexicalEnvironment *scope,
                       NativeFunction fun, bool strict, uint32_t len,
                       bool needs_this_binding)
    : strict_(strict)
    , len_(len)
    , fun_(fun)
    , code_(NULL)
    , scope_(scope)
    , needs_args_obj_(false)
    , needs_this_binding_(needs_this_binding)
{
}

EsFunction::EsFunction(EsLexicalEnvironment *scope,
                       FunctionLiteral *code,
                       bool needs_this_binding)
    : strict_(code->is_strict_mode())
    , len_(static_cast<uint32_t>(code->parameters().size()))
    , fun_(NULL)
    , code_(code)
    , scope_(scope)
    , needs_args_obj_(false)
    , needs_this_binding_(needs_this_binding)
{
}

EsFunction::~EsFunction()
{
}

void EsFunction::make_inst(bool has_prototype)
{
    prototype_ = es_proto_fun();
    class_ = _USTR("Function");
    extensible_ = true;
    
    // 13.2 Creating Function Objects
    EsObject *proto = EsObject::create_inst();
    proto->define_new_own_property(property_keys.constructor,
        EsPropertyDescriptor(false, true, true, EsValue::from_obj(this)));

    define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, false, false, EsValue::from_u32(len_)));        // VERIFIED: 15.3.5.1
    
    if (has_prototype)
    {
        define_new_own_property(property_keys.prototype,
            EsPropertyDescriptor(false, false, true, EsValue::from_obj(proto)));    // VERIFIED: 15.3.5.2
    }

    if (strict_)
    {
        EsFunction *thrower = es_throw_type_err(); // [[ThrowTypeError]]
        
        define_new_own_property(property_keys.caller,
            EsPropertyDescriptor(false, false,
                                 EsValue::from_obj(thrower),
                                 EsValue::from_obj(thrower)));
        
        define_new_own_property(property_keys.arguments,
            EsPropertyDescriptor(false, false,
                                 EsValue::from_obj(thrower),
                                 EsValue::from_obj(thrower)));
    }
}

void EsFunction::make_proto()
{
    prototype_ = es_proto_obj();
    class_ = _USTR("Function");
    extensible_ = true;

    // 15.3.3.1
    define_new_own_property(property_keys.length,
        EsPropertyDescriptor(false, false, false, EsValue::from_u32(0)));   // VERIFIED: 15.3.4
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_fun_proto_to_str, 0);
    ES_DEFINE_PROP_FUN(property_keys.apply, es_std_fun_proto_apply, 2);
    ES_DEFINE_PROP_FUN(property_keys.call, es_std_fun_proto_call, 1);
    ES_DEFINE_PROP_FUN(property_keys.bind, es_std_fun_proto_bind, 1);
}

EsFunction *EsFunction::create_raw(bool strict)
{
    return new (GC)EsFunction(NULL, NULL, strict, 0, false);
}

EsFunction *EsFunction::create_inst(EsLexicalEnvironment *scope,
                                    NativeFunction fun, bool strict,
                                    uint32_t len)
{
    EsFunction *f = new (GC)EsFunction(scope, fun, strict, len, true);
    f->make_inst(true);
    
    return f;
}

EsFunction *EsFunction::create_inst(EsLexicalEnvironment *scope,
                                    FunctionLiteral *code)
{
    EsFunction *f = new (GC)EsFunction(scope, code, true);
    f->make_inst(true);
    return f;
}

EsFunction *EsFunction::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsFunctionConstructor::create_inst();
    
    return default_constr_;
}

bool EsFunction::callT(EsCallFrame &frame, int flags)
{
    // FIXME: What about fast calls, where this step is not needed?
    EsFunctionContext ctx(strict_, scope_);

    // Invoke the function code.
    if (fun_)
    {
        return fun_(EsContextStack::instance().top(),
                    frame.argc(), frame.fp(), frame.vp());
    }
    else if (code_)
    {
        Evaluator eval(code_, Evaluator::TYPE_FUNCTION, frame);
        return eval.exec(EsContextStack::instance().top());
    }

    return true;
}

bool EsFunction::getT(EsPropertyKey p, EsPropertyReference &prop)
{
    if (strict_ && p == property_keys.caller)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_PROP_CALLER));
        return false;
    }

    return EsObject::getT(p, prop);
}

bool EsFunction::constructT(EsCallFrame &frame)
{
    // 13.2.2
    EsObject *obj = NULL;

    EsValue proto;
    if (!EsObject::getT(property_keys.prototype, proto))
        return false;

    if (proto.is_object())
        obj = EsObject::create_inst_with_prototype(proto.as_object());
    else
        obj = EsObject::create_inst_with_prototype(es_proto_obj());
    
    frame.set_this_value(EsValue::from_obj(obj));
    if (!callT(frame))
        return false;

    // FIXME:
    EsValue constr_result = frame.result();
    frame.set_result(EsValue::from_obj(constr_result.is_object() ?
                                       constr_result.as_object() : obj));
    return true;
}

bool EsFunction::has_instanceT(const EsValue &v, bool &result)
{
    // 15.3.5.3
    if (!v.is_object())
    {
        result = false;
        return true;
    }

    EsValue o;
    EsObject::getT(property_keys.prototype, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_INST_OBJ));
        return false;
    }

    EsObject *v_obj = v.as_object(), *o_obj = o.as_object();
    while (true)
    {
        v_obj = v_obj->prototype_;
        if (!v_obj)
        {
            result = false;
            return true;
        }
        
        if (o_obj == v_obj)
        {
            result = true;
            return true;
        }
    }

    assert(false);

    result = false;
    return true;
}

EsBuiltinFunction::EsBuiltinFunction(EsLexicalEnvironment *scope,
                                     NativeFunction fun, bool strict,
                                     uint32_t len, bool needs_this_binding)
    : EsFunction(scope, fun, strict, len, needs_this_binding)
{
}

EsBuiltinFunction::~EsBuiltinFunction()
{
}

EsBuiltinFunction *EsBuiltinFunction::create_inst(
        EsLexicalEnvironment *scope, NativeFunction fun, uint32_t len,
        bool strict)
{
    EsBuiltinFunction *f = new (GC)EsBuiltinFunction(scope, fun, strict, len,
                                                     false);

    // 15: Built-in objects doesn't have a prototype property.
    f->make_inst(false);

    return f;
}

bool EsBuiltinFunction::callT(EsCallFrame &frame, int flags)
{
    EsFunctionContext ctx(strict_, scope_);

    // Invoke the function code.
    assert(fun_);
    return fun_(EsContextStack::instance().top(),
                frame.argc(), frame.fp(), frame.vp());
}

bool EsBuiltinFunction::constructT(EsCallFrame &frame)
{
    // 15: Built-in objects doesn't have [Construct].
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_BUILTIN_CONSTRUCT));
    return false;
}

EsEvalFunction::EsEvalFunction()
    : EsBuiltinFunction(es_global_env(), es_std_eval, false, 1, true)
{
    needs_this_binding_ = true;
}

EsEvalFunction::~EsEvalFunction()
{
}

EsEvalFunction *EsEvalFunction::create_inst()
{
    EsEvalFunction *f = new (GC)EsEvalFunction();

    // 15: Built-in objects doesn't have a prototype property.
    f->make_inst(false);

    return f;
}

bool EsEvalFunction::callT(EsCallFrame &frame, int flags)
{
    bool direct_eval_call = flags & CALL_DIRECT_EVAL;

    // Parse the program.
    EsValue prog_arg = frame.arg(0);

    FunctionLiteral *prog = NULL;

    if (prog_arg.is_string())
    {
        try
        {
            StringStream str(prog_arg.as_string()->str());
            Lexer lexer(str);
            Parser parser(lexer, Parser::CODE_EVAL, direct_eval_call && EsContextStack::instance().top()->is_strict());

            prog = parser.parse();
        }
        catch (ParseException &e)
        {
            switch (e.kind())
            {
                case ParseException::KIND_REFERENCE:
                    ES_THROW(EsReferenceError,
                            EsString::create_from_utf8(e.what().c_str()));
                    return false;

                case ParseException::KIND_SYNTAX:
                    ES_THROW(EsSyntaxError,
                            EsString::create_from_utf8(e.what().c_str()));
                    return false;
            }

            assert(false);
        }
    }

    if (prog == NULL)   // Argument is not of string type.
    {
        frame.set_result(prog_arg);
        return true;
    }

    bool strict = prog->is_strict_mode();

    if (direct_eval_call || strict)
    {
        EsEvalContext ctx(strict);

        EsCallFrame eval_frame = EsCallFrame::push_eval_direct(
            this, frame.this_value());

        Evaluator eval(prog, Evaluator::TYPE_EVAL, eval_frame);
        if (!eval.exec(ctx))
            return false;

        frame.set_result(eval_frame.result());
        return true;
    }
    else
    {
        EsGlobalContext ctx(strict);

        EsCallFrame eval_frame = EsCallFrame::push_eval_indirect(this);

        Evaluator eval(prog, Evaluator::TYPE_EVAL, eval_frame);
        if (!eval.exec(ctx))
            return false;

        frame.set_result(eval_frame.result());
        return true;
    }
}

EsFunctionBind::EsFunctionBind(const EsValue &bound_this, EsFunction *target_fun,
                               const EsValueVector &args)
    : EsFunction(target_fun->scope(), NULL, target_fun->is_strict(),
                 target_fun->length(), false),
      target_fun_(target_fun), bound_this_(bound_this), bound_args_(args)
{
    assert(target_fun_);
}

EsFunctionBind::~EsFunctionBind()
{
}

EsFunctionBind *EsFunctionBind::create_inst(EsFunction *target, const EsValue &bound_this,
                                            const EsValueVector &args)
{
    EsFunctionBind *f = new (GC)EsFunctionBind(bound_this, target, args);
    
    f->prototype_ = es_proto_fun();
    f->class_ = _USTR("Function");
    f->extensible_ = true;

    if (target->class_name() == _USTR("Function"))
    {
        // FIXME: Use .length().
        EsValue target_len;
        static_cast<EsObject *>(target)->getT(property_keys.length, target_len);

        int32_t len = std::max(target_len.primitive_to_int32() - static_cast<int32_t>(args.size()), 0);
        
        f->define_new_own_property(property_keys.length,
            EsPropertyDescriptor(false, false, false, EsValue::from_i32(len)));
    }
    else
    {
        f->define_new_own_property(property_keys.length,
            EsPropertyDescriptor(false, false, false, EsValue::from_u32(0)));
    }
    
    EsFunction *thrower = es_throw_type_err(); // [[ThrowTypeError]]
    
    f->define_new_own_property(property_keys.caller,
        EsPropertyDescriptor(false, false,
                             EsValue::from_obj(thrower),
                             EsValue::from_obj(thrower)));
    
    f->define_new_own_property(property_keys.arguments,
        EsPropertyDescriptor(false, false,
                             EsValue::from_obj(thrower),
                             EsValue::from_obj(thrower)));
    
    return f;
}

bool EsFunctionBind::callT(EsCallFrame &frame, int flags)
{
    assert(target_fun_);

    // FIXME: Consider growing the current stack frame instead.
    EsCallFrame target_frame = EsCallFrame::push_function(
        frame.argc() + bound_args_.size(), target_fun_, bound_this_);

    // FIXME: Replace fp_pos with pointer.
    uint32_t fp_pos = 0;
    for (const EsValue &arg : bound_args_)
        target_frame.fp()[fp_pos++] = arg;
    
    for (uint32_t i = 0; i < frame.argc(); i++)
        target_frame.fp()[fp_pos++] = frame.fp()[i];
    
    if (!target_fun_->callT(target_frame))
        return false;

    frame.set_result(target_frame.result());
    return true;
}

bool EsFunctionBind::constructT(EsCallFrame &frame)
{
    assert(target_fun_);

    // FIXME: Consider growing the current stack frame instead.
    EsCallFrame target_frame = EsCallFrame::push_function(
        frame.argc() + bound_args_.size(), target_fun_, EsValue::undefined);
    
    // FIXME: Replace fp_pos with pointer.
    uint32_t fp_pos = 0;
    for (const EsValue &arg : bound_args_)
        target_frame.fp()[fp_pos++] = arg;
    
    for (uint32_t i = 0; i < frame.argc(); i++)
        target_frame.fp()[fp_pos++] = frame.fp()[i];
    
    if (!target_fun_->constructT(target_frame))
        return false;

    frame.set_result(target_frame.result());
    return true;
}

bool EsFunctionBind::has_instanceT(const EsValue &v, bool &result)
{
    assert(target_fun_);
    return target_fun_->has_instanceT(v, result);
}

EsRegExp::MatchResult::MatchResult(const char *subject, int *ptr, int count)
    : end_index_(0)
{
    for (int i = 0; i < count; i++)
    {
        int start = ptr[2 * i];
        int end = ptr[2 * i + 1];

        if (start == -1 || end == -1)
        {
            matches_.push_back(MatchState());
        }
        else
        {
            const char *substr_start = subject + start;
            int substr_length = end - start;

            end_index_ = std::max(end_index_, end);
            matches_.push_back(MatchState(
                    start, substr_length, EsString::create_from_utf8(
                            substr_start, substr_length)));
        }
    }
}

EsFunction *EsRegExp::default_constr_ = NULL;

EsRegExp::EsRegExp(const EsString *pattern, bool global, bool ignore_case,
                   bool multiline)
    : pattern_(pattern)
    , global_(global)
    , ignore_case_(ignore_case)
    , multiline_(multiline)
    , re_(NULL)
    , re_out_ptr_(NULL)
    , re_out_len_(0)
    , re_capt_cnt_(0)
{
}

EsRegExp::~EsRegExp()
{
}

bool EsRegExp::compile()
{
    assert(!re_);

    // Compile expression.
    const char *err = NULL;
    int err_off = 0;

    int flags = PCRE_JAVASCRIPT_COMPAT | PCRE_UTF8 | PCRE_NO_UTF8_CHECK;
    if (ignore_case_)
        flags |= PCRE_CASELESS;
    if (multiline_)
        flags |= PCRE_MULTILINE;

    // FIXME: That RegExp.exec algorithm in the specification assumes anchored
    // matching. It would probably be better to allow PCRE to do as it wish
    // without being tied to the way it's written in ECMA-262.
    flags |= PCRE_ANCHORED;

    re_ = pcre_compile(pattern_->utf8().c_str(), flags, &err, &err_off, NULL);
    if (re_ == NULL)
    {
        ES_THROW(EsSyntaxError, es_fmt_msg(
                ES_MSG_SYNTAX_REGEXP_COMPILE, err_off, err));
        return false;
    }

    // Find out how many capturing sub-patterns there are.
    re_capt_cnt_ = 0;
    if (pcre_fullinfo(re_, NULL, PCRE_INFO_CAPTURECOUNT, &re_capt_cnt_) != 0)
    {
        ES_THROW(EsSyntaxError, es_fmt_msg(
                ES_MSG_SYNTAX_REGEXP_EXAMINE, pattern_->utf8().c_str()));
        return false;
    }

    re_out_len_ = (re_capt_cnt_ + 1) * 3;
    re_out_ptr_ = static_cast<int *>(GC_MALLOC_ATOMIC(re_out_len_ * sizeof(int)));
    if (!re_out_ptr_)
        THROW(MemoryException);

    // PCRE seems to not always initialize the output buffer.
    for (int i = 0; i < re_out_len_; i++)
        re_out_ptr_[i] = -1;

    return true;
}

void EsRegExp::make_inst()
{
    prototype_ = es_proto_reg_exp();
    class_ = _USTR("RegExp");       // VERIFIED: 15.10.7
    extensible_ = true;
}

void EsRegExp::make_proto()
{
    prototype_ = es_proto_obj();    // VERIFIED: 15.10.6
    class_ = _USTR("RegExp");       // VERIFIED: 15.10.6
    extensible_ = true;

    // NOTE: The RegExp prototype is a RegExp instance in contrast to most
    //       other built-in objects.

    // 15.10.6
    ES_DEFINE_PROP_OBJ(property_keys.constructor, default_constr());
    ES_DEFINE_PROP_FUN(property_keys.exec, es_std_reg_exp_proto_exec, 1);
    ES_DEFINE_PROP_FUN(property_keys.test, es_std_reg_exp_proto_test, 1);
    ES_DEFINE_PROP_FUN(property_keys.to_string, es_std_reg_exp_proto_to_str, 0);

    // 15.10.7
    define_new_own_property(property_keys.source,
            EsPropertyDescriptor(false, false, false,
                    EsValue::from_str(EsString::create())));    // VERIFIED: 15.10.7.1
    define_new_own_property(property_keys.global,
            EsPropertyDescriptor(false, false, false,
                    EsValue::from_bool(false)));                // VERIFIED: 15.10.7.2
    define_new_own_property(property_keys.ignore_case,
            EsPropertyDescriptor(false, false, false,
                    EsValue::from_bool(false)));                // VERIFIED: 15.10.7.3
    define_new_own_property(property_keys.multiline,
            EsPropertyDescriptor(false, false, false,
                    EsValue::from_bool(false)));                // VERIFIED: 15.10.7.4
    define_new_own_property(property_keys.last_index,
            EsPropertyDescriptor(false, false, true,
                    EsValue::from_u32(0)));                     // VERIFIED: 15.10.7.5
}

EsRegExp *EsRegExp::create_raw()
{
    return new (GC)EsRegExp(EsString::create(), false, false, false);
}

EsRegExp *EsRegExp::create_inst(const EsString *pattern, bool global,
                                bool ignore_case, bool multiline)
{
    EsRegExp *r = new (GC)EsRegExp(pattern, global, ignore_case, multiline);
    r->make_inst();

    // 15.10.7
    r->define_new_own_property(property_keys.source,
       EsPropertyDescriptor(false, false, false, EsValue::from_str(r->pattern_)));      // VERIFIED: 15.10.7.1
    r->define_new_own_property(property_keys.global,
       EsPropertyDescriptor(false, false, false, EsValue::from_bool(r->global_)));      // VERIFIED: 15.10.7.2
    r->define_new_own_property(property_keys.ignore_case,
       EsPropertyDescriptor(false, false, false, EsValue::from_bool(r->ignore_case_))); // VERIFIED: 15.10.7.3
    r->define_new_own_property(property_keys.multiline,
       EsPropertyDescriptor(false, false, false, EsValue::from_bool(r->multiline_)));   // VERIFIED: 15.10.7.4
    r->define_new_own_property(property_keys.last_index,
       EsPropertyDescriptor(false, false, true, EsValue::from_u32(0)));                 // VERIFIED: 15.10.7.5

    if (!r->compile())
        return NULL;

    return r;
}

EsRegExp *EsRegExp::create_inst(const EsString *pattern, const EsString *flags)
{
    // Parse the flags.
    bool global = false, ignore_case = false, multiline = false;
    for (size_t i = 0; i < flags->length(); i++)
    {
        switch (flags->at(i))
        {
            case 'g':
                if (global)
                {
                    ES_THROW(EsSyntaxError, es_fmt_msg(
                            ES_MSG_SYNTAX_REGEXP_ILLEGAL_FLAG, (char)flags->at(i)));
                    return NULL;
                }
                global = true;
                break;
            case 'i':
                if (ignore_case)
                {
                    ES_THROW(EsSyntaxError, es_fmt_msg(
                            ES_MSG_SYNTAX_REGEXP_ILLEGAL_FLAG, (char)flags->at(i)));
                    return NULL;
                }
                ignore_case = true;
                break;
            case 'm':
                if (multiline)
                {
                    ES_THROW(EsSyntaxError, es_fmt_msg(
                            ES_MSG_SYNTAX_REGEXP_ILLEGAL_FLAG, (char)flags->at(i)));
                    return NULL;
                }
                multiline = true;
                break;
            default:
                ES_THROW(EsSyntaxError, es_fmt_msg(
                        ES_MSG_SYNTAX_REGEXP_ILLEGAL_FLAG, (char)flags->at(i)));
                return NULL;
        }
    }

    return create_inst(pattern, global, ignore_case, multiline);
}

const EsString *EsRegExp::flags() const
{
    // FIXME: This only works because the properties are read-only.
    EsStringBuilder sb;
    if (global_)
        sb.append('g');
    if (ignore_case_)
        sb.append('i');
    if (multiline_)
        sb.append('m');

    return sb.string();
}

EsRegExp::MatchResult *EsRegExp::match(const EsString *subject, int offset)
{
    assert(re_);
    if (re_ == NULL)
        return NULL;

    std::string utf8_subject = subject->utf8();
    size_t utf8_offset = utf8_off(reinterpret_cast<const byte *>(utf8_subject.c_str()),
                                  utf8_subject.size(), offset);

    int rc = pcre_exec(re_, NULL, utf8_subject.c_str(), utf8_subject.size(), utf8_offset, PCRE_NO_UTF8_CHECK, re_out_ptr_, re_out_len_);
    assert(rc != 0);    // re_out_ptr_ should be big enough.

    if (rc == PCRE_ERROR_NOMATCH)
        return NULL;    // No match.

    if (rc < 0)
    {
        assert(false);
        return NULL;
    }

    return new (GC)MatchResult(utf8_subject.c_str(), re_out_ptr_, re_capt_cnt_ + 1);
}

EsFunction *EsRegExp::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsRegExpConstructor::create_inst();
    
    return default_constr_;
}

EsArrayConstructor::EsArrayConstructor(EsLexicalEnvironment *scope,
                                       NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

EsFunction *EsArrayConstructor::create_inst()
{
    EsArrayConstructor *f = new (GC)EsArrayConstructor(es_global_env(), es_std_arr, false);
    
    f->prototype_ = es_proto_fun();    // VERIFIED: 15.4.3
    f->class_ = _USTR("Function");
    f->extensible_ = true;
    
    // 15.4.3
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(1)));                // VERIFIED: 15.4.3
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_arr())));   // VERIFIED: 15.4.3.1
    f->define_new_own_property(property_keys.is_array,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_arr_constr_is_arr, 1))));   // VERIFIED: 15.4.3.2
    
    return f;
}

bool EsArrayConstructor::constructT(EsCallFrame &frame)
{
    uint32_t argc = frame.argc();
    EsValue *argv = frame.fp();

    EsArray *a = NULL;

    if (argc == 1)
    {
        const EsValue &val = argv[0];
        if (val.is_number())
        {
            double len = val.as_number();
            uint32_t ilen = es_to_uint32(len);
            if (static_cast<double>(ilen) == len)
            {
                a = EsArray::create_inst(len);
            }
            else
            {
                ES_THROW(EsRangeError, es_fmt_msg(
                        ES_MSG_RANGE_INVALID_ARRAY,
                        es_num_to_str(len)->utf8().c_str()));
                return false;
            }
        }
        else
        {
            a = EsArray::create_inst(1);
            a->define_new_own_property(EsPropertyKey::from_u32(0),
                                       EsPropertyDescriptor(true, true, true, argv[0]));    // VERIFIED: 15.4.2.2
        }
    }
    else    // 0 or more than 1 argument(s).
    {
        a = EsArray::create_inst(static_cast<uint32_t>(argc));

        for (uint32_t i = 0; i < argc; i++)
        {
            if (argv[i].is_nothing())   // FIXME: This should never occur, nothing literals are only provided through array literals.
                continue;

            a->define_new_own_property(EsPropertyKey::from_u32(i), EsPropertyDescriptor(true, true, true, argv[i]));
        }
    }

    assert(a);
    frame.set_result(EsValue::from_obj(a));
    return true;
}

EsBooleanConstructor::EsBooleanConstructor(EsLexicalEnvironment *scope,
                                           NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

EsFunction *EsBooleanConstructor::create_inst()
{
    EsBooleanConstructor *f = new (GC)EsBooleanConstructor(es_global_env(), es_std_bool, false);
    
    f->prototype_ = es_proto_fun();    // VERIFIED: 15.6.3
    f->class_ = _USTR("Boolean");
    f->extensible_ = true;
    
    // 15.6.3
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(1)));                // VERIFIED: 15.6.3
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_bool())));  // VERIFIED: 15.6.3.1
    
    return f;
}

bool EsBooleanConstructor::constructT(EsCallFrame &frame)
{
    bool value = false;
    if (frame.argc() > 0)
        value = frame.fp()[0].to_boolean();
    
    frame.set_result(EsValue::from_obj(EsBooleanObject::create_inst(value)));
    return true;
}

EsDateConstructor::EsDateConstructor(EsLexicalEnvironment *scope,
                                     NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 7, false)
{
}

EsFunction *EsDateConstructor::create_inst()
{
    EsDateConstructor *f = new (GC)EsDateConstructor(es_global_env(), es_std_date, false);
    
    f->prototype_ = es_proto_fun();     // VERIFIED: 15.9.4
    f->class_ = _USTR("Date");
    f->extensible_ = true;
    
    // 15.9.4
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(7)));                // VERIFIED: 15.9.4
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_date())));  // VERIFIED: 15.9.4.1
    f->define_new_own_property(property_keys.parse,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_date_constr_parse, 1))));   // VERIFIED: 15.9.4.2
    f->define_new_own_property(property_keys.utc,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_date_constr_utc, 7))));     // VERIFIED: 15.9.4.3
    f->define_new_own_property(property_keys.now,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_date_constr_now, 0))));     // VERIFIED: 15.9.4.4
    
    return f;
}

bool EsDateConstructor::constructT(EsCallFrame &frame)
{
    uint32_t argc = frame.argc();
    EsValue *argv = frame.fp();

    EsValue result;
    if (argc == 0)
    {
        if (!EsDate::create_inst(result))              // 15.9.3.1
            return false;
    }
    else if (argc == 1)
    {
        if (!EsDate::create_inst(argv[0], result))     // 15.9.3.2
            return false;
    }
    else
    {
        if (!EsDate::create_inst(argv[0],                       // Year.
                                 argv[1],                       // Month.
                                 argc > 2 ? &argv[2] : NULL,    // Date (optional).
                                 argc > 3 ? &argv[3] : NULL,    // Hours (optional).
                                 argc > 4 ? &argv[4] : NULL,    // Minutes (optional).
                                 argc > 5 ? &argv[5] : NULL,    // Seconds (optional).
                                 argc > 6 ? &argv[6] : NULL,    // Milliseconds (optional).
                                 result))
            return false;
    }

    frame.set_result(result);
    return true;
}

EsNumberConstructor::EsNumberConstructor(EsLexicalEnvironment *scope,
                                         NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

EsFunction *EsNumberConstructor::create_inst()
{
    EsNumberConstructor *f = new (GC)EsNumberConstructor(es_global_env(), es_std_num, false);
    
    f->prototype_ = es_proto_fun();     // VERIFIED: 15.3.4
    f->class_ = _USTR("Number");
    f->extensible_ = true;
    
    // 15.7.3
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(1)));                    // VERIFIED: 15.7.3
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_num())));       // VERIFIED: 15.7.3.1
    f->define_new_own_property(property_keys.max_value,
       EsPropertyDescriptor(false, false, false,
                            EsValue::from_num(std::numeric_limits<double>::max())));        // VERIFIED: 15.7.3.2
    f->define_new_own_property(property_keys.min_value,
       EsPropertyDescriptor(false, false, false, EsValue::from_num(ES_DOUBLE_MIN)));        // VERIFIED: 15.7.3.3
    f->define_new_own_property(property_keys.nan,
       EsPropertyDescriptor(false, false, false,
                            EsValue::from_num(std::numeric_limits<double>::quiet_NaN())));  // VERIFIED: 15.7.3.4
    f->define_new_own_property(property_keys.negative_infinity,
       EsPropertyDescriptor(false, false, false,
                            EsValue::from_num(-std::numeric_limits<double>::infinity())));  // VERIFIED: 15.7.3.5
    f->define_new_own_property(property_keys.positive_infinity,
       EsPropertyDescriptor(false, false, false,
                            EsValue::from_num(std::numeric_limits<double>::infinity())));   // VERIFIED: 15.7.3.6
    
    return f;
}

bool EsNumberConstructor::constructT(EsCallFrame &frame)
{
    double value = 0.0;
    if (frame.argc() > 0 && !frame.fp()[0].to_numberT(value))
        return false;
    
    frame.set_result(EsValue::from_obj(EsNumberObject::create_inst(value)));
    return true;
}

EsFunctionConstructor::EsFunctionConstructor(EsLexicalEnvironment *scope,
                                             NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

EsFunction *EsFunctionConstructor::create_inst()
{
    EsFunctionConstructor *f = new (GC)EsFunctionConstructor(es_global_env(), es_std_fun, false);
    
    f->prototype_ = es_proto_fun();
    f->class_ = _USTR("Function");
    f->extensible_ = true;
    
    // 15.3.3
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_fun())));   // VERIFIED: 15.3.3.1
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(1)));                // VERIFIED: 15.3.3.2
    
    return f;
}

bool EsFunctionConstructor::constructT(EsCallFrame &frame)
{
    uint32_t argc = frame.argc();
    EsValue *argv = frame.fp();
    // 15.3.2.1
    EsValue body = argc == 0
        ? EsValue::from_str(EsString::create())
        : argv[argc - 1];

    // Concatenate all arguments to a string for parsing later.
    EsStringBuilder p;
    if (argc > 1)
    {
        // Concatenate all arguments to a string and then parse it as a formal
        // parameter list.
        p.append("function $(");
        for (uint32_t i = 0; i < argc - 1; i++)
        {
            if (i > 0)
                p.append(',');

            const EsString *str = argv[i].to_stringT();
            if (!str)
                return false;

            p.append(str);
        }
        p.append(") {}");
    }

    const EsString *body_str = body.to_stringT();
    if (!body_str)
        return false;

    // Parse the body.
    FunctionLiteral *prog = NULL;
    try
    {
        StringStream str(body_str->str());
        Lexer lexer(str);
        Parser parser(lexer, Parser::CODE_FUNCTION);

        prog = parser.parse();
    }
    catch (ParseException &e)
    {
        ES_THROW(EsSyntaxError, EsString::create_from_utf8(e.what().c_str()));
        return false;
    }

    // Parse the parameters.
    if (argc > 1)
    {
        // The way we parse the formal parameter list is by putting it into a
        // function declaration and then parse it using the standard parser.
        StringStream str(p.string()->str());
        Lexer lexer(str);
        Parser parser(lexer, Parser::CODE_PROGRAM, prog->is_strict_mode());

        try
        {
            FunctionLiteral *root = parser.parse();
            assert(!root->declarations().empty());
            if (root->declarations().empty())
            {
                ES_THROW(EsSyntaxError, es_fmt_msg(ES_MSG_SYNTAX_FUN_PARAM));
                return false;
            }

            FunctionLiteral *fun = safe_cast<FunctionLiteral *>(root->declarations()[0]);
            if (!fun)
            {
                ES_THROW(EsSyntaxError, es_fmt_msg(ES_MSG_SYNTAX_FUN_PARAM));
                return false;
            }

            const StringVector &params = fun->parameters();

            for (StringVector::const_iterator it = params.begin(); it != params.end(); ++it)
                prog->push_param(*it);
        }
        catch (ParseException &e)
        {
            ES_THROW(EsSyntaxError, es_fmt_msg(ES_MSG_SYNTAX_FUN_PARAM));
            return false;
        }
    }

    frame.set_result(EsValue::from_obj(EsFunction::create_inst(
        EsContextStack::instance().top()->var_env(), prog)));
    return true;
}

EsObjectConstructor::EsObjectConstructor(EsLexicalEnvironment *scope,
                                         NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

EsFunction *EsObjectConstructor::create_inst()
{
    EsObjectConstructor *f = new (GC)EsObjectConstructor(es_global_env(), es_std_obj, false);
    
    f->prototype_ = es_proto_fun();
    f->class_ = _USTR("Object");
    f->extensible_ = true;
    
    // 15.2.3
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_obj())));
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(1)));        // VERIFIED: 15.2.3
    f->define_new_own_property(property_keys.get_prototype_of,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_get_proto_of, 1))));
    f->define_new_own_property(property_keys.get_own_property_descriptor,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_get_own_prop_desc, 2))));
    f->define_new_own_property(property_keys.get_own_property_names,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_get_own_prop_names, 1))));
    f->define_new_own_property(property_keys.create,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_create, 2))));
    f->define_new_own_property(property_keys.define_property,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_def_prop, 3))));
    f->define_new_own_property(property_keys.define_properties,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_def_props, 2))));
    f->define_new_own_property(property_keys.seal,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_seal, 1))));
    f->define_new_own_property(property_keys.freeze,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_freeze, 1))));
    f->define_new_own_property(property_keys.prevent_extensions,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_prevent_exts, 1))));
    f->define_new_own_property(property_keys.is_sealed,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_is_sealed, 1))));
    f->define_new_own_property(property_keys.is_frozen,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_is_frozen, 1))));
    f->define_new_own_property(property_keys.is_extensible,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_is_extensible, 1))));
    f->define_new_own_property(property_keys.keys,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_obj_keys, 1))));
    
    return f;
}

bool EsObjectConstructor::constructT(EsCallFrame &frame)
{
    uint32_t argc = frame.argc();
    EsValue *argv = frame.fp();

    if (argc > 0)
    {
        const EsValue &val = argv[0];
        if (val.is_object())
        {
            frame.set_result(EsValue::from_obj(val.as_object()));
            return true;
        }
        
        if (val.is_boolean() || val.is_number() || val.is_string())
        {
            // Will never throw given the if expression above.
            frame.set_result(EsValue::from_obj(val.to_objectT()));
            return true;
        }
    }

    frame.set_result(EsValue::from_obj(EsObject::create_inst()));
    return true;
}

EsStringConstructor::EsStringConstructor(EsLexicalEnvironment *scope,
                                         NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

EsFunction *EsStringConstructor::create_inst()
{
    EsStringConstructor *f = new (GC)EsStringConstructor(es_global_env(), es_std_str, false);
    
    f->prototype_ = es_proto_fun();     // VERIFIED: 15.5.3
    f->class_ = _USTR("String");
    f->extensible_ = true;
    
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_str())));   // VERIFIED: 15.5.3.1
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(1)));                // VERIFIED: 15.5.3
    f->define_new_own_property(property_keys.from_char_code,
       EsPropertyDescriptor(false, true, true,
                            EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                             es_std_str_from_char_code, 1))));  // VERIFIED: 15.5.3.2
    
    return f;
}

bool EsStringConstructor::constructT(EsCallFrame &frame)
{
    const EsString *value = EsString::create();

    if (frame.argc() > 0)
    {
        value = frame.fp()[0].to_stringT();
        if (!value)
            return false;
    }
    
    frame.set_result(EsValue::from_obj(EsStringObject::create_inst(value)));
    return true;
}

EsRegExpConstructor::EsRegExpConstructor(EsLexicalEnvironment *scope,
                                         NativeFunction fun, bool strict)
    : EsFunction(scope, fun, strict, 2, false)
{
}

EsFunction *EsRegExpConstructor::create_inst()
{
    EsRegExpConstructor *f = new (GC)EsRegExpConstructor(es_global_env(), es_std_reg_exp, false);
    
    f->prototype_ = es_proto_fun();     // VERIFIED: 15.10.5.1
    f->class_ = _USTR("RegExp");
    f->extensible_ = true;
    
    f->define_new_own_property(property_keys.prototype,
       EsPropertyDescriptor(false, false, false, EsValue::from_obj(es_proto_reg_exp())));   // VERIFIED: 15.10.5.1
    f->define_new_own_property(property_keys.length,
       EsPropertyDescriptor(false, false, false, EsValue::from_u32(2)));                    // VERIFIED: 15.10.5
    
    return f;
}

bool EsRegExpConstructor::constructT(EsCallFrame &frame)
{
    uint32_t argc = frame.argc();
    EsValue *argv = frame.fp();

    EsValue pattern_arg = argc >= 1 ? argv[0] : EsValue::undefined;
    EsValue flags_arg = argc >= 2 ? argv[1] : EsValue::undefined;

    const EsString *pattern = EsString::create(), *flags = EsString::create();

    // FIXME: es_as_regexp
    if (pattern_arg.is_object())
    {
        EsObject *o = pattern_arg.as_object();
        if (o->class_name() == _USTR("RegExp"))
        {
            if (flags_arg.is_undefined())
            {
                EsRegExp *re = safe_cast<EsRegExp *>(o);
                pattern = re->pattern();
                flags = re->flags();
            }
            else
            {
                ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_REGEXP_FLAGS));
                return false;
            }
        }
        else
        {
            if (!pattern_arg.is_undefined())
            {
                pattern = pattern_arg.to_stringT();
                if (!pattern)
                    return false;
            }

            if (!flags_arg.is_undefined())
            {
                flags = flags_arg.to_stringT();
                if (!flags)
                    return false;
            }
        }
    }
    else
    {
        if (!pattern_arg.is_undefined())
        {
            pattern = pattern_arg.to_stringT();
            if (!pattern)
                return false;
        }

        if (!flags_arg.is_undefined())
        {
            flags = flags_arg.to_stringT();
            if (!flags)
                return false;
        }
    }
    
    assert(pattern);
    assert(flags);
    EsRegExp *obj = EsRegExp::create_inst(pattern, flags);
    if (obj == NULL)
        return false;

    frame.set_result(EsValue::from_obj(obj));
    return true;
}

ES_API_FUN(es_std_dummy)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    return true;
}

EsArgumentGetter::EsArgumentGetter(EsValue *val)
    : EsFunction(NULL, es_std_dummy, false, 0, false)
    , val_(val)
{
}

EsArgumentGetter *EsArgumentGetter::create_inst(EsValue *val)
{
    EsArgumentGetter *f = new (GC)EsArgumentGetter(val);

    // FIXME: This isn't nice.
    f->make_inst(true);

    return f;
}

bool EsArgumentGetter::callT(EsCallFrame &frame, int flags)
{
    frame.set_result(*val_);
    return true;
}

EsArgumentSetter::EsArgumentSetter(EsValue *val)
    : EsFunction(NULL, es_std_dummy, false, 1, false)
    , val_(val)
{
}

EsArgumentSetter *EsArgumentSetter::create_inst(EsValue *val)
{
    EsArgumentSetter *f = new (GC)EsArgumentSetter(val);

    // FIXME: This isn't nice.
    f->make_inst(true);

    return f;
}

bool EsArgumentSetter::callT(EsCallFrame &frame, int flags)
{
    assert(frame.argc() > 0);
    *val_ = frame.arg(0);
    return true;
}
