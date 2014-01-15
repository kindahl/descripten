/*
 * descripten - ECMAScript to native compiler
 * Copyright (C) 2011-2013 Christian Kindahl
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
#include <gc_cpp.h>
#include "common/exception.hh"
#include "environment.hh"
#include "error.hh"
#include "messages.hh"
#include "object.hh"
#include "property.hh"
#include "utility.hh"

EsDeclarativeEnvironmentRecord::EsDeclarativeEnvironmentRecord()
    : storage_(NULL)
    , storage_len_(0)
{
}

void EsDeclarativeEnvironmentRecord::link_mutable_binding(const EsPropertyKey &n, bool d,
                                                          EsValue *v, bool inherit)
{
    if (inherit)
    {
        VariableMap::iterator it = variables_.find(n);
        if (it != variables_.end())
            *v = *it->second.val_;
    }

    variables_.insert(std::make_pair(n, Value(v, true, d)));
}

void EsDeclarativeEnvironmentRecord::link_immutable_binding(const EsPropertyKey &n, EsValue *v)
{
    VariableMap::iterator it = variables_.find(n);
    if (it != variables_.end())
        *v = *it->second.val_;

    variables_.insert(std::make_pair(n, Value(v, false, false)));
}

bool EsDeclarativeEnvironmentRecord::has_binding(const EsPropertyKey &n)
{
    return variables_.count(n) > 0;
}

void EsDeclarativeEnvironmentRecord::create_mutable_binding(const EsPropertyKey &n, bool d)
{
    assert(variables_.count(n) == 0);
    variables_.insert(std::make_pair(n, Value(new (GC)EsValue(EsValue::undefined), true, d)));  // FIXME: HEAP
}

void EsDeclarativeEnvironmentRecord::set_mutable_binding(const EsPropertyKey &n, const EsValue &v)
{
    VariableMap::iterator it = variables_.find(n);
    assert(it != variables_.end());

    EsDeclarativeEnvironmentRecord::Value &val = it->second;
    if (val.mutable_)
        *val.val_ = v;
}

bool EsDeclarativeEnvironmentRecord::set_mutable_bindingT(const EsPropertyKey &n, const EsValue &v, bool s)
{
    VariableMap::iterator it = variables_.find(n);
    assert(it != variables_.end());

    EsDeclarativeEnvironmentRecord::Value &val = it->second;
    if (val.mutable_)
    {
        *val.val_ = v;
    }
    else if (s)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NONMUTABLE, n.to_string().utf8().c_str()));
        return false;
    }

    return true;
}

bool EsDeclarativeEnvironmentRecord::get_binding_valueT(const EsPropertyKey &n, bool s, EsValue &v)
{
    VariableMap::iterator it = variables_.find(n);
    assert(it != variables_.end());

    EsDeclarativeEnvironmentRecord::Value &val = it->second;
    if (!val.mutable_ && val.val_->is_undefined())
    {
        if (!s)
        {
            v = EsValue::undefined;
            return true;
        }

        ES_THROW(EsReferenceError, es_fmt_msg(ES_MSG_REF_NOT_DEFINED, n.to_string().utf8().c_str()));
        return false;
    }

    v = *val.val_;
    return true;
}

bool EsDeclarativeEnvironmentRecord::delete_bindingT(const EsPropertyKey &n, bool &deleted)
{
    VariableMap::iterator it = variables_.find(n);
    if (it == variables_.end())
    {
        deleted = true;
        return true;
    }

    EsDeclarativeEnvironmentRecord::Value &val = it->second;
    if (!val.removable_)
    {
        deleted = false;
        return true;
    }

    variables_.erase(n);

    deleted = true;
    return true;
}

EsValue EsDeclarativeEnvironmentRecord::implicit_this_value() const
{
    return EsValue::undefined;
}

void EsDeclarativeEnvironmentRecord::create_immutable_binding(const EsPropertyKey &n,
                                                              const EsValue &v)
{
    assert(variables_.count(n) == 0);
    variables_.insert(std::make_pair(n, Value(new (GC)EsValue(v),
                                              false, false)));  // FIXME: HEAP
}

EsObjectEnvironmentRecord::EsObjectEnvironmentRecord(EsObject *binding_object,
                                                     bool provide_this)
    : provide_this_(provide_this)
    , binding_object_(binding_object)
{
    assert(binding_object_);
}

bool EsObjectEnvironmentRecord::has_binding(const EsPropertyKey &n)
{
    EsObject *bindings = binding_object_;
    assert(bindings);

    return bindings->has_property(n);
}

EsValue EsObjectEnvironmentRecord::implicit_this_value() const
{
    return provide_this_ ? EsValue::from_obj(binding_object_) : EsValue::undefined;
}

EsLexicalEnvironment::EsLexicalEnvironment(EsLexicalEnvironment *outer,
                                           EsEnvironmentRecord *env_rec)
    : outer_(outer), env_rec_(env_rec)
{
}

EsLexicalEnvironment *EsLexicalEnvironment::outer()
{
    return outer_;
}

EsEnvironmentRecord *EsLexicalEnvironment::env_rec()
{
    return env_rec_;
}

EsValue es_get_this_value(EsLexicalEnvironment *lex, const EsPropertyKey &key)
{
    for (; lex; lex = lex->outer())
    {
        EsEnvironmentRecord *env_rec = lex->env_rec();
        if (env_rec->has_binding(key))
            return env_rec->implicit_this_value();
    }

    return EsValue::nothing;
}

EsLexicalEnvironment *es_new_decl_env(EsLexicalEnvironment *e)
{
    return new (GC)EsLexicalEnvironment(e,
        new (GC)EsDeclarativeEnvironmentRecord());
}

EsLexicalEnvironment *es_new_obj_env(EsObject *o, EsLexicalEnvironment *e,
                                     bool provide_this)
{
    return new (GC)EsLexicalEnvironment(e,
        new (GC)EsObjectEnvironmentRecord(o, provide_this));
}
