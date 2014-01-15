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

#include "conversion.hh"
#include "error.hh"
#include "frame.hh"
#include "global.hh"
#include "property.hh"
#include "prototype.hh"
#include "standard.hh"
#include "utility.hh"

EsFunction::NativeFunction EsError::default_fun_ = es_std_err;

EsFunction *EsError::default_constr_ = NULL;

EsObject *EsError::prototype()
{
    return es_proto_err();
}

EsError::EsError()
    : name_(_ESTR("Error"))     // VERIFIED: 15.11.4.2
{
}

EsError::EsError(const EsString *message)
    : name_(_ESTR("Error"))
    , message_(message)
{
}

EsError::EsError(const EsString *name, const EsString *message)
    : name_(name)
    , message_(message)
{
}

EsError::~EsError()
{
}

void EsError::make_proto()
{
    prototype_ = es_proto_obj();    // VERIFIED: 15.11.4
    class_ = _USTR("Error");        // VERIFIED: 15.11.4
    extensible_ = true;
    
    // 15.11.4
    define_new_own_property(property_keys.constructor,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_obj(default_constr()))); // VERIFIED: 15.11.4.1
    define_new_own_property(property_keys.name,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_str(_ESTR("Error"))));   // VERIFIED: 15.11.4.2
    define_new_own_property(property_keys.message,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_str(EsString::create())));   // VERIFIED: 15.11.4.3
    define_new_own_property(property_keys.to_string,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_obj(EsBuiltinFunction::create_inst(es_global_env(),
                                                                   es_std_err_proto_to_str, 0))));      // VERIFIED: 15.11.4.4
}

EsError *EsError::create_raw()
{
    return new (GC)EsError();
}

EsError *EsError::create_inst(const EsString *message)
{
    EsError *e = new (GC)EsError(message);
    
    e->prototype_ = es_proto_err(); // VERIFIED: 15.11.5
    e->class_ = _USTR("Error");     // VERIFIED: 15.11.5
    e->extensible_ = true;
    
    if (!message->empty())
    {
        e->define_new_own_property(property_keys.message,
                                   EsPropertyDescriptor(false, true, true,
                                                        EsValue::from_str(message)));
    }
    
    return e;
}

EsFunction *EsError::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsErrorConstructor<EsError>::create_inst();
     
    return default_constr_;
}

template <typename T>
EsFunction *EsNativeError<T>::default_constr_ = NULL;

template <typename T>
EsNativeError<T>::EsNativeError(const EsString *name, const EsString *message)
    : EsError(name, message)
{
}

template <typename T>
EsNativeError<T>::~EsNativeError()
{
}

template <typename T>
void EsNativeError<T>::make_proto()
{
    prototype_ = es_proto_err();    // VERIFIED: 15.11.7.7
    class_ = _USTR("Error");        // VERIFIED: 15.11.7.7
    extensible_ = true;
    
    // 15.11.7
    define_new_own_property(property_keys.constructor,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_obj(default_constr()))); // VERIFIED: 15.11.7.8
    define_new_own_property(property_keys.name,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_str(name())));           // VERIFIED: 15.11.7.9
    define_new_own_property(property_keys.message,
                            EsPropertyDescriptor(false, true, true,
                                                 EsValue::from_str(EsString::create())));   // VERIFIED: 15.11.7.10
}

template <typename T>
T *EsNativeError<T>::create_raw()
{
    return new (GC)T(EsString::create());
}

template <typename T>
T *EsNativeError<T>::create_inst(const EsString *message)
{
    T *e = new (GC)T(message);
    
    e->prototype_ = T::prototype(); // VERIFIED: 15.11.7.2
    e->class_ = _USTR("Error");     // VERIFIED: 15.11.7.2
    e->extensible_ = true;          // VERIFIED: 15.11.7.2

    if (!message->empty())
    {
        e->define_new_own_property(property_keys.message,
                                   EsPropertyDescriptor(false, true, true,
                                                        EsValue::from_str(message)));
    }
    
    return e;
}

template <typename T>
EsFunction *EsNativeError<T>::default_constr()
{
    if (default_constr_ == NULL)
        default_constr_ = EsErrorConstructor<T>::create_inst();
    
    return default_constr_;
}

template <typename T>
EsErrorConstructor<T>::EsErrorConstructor(EsLexicalEnvironment *scope,
                                          NativeFunction fun, int len, bool strict)
    : EsFunction(scope, fun, strict, 1, false)
{
}

template <typename T>
EsFunction *EsErrorConstructor<T>::create_inst()
{
    EsErrorConstructor *f = new (GC)EsErrorConstructor(es_global_env(), T::default_fun_, 1, false);

    f->prototype_ = es_proto_fun();     // VERIFIED: 15.11.7.5
    f->class_ = _USTR("Function");
    f->extensible_ = true;
    
    // 15.11.7
    f->define_new_own_property(property_keys.length,
                               EsPropertyDescriptor(false, false, false,
                                                    EsValue::from_u32(1)));                 // VERIFIED: 15.11.7.5
    f->define_new_own_property(property_keys.prototype,
                               EsPropertyDescriptor(false, false, false,
                                                    EsValue::from_obj(T::prototype())));    // VERIFIED: 15.11.7.6
    
    return f;
}

template <typename T>
bool EsErrorConstructor<T>::constructT(EsCallFrame &frame)
{
    const EsString *msg_str = EsString::create();

    EsValue msg = frame.arg(0);
    if (!msg.is_undefined())
    {
        msg_str = msg.to_stringT();
        if (!msg_str)
            return false;
    }
    
    frame.set_result(EsValue::from_obj(T::create_inst(msg_str)));
    return true;
}

EsObject *EsEvalError::prototype()
{
    return es_proto_eval_err();
}

EsObject *EsRangeError::prototype()
{
    return es_proto_range_err();
}

EsObject *EsReferenceError::prototype()
{
    return es_proto_ref_err();
}

EsObject *EsSyntaxError::prototype()
{
    return es_proto_syntax_err();
}

EsObject *EsTypeError::prototype()
{
    return es_proto_type_err();
}

EsObject *EsUriError::prototype()
{
    return es_proto_uri_err();
}

// Template specialization.
template <>
EsFunction::NativeFunction EsNativeError<EsEvalError>::default_fun_ = es_std_eval_err;
template <>
EsFunction::NativeFunction EsNativeError<EsRangeError>::default_fun_ = es_std_range_err;
template <>
EsFunction::NativeFunction EsNativeError<EsReferenceError>::default_fun_ = es_std_ref_err;
template <>
EsFunction::NativeFunction EsNativeError<EsSyntaxError>::default_fun_ = es_std_syntax_err;
template <>
EsFunction::NativeFunction EsNativeError<EsTypeError>::default_fun_ = es_std_type_err;
template <>
EsFunction::NativeFunction EsNativeError<EsUriError>::default_fun_ = es_std_uri_err;

// Explicit template instantiation.
template class EsNativeError<EsEvalError>;
template class EsNativeError<EsRangeError>;
template class EsNativeError<EsReferenceError>;
template class EsNativeError<EsSyntaxError>;
template class EsNativeError<EsTypeError>;
template class EsNativeError<EsUriError>;
