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

#include <limits>
#include "conversion.hh"
#include "error.hh"
#include "messages.hh"
#include "native.hh"
#include "object.hh"
#include "value.hh"

const EsValue EsValue::null(TYPE_NULL);

const EsValue EsValue::undefined(TYPE_UNDEFINED);

const EsValue EsValue::nothing(TYPE_NOTHING);

bool EsValue::to_primitive(EsTypeHint hint, EsValue &value) const
{
    if (type() == TYPE_OBJECT)
        return as_object()->default_valueT(hint, value);

    assert(!is_object());
    value = *this;
    return true;
}

double EsValue::primitive_to_number() const
{
    switch (type())
    {
        case TYPE_UNDEFINED:
            return std::numeric_limits<double>::quiet_NaN();
        case TYPE_NULL:
            return 0.0;
        case TYPE_BOOLEAN:
            return as_boolean() ? 1.0 : 0.0;
        case TYPE_NUMBER:
            return as_number();
        case TYPE_STRING:
            return es_str_to_num(as_string());
        default:
            assert(false);
            return std::numeric_limits<double>::quiet_NaN();
    }

    assert(false);
    return std::numeric_limits<double>::quiet_NaN();
}

int64_t EsValue::primitive_to_integer() const
{
    double num = primitive_to_number();

    if (std::isnan(num))
        return 0;

    if (!std::isfinite(num))
        return num < 0.0 ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int64_t>::max();

    return static_cast<int64_t>(num);
}

int32_t EsValue::primitive_to_int32() const
{
    return es_to_int32(primitive_to_number());
}

uint32_t EsValue::primitive_to_uint32() const
{
    return es_to_uint32(primitive_to_number());
}

String EsValue::primitive_to_string() const
{
    switch (type())
    {
        case TYPE_UNDEFINED:
            return _USTR("undefined");
        case TYPE_NULL:
            return _USTR("null");
        case TYPE_BOOLEAN:
            return as_boolean() ? _USTR("true") : _USTR("false");
        case TYPE_NUMBER:
            return es_num_to_str(as_number());
        case TYPE_STRING:
            return as_string();
        default:
            assert(false);
            return String();
    }

    assert(false);
    return String();
}

bool EsValue::to_boolean() const
{
    switch (type())
    {
        case TYPE_UNDEFINED:
        case TYPE_NULL:
            return false;
        case TYPE_BOOLEAN:
            return as_boolean();
        case TYPE_NUMBER:
            return as_number() != 0.0 && !std::isnan(as_number());
        case TYPE_STRING:
            return as_string().length() > 0;
        case TYPE_OBJECT:
            return true;
        default:
            assert(false);
            break;
    }

    return false;
}

bool EsValue::to_number(double &result) const
{
    switch (type())
    {
        case TYPE_UNDEFINED:
            result = std::numeric_limits<double>::quiet_NaN();
            return true;
        case TYPE_NULL:
            result = 0.0;
            return true;
        case TYPE_BOOLEAN:
            result = as_boolean() ? 1.0 : 0.0;
            return true;
        case TYPE_NUMBER:
            result = as_number();
            return true;
        case TYPE_STRING:
            result = es_str_to_num(as_string());
            return true;
        case TYPE_OBJECT:
        {
            EsValue v;
            if (!to_primitive(ES_HINT_NUMBER, v))
                return false;

            result = v.primitive_to_number();
            return true;
        }
        default:
            assert(false);
            result = std::numeric_limits<double>::quiet_NaN();
            return true;
    }

    result = std::numeric_limits<double>::quiet_NaN();
    return true;
}

bool EsValue::to_integer(int64_t &result) const
{
    double num = 0.0;
    if (!to_number(num))
        return false;

    if (std::isnan(num))
        result = 0;
    else if (!std::isfinite(num))
        result = num < 0.0 ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int64_t>::max();
    else
        result = static_cast<int64_t>(num);

    return true;
}

bool EsValue::to_int32(int32_t &result) const
{
    double num = 0.0;
    if (!to_number(num))
        return false;

    result = es_to_int32(num);
    return true;
}

bool EsValue::to_uint32(uint32_t &result) const
{
    double num = 0.0;
    if (!to_number(num))
        return false;

    result = es_to_uint32(num);
    return true;
}

bool EsValue::to_string(String &result) const
{
    switch (type())
    {
        case TYPE_UNDEFINED:
            result = _USTR("undefined");
            return true;
        case TYPE_NULL:
            result = _USTR("null");
            return true;
        case TYPE_BOOLEAN:
            result = as_boolean() ? _USTR("true") : _USTR("false");
            return true;
        case TYPE_NUMBER:
            result = es_num_to_str(as_number());
            return true;
        case TYPE_STRING:
            result = as_string();
            return true;
        case TYPE_OBJECT:
        {
            EsValue v;
            if (!to_primitive(ES_HINT_STRING, v))
                return false;

            return v.to_string(result);
        }
        default:
            assert(false);
            result = String();
            return true;
    }

    assert(false);
    result = String();
    return true;
}

EsObject *EsValue::to_objectT() const
{
    switch (type())
    {
        case TYPE_UNDEFINED:
        case TYPE_NULL:
            ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NULL_UNDEF_TO_OBJ));
            return NULL;
        case TYPE_BOOLEAN:
            return EsBooleanObject::create_inst(as_boolean());
        case TYPE_NUMBER:
            return EsNumberObject::create_inst(as_number());
        case TYPE_STRING:
            return EsStringObject::create_inst(as_string());
        case TYPE_OBJECT:
            return as_object();
        default:
            assert(false);
            return NULL;
    }

    return NULL;
}

bool EsValue::chk_obj_coercibleT() const
{
    if (type() == TYPE_UNDEFINED || type() == TYPE_NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_OBJ_TO_PRIMITIVE));
        return false;
    }

    return true;
}

bool EsValue::is_callable() const
{
    if (type() == TYPE_OBJECT)
        return dynamic_cast<EsFunction *>(as_object());

    return false;
}

EsFunction *EsValue::as_function() const
{
    assert(is_object());
    return is_object() ? safe_cast<EsFunction *>(as_object()) : NULL;
}

bool EsValue::operator==(const EsValue &rhs) const
{
    if (type() != rhs.type())
        return false;

    switch (type())
    {
        case TYPE_UNDEFINED:
        case TYPE_NULL:
            return true;
        case TYPE_BOOLEAN:
            return as_boolean() == rhs.as_boolean();
        case TYPE_NUMBER:
        {
            double num = as_number();
            double rhs_num = rhs.as_number();
            return std::isnan(num) ? std::isnan(rhs_num) : num == rhs_num;
        }
        case TYPE_STRING:
            return as_string() == rhs.as_string();
        case TYPE_OBJECT:
            return as_object() == rhs.as_object();
        default:
            assert(false);
            return false;
    }

    return false;
}

bool EsValue::operator!=(const EsValue &rhs) const
{
    return !(*this == rhs);
}
