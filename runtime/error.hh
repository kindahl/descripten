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

#pragma once
#include <sstream>
#include <string>
#include <gc/gc_cpp.h>
#include "common/core.hh"
#include "common/exception.hh"
#include "common/stringbuilder.hh"
#include "context.hh"
#include "object.hh"

#ifdef DEBUG
#  ifndef ES_THROW
#    define ES_THROW(type, msg) es_throw<type>(__FILE__, __LINE__, (msg))
#  endif
#else
#  ifndef ES_THROW
#    define ES_THROW(type, msg) es_throw<type>((msg))
#  endif
#endif

template <typename T>
#ifdef DEBUG
void es_throw(const char *file, int line, const String &orig_message)
{
    String message = StringBuilder::sprintf("[%s:%d] %S", file, line, orig_message.data());
#else
inline void es_throw(const String &message)
{
#endif
    EsValue e = EsValue::from_obj(T::create_inst(message));
    
    EsContextStack::instance().top()->set_pending_exception(e);
}

/**
 * @brief Native error class.
 */
class EsError : public EsObject
{
public:
    static EsFunction::NativeFunction default_fun_;  ///< Function to call when calling constructor as a function.
    
    static EsObject *prototype();
    
private:
    String name_;
    String message_;
    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.
    
    EsError();
    EsError(const String &message);
    
protected:
    EsError(const String &name, const String &message);
    
public:
    virtual ~EsError();
    
    static EsError *create_raw();
    static EsError *create_inst(const String &message);
    
    const String &name() const { return name_; }
    const String &message() const { return message_; }
    
    /**
     * @return Default error constructor.
     */
    static EsFunction *default_constr();
    
    /**
     * Turns the object into an error prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;
};

/**
 * @brief Native error class.
 */
template <typename T>
class EsNativeError : public EsError
{
public:
    static EsFunction::NativeFunction default_fun_;  ///< Function to call when calling constructor as a function.

private:    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.
    
protected:
    EsNativeError(const String &name, const String &message);
    
public:
    virtual ~EsNativeError();
    
    static T *create_raw();
    static T *create_inst(const String &message);
    
    /**
     * @return Default native error constructor.
     */
    static EsFunction *default_constr();
    
    /**
     * Turns the object into a native error prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;
};

/**
 * @brief Evaluation error class.
 */
class EsEvalError : public EsNativeError<EsEvalError>
{
public:
    static EsObject *prototype();
    
public:
    EsEvalError(const String &message) : EsNativeError<EsEvalError>(_USTR("EvalError"), message) {}
};

/**
 * @brief Range error class.
 */
class EsRangeError : public EsNativeError<EsRangeError>
{
public:
    static EsObject *prototype();
    
public:
    EsRangeError(const String &message) : EsNativeError<EsRangeError>(_USTR("RangeError"), message) {}
};

/**
 * @brief Reference error class.
 */
class EsReferenceError : public EsNativeError<EsReferenceError>
{
public:
    static EsObject *prototype();
    
public:
    EsReferenceError(const String &message) : EsNativeError<EsReferenceError>(_USTR("ReferenceError"), message) {}
};

/**
 * @brief Syntax error class.
 */
class EsSyntaxError : public EsNativeError<EsSyntaxError>
{
public:
    static EsObject *prototype();
    
public:
    EsSyntaxError(const String &message) : EsNativeError<EsSyntaxError>(_USTR("SyntaxError"), message) {}
};

/**
 * @brief Type error class.
 */
class EsTypeError : public EsNativeError<EsTypeError>
{
public:
    static EsObject *prototype();
    
public:
    EsTypeError(const String &message) : EsNativeError<EsTypeError>(_USTR("TypeError"), message) {}
};

/**
 * @brief URI error class.
 */
class EsUriError : public EsNativeError<EsUriError>
{
public:
    static EsObject *prototype();
    
public:
    EsUriError(const String &message) : EsNativeError<EsUriError>(_USTR("URIError"), message) {}
};

/**
 * @brief Error constructor class.
 */
template <typename T>
class EsErrorConstructor : public EsFunction
{
private:
    EsErrorConstructor(EsLexicalEnvironment *scope, NativeFunction func, int len, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::construct
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};
