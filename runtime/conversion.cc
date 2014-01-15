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
#include <cmath>
#include <limits>
#include <sstream>
#include <gc_cpp.h>
#include "common/cast.hh"
#include "common/exception.hh"
#include "common/lexical.hh"
#include "common/stringbuilder.hh"
#include "conversion.hh"
#include "error.hh"
#include "messages.hh"
#include "property.hh"
#include "types.hh"
#include "utility.hh"
#include "value.hh"

double es_str_to_num(const String &str)
{
    // 9.3.1

    // Optimization.
    if (str.length() == 1)
    {
        uni_char c = str[0];
        if (c >= '0' && c <= '9')
            return static_cast<double>(c - '0');

        if (c == 0 || es_is_white_space(c))
            return 0.0;

        return std::numeric_limits<double>::quiet_NaN();
    }

    const uni_char *ptr = str.data();
    const uni_char *end = ptr + str.length();

    es_str_skip_white_spaces(ptr);

    // Parse hexadecimal value.
    if (end - ptr >= 2 && ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
    {
        double val = es_strtou(ptr + 2, &ptr, 16);
        if (std::isnan(val)) // Failure.
            return std::numeric_limits<double>::quiet_NaN();

        es_str_skip_white_spaces(ptr);
        if (ptr != end)
            return std::numeric_limits<double>::quiet_NaN();

        return val;
    }

    // Parse decimal value.
    double val = es_strtod(ptr, &ptr);

    es_str_skip_white_spaces(ptr);
    if (ptr != end)
        return std::numeric_limits<double>::quiet_NaN();

    return val;
}

extern "C" char *dtoa(double d, int mode, int ndigits,
                      int *decpt, int *sign, char **rve);

String es_num_to_str(double m, int num_digits)
{
    // FIXME: Use StringBuilder.

    // 9.8.1
    if (std::isnan(m))
        return _USTR("NaN");
    
    if (m == 0.0 || m == -0.0)
        return _USTR("0");
    
    if (m < 0.0)
        return _USTR("-") + es_num_to_str(-m, num_digits);
    
    if (std::isinf(m))
        return _USTR("Infinity");

    bool fixed = num_digits != INT_MIN;

    int point = 0, sign = 0;
    char *end_ptr = NULL;
    char *beg_ptr = dtoa(m,
                         fixed ? 3 : 0,
                         fixed ? num_digits : 0,
                         &point, &sign, &end_ptr);
    
    int length = static_cast<int>(end_ptr - beg_ptr);
    
    // 9.8.1:6
    if (length <= point && point <= 21)
    {
        String res(beg_ptr, length);
        
        int pad_len = point - length;
        char zeros[] = "00000000000000000000000000000000";
        
        while (pad_len > 0)
        {
            int cur_pad_len = pad_len > static_cast<int>(sizeof(zeros))
                ? static_cast<int>(sizeof(zeros)) : pad_len;

            res = res + String(zeros, cur_pad_len);
            pad_len -= cur_pad_len;
        }
        
        return res;
    }
    
    // 9.8.1:7
    if (0 < point && point <= 21)
    {
        String res(beg_ptr, point);
        res = res + _USTR(".");
        res = res + String(beg_ptr + point, length - point);
        
        return res;
    }
    
    // 9.8.1:8
    if (point <= 0 && point > -6)
    {
        String res = _USTR("0.");
        
        int pad_len = -point;
        char zeros[] = "00000000000000000000000000000000";
        
        while (pad_len > 0)
        {
            int cur_pad_len = pad_len > static_cast<int>(sizeof(zeros))
                ? static_cast<int>(sizeof(zeros)) : pad_len;
            
            res = res + String(zeros, cur_pad_len);
            pad_len -= cur_pad_len;
        }
        
        res = res + String(beg_ptr, length);
        
        return res;
    }
    
    // 9.8.1:9-10
    String res(static_cast<uni_char>(beg_ptr[0]));
    if (length != 1)
    {
        res = res + _USTR(".");
        res = res + String(beg_ptr + 1, length - 1);
    }
    
    char buffer2[2 + 64] = { 'e', point >= 0 ? '+' : '-' };
    int buffer2_pos = 2;

    int exponent = point - 1;
    if (exponent < 0)
        exponent = -exponent;
    
    // Add exponent as string.
    uint32_t number = static_cast<uint32_t>(exponent);
    if (exponent < 0)
        number = static_cast<uint32_t>(-exponent);

    int digits = 1;
    for (uint32_t factor = 10; digits < 10; digits++, factor *= 10)
    {
        if (factor > number)
            break;
    }
    
    buffer2_pos += digits;
    for (int i = 1; i <= digits; i++)
    {
        buffer2[buffer2_pos - i] = '0' + static_cast<char>(number % 10);
        number /= 10;
    }
    
    res = res + String(buffer2, buffer2_pos);
    return res;
}

String es_date_to_str(struct tm *timeinfo)
{
    assert(timeinfo);
    assert(timeinfo->tm_sec >= 0 && timeinfo->tm_sec <= 60);
    assert(timeinfo->tm_min >= 0 && timeinfo->tm_min <= 59);
    assert(timeinfo->tm_hour >= 0 && timeinfo->tm_hour <= 23);
    assert(timeinfo->tm_mday >= 1 && timeinfo->tm_mday <= 31);
    assert(timeinfo->tm_mon >= 0 && timeinfo->tm_mon <= 11);
    assert(timeinfo->tm_wday >= 0 && timeinfo->tm_wday <= 6);
    assert(timeinfo->tm_yday >= 0 && timeinfo->tm_yday <= 365);
    assert(timeinfo->tm_isdst >= -1 && timeinfo->tm_isdst <= 1);

    static const char *day[] =
    {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Say"
    };

    static const char *mon[] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    // Format: Thu Feb 16 2012 19:09:52 GMT+0100 (CET)
    long gmt = timeinfo->tm_gmtoff / 3600;  // FIXME: Does fraction of an hour offsets exist? Consider the +0100 format.
    return StringBuilder::sprintf("%s %s %.2d %d %.2d:%.2d:%.2d GMT%s%d (%s)",
                                  day[timeinfo->tm_wday], mon[timeinfo->tm_mon],
                                  timeinfo->tm_mday, timeinfo->tm_year + 1900,
                                  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                                  gmt < 0 ? "-" : "+", gmt, timeinfo->tm_zone);
}

EsValue es_from_property_descriptor(const EsPropertyReference &prop)
{
    if (!prop)
        return EsValue::undefined;

    EsObject *obj = EsObject::create_inst();

    if (prop->is_data())
    {
        obj->define_new_own_property(property_keys.value,
                                     EsPropertyDescriptor(true, true, true,
                                                          prop->value_or_undefined()));
        obj->define_new_own_property(property_keys.writable,
                                     EsPropertyDescriptor(true, true, true,
                                                          EsValue::from_bool(prop->is_writable())));
    }
    else
    {
        obj->define_new_own_property(property_keys.get,
                                     EsPropertyDescriptor(true, true, true,
                                                          prop->getter_or_undefined()));
        obj->define_new_own_property(property_keys.set,
                                     EsPropertyDescriptor(true, true, true,
                                                          prop->setter_or_undefined()));
    }

    obj->define_new_own_property(property_keys.enumerable,
                                 EsPropertyDescriptor(true, true ,true,
                                                      EsValue::from_bool(prop->is_enumerable())));
    obj->define_new_own_property(property_keys.configurable,
                                 EsPropertyDescriptor(true, true, true,
                                                      EsValue::from_bool(prop->is_configurable())));

    return EsValue::from_obj(obj);
}

bool es_to_property_descriptor(const EsValue &val, EsPropertyDescriptor &desc)
{
    if (!val.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *obj = val.as_object();

    Maybe<bool> enumerable;
    if (obj->has_property(property_keys.enumerable))
    {
        EsValue enumerable_val;
        if (!obj->getT(property_keys.enumerable, enumerable_val))
            return false;

        enumerable = enumerable_val.to_boolean();
    }

    Maybe<bool> configurable;
    if (obj->has_property(property_keys.configurable))
    {
        EsValue configurable_val;
        if (!obj->getT(property_keys.configurable, configurable_val))
            return false;

        configurable = configurable_val.to_boolean();
    }

    Maybe<EsValue> value;
    if (obj->has_property(property_keys.value))
    {
        EsValue value_val;
        if (!obj->getT(property_keys.value, value_val))
            return false;

        value = value_val;
    }

    Maybe<bool> writable;
    if (obj->has_property(property_keys.writable))
    {
        EsValue writable_val;
        if (!obj->getT(property_keys.writable, writable_val))
            return false;

        writable = writable_val.to_boolean();
    }

    Maybe<EsValue> getter;
    if (obj->has_property(property_keys.get))
    {
        EsValue getter_val;
        if (!obj->getT(property_keys.get, getter_val))
            return false;

        if (!getter_val.is_callable() && !getter_val.is_undefined())
        {
            ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_PROP_CONV_GETTER));
            return false;
        }

        getter = getter_val;
    }

    Maybe<EsValue> setter;
    if (obj->has_property(property_keys.set))
    {
        EsValue setter_val;
        if (!obj->getT(property_keys.set, setter_val))
            return false;

        if (!setter_val.is_callable() && !setter_val.is_undefined())
        {
            ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_PROP_CONV_SETTER));
            return false;
        }

        setter = setter_val;
    }

    if (getter || setter)
    {
        if (value || writable)
        {
            ES_THROW(EsTypeError, String());    // FIXME: What's this?
            return false;
        }

        desc = EsPropertyDescriptor(enumerable, configurable, getter, setter);
    }
    else
    {
        desc = EsPropertyDescriptor(enumerable, configurable, writable, value);
    }

    return true;
}
