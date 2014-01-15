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
#include <limits>
#include <sstream>
#include <stdio.h>
#include <gc_cpp.h>
#include "common/cast.hh"
#include "common/exception.hh"
#include "common/lexical.hh"
#include "common/unicode.hh"
#include "parser/parser.hh"
#include "algorithm.hh"
#include "context.hh"
#include "conversion.hh"
#include "date.hh"
#include "error.hh"
#include "global.hh"
#include "json.hh"
#include "messages.hh"
#include "native.hh"
#include "operation.hh"
#include "platform.hh"
#include "property.hh"
#include "prototype.hh"
#include "standard.hh"
#include "utility.hh"
#include "unique.hh"
#include "uri.hh"

using parser::StringStream;

ES_API_FUN(es_std_print)
{
    result = EsValue::undefined;

    if (argc == 0)
        return true;

    String msg;
    if (!argv[0].to_string(msg))
        return false;

    printf("%s\n", msg.utf8().c_str());
    return true;
}

ES_API_FUN(es_std_error)
{
    if (argc == 0)
    {
        result = EsValue::undefined;
        return true;
    }
    
    String msg;
    if (!argv[0].to_string(msg))
        return false;

    ES_THROW(EsError, (_USTR("test262 error: ") + msg));
    return false;
}

ES_API_FUN(es_std_run_test_case)
{
    if (argc == 0 || !argv[0].is_callable())
        ES_THROW(EsError, _USTR("test262 error: runTestCase failed, no test function."));

    EsValue test_res;
    if (!argv[0].as_function()->callT(ctx->this_value(), test_res))
        return false;

    if (!test_res.to_boolean())
        ES_THROW(EsError, _USTR("test262 error: runTestCase failed."));

    result = EsValue::undefined;
    return true;
}

ES_API_FUN(es_std_fn_glob_obj)
{
    result = EsValue::from_obj(es_global_obj());
    return true;
}

ES_API_FUN(es_std_fn_exists)
{
    ES_API_PARAMETER(0, name_arg);

    String name;
    if (!name_arg.to_string(name))
        return false;

    for (auto lex = ctx->lex_env(); lex; lex = lex->outer())
    {
        EsPropertyKey key = EsPropertyKey::from_str(name);

        EsEnvironmentRecord *env_rec = lex->env_rec();
        if (env_rec->is_obj_env())
        {
            EsObject *obj = safe_cast<EsObjectEnvironmentRecord *>(env_rec)->binding_object();
            if (obj->has_property(key))
            {
                result = EsValue::from_bool(true);
                return true;
            }
        }
        else
        {
            EsDeclarativeEnvironmentRecord *env =
                safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

            if (env->has_binding(key))
            {
                result = EsValue::from_bool(true);
                return true;
            }
        }
    }

    result = EsValue::from_bool(false);
    return true;
}

struct CompareArraySortComparator
{
    bool operator() (const EsValue &e1, const EsValue &e2)
    {
        EsValue result;
        if (!op_c_lt(e1, e2, result))
            throw static_cast<int>(0);

        return result.to_boolean();
    }
};

ES_API_FUN(es_std_compare_array)
{
    ES_API_PARAMETER(0, a1);
    ES_API_PARAMETER(1, a2);

    EsObject *a1_obj = a1.to_objectT();
    if (!a1_obj)
        return false;
    EsObject *a2_obj = a2.to_objectT();
    if (!a2_obj)
        return false;

    EsValue a1_len_val;
    if (!a1_obj->getT(property_keys.length, a1_len_val))
        return false;
    EsValue a2_len_val;
    if (!a2_obj->getT(property_keys.length, a2_len_val))
        return false;

    uint32_t a1_len = 0;
    if (!a1_len_val.to_uint32(a1_len))
        return false;
    uint32_t a2_len = 0;
    if (!a2_len_val.to_uint32(a2_len))
        return false;

    if (a1_len != a2_len)
    {
        result = EsValue::from_bool(false);
        return true;
    }

    CompareArraySortComparator comparator;

    EsValueVector a1_vec;
    for (uint32_t i = 0; i < a1_len; i++)
    {
        EsValue val;
        if (!a1_obj->getT(EsPropertyKey::from_u32(i), val))
            return false;

        a1_vec.push_back(val);
    }

    // FIXME: This is plain... ugly.
    try
    {
        std::sort(a1_vec.begin(), a1_vec.end(), comparator);
    }
    catch (int)
    {
        return false;
    }

    for (uint32_t i = 0; i < a1_len; i++)
    {
        if (!a1_obj->putT(EsPropertyKey::from_u32(i), a1_vec[i], false))
            return false;
    }

    EsValueVector a2_vec;
    for (uint32_t i = 0; i < a2_len; i++)
    {
        EsValue val;
        if (!a2_obj->getT(EsPropertyKey::from_u32(i), val))
            return false;

        a2_vec.push_back(val);
    }

    std::sort(a2_vec.begin(), a2_vec.end(), comparator);

    for (uint32_t i = 0; i < a2_len; i++)
    {
        if (!a2_obj->putT(EsPropertyKey::from_u32(i), a2_vec[i], false))
            return false;
    }

    for (uint32_t i = 0; i < a1_len; i++)
    {
        if (!algorithm::strict_eq_comp(a1_vec[i], a2_vec[i]))
        {
            result = EsValue::from_bool(false);
            return true;
        }
    }

    result = EsValue::from_bool(true);
    return true;
}

ES_API_FUN(es_std_array_contains)
{
    ES_API_PARAMETER(0, arr);
    ES_API_PARAMETER(1, exp);

    EsObject *arr_obj = arr.to_objectT();
    if (!arr_obj)
        return false;
    EsObject *exp_obj = exp.to_objectT();
    if (!exp_obj)
        return false;

    EsValue arr_len_val;
    if (!arr_obj->getT(property_keys.length, arr_len_val))
        return false;

    EsValue exp_len_val;
    if (!exp_obj->getT(property_keys.length, exp_len_val))
        return false;

    uint32_t arr_len = 0;
    if (!arr_len_val.to_uint32(arr_len))
        return false;
    uint32_t exp_len = 0;
    if (!exp_len_val.to_uint32(exp_len))
        return false;

    for (uint32_t i = 0; i < exp_len; i++)
    {
        bool found = false;

        EsValue e;
        if (!exp_obj->getT(EsPropertyKey::from_u32(i), e))
            return false;

        for (uint32_t j = 0; j < arr_len; j++)
        {
            EsValue a;
            if (!arr_obj->getT(EsPropertyKey::from_u32(j), a))
                return false;

            if (algorithm::strict_eq_comp(e, a))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            result = EsValue::from_bool(false);
            return true;
        }
    }

    result = EsValue::from_bool(true);
    return true;
}

ES_API_FUN(es_std_decode_uri)
{
    ES_API_PARAMETER(0, encoded_uri);

    String encoded_uri_str;
    if (!encoded_uri.to_string(encoded_uri_str))
        return false;

    String decoded_str;
    if (!es_uri_decode(encoded_uri_str, es_uri_reserved_predicate,
                       decoded_str))
        return false;

    result = EsValue::from_str(decoded_str);
    return true;
}

ES_API_FUN(es_std_decode_uri_component)
{
    ES_API_PARAMETER(0, encoded_uri_component);

    String encoded_uri_component_str;
    if (!encoded_uri_component.to_string(encoded_uri_component_str))
        return false;

    String decoded_component;
    if (!es_uri_decode(encoded_uri_component_str,
                       es_uri_component_reserved_predicate, decoded_component))
        return false;

    result = EsValue::from_str(decoded_component);
    return true;
}

ES_API_FUN(es_std_encode_uri)
{
    ES_API_PARAMETER(0, uri);

    String uri_str;
    if (!uri.to_string(uri_str))
        return false;

    String encoded_str;
    if (!es_uri_encode(uri_str, es_uri_unescaped_predicate, encoded_str))
        return false;

    result = EsValue::from_str(encoded_str);
    return true;
}

ES_API_FUN(es_std_encode_uri_component)
{
    ES_API_PARAMETER(0, uri_component);

    String uri_component_str;
    if (!uri_component.to_string(uri_component_str))
        return false;

    String encoded_component;
    if (!es_uri_encode(uri_component_str,
                       es_uri_component_unescaped_predicate, encoded_component))
        return false;

    result = EsValue::from_str(encoded_component);
    return true;
}

ES_API_FUN(es_std_eval)
{
    assert(false);

    result = EsValue::undefined;
    return true;
}

ES_API_FUN(es_std_is_nan)
{
    ES_API_PARAMETER(0, number);
    
    double num = 0.0;
    if (!number.to_number(num))
        return false;

    result = EsValue::from_bool(!!std::isnan(num));
    return true;
}

ES_API_FUN(es_std_is_finite)
{
    ES_API_PARAMETER(0, number);
    
    double num = 0.0;
    if (!number.to_number(num))
        return false;

    result = EsValue::from_bool(!!std::isfinite(num));
    return true;
}

ES_API_FUN(es_std_parse_float)
{
    ES_API_PARAMETER(0, string);

    String input_str;
    if (!string.to_string(input_str))
        return false;

    const uni_char *input_str_ptr = input_str.data();   // WARNING: May return NULL for empty strings.
    es_str_skip_white_spaces(input_str_ptr);

    if (!input_str_ptr || !*input_str_ptr)
    {
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
        return true;
    }

    const uni_char *trimmed_str = input_str_ptr;

    result = EsValue::from_num(es_strtod(trimmed_str, &trimmed_str));
    return true;
}

ES_API_FUN(es_std_parse_int)
{
    ES_API_PARAMETER(0, string);
    ES_API_PARAMETER(1, radix);

    String input_str;
    if (!string.to_string(input_str))
        return false;

    const uni_char *input_ptr = input_str.data();   // WARNING: May return NULL for empty strings.
    es_str_skip_white_spaces(input_ptr);

    if (!input_ptr || !*input_ptr)
    {
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
        return true;
    }

    int sign = 1;
    if (*input_ptr && *input_ptr == '-')
        sign = -1;

    if (*input_ptr && (*input_ptr == '-' || *input_ptr == '+'))
        input_ptr++;

    bool strip_prefix = true;

    int32_t r = 0;
    if (!radix.to_int32(r))
        return false;

    if (r != 0)
    {
        if (r < 2 || r > 36)
        {
            result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
            return true;
        }

        if (r != 16)
            strip_prefix = false;
    }
    else
    {
        r = 10;
    }

    if (strip_prefix)
    {
        if (input_ptr[0] == '0' && (input_ptr[1] == 'x' || input_ptr[1] == 'X'))
        {
            r = 16;
            input_ptr += 2;
        }
    }

    double math_int = 0.0;
    size_t acc = 0; // Number of accumulated characters.

    while (*input_ptr)
    {
        uni_char c = *input_ptr++;

        int val = 0;
        if (c >= '0' && c <= '9')
            val = c - '0';
        else if (c >= 'a' && c <= 'z')
            val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            val = c - 'A' + 10;
        else
        {
            if (acc == 0)
            {
                result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
                return true;
            }
            break;
        }

        if (val >= r)
            break;

        math_int *= r;
        math_int += static_cast<double>(val);
        acc++;
    }

    if (acc == 0)
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
    else
        result = EsValue::from_num(math_int * sign);

    return true;
}

ES_API_FUN(es_std_arr_proto_to_str)
{
    EsObject *array = ctx->this_value().to_objectT();
    if (!array)
        return false;

    EsValue fun;
    if (!array->getT(property_keys.join, fun))
        return false;

    if (!fun.is_callable())
        if (!es_proto_obj()->getT(property_keys.to_string, fun))
            return false;
    
    return fun.as_function()->callT(EsValue::from_obj(array), result);
}

ES_API_FUN(es_std_arr_proto_to_locale_str)
{
    EsObject *array = ctx->this_value().to_objectT();
    if (!array)
        return false;

    EsValue len_val;
    if (!array->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (len == 0)
    {
        result = EsValue::from_str(String());
        return true;
    }

    String separator = _USTR(",");  // FIXME: Should be locale specific.

    String r;

    EsValue first_elem;
    if (!array->getT(EsPropertyKey::from_u32(0), first_elem))
        return false;

    if (!first_elem.is_undefined() && !first_elem.is_null())
    {
        EsObject *elem_obj = first_elem.to_objectT();
        if (!elem_obj)
            return false;

        EsValue fun;
        if (!elem_obj->getT(property_keys.to_locale_string, fun))
            return false;

        if (!fun.is_callable())
        {
            ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_CALLABLE));
            return false;
        }

        EsValue r_val;
        if (!fun.as_function()->callT(EsValue::from_obj(elem_obj), r_val))
            return false;

        if (!r_val.to_string(r))    // CUSTOM: to_string().
            return false;
    }

    for (uint32_t k = 1; k < len; k++)
    {
        r = r + separator;

        EsValue next_elem;
        if (!array->getT(EsPropertyKey::from_u32(k), next_elem))
            return false;

        String next;
        if (!next_elem.is_undefined() && !next_elem.is_null())
        {
            EsObject *elem_obj = next_elem.to_objectT();
            if (!elem_obj)
                return false;

            EsValue fun;
            if (!elem_obj->getT(property_keys.to_locale_string, fun))
                return false;

            if (!fun.is_callable())
            {
                ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_CALLABLE));
                return false;
            }

            EsValue next_val;
            if (!fun.as_function()->callT(EsValue::from_obj(elem_obj), next_val))
                return false;

            if (!next_val.to_string(next))  // CUSTOM: to_string().
                return false;
        }

        r = r + next;
    }

    result = EsValue::from_str(r);
    return true;
}

/**
 * Appends an value to an array. If the value is an array, all its items will
 * be appended.
 * @param [out] a Array to append to.
 * @param [in] v Value to append.
 * @param [in,out] n Index of item to be appended. It will be updated with the
 *                   next available index before returning.
 */
static bool es_std_arr_proto_concat_value(EsArray *a, const EsValue &v, uint32_t &n)
{
    EsObject *v_obj = NULL;
    if (es_as_object(v, v_obj, "Array"))
    {
        EsValue len_val;
        if (!v_obj->getT(property_keys.length, len_val))
            return false;

        uint32_t len = 0;
        if (!len_val.to_uint32(len))
            return false;

        for (uint32_t k = 0; k < len; k++, n++)
        {
            if (v_obj->has_property(EsPropertyKey::from_u32(k)))
            {
                EsValue sub_elem;
                if (!v_obj->getT(EsPropertyKey::from_u32(k), sub_elem))
                    return false;

                if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(n), sub_elem, true, true, true))
                    return false;
            }
        }
    }
    else
    {
        if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(n++), v, true, true, true))
            return false;
    }

    return true;
}

ES_API_FUN(es_std_arr_proto_concat)
{
    // 15.4.4.4 explicitly tells to call ToObject(this), but 15.4.4.4-5-c-i-1.js
    // seems to disagree. Other implementations doesn't call ToObject(this) from
    // what I can tell.
    //EsObject *o = ctx->this_value().to_object();
    EsValue o = ctx->this_value();
    EsArray *a = EsArray::create_inst();

    uint32_t n = 0;
    if (!es_std_arr_proto_concat_value(a, o, n))
        return false;

    EsValueVector::const_iterator it;
    for (int i = 0; i < argc; i++)
        if (!es_std_arr_proto_concat_value(a, argv[i], n))
            return false;

    result = EsValue::from_obj(a);

    // The standard does not specify this, but it seems to be necessary since
    // we only add properties that can be found (in array arguments). This
    // means that "nothing" items doesn't contribute to the overall length
    // of the array so we must update it manually.
    return a->putT(property_keys.length, EsValue::from_u32(n), false);
}

ES_API_FUN(es_std_arr_proto_join)
{
    ES_API_PARAMETER(0, separator);
    
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;
    
    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    String sep = _USTR(",");
    if (!separator.is_undefined() && !separator.to_string(sep))
        return false;

    if (len == 0)
    {
        result = EsValue::from_str(String());
        return true;
    }
    
    String r;
    
    EsValue element0;
    if (!o->getT(EsPropertyKey::from_u32(0), element0))
        return false;

    if (!element0.is_undefined() && !element0.is_null())
        if (!element0.to_string(r))
            return false;
    
    for (uint32_t k = 1; k < len; k++)
    {
        r = r + sep;

        EsValue element;
        if (!o->getT(EsPropertyKey::from_u32(k), element))
            return false;

        String next;
        if (!element.is_undefined() && !element.is_null())
            if (!element.to_string(next))
                return false;

        r = r + next;
    }
    
    result = EsValue::from_str(r);
    return true;
}

ES_API_FUN(es_std_arr_proto_pop)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
        if (!len_val.to_uint32(len))
            return false;

    if (len == 0)
    {
        result = EsValue::undefined;
        return o->putT(property_keys.length, EsValue::from_u32(0), true);
    }

    uint32_t indx = len - 1;

    EsValue elem;
    if (!o->getT(EsPropertyKey::from_u32(indx), elem))
        return false;

    result = elem;

    if (!o->removeT(EsPropertyKey::from_u32(indx), true))
        return false;

    return o->putT(property_keys.length, EsValue::from_u32(indx), true);
}

ES_API_FUN(es_std_arr_proto_push)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    uint64_t n = len;   // Use uint64_t since we will add new items below, see S15.4.4.7_A4_T2.js.
    
    EsValueVector::const_iterator it;
    for (int i = 0; i < argc; i++, n++)
    {
        // FIXME: No need to go through put, we know that the property does not exist.
        if (n > ES_ARRAY_INDEX_MAX)
        {
            // FIXME: This looks like a mess.
            if (!o->putT(EsPropertyKey::from_str(String(lexical_cast<const char *>(n))), argv[i], true))
                return false;
        }
        else
        {
            if (!o->putT(EsPropertyKey::from_u32(static_cast<uint32_t>(n)), argv[i], true))
                return false;
        }
    }
    
    result = EsValue::from_u64(n);
    return o->putT(property_keys.length, result, true);
}

ES_API_FUN(es_std_arr_proto_reverse)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    uint32_t lower = 0, middle = len / 2;
    while (lower != middle)
    {
        uint32_t upper = len - lower - 1;

        EsValue lower_val;
        if (!o->getT(EsPropertyKey::from_u32(lower), lower_val))
            return false;

        EsValue upper_val;
        if (!o->getT(EsPropertyKey::from_u32(upper), upper_val))
            return false;

        bool lower_exist = o->has_property(EsPropertyKey::from_u32(lower));
        bool upper_exist = o->has_property(EsPropertyKey::from_u32(upper));

        if (lower_exist && upper_exist)
        {
            if (!o->putT(EsPropertyKey::from_u32(lower), upper_val, true))
                return false;
            if (!o->putT(EsPropertyKey::from_u32(upper), lower_val, true))
                return false;
        }
        else if (!lower_exist && upper_exist)
        {
            if (!o->putT(EsPropertyKey::from_u32(lower), upper_val, true))
                return false;
            if (!o->removeT(EsPropertyKey::from_u32(upper), true))
                return false;
        }
        else if (lower_exist && !upper_exist)
        {
            if (!o->removeT(EsPropertyKey::from_u32(lower), true))
                return false;
            if (!o->putT(EsPropertyKey::from_u32(upper), lower_val, true))
                return false;
        }

        lower++;
    }

    result = EsValue::from_obj(o);
    return true;
}

ES_API_FUN(es_std_arr_proto_shift)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (len == 0)
    {
        result = EsValue::undefined;
        return o->putT(property_keys.length, EsValue::from_u32(0), true);
    }

    EsValue first;
    if (!o->getT(EsPropertyKey::from_u32(0), first))
        return false;

    for (uint32_t k = 1; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue from_val;
            if (!o->getT(EsPropertyKey::from_u32(k), from_val))
                return false;

            if (!o->putT(EsPropertyKey::from_u32(k - 1), from_val, true))
                return false;
        }
        else
        {
            if (!o->removeT(EsPropertyKey::from_u32(k - 1), true))
                return false;
        }
    }

    result = first;

    if (!o->removeT(EsPropertyKey::from_u32(len - 1), true))
        return false;

    return o->putT(property_keys.length, EsValue::from_u32(len - 1), true);
}

ES_API_FUN(es_std_arr_proto_slice)
{
    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, end);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsArray *a = EsArray::create_inst();

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    int64_t rel_start = 0;
    if (!start.to_integer(rel_start))
        return false;

    uint32_t k = rel_start < 0 ?
        std::max((len + rel_start), static_cast<int64_t>(0)) :
        std::min(rel_start, static_cast<int64_t>(len));

    int64_t rel_end = len;
    if (!end.is_undefined() && !end.to_integer(rel_end))
        return false;

    uint32_t final = rel_end < 0 ?
        std::max((len + rel_end), static_cast<int64_t>(0)) :
        std::min(rel_end, static_cast<int64_t>(len));

    uint32_t n = 0;
    while (k < final)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            // FIXME: Re-use property key.
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Can we optimize this since we know that a colliding property doesn't exist?
            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(n), k_val, true, true, true))
                return false;
        }

        k++; n++;
    }

    result = EsValue::from_obj(a);
    return true;
}

struct ArraySortComparator
{
    EsObject *obj_;
    EsFunction *compare_fun_;

    ArraySortComparator(EsObject *obj, EsFunction *compare_fun)
        : obj_(obj), compare_fun_(compare_fun) {}

    bool operator() (uint32_t j, uint32_t k)
    {
        double result = 0.0;
        if (!algorithm::sort_compare(obj_, j, k, compare_fun_, result))
            throw static_cast<int>(0);

        return result < 0.0;
    }
};

ES_API_FUN(es_std_arr_proto_sort)
{
    ES_API_PARAMETER(0, comparefn);

    EsObject *obj = ctx->this_value().to_objectT();
    if (!obj)
        return false;

    EsValue len_val;
    if (!obj->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    EsFunction *compare_fun = NULL;
    if (!comparefn.is_undefined())
    {
        if (!comparefn.is_callable())
        {
            ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_CALLABLE));
            return false;
        }

        compare_fun = comparefn.as_function();
    }

    std::vector<uint32_t> indexes;
    EsValueVector entities(len, EsValue::undefined);

    for (uint32_t i = 0; i < len; i++)
    {
        if (obj->has_property(EsPropertyKey::from_u32(i)))
        {
            indexes.push_back(i);

            EsValue val;
            if (!obj->getT(EsPropertyKey::from_u32(i), val))
                return false;

            entities[i] = val;
        }
    }

    // FIXME: This is just plain... ugly.
    try
    {
        ArraySortComparator comparator(obj, compare_fun);
        std::sort(indexes.begin(), indexes.end(), comparator);
    }
    catch (int)
    {
        return false;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        if (!obj->removeT(EsPropertyKey::from_u32(i), false))
            return false;
    }

    uint32_t i = 0;
    std::vector<uint32_t>::iterator it;
    for (it = indexes.begin(); it != indexes.end(); ++it, i++)
    {
        if (!obj->putT(EsPropertyKey::from_u32(i), entities[*it], false))
            return false;
    }

    result = EsValue::from_obj(obj);
    return true;
}

ES_API_FUN(es_std_arr_proto_splice)
{
    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, del_count);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsArray *a = EsArray::create_inst();

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    int64_t rel_start = 0;
    if (!start.to_integer(rel_start))
        return false;

    uint32_t act_start = rel_start < 0
        ? std::max((len + rel_start), static_cast<int64_t>(0))
        : std::min(rel_start, static_cast<int64_t>(len));

    int64_t del_count_int = 0;
    if (!del_count.to_integer(del_count_int))
        return false;

    uint32_t act_del_count = std::min(std::max(del_count_int,
                                               static_cast<int64_t>(0)),
                                      static_cast<int64_t>(len - act_start));

    for (uint32_t k = 0; k < act_del_count; k++)
    {
        uint32_t from = act_start + k;
        if (o->has_property(EsPropertyKey::from_u32(from)))
        {
            EsValue from_val;
            if (!o->getT(EsPropertyKey::from_u32(from), from_val))
                return false;

            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(k), from_val, true, true, true))
                return false;
        }
    }

    uint32_t item_count = argc > 2 ? argc - 2 : 0;
    if (item_count < act_del_count)
    {
        for (uint32_t k = act_start; k < (len - act_del_count); k++)
        {
            uint32_t from = k + act_del_count;
            uint32_t to = k + item_count;

            if (o->has_property(EsPropertyKey::from_u32(from)))
            {
                EsValue from_val;
                if (!o->getT(EsPropertyKey::from_u32(from), from_val))
                    return false;

                if (!o->putT(EsPropertyKey::from_u32(to), from_val, true))
                    return false;
            }
            else
            {
                if (!o->removeT(EsPropertyKey::from_u32(to), true))
                    return false;
            }
        }

        for (uint32_t k = len; k > (len - act_del_count + item_count); k--)
        {
            if (!o->removeT(EsPropertyKey::from_u32(k - 1), true))
                return false;
        }
    }
    else if (item_count > act_del_count)
    {
        for (uint32_t k = len - act_del_count; k > act_start; k--)
        {
            uint32_t from = k + act_del_count - 1;
            uint32_t to = k + item_count - 1;

            if (o->has_property(EsPropertyKey::from_u32(from)))
            {
                EsValue from_val;
                if (!o->getT(EsPropertyKey::from_u32(from), from_val))
                    return false;

                if (!o->putT(EsPropertyKey::from_u32(to), from_val, true))
                    return false;
            }
            else
            {
                if (!o->removeT(EsPropertyKey::from_u32(to), true))
                    return false;
            }
        }
    }

    uint32_t k = act_start;
    for (int i = 2; i < argc; i++, k++)
    {
        if (!o->putT(EsPropertyKey::from_u32(k), argv[i], true))
            return false;
    }

    result = EsValue::from_obj(a);
    return o->putT(property_keys.length, EsValue::from_i64(len - act_del_count + item_count), true);
}

ES_API_FUN(es_std_arr_proto_unshift)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    uint32_t arg_count = static_cast<uint32_t>(argc);

    for (uint32_t k = len; k > 0; k--)
    {
        uint32_t from = k - 1;
        uint32_t to = k + arg_count - 1;    // FIXME: Overflow.
        if (o->has_property(EsPropertyKey::from_u32(from)))
        {
            EsValue from_val;
            if (!o->getT(EsPropertyKey::from_u32(from), from_val))
                return false;

            if (!o->putT(EsPropertyKey::from_u32(to), from_val, true))
                return false;
        }
        else
        {
            if (!o->removeT(EsPropertyKey::from_u32(to), true))
                return false;
        }
    }

    for (uint32_t j = 0; j < arg_count; j++)
    {
        if (!o->putT(EsPropertyKey::from_u32(j), argv[j], true))
            return false;
    }

    result = EsValue::from_i64(static_cast<int64_t>(len) + arg_count);
    return o->putT(property_keys.length, result, true);
}

ES_API_FUN(es_std_arr_proto_index_of)
{
    ES_API_PARAMETER(0, search_element);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (len == 0)
    {
        result = EsValue::from_i64(-1);
        return true;
    }

    int64_t n = 0;
    if (argc > 1 && !argv[1].to_integer(n))
        return false;

    if (n >= len)
    {
        result = EsValue::from_i64(-1);
        return true;
    }

    uint32_t k = 0;
    if (n >= 0)
    {
        k = static_cast<uint32_t>(n);
    }
    else
    {
        k = len - std::abs(static_cast<uint32_t>(n));
        if (k < 0)
            k = 0;
    }

    for (;k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue elem_k;
            if (!o->getT(EsPropertyKey::from_u32(k), elem_k))
                return false;

            if (algorithm::strict_eq_comp(search_element, elem_k))
            {
                result = EsValue::from_i64(k);
                return true;
            }
        }
    }

    result = EsValue::from_i64(-1);
    return true;
}

ES_API_FUN(es_std_arr_proto_last_index_of)
{
    ES_API_PARAMETER(0, search_element);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (len == 0)
    {
        result = EsValue::from_i64(-1);
        return true;
    }

    int64_t n = len - 1;
    if (argc > 1 && !argv[1].to_integer(n))
        return false;

    int64_t k = n >= 0 ?
        std::min(n, static_cast<int64_t>(len) - 1) :
        len - std::abs(n);

    for (;k >= 0; k--)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue elem_k;
            if (!o->getT(EsPropertyKey::from_u32(k), elem_k))
                return false;

            if (algorithm::strict_eq_comp(search_element, elem_k))
            {
                result = EsValue::from_i64(k);
                return true;
            }
        }
    }

    result = EsValue::from_i64(-1);
    return true;
}

ES_API_FUN(es_std_arr_proto_every)
{
    ES_API_PARAMETER(0, comparefn);
    ES_API_PARAMETER(1, t);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!comparefn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_CALLABLE));
        return false;
    }

    EsValue comparefn_args[3];
    comparefn_args[2].set_obj(o);

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            comparefn_args[0] = k_val;
            comparefn_args[1].set_i64(k);

            EsValue comparefn_res;
            if (!comparefn.as_function()->callT(t, 3, comparefn_args, comparefn_res))
                return false;

            // FIXME: Let comparefn pass result to result directly.
            if (!comparefn_res.to_boolean())
            {
                result = EsValue::from_bool(false);
                return true;
            }
        }
    }

    result = EsValue::from_bool(true);
    return true;
}

ES_API_FUN(es_std_arr_proto_some)
{
    ES_API_PARAMETER(0, comparefn);
    ES_API_PARAMETER(1, t);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!comparefn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_CALLABLE));
        return false;
    }

    EsValue comparefn_args[3];
    comparefn_args[2].set_obj(o);

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            comparefn_args[0] = k_val;
            comparefn_args[1].set_i64(k);

            EsValue comparefn_res;
            if (!comparefn.as_function()->callT(t, 3, comparefn_args, comparefn_res))
                return false;

            // FIXME: Let comparefn pass result to result directly?
            if (comparefn_res.to_boolean())
            {
                result = EsValue::from_bool(true);
                return true;
            }
        }
    }

    result = EsValue::from_bool(false);
    return true;
}

ES_API_FUN(es_std_arr_proto_for_each)
{
    ES_API_PARAMETER(0, callbackfn);
    ES_API_PARAMETER(1, t);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!callbackfn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_PARAM_CALLABLE));
        return false;
    }

    EsValue callback_args[3];
    callback_args[2].set_obj(o);

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            callback_args[0] = k_val;
            callback_args[1].set_i64(k);

            EsValue tmp;
            if (!callbackfn.as_function()->callT(t, 3, callback_args, tmp))
                return false;
        }
    }

    result = EsValue::undefined;
    return true;
}

ES_API_FUN(es_std_arr_proto_map)
{
    ES_API_PARAMETER(0, callbackfn);
    ES_API_PARAMETER(1, t);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!callbackfn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_PARAM_CALLABLE));
        return false;
    }

    EsArray *a = EsArray::create_inst(len);

    EsValue callback_args[3];
    callback_args[2].set_obj(o);

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            callback_args[0] = k_val;
            callback_args[1].set_i64(k);

            EsValue mapped_val;
            if (!callbackfn.as_function()->callT(t, 3, callback_args, mapped_val))
                return false;

            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(k), mapped_val, true, true, true))
                return false;
        }
    }

    result = EsValue::from_obj(a);
    return true;
}

ES_API_FUN(es_std_arr_proto_filter)
{
    ES_API_PARAMETER(0, callbackfn);
    ES_API_PARAMETER(1, t);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!callbackfn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_PARAM_CALLABLE));
        return false;
    }

    EsArray *a = EsArray::create_inst();

    EsValue callback_args[3];
    callback_args[2].set_obj(o);

    for (uint32_t k = 0, to = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            callback_args[0] = k_val;
            callback_args[1].set_i64(k);

            EsValue comparefn_res;
            if (!callbackfn.as_function()->callT(t, 3, callback_args, comparefn_res))
                return false;

            if (comparefn_res.to_boolean())
            {
                if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(to++), k_val, true, true, true))
                    return false;
            }
        }
    }

    result = EsValue::from_obj(a);
    return true;
}

ES_API_FUN(es_std_arr_proto_reduce)
{
    ES_API_PARAMETER(0, callbackfn);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!callbackfn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_PARAM_CALLABLE));
        return false;
    }

    if (len == 0 && argc < 2)
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_REDUCE_INIT_VAL));
        return false;
    }

    uint32_t k = 0;

    EsValue accumulator = EsValue::undefined;
    if (argc > 1)
    {
        accumulator = argv[1];
    }
    else
    {
        bool k_present = false;

        for (; k < len && !k_present; k++)
        {
            k_present = o->has_property(EsPropertyKey::from_u32(k));
            if (k_present)
                if (!o->getT(EsPropertyKey::from_u32(k), accumulator))
                    return false;
        }

        if (!k_present)
        {
            ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_REDUCE_INIT_VAL));
            return false;
        }
    }

    EsValue callback_args[4];
    callback_args[3].set_obj(o);

    for (; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            callback_args[0] = accumulator;
            callback_args[1] = k_val;
            callback_args[2].set_i64(k);

            if (!callbackfn.as_function()->callT(EsValue::undefined,
                                                 4, callback_args, accumulator))
                return false;
        }
    }

    result = accumulator;
    return true;
}

ES_API_FUN(es_std_arr_proto_reduce_right)
{
    ES_API_PARAMETER(0, callbackfn);

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (!callbackfn.is_callable())
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_PARAM_CALLABLE));
        return false;
    }

    if (len == 0 && argc < 2)
    {
        ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_REDUCE_INIT_VAL));
        return false;
    }

    int64_t k = static_cast<int64_t>(len) - 1;

    EsValue accumulator = EsValue::undefined;
    if (argc > 1)
    {
        accumulator = argv[1];
    }
    else
    {
        bool k_present = false;

        for (; k >= 0 && !k_present; k--)
        {
            k_present = o->has_property(EsPropertyKey::from_u32(k));
            if (k_present)
                if (!o->getT(EsPropertyKey::from_u32(k), accumulator))
                    return false;
        }

        if (!k_present)
        {
            ES_THROW(EsTypeError, es_get_msg(ES_MSG_TYPE_REDUCE_INIT_VAL));
            return false;
        }
    }

    EsValue callback_args[4];
    callback_args[3] = EsValue::from_obj(o);

    for (; k >= 0; k--)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            callback_args[0] = accumulator;
            callback_args[1] = k_val;
            callback_args[2].set_i64(k);

            if (!callbackfn.as_function()->callT(EsValue::undefined,
                                                 4, callback_args, accumulator))
                return false;
        }
    }

    result = accumulator;
    return true;
}

ES_API_FUN(es_std_arr_constr_is_arr)
{
    ES_API_PARAMETER(0, arg);
    
    if (!arg.is_object())
    {
        result = EsValue::from_bool(false);
        return true;
    }

    EsObject *o = arg.as_object();
    
    result = EsValue::from_bool(o->class_name() == _USTR("Array"));
    return true;
}

ES_API_FUN(es_std_arr)
{
    // 15.4.1
    return EsArray::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_bool_proto_to_str)
{
    bool val = false;
    if (es_as_boolean(ctx->this_value(), val))
    {
        result = EsValue::from_str(val ? _USTR("true") : _USTR("false"));
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "boolean"));
    return false;
}

ES_API_FUN(es_std_bool_proto_val_of)
{
    // 15.6.4.3
    bool val = false;
    if (es_as_boolean(ctx->this_value(), val))
    {
        result = EsValue::from_bool(val);
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "boolean"));
    return false;
}

ES_API_FUN(es_std_bool)
{
    ES_API_PARAMETER(0, value);
    
    result = EsValue::from_bool(value.to_boolean());
    return true;
}

ES_API_FUN(es_std_date_proto_to_str)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    time_t raw_time = this_date->date_value();

    struct tm *local_time = localtime(&raw_time);
    if (local_time)
        result = EsValue::from_str(es_date_to_str(local_time));
    else
        result = EsValue::from_str(_USTR("Invalid Date"));

    return true;
}

ES_API_FUN(es_std_date_proto_to_date_str)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_date_str is not implemented.");
}

ES_API_FUN(es_std_date_proto_to_time_str)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_time_str is not implemented.");
}

ES_API_FUN(es_std_date_proto_to_locale_str)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_locale_str is not implemented.");
}

ES_API_FUN(es_std_date_proto_to_locale_date_str)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_locale_date_str is not implemented.");
}

ES_API_FUN(es_std_date_proto_to_locale_time_str)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_locale_time_str is not implemented.");
}

ES_API_FUN(es_std_date_proto_val_of)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }
    
    result = EsValue::from_num(this_date->primitive_value());
    return true;
}

ES_API_FUN(es_std_date_proto_get_time)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    result = EsValue::from_num(this_date->primitive_value());
    return true;
}

ES_API_FUN(es_std_date_proto_get_full_year)
{
    THROW(InternalException, "internal error: es_std_date_proto_get_full_year is not implemented.");
}

ES_API_FUN(es_std_date_proto_get_utc_full_year)
{
    THROW(InternalException, "internal error: es_std_date_proto_get_utc_full_year is not implemented.");
}

ES_API_FUN(es_std_date_proto_get_month)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_month_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_month)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_month_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_date)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_date_from_time(es_local_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_date)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_date_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_day)
{
    THROW(InternalException, "internal error: es_std_date_proto_get_day is not implemented.");
}

ES_API_FUN(es_std_date_proto_get_utc_day)
{
    THROW(InternalException, "internal error: es_std_date_proto_get_utc_day is not implemented.");
}

ES_API_FUN(es_std_date_proto_get_hours)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_hour_from_time(es_local_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_hours)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_hour_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_minutes)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_min_from_time(es_local_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_minutes)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_min_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_seconds)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_sec_from_time(es_local_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_seconds)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_sec_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_milliseconds)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_ms_from_time(es_local_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_milliseconds)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        result = EsValue::from_num(this_date->primitive_value());
    else
        result = EsValue::from_i64(es_ms_from_time(this_date->primitive_value()));

    return true;
}

ES_API_FUN(es_std_date_proto_get_time_zone_off)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    double t = this_date->primitive_value();
    if (std::isnan(t))
    {
        result = EsValue::from_num(t);
    }
    else
    {
        int64_t ms_per_minute = 60000;
        result = EsValue::from_num((t - es_local_time(t)) / ms_per_minute);
    }

    return true;
}

ES_API_FUN(es_std_date_proto_set_time)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_time is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_milliseconds)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_milliseconds is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_milliseconds)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_milliseconds is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_seconds)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_seconds is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_seconds)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_seconds is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_minutes)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_minutes is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_minutes)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_minutes is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_hours)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_hours is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_hours)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_hours is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_date)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_date is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_date)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_date is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_month)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_month is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_month)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_month is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_full_year)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_full_year is not implemented.");
}

ES_API_FUN(es_std_date_proto_set_utc_full_year)
{
    THROW(InternalException, "internal error: es_std_date_proto_set_utc_full_year is not implemented.");
}

ES_API_FUN(es_std_date_proto_to_utc_str)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_utc_str is not implemented.");
}

ES_API_FUN(es_std_date_proto_to_iso_str)
{
    EsDate *this_date = es_as_date(ctx->this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (!std::isfinite(this_date->primitive_value()))
    {
        ES_THROW(EsRangeError, es_get_msg(ES_MSG_RANGE_INFINITE_DATE));
        return false;
    }

    result = EsValue::from_str(es_date_time_iso_str(this_date->primitive_value()));
    return true;
}

ES_API_FUN(es_std_date_proto_to_json)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_json is not implemented.");
}

ES_API_FUN(es_std_date_constr_parse)
{
    ES_API_PARAMETER(0, string);

    String s;
    if (!string.to_string(s))
        return false;

    result = EsValue::from_num(es_date_parse(s));
    return true;
}

ES_API_FUN(es_std_date_constr_utc)
{
    THROW(InternalException, "internal error: es_std_date_constr_utc is not implemented.");
}

ES_API_FUN(es_std_date_constr_now)
{
    result = EsValue::from_num(time_now());
    return true;
}

ES_API_FUN(es_std_date)
{
    // 15.9.2
    time_t raw_time = static_cast<time_t>(time_now() / 1000.0);

    struct tm *local_time = localtime(&raw_time);
    assert(local_time);

    result = EsValue::from_str(es_date_to_str(local_time));
    return true;
}

ES_API_FUN(es_std_err_proto_to_str)
{
    if (!ctx->this_value().is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_OBJ));
        return false;
    }
    
    EsObject *o = ctx->this_value().as_object();

    EsValue name_val;
    if (!o->getT(property_keys.name, name_val))
        return false;

    String name;
    if (name_val.is_undefined())
        name = _USTR("Error");
    else
        if (!name_val.to_string(name))
            return false;

    EsValue msg_val;
    if (!o->getT(property_keys.message, msg_val))
        return false;
    
    String msg;
    if (!msg_val.is_undefined() && !msg_val.to_string(msg))
        return false;
    
    if (name.empty())
    {
        result = EsValue::from_str(msg);
        return true;
    }
    
    if (msg.empty())
    {
        result = EsValue::from_str(name);
        return true;
    }
    
    String res = name;
    res = res + _USTR(": ");
    res = res + msg;
    
    result = EsValue::from_str(res);
    return true;
}

ES_API_FUN(es_std_err)
{
    return EsError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_fun_proto_to_str)
{
    if (!ctx->this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }

    result = EsValue::from_str(_USTR("function Function() { [native code] }"));
    return true;
}

ES_API_FUN(es_std_fun_proto_apply)
{
    ES_API_PARAMETER(0, this_arg);
    ES_API_PARAMETER(1, arg_array);
    
    if (!ctx->this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    EsFunction *fun = ctx->this_value().as_function();
    if (arg_array.is_null() || arg_array.is_undefined())
        return fun->callT(this_arg, result);

    if (!arg_array.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *arg_array_obj = arg_array.as_object();

    EsValue len;
    if (!arg_array_obj->getT(property_keys.length, len))
        return false;

    uint32_t n = len.primitive_to_uint32();
    
    EsValueVector arg_list;
    arg_list.resize(n, EsValue::undefined);
    
    for (uint32_t i = 0; i < n; i++)
    {
        EsValue next_arg;
        if (!arg_array_obj->getT(EsPropertyKey::from_u32(i), next_arg))
            return false;

        arg_list[i] = next_arg;
    }
    
    return fun->callT(this_arg, static_cast<int>(arg_list.size()),
                      &arg_list[0], result);
}

ES_API_FUN(es_std_fun_proto_call)
{
    ES_API_PARAMETER(0, this_arg);
    
    if (!ctx->this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    EsValueVector arg_list;
    if (argc > 1)
        arg_list.resize(argc - 1, EsValue::undefined);
    for (int i = 1; i < argc; i++)
        arg_list[i - 1] = argv[i];

    return ctx->this_value().as_function()->callT(this_arg, static_cast<int>(arg_list.size()),
                                                  &arg_list[0], result);
}

ES_API_FUN(es_std_fun_proto_bind)
{
    ES_API_PARAMETER(0, this_arg);
    
    if (!ctx->this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    EsValueVector a;
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
            a.push_back(argv[i]);
    }

    result = EsValue::from_obj(EsFunctionBind::create_inst(ctx->this_value().as_function(),
                                                           this_arg, a));
    return true;
}

ES_API_FUN(es_std_fun)
{
    // 15.3.1
    return EsFunction::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_json_parse)
{
    ES_API_PARAMETER(0, text);
    ES_API_PARAMETER(1, reviver);

    String text_str;
    if (!text.to_string(text_str))
        return false;

    StringStream jtext(text_str);
    JsonParser parser(jtext);

    EsValue unfiltered;
    if (!parser.parse(unfiltered))
        return false;

    if (reviver.is_callable())
    {
        EsObject *root = EsObject::create_inst();
        if (!ES_DEF_PROPERTY(root, EsPropertyKey::from_str(String()), unfiltered, true, true, true))
            return false;

        return algorithm::json_walk(String(), root, reviver.as_function(), result);
    }

    result = unfiltered;
    return true;
}

ES_API_FUN(es_std_json_stringify)
{
    ES_API_PARAMETER(0, value);
    ES_API_PARAMETER(1, replacer);
    ES_API_PARAMETER(2, space);

    algorithm::JsonState state;

    if (replacer.is_object())
    {
        EsObject *replacer_obj = replacer.as_object();

        state.replacer_fun = dynamic_cast<EsFunction *>(replacer_obj);

        if (!state.replacer_fun && replacer_obj->class_name() == _USTR("Array"))
        {
            std::vector<uint32_t> indexes;

            // Find all indexable properties and add them to the indexes vector.
            {
                EsObject::Iterator it;
                for (it = replacer_obj->begin(); it != replacer_obj->end(); ++it)
                {
                    EsPropertyKey key = *it;
                    if (key.is_index())
                        indexes.push_back(key.as_index());
                }
            }

            std::sort(indexes.begin(), indexes.end());

            // For each indexable property.
            std::vector<uint32_t>::const_iterator it;
            for (it = indexes.begin(); it != indexes.end(); ++it)
            {
                EsValue v;
                if (!replacer_obj->getT(EsPropertyKey::from_u32(*it), v))
                    return false;

                if (v.is_string())
                    state.prop_list.push_back(v.as_string());
                else if (v.is_number())
                    state.prop_list.push_back(v.primitive_to_string());
                else if (v.is_object())
                {
                    String class_name = v.as_object()->class_name();
                    if (class_name == _USTR("String") || class_name == _USTR("Number"))
                    {
                        String v_str;
                        if (!v.to_string(v_str))
                            return false;

                        state.prop_list.push_back(v_str);
                    }
                }
            }
        }
    }

    // Calculate gap.
    double space_num = 0.0;
    String space_str;

    if (es_as_number(space, space_num))
    {
        int64_t space_int = 0;
        if (!space.to_integer(space_int))
            return false;

        StringBuilder sb;
        for (int64_t i = 0; i < std::min(static_cast<int64_t>(10), space_int); i++)
            sb.append(' ');

        state.gap = sb.string();
    }
    else if (es_as_string(space, space_str))
    {
        state.gap = space_str.length() <= 10 ? space_str : space_str.take(10);
    }

    EsObject *wrapper = EsObject::create_inst();
    if (!ES_DEF_PROPERTY(wrapper, EsPropertyKey::from_str(String()), value, true, true ,true))
        return false;

    return algorithm::json_str(String(), wrapper, state, result);
}

ES_API_FUN(es_std_math_abs)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(fabs(x_num));
    return true;
}

ES_API_FUN(es_std_math_acos)
{
    ES_API_PARAMETER(0, x);

    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(acos(x_num));
    return true;
}

ES_API_FUN(es_std_math_asin)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(asin(x_num));
    return true;
}

ES_API_FUN(es_std_math_atan)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(atan(x_num));
    return true;
}

ES_API_FUN(es_std_math_atan2)
{
    ES_API_PARAMETER(0, x);
    ES_API_PARAMETER(1, y);

    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    double y_num = 0.0;
    if (!y.to_number(y_num))
        return false;

    result = EsValue::from_num(atan2(x_num, y_num));
    return true;
}

ES_API_FUN(es_std_math_ceil)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(ceil(x_num));
    return true;
}

ES_API_FUN(es_std_math_cos)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(cos(x_num));
    return true;
}

ES_API_FUN(es_std_math_log)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(log(x_num));
    return true;
}

ES_API_FUN(es_std_math_max)
{
    // 15.8.2.11
    if (argc == 0)
    {
        result = EsValue::from_num(-std::numeric_limits<double>::infinity());
        return true;
    }

    double max = 0.0;
    if (!argv[0].to_number(max))
        return false;

    for (int i = 1; i < argc; i++)
    {
        // CUSTOM: CK 2012-02-11
        // Should use algorithm in 11.8.5.
        double v = 0.0;
        if (!argv[i].to_number(v))
            return false;

        if (std::isnan(v))
        {
            result = EsValue::from_num(v);
            return true;
        }

        if (v > max)
            max = v;
    }

    result = EsValue::from_num(max);
    return true;
}

ES_API_FUN(es_std_math_min)
{
    // 15.8.2.12
    if (argc == 0)
    {
        result = EsValue::from_num(std::numeric_limits<double>::infinity());
        return true;
    }

    double min = std::numeric_limits<double>::max();
    for (int i = 0; i < argc; i++)
    {
        // CUSTOM: CK 2012-02-11
        // Should use algorithm in 11.8.5.
        double v = 0.0;
        if (!argv[i].to_number(v))
            return false;

        if (std::isnan(v))
        {
            result = EsValue::from_num(v);
            return true;
        }

        if (v < min)
            min = v;
    }

    result = EsValue::from_num(min);
    return true;
}

ES_API_FUN(es_std_math_exp)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(exp(x_num));
    return true;
}

ES_API_FUN(es_std_math_floor)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(floor(x_num));
    return true;
}

ES_API_FUN(es_std_math_pow)
{
    ES_API_PARAMETER(0, x);
    ES_API_PARAMETER(1, y);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    double y_num = 0.0;
    if (!y.to_number(y_num))
        return false;

    // FIXME: Try optimize by using a lookup table.
    if (std::isnan(y_num))
    {
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
        return true;
    }
    if (y_num == 0.0 || y_num == -0.0)
    {
        result = EsValue::from_num(1.0);
        return true;
    }
    if (std::isnan(x_num) && y_num != 0.0)
    {
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
        return true;
    }
    if (fabs(x_num) > 1.0)
    {
        if (y_num == std::numeric_limits<double>::infinity())
        {
            result = EsValue::from_num(std::numeric_limits<double>::infinity());
            return true;
        }
        else if (y_num == -std::numeric_limits<double>::infinity())
        {
            result = EsValue::from_num(0.0);
            return true;
        }
    }
    if (fabs(x_num) == 1.0 && !std::isfinite(y_num))
    {
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
        return true;
    }
    if (fabs(x_num) < 1.0 && !std::isfinite(y_num))
    {
        if (y_num == std::numeric_limits<double>::infinity())
        {
            result = EsValue::from_num(0.0);
            return true;
        }
        else if (y_num == -std::numeric_limits<double>::infinity())
        {
            result = EsValue::from_num(std::numeric_limits<double>::infinity());
            return true;
        }
    }
    if (x_num == std::numeric_limits<double>::infinity())
    {
        if (y_num > 0.0)
        {
            result = EsValue::from_num(std::numeric_limits<double>::infinity());
            return true;
        }
        if (y_num < 0.0)
        {
            result = EsValue::from_num(0.0);
            return true;
        }
    }
    if (x_num == -std::numeric_limits<double>::infinity())
    {
        if (y_num > 0.0)
        {
            // Test if odd integer.
            int64_t y_int = static_cast<int64_t>(y_num);
            result = EsValue::from_num(static_cast<double>(y_num == static_cast<double>(y_int) && (y_int & 1)
                ? -std::numeric_limits<double>::infinity()
                :  std::numeric_limits<double>::infinity()));
            return true;
        }
        if (y_num < 0.0)
        {
            // Test if odd integer.
            int64_t y_int = static_cast<int64_t>(y_num);
            result = EsValue::from_num(y_num == static_cast<double>(y_int) && (y_int & 1) ? -0.0 : 0.0);
            return true;
        }
    }
    if (x_num == 0.0)
    {
        // Test if x_num is positive or negative zero.
        if (copysign(1.0, x_num) > 0.0)
        {
            if (y_num > 0.0)
            {
                result = EsValue::from_num(0.0);
                return true;
            }
            if (y_num < 0.0)
            {
                result = EsValue::from_num(std::numeric_limits<double>::infinity());
                return true;
            }
        }
        else
        {
            if (y_num > 0.0)
            {
                // Test if odd integer.
                int64_t y_int = static_cast<int64_t>(y_num);
                result = EsValue::from_num(y_num == static_cast<double>(y_int) && (y_int & 1) ? -0.0 : 0.0);
                return true;
            }
            if (y_num < 0.0)
            {
                // Test if odd integer.
                int64_t y_int = static_cast<int64_t>(y_num);
                result = EsValue::from_num(y_num == static_cast<double>(y_int) && (y_int & 1)
                    ? -std::numeric_limits<double>::infinity()
                    :  std::numeric_limits<double>::infinity());
                return true;
            }
        }
    }
    bool y_is_int = y_num == static_cast<double>(static_cast<int64_t>(y_num));
    if (x_num < 0.0 && std::isfinite(x_num) && std::isfinite(y_num) && !y_is_int)
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
    else
        result = EsValue::from_num(pow(x_num, y_num));
    return true;
}

ES_API_FUN(es_std_math_random)
{
    result = EsValue::from_num(static_cast<double>(rand()) /
                               static_cast<double>(RAND_MAX));
    return true;
}

ES_API_FUN(es_std_math_round)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    if (x_num <= 0.0 && x_num > 0.5)
    {
        if ((x_num == 0.0 && copysign(1.0, x_num) < 0.0) ||
            (x_num < 0.0 && x_num > -0.5))
        {
            result = EsValue::from_num(-0.0);
            return true;
        }
    }

    result = EsValue::from_num(floor(x_num + 0.5));
    return true;
}

ES_API_FUN(es_std_math_sin)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(sin(x_num));
    return true;
}

ES_API_FUN(es_std_math_sqrt)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(sqrt(x_num));
    return true;
}

ES_API_FUN(es_std_math_tan)
{
    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    result = EsValue::from_num(tan(x_num));
    return true;
}

ES_API_FUN(es_std_eval_err)
{
    return EsEvalError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_range_err)
{
    return EsRangeError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_ref_err)
{
    return EsReferenceError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_syntax_err)
{
    return EsSyntaxError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_type_err)
{
    return EsTypeError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_uri_err)
{
    return EsUriError::default_constr()->constructT(argc, argv, result);
}

ES_API_FUN(es_std_num_proto_to_str)
{
    int32_t radix = 10;
    if (argc >= 1 && !argv[0].is_undefined() && !argv[0].to_int32(radix))
        return false;
    
    // NOTE: This might result in endless recursion which is dangerous, don't
    //       handle this special case but process base 10 as any other.
    //if (radix == 10)
    //    return ctx->this_value().to_string();
    
    if (radix < 2 || radix > 36)
    {
        ES_THROW(EsRangeError, es_fmt_msg(ES_MSG_RANGE_RADIX));
        return false;
    }

    // Convert.
    char buffer[2048];  // Should be large enough to hold the largest possible value.

    double val = 0.0;
    if (es_as_number(ctx->this_value(), val))
    {
        if (radix == 10)
        {
            result = EsValue::from_str(es_num_to_str(val));
        }
        else
        {
            double_to_cstring(val, radix, buffer);
            result = EsValue::from_str(String(buffer));
        }
        return true;
    }

    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));
    return false;
}

ES_API_FUN(es_std_num_proto_to_locale_str)
{
    THROW(InternalException, "internal error: es_std_num_proto_to_locale_str is not implemented.");
}

ES_API_FUN(es_std_num_proto_val_of)
{
    double val = 0.0;
    if (es_as_number(ctx->this_value(), val))
    {
        result = EsValue::from_num(val);
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));
    return false;
}

ES_API_FUN(es_std_num_proto_to_fixed)
{
    ES_API_PARAMETER(0, fraction_digits);

    int64_t f = 0;
    if (!fraction_digits.is_undefined() && !fraction_digits.to_integer(f))
        return false;

    if (f < 0 || f > 20)
    {
        ES_THROW(EsRangeError, es_fmt_msg(ES_MSG_RANGE_FRAC_DIGITS));
        return false;
    }

    double x = 0.0;
    if (!es_as_number(ctx->this_value(), x))
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));        // CUSTOM: CK 2012-02-11
        return false;
    }

    // FIXME: Use StringBuilder to construct s.
    String s = es_num_to_str(x, static_cast<int>(f));

    if (x < 1e21)
    {
        // s will be rounded to the specified number of bits but we still have to
        // do padding.
        bool found_point = false;

        int pad_digits = static_cast<int>(f);
        for (size_t i = 0; i < s.length(); i++)
        {
            if (s[i] == '.')
                found_point = true;
            else if (found_point)
                pad_digits--;
        }

        if (!found_point && pad_digits > 0)
            s = s + _USTR(".");

        for (int i = 0; i < pad_digits; i++)
            s = s + _USTR("0");
    }

    result = EsValue::from_str(s);
    return true;
}

ES_API_FUN(es_std_num_proto_to_exp)
{
    THROW(InternalException, "internal error: es_std_num_proto_to_exp is not implemented.");
}

ES_API_FUN(es_std_num_proto_to_prec)
{
    ES_API_PARAMETER(0, precision);

    double x = 0.0;
    if (!es_as_number(ctx->this_value(), x))
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));        // CUSTOM: CK 2012-02-11
        return false;
    }

    if (precision.is_undefined())
    {
        result = EsValue::from_str(es_num_to_str(x));
        return true;
    }

    int64_t p = 0;
    if (!precision.to_integer(p))
        return false;

    if (p < 1 || p > 21)
    {
        ES_THROW(EsRangeError, es_fmt_msg(ES_MSG_RANGE_PRECISION));
        return false;
    }

    result = EsValue::from_str(es_num_to_str(x, p));
    return true;
}

ES_API_FUN(es_std_num)
{
    ES_API_PARAMETER(0, value);

    // FIXME: What's this!?
    if (argc == 0)
    {
        result = EsValue::from_u32(0);
    }
    else
    {
        double num = 0.0;
        if (!value.to_number(num))
            return false;

        result = EsValue::from_num(num);
    }
    
    return true;
}

ES_API_FUN(es_std_obj_proto_to_str)
{
    if (ctx->this_value().is_undefined())
    {
        result = EsValue::from_str(_USTR("[object Undefined]"));
        return true;
    }

    if (ctx->this_value().is_null())
    {
        result = EsValue::from_str(_USTR("[object Null]"));
        return true;
    }
    
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;
    
    String res = _USTR("[object ") + o->class_name() + _USTR("]");
    result = EsValue::from_str(res);
    return true;
}

ES_API_FUN(es_std_obj_proto_to_loc_str)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsValue to_string;
    if (!o->getT(property_keys.to_string, to_string))
        return false;

    if (!to_string.is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    return to_string.as_function()->callT(EsValue::from_obj(o), result);
}

ES_API_FUN(es_std_obj_proto_val_of)
{
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    result = EsValue::from_obj(o);
    return true;
}

ES_API_FUN(es_std_obj_proto_has_own_prop)
{
    ES_API_PARAMETER(0, v);

    String p;
    if (!v.to_string(p))
        return false;

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    result = EsValue::from_bool(o->get_own_property(EsPropertyKey::from_str(p)));
    return true;
}

ES_API_FUN(es_std_obj_proto_is_proto_of)
{
    // FIXME: Can we use ES_API_PARAMETER_OBJ here?
    ES_API_PARAMETER(0, v);
    
    if (!v.is_object())
    {
        result = EsValue::from_bool(false);
        return true;
    }
    
    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;

    EsObject *v_obj = v.as_object();

    while (true)
    {
        v_obj = v_obj->prototype();
        if (v_obj == NULL)
        {
            result = EsValue::from_bool(false);
            return true;
        }
        
        if (v_obj == o)
        {
            result = EsValue::from_bool(true);
            return true;
        }
    }

    assert(false);
    result = EsValue::undefined;
    return true;
}

ES_API_FUN(es_std_obj_proto_prop_is_enum)
{
    ES_API_PARAMETER(0, v);

    String p;
    if (!v.to_string(p))
        return false;

    EsObject *o = ctx->this_value().to_objectT();
    if (!o)
        return false;
    
    EsPropertyReference prop = o->get_own_property(EsPropertyKey::from_str(p));
    result = EsValue::from_bool(prop && prop->is_enumerable());
    return true;
}

ES_API_FUN(es_std_obj)
{
    if (argc == 0 || argv[0].is_null() || argv[0].is_undefined())
    {
        result = EsValue::from_obj(EsObject::create_inst(argc, argv));
    }
    else
    {
        EsObject *o = argv[0].to_objectT();
        if (!o)
            return false;

        result = EsValue::from_obj(o);
    }
    return true;
}

ES_API_FUN(es_std_obj_get_proto_of)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    result = o_obj->prototype() ? EsValue::from_obj(o_obj->prototype()) : EsValue::null;
    return true;
}

ES_API_FUN(es_std_obj_get_own_prop_desc)
{
    ES_API_PARAMETER(0, o);
    ES_API_PARAMETER(1, p);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    String name;
    if (!p.to_string(name))
        return false;

    result = es_from_property_descriptor(o_obj->get_own_property(EsPropertyKey::from_str(name)));
    return true;
}

ES_API_FUN(es_std_obj_get_own_prop_names)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    size_t i = 0;
    EsArray *array = EsArray::create_inst();

    // 15.2.3.4: If o is a string instance we should include the implicit
    //           string indexing properties.
    if (EsStringObject *str = dynamic_cast<EsStringObject *>(o_obj))
    {
        String val = str->primitive_value();
        for (size_t j = 0; j < val.length(); j++)
        {
            if (!ES_DEF_PROPERTY(array, EsPropertyKey::from_u32(i++), EsValue::from_str(String(j)), true, true, true))
                return false;
        }
    }

    EsObject::Iterator it;
    for (it = o_obj->begin(); it != o_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        if (!ES_DEF_PROPERTY(array, EsPropertyKey::from_u32(i++),
                             EsValue::from_str(key.to_string()), true, true, true))
        {
            return false;
        }
    }

    result = EsValue::from_obj(array);
    return true;
}

ES_API_FUN(es_std_obj_create)
{
    ES_API_PARAMETER(0, o);
    ES_API_PARAMETER(1, props);

    if (!o.is_object() && !o.is_null())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *obj = EsObject::create_inst_with_prototype(
        o.is_null() ? NULL : o.as_object());

    if (!props.is_undefined())
    {
        EsValue def_props_args[2];
        def_props_args[0].set_obj(obj);
        def_props_args[1] = props;

        es_std_obj_def_props(ctx, callee, 2, def_props_args, result);
    }

    result = EsValue::from_obj(obj);    // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_def_prop)
{
    ES_API_PARAMETER(0, o);
    ES_API_PARAMETER(1, p);
    ES_API_PARAMETER(2, attributes);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    String name;
    if (!p.to_string(name))
        return false;

    EsPropertyDescriptor prop;
    if (!es_to_property_descriptor(attributes, prop))
        return false;

    result = EsValue::from_obj(o_obj);  // FIXME: o?
    return o_obj->define_own_propertyT(EsPropertyKey::from_str(name), prop, true);
}

ES_API_FUN(es_std_obj_def_props)
{
    ES_API_PARAMETER(0, o);
    ES_API_PARAMETER(1, properties);

    EsObject *properties_obj = properties.to_objectT();
    if (!properties_obj)
        return false;

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    EsObject::Iterator it;
    for (it = properties_obj->begin(); it != properties_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        EsPropertyReference it_prop = properties_obj->get_property(key);
        assert(it_prop);
        if (!it_prop->is_enumerable())
            continue;

        // FIXME: Double get!?
        EsValue desc_obj;
        if (!properties_obj->getT(key, desc_obj))
            return false;

        EsPropertyDescriptor prop;
        if (!es_to_property_descriptor(desc_obj, prop))
            return false;

        if (!o_obj->define_own_propertyT(key, prop, true))
            return false;
    }

    result = EsValue::from_obj(o_obj);  // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_seal)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    for (EsObject::Iterator it = o_obj->begin(); it != o_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        EsPropertyReference prop = o_obj->get_property(key);
        if (prop->is_configurable())
            prop->set_configurable(false);
    }

    o_obj->set_extensible(false);
    result = EsValue::from_obj(o_obj);  // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_freeze)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    for (EsObject::Iterator it = o_obj->begin(); it != o_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        EsPropertyReference prop = o_obj->get_property(key);
        if (prop->is_data() && prop->is_writable())
            prop->set_writable(false);
        if (prop->is_configurable())
            prop->set_configurable(false);
    }

    o_obj->set_extensible(false);
    result = EsValue::from_obj(o_obj);  // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_prevent_exts)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    o_obj->set_extensible(false);
    result = EsValue::from_obj(o_obj);  // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_is_sealed)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    for (EsObject::Iterator it = o_obj->begin(); it != o_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        EsPropertyReference prop = o_obj->get_property(key);
        if (prop->is_configurable())
        {
            result = EsValue::from_bool(false);
            return true;
        }
    }

    result = EsValue::from_bool(!o_obj->is_extensible());
    return true;
}

ES_API_FUN(es_std_obj_is_frozen)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    for (EsObject::Iterator it = o_obj->begin(); it != o_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        EsPropertyReference prop = o_obj->get_property(key);
        if (prop->is_data() && prop->is_writable())
        {
            result = EsValue::from_bool(false);
            return true;
        }

        if (prop->is_configurable())
        {
            result = EsValue::from_bool(false);
            return true;
        }
    }

    result = EsValue::from_bool(!o_obj->is_extensible());
    return true;
}

ES_API_FUN(es_std_obj_is_extensible)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    result = EsValue::from_bool(o_obj->is_extensible());
    return true;
}

ES_API_FUN(es_std_obj_keys)
{
    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    // If being strict, the array should be created like: new Array(n). For
    // efficiency reasons we create it like: new Array() and then manually
    // update the length property.
    EsArray *array = EsArray::create_inst();

    // Calculate the number of enumerable own properties.
    uint32_t n = 0, index = 0;

    for (EsObject::Iterator it = o_obj->begin(); it != o_obj->end(); ++it)
    {
        EsPropertyKey key = *it;

        EsPropertyReference prop = o_obj->get_property(key);
        if (!prop->is_enumerable())
            continue;

        n++;
        if (!ES_DEF_PROPERTY(array, EsPropertyKey::from_u32(index++),
                             EsValue::from_str(key.to_string()), true, true, true))
        {
            return false;
        }
    }

    result = EsValue::from_obj(array);
    return ES_DEF_PROPERTY(array, property_keys.length,
                           EsValue::from_u32(n), true, false, false);  // VERIFIED: 15.4.5.2
}

ES_API_FUN(es_std_str_proto_to_str)
{
    // 15.5.4.2
    String val;
    if (es_as_string(ctx->this_value(), val))
    {
        result = EsValue::from_str(val);
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "string"));
    return false;
}

ES_API_FUN(es_std_str_proto_val_of)
{
    // 15.5.4.3
    String val;
    if (es_as_string(ctx->this_value(), val))
    {
        result = EsValue::from_str(val);
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "string"));
    return false;
}

ES_API_FUN(es_std_str_proto_char_at)
{
    ES_API_PARAMETER(0, pos);
    
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;
    
    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    int64_t position = 0;
    if (!pos.to_integer(position))
        return false;

    size_t size = s.length();
    
    if (position < 0 || position >= static_cast<int64_t>(size))
        result = EsValue::from_str(String());
    else
        result = EsValue::from_str(String(s[position]));
    
    return true;
}

ES_API_FUN(es_std_str_proto_char_code_at)
{
    ES_API_PARAMETER(0, pos);
    
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;
    
    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    int64_t pos_int = 0;
    if (!pos.to_integer(pos_int))
        return false;
    
    if (pos_int < 0 || pos_int >= static_cast<int64_t>(s.length()))
        result = EsValue::from_num(std::numeric_limits<double>::quiet_NaN());
    else
        result = EsValue::from_i32(s[pos_int]);

    return true;
}

ES_API_FUN(es_std_str_proto_concat)
{
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String r;
    if (!ctx->this_value().to_string(r))
        return false;

    EsValueVector::const_iterator it;
    for (int i = 0; i < argc; i++)
    {
        String str;
        if (!argv[i].to_string(str))
            return false;

        r = r + str;
    }

    result = EsValue::from_str(r);
    return true;
}

ES_API_FUN(es_std_str_proto_index_of)
{
    ES_API_PARAMETER(0, search_string);
    ES_API_PARAMETER(1, position);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    String search_str;
    if (!search_string.to_string(search_str))
        return false;

    int64_t pos = 0;
    if (!position.is_undefined() && !position.to_integer(pos))
        return false;

    int64_t len = static_cast<int64_t>(s.length());

    int64_t start = std::min(std::max(pos, static_cast<int64_t>(0)), len);
    result = EsValue::from_i64(s.index_of(search_str, start));
    return true;
}

ES_API_FUN(es_std_str_proto_last_index_of)
{
    ES_API_PARAMETER(0, search_string);
    ES_API_PARAMETER(1, position);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    String search_str;
    if (!search_string.to_string(search_str))
        return false;

    int64_t pos = 0;
    if (!position.to_integer(pos))  // 0xffffffff is not really positive infinity.
        return false;

    int64_t len = static_cast<int64_t>(s.length());

    int64_t start = std::min(std::max(pos, static_cast<int64_t>(0)), len);
    result = EsValue::from_i64(s.last_index_of(search_str, start));
    return true;
}

ES_API_FUN(es_std_str_proto_locale_compare)
{
    ES_API_PARAMETER(0, that);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;
    String t;
    if (!that.to_string(t))
        return false;

    // From ECMA-262 15.5.4.9:
    // If no language-sensitive comparison at all is available from the host
    // environment, this function may perform a bitwise comparison.
    result = EsValue::from_i32(s.compare(t));
    return true;
}

ES_API_FUN(es_std_str_proto_match)
{
    ES_API_PARAMETER(0, regexp);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    EsRegExp *rx = es_as_reg_exp(regexp);
    if (!rx)
    {
        String regexp_str;
        if (!regexp.to_string(regexp_str))
            return false;

        rx = EsRegExp::create_inst(regexp.is_undefined() ? String() : regexp_str, String());
        if (rx == NULL)
            return false;
    }

    EsValue exec_val;
    if (!rx->getT(property_keys.exec, exec_val))
        return false;

    EsFunction *exec = exec_val.as_function();

    EsValue exec_args[1];
    exec_args[0] = EsValue::from_str(s);

    EsValue global_val;
    if (!rx->getT(property_keys.global, global_val))
        return false;

    if (!global_val.to_boolean())
        return exec->callT(EsValue::from_obj(rx), 1, exec_args, result);

    if (!rx->putT(property_keys.last_index, EsValue::from_u32(0), true))
        return false;

    EsArray *a = EsArray::create_inst();
    int64_t prev_last_index = 0, n = 0;

    bool last_match = true;
    while (last_match)
    {
        EsValue exec_res;
        if (!exec->callT(EsValue::from_obj(rx), 1, exec_args, exec_res))
            return false;

        if (exec_res.is_null())
        {
            last_match = false;
        }
        else
        {
            EsValue this_index_val;
            if (!rx->getT(property_keys.last_index, this_index_val))
                return false;

            int64_t this_index = 0;
            if (!this_index_val.to_integer(this_index)) // CUSTOM to_integer(): CK 2012-07-18
                return false;

            if (this_index == prev_last_index)
            {
                if (!rx->putT(property_keys.last_index, EsValue::from_i64(this_index + 1), true))
                    return false;

                prev_last_index = this_index + 1;
            }
            else
            {
                prev_last_index = this_index;
            }

            EsValue match_str;
            if (!exec_res.to_objectT()->getT(EsPropertyKey::from_u32(0), match_str))    // CUSTOM to_object(): CK 2012-07-18
                return false;

            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(n++), match_str, true, true, true))
                return false;
        }
    }

    result = n == 0 ? EsValue::null : EsValue::from_obj(a);
    return true;
}

ES_API_FUN(es_std_str_proto_replace)
{
    ES_API_PARAMETER(0, search_value);
    ES_API_PARAMETER(1, replace_value);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    typedef std::vector<EsRegExp::MatchResult::MatchStateVector,
                        gc_allocator<EsRegExp::MatchResult::MatchStateVector> > MatchVector;
    MatchVector matches;

    bool using_reg_ex = false;
    if (EsRegExp *rx = es_as_reg_exp(search_value))
    {
        using_reg_ex = true;

        EsValue global_val;
        if (!rx->getT(property_keys.global, global_val))
            return false;

        if (!global_val.to_boolean())
        {
            EsRegExp::MatchResult *res = NULL;

            for (size_t i = 0; i <= s.length(); i++)
            {
                res = rx->match(s, i);
                if (res != NULL)
                    break;
            }

            if (!rx->putT(property_keys.last_index, EsValue::from_i32(res ? res->end_index() : 0), true))
                return false;

            if (res)
                matches.push_back(res->matches());
        }
        else
        {
            int64_t last_index = 0, prev_last_index = 0;

            while (true)
            {
                EsRegExp::MatchResult *res = NULL;

                for (size_t i = last_index; i >= 0 && i <= s.length(); i++)
                {
                    res = rx->match(s, i);
                    if (res != NULL)
                        break;
                }

                last_index = res ? res->end_index() : 0;
                if (!rx->putT(property_keys.last_index, EsValue::from_i64(last_index), true))
                    return false;

                if (!res)
                    break;

                if (last_index == prev_last_index)
                {
                    last_index++;
                    if (!rx->putT(property_keys.last_index, EsValue::from_i64(last_index), true))
                        return false;
                }

                prev_last_index = last_index;

                matches.push_back(res->matches());
            }
        }
    }
    else
    {
        String search_str;
        if (!search_value.to_string(search_str))
            return false;

        ssize_t i = s.index_of(search_str);
        if (i != -1)
        {
            EsRegExp::MatchResult::MatchStateVector tmp;
            tmp.push_back(EsRegExp::MatchState(i, search_str.length(), search_str));

            matches.push_back(tmp);
        }
    }

    StringBuilder sb;

    int last_off = 0;

    MatchVector::const_iterator it;
    for (it = matches.begin(); it != matches.end(); ++it)
    {
        assert(!(*it).empty());
        const EsRegExp::MatchState &state = (*it)[0];

        if (state.empty())
            continue;

        int off = last_off;
        int len = state.offset() - last_off;

        if (!(state.offset() >= last_off))
            continue;

        if (off + len <= static_cast<int>(s.length()))
            sb.append(s.data() + off, len);

        last_off = state.offset() + state.length();

        // Append the replaced text.
        if (replace_value.is_callable())
        {
            EsValueVector fun_args;

            if (using_reg_ex)
            {
                EsRegExp::MatchResult::MatchStateVector::const_iterator it_sub;
                for (it_sub = (*it).begin(); it_sub != (*it).end(); ++it_sub)
                    fun_args.push_back(EsValue::from_str((*it_sub).string()));
            }
            else
            {
                fun_args.push_back(EsValue::from_str(state.string()));
            }

            fun_args.push_back(EsValue::from_i32(state.offset()));
            fun_args.push_back(EsValue::from_str(s));

            EsValue fun_res;
            if (!replace_value.as_function()->callT(EsValue::undefined, static_cast<int>(fun_args.size()),
                                                    &fun_args[0], fun_res))
            {
                return false;
            }

            String fun_res_str;
            if (!fun_res.to_string(fun_res_str))
                return false;

            sb.append(fun_res_str);
        }
        else
        {
            String replace_str;
            if (!replace_value.to_string(replace_str))
                return false;

            if (replace_str.empty())
                continue;

            StringBuilder sb2;

            bool dollar_mode = false;
            for (size_t i = 0; i < replace_str.length(); i++)
            {
                uni_char c = replace_str[i];

                if (!dollar_mode && c == '$')
                {
                    dollar_mode = true;
                    continue;
                }

                if (dollar_mode)
                {
                    switch (c)
                    {
                        case '$':
                            sb2.append('$');
                            break;
                        case '&':
                            sb2.append(state.string());
                            break;
                        case '`':
                            sb2.append(s.substr(0, state.offset()));
                            break;
                        case '\'':
                        {
                            int off = state.offset() + state.length();
                            int len = s.length() - off;
                            sb2.append(s.substr(off, len));
                            break;
                        }
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        {
                            size_t n = 0;

                            int j = i + 1;
                            if (j < static_cast<int>(replace_str.length()) &&
                                es_is_dec_digit(replace_str[j]) &&
                                (*it).size() > 9)   // Don't try to interpret two digits if one would be sufficient.
                            {
                                n = static_cast<size_t>(c - '0') * 10 + static_cast<size_t>(replace_str[j] - '0');
                                i++;    // We just consumed yet another character.
                            }
                            else
                            {
                                n = static_cast<size_t>(c - '0');
                            }

                            if (n < (*it).size())
                                sb2.append((*it)[n].string());
                            break;
                        }
                        default:
                        {
                            sb2.append(c);
                            break;
                        }
                    }

                    dollar_mode = false;
                }
                else
                {
                    sb2.append(c);
                }
            }

            if (dollar_mode)
            {
                // Must have been the last character.
                sb2.append(replace_str[replace_str.length() - 1]);
            }

            sb.append(sb2.string());
        }
    }

    if (last_off < static_cast<int>(s.length()))
        sb.append(s.data() + last_off, s.length() - last_off);

    result = EsValue::from_str(sb.string());
    return true;
}

ES_API_FUN(es_std_str_proto_search)
{
    ES_API_PARAMETER(0, regexp);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    EsRegExp *rx = es_as_reg_exp(regexp);
    if (!rx)
    {
        String regexp_str;
        if (!regexp.to_string(regexp_str))
            return false;

        rx = EsRegExp::create_inst(regexp.is_undefined() ? String() : regexp_str, String());
        if (rx == NULL)
            return false;
    }

    for (size_t i = 0; i <= s.length(); i++)
    {
        EsRegExp::MatchResult *res = rx->match(s, i);
        if (res)
        {
            assert(res->length() > 0);
            result = EsValue::from_i32(res->matches()[0].offset());
            return true;
        }
    }

    result = EsValue::from_i32(-1.0);
    return true;
}

ES_API_FUN(es_std_str_proto_slice)
{
    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, end);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    int64_t len = s.length();

    int64_t int_start = 0;
    if (!start.to_integer(int_start))
        return false;
    int64_t int_end = len;
    if (!end.is_undefined() && !end.to_integer(int_end))
        return false;

    int64_t from = int_start < 0
        ? std::max(len + int_start, static_cast<int64_t>(0))
        : std::min(int_start, len);
    int64_t to = int_end < 0
        ? std::max(len + int_end, static_cast<int64_t>(0))
        : std::min(int_end, len);

    int64_t span = std::max(to - from, static_cast<int64_t>(0));

    result = EsValue::from_str(s.substr(from, span));
    return true;
}

ES_API_FUN(es_std_str_proto_split)
{
    ES_API_PARAMETER(0, separator);
    ES_API_PARAMETER(1, limit);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    EsArray *a = EsArray::create_inst();
    uint32_t lim = 0xffffffff;
    if (!limit.is_undefined() && !limit.to_uint32(lim))
        return false;

    EsRegExp *r_reg = es_as_reg_exp(separator);
    String r_str;
    if (r_reg == NULL && !separator.to_string(r_str))
        return false;

    if (lim == 0)
    {
        result = EsValue::from_obj(a);
        return true;
    }

    if (separator.is_undefined())
    {
        result = EsValue::from_obj(a);
        return ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(0), EsValue::from_str(s), true, true, true);
    }

    if (s.empty())
    {
        algorithm::MatchResult *z = NULL;
        if (r_reg)
            z = algorithm::split_match(s, 0, r_reg);
        else
            z = algorithm::split_match(s, 0, r_str);

        if (z)
        {
            result = EsValue::from_obj(a);
            return true;
        }

        result = EsValue::from_obj(a);
        return ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(0), EsValue::from_str(s), true, true, true);
    }

    uint32_t length_a = 0, p = 0, q = 0;

    while (q != static_cast<uint32_t>(s.length()))
    {
        algorithm::MatchResult *z = NULL;
        if (r_reg)
            z = algorithm::split_match(s, q, r_reg);
        else
            z = algorithm::split_match(s, q, r_str);

        if (!z)
        {
            q++;
        }
        else
        {
            uint32_t e = static_cast<uint32_t>(z->end_index_);
            StringVector &cap = z->cap_;

            if (e == p)
            {
                q++;
            }
            else
            {
                String t = s.substr(p, q - p);
                if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(length_a++), EsValue::from_str(t), true, true, true))
                    return false;

                if (length_a == lim)
                {
                    result = EsValue::from_obj(a);
                    return true;
                }

                p = e;

                for (size_t i = 1; i < cap.size(); i++)
                {
                    if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(length_a++), EsValue::from_str(cap[i]), true, true, true))
                        return false;

                    if (length_a == lim)
                    {
                        result = EsValue::from_obj(a);
                        return true;
                    }
                }

                q = p;
            }
        }
    }

    result = EsValue::from_obj(a);

    String t = s.substr(p, s.length() - p);
    return ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(length_a), EsValue::from_str(t), true, true, true);
}

ES_API_FUN(es_std_str_proto_substr)
{
    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, length);

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    int64_t s_len = static_cast<int64_t>(s.length());

    int64_t length_int = 0;
    if (!length.to_integer(length_int))
        return false;

    int64_t int_start = 0;
    if (!start.to_integer(int_start))
        return false;

    int64_t int_length = length.is_undefined()
        ? std::numeric_limits<int64_t>::max()
        : length_int;

    int64_t final_start = int_start >= 0 ? int_start : std::max(s_len + int_start,
                                                                static_cast<int64_t>(0));
    int64_t final_length = std::min(std::max(int_length, static_cast<int64_t>(0)),
                                             s_len - final_start);

    result = EsValue::from_str(final_length <= 0 ?
        String() : s.substr(final_start, final_length));
    return true;
}

ES_API_FUN(es_std_str_proto_substring)
{
    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, end);

    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    int64_t len = static_cast<int64_t>(s.length());

    int64_t int_start = 0;
    if (!start.to_integer(int_start))
        return false;

    int64_t int_end = len;
    if (!end.is_undefined() && !end.to_integer(int_end))
        return false;

    int64_t final_start = std::min(std::max(int_start, static_cast<int64_t>(0)), len);
    int64_t final_end = std::min(std::max(int_end, static_cast<int64_t>(0)), len);

    int64_t from = std::min(final_start, final_end);
    int64_t to = std::max(final_start, final_end);

    result = EsValue::from_str(s.substr(from, to - from));
    return true;
}

ES_API_FUN(es_std_str_proto_to_lower_case)
{
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    result = EsValue::from_str(s.lower());
    return true;
}

ES_API_FUN(es_std_str_proto_to_locale_lower_case)
{
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    result = EsValue::from_str(s.lower());
    return true;
}

ES_API_FUN(es_std_str_proto_to_upper_case)
{
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    result = EsValue::from_str(s.upper());
    return true;
}

ES_API_FUN(es_std_str_proto_to_locale_upper_case)
{
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    result = EsValue::from_str(s.upper());
    return true;
}

bool es_is_white_space_or_line_term(uni_char c)
{
    return es_is_white_space(c) || es_is_line_terminator(c);
}

ES_API_FUN(es_std_str_proto_trim)
{
    if (!ctx->this_value().chk_obj_coercibleT())
        return false;

    String s;
    if (!ctx->this_value().to_string(s))
        return false;

    result = EsValue::from_str(s.trim(es_is_white_space_or_line_term));
    return true;
}

ES_API_FUN(es_std_str)
{
    String str;
    if (argc > 0 && !argv[0].to_string(str))
        return false;

    result = EsValue::from_str(str);
    return true;
}

ES_API_FUN(es_std_str_from_char_code)
{
    byte *buf = new byte[argc * 6 + 1];     // One UTF-8 character may potentially be 6 bytes, not in this particular case but in general.
    byte *ptr = buf;
    
    for (int i = 0; i < argc; i++)
    {
        const EsValue &arg = argv[i];
        
        double arg_num = 0.0;
        if (!arg.to_number(arg_num))
            return false;

        uint16_t num = es_to_uint16(arg_num);
        utf8_enc(ptr, static_cast<uni_char>(num));
    }
    
    *ptr = '\0';
    
    String str(reinterpret_cast<const char *>(buf), static_cast<int>(ptr - buf));
    
    delete [] buf;
    result = EsValue::from_str(str);
    return true;
}

ES_API_FUN(es_std_reg_exp_proto_exec)
{
    ES_API_PARAMETER(0, string);

    EsRegExp *r = es_as_reg_exp(ctx->this_value());
    if (!r)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "regular expression"));
        return false;
    }

    String s;
    if (!string.to_string(s))
        return false;

    size_t length = s.length();

    EsValue last_index_val;
    if (!r->getT(property_keys.last_index, last_index_val))
        return false;

    int64_t i = 0;
    if (!last_index_val.to_integer(i))
        return false;

    EsValue global_val;
    if (!r->getT(property_keys.global, global_val))
        return false;

    if (!global_val.to_boolean())
        i = 0;

    EsRegExp::MatchResult *res = NULL;

    bool match_succeeded = false;
    while (!match_succeeded)
    {
        if (i < 0 || i > static_cast<int>(length))
        {
            result = EsValue::null;
            return r->putT(property_keys.last_index, EsValue::from_u32(0), true);
        }

        EsRegExp::MatchResult *tmp = r->match(s, i);
        if (tmp == NULL)
        {
            i++;
        }
        else
        {
            res = tmp;
            match_succeeded = true;
        }
    }

    assert(res);
    if (!r->putT(property_keys.last_index, EsValue::from_i32(res->end_index()), true))
        return false;

    EsArray *a = EsArray::create_inst();
    a->define_new_own_property(property_keys.index,
                               EsPropertyDescriptor(true, true, true,
                                                    EsValue::from_i64(i)));
    a->define_new_own_property(property_keys.input,
                               EsPropertyDescriptor(true, true, true,
                                                    EsValue::from_str(s)));
    if (!ES_DEF_PROPERTY(a, property_keys.length, EsValue::from_i64(res->length() + 1), true, true, true))    // FIXME: This shouldn't fail.
        return false;

    size_t index = 0;

    EsRegExp::MatchResult::const_iterator it;
    for (it = res->begin(); it != res->end(); ++it, index++)
    {
        const EsRegExp::MatchState &state = *it;
        if (state.empty())
        {
            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(index), EsValue::undefined, true, true, true))
                return false;
        }
        else
        {
            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(index), EsValue::from_str(state.string()), true, true, true))
                return false;
        }
    }

    result = EsValue::from_obj(a);
    return true;
}

ES_API_FUN(es_std_reg_exp_proto_test)
{
    ES_API_PARAMETER(0, string);

    EsRegExp *r = es_as_reg_exp(ctx->this_value());
    if (!r)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "regular expression"));
        return false;
    }

    String s;
    if (!string.to_string(s))
        return false;

    int64_t length = s.length();

    EsValue last_index_val;
    if (!r->getT(property_keys.last_index, last_index_val))
        return false;

    int64_t i = 0;
    if (!last_index_val.to_integer(i))
        return false;

    EsValue global_val;
    if (!r->getT(property_keys.global, global_val))
        return false;

    if (!global_val.to_boolean())
        i = 0;

    EsRegExp::MatchResult *res = NULL;

    bool match_succeeded = false;
    while (!match_succeeded)
    {
        if (i < 0 || i > length)
        {
            result = EsValue::from_bool(false);
            return r->putT(property_keys.last_index, EsValue::from_u32(0), true);
        }

        EsRegExp::MatchResult *tmp = r->match(s, i);
        if (tmp == NULL)
        {
            i++;
        }
        else
        {
            res = tmp;
            match_succeeded = true;
        }
    }

    assert(res);
    if (!r->putT(property_keys.last_index, EsValue::from_i32(res->end_index()), true))
        return false;

    result = EsValue::from_bool(true);
    return true;
}

ES_API_FUN(es_std_reg_exp_proto_to_str)
{
    EsRegExp *r = es_as_reg_exp(ctx->this_value());
    if (!r)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "regular expression"));
        return false;
    }

    StringBuilder sb;
    sb.append('/');
    sb.append(r->pattern());
    sb.append('/');

    EsValue global_val;
    if (!r->getT(property_keys.global, global_val))
        return false;

    EsValue ignore_case_val;
    if (!r->getT(property_keys.ignore_case, ignore_case_val))
        return false;

    EsValue multiline_val;
    if (!r->getT(property_keys.multiline, multiline_val))
        return false;

    if (global_val.to_boolean())
        sb.append('g');
    if (ignore_case_val.to_boolean())
        sb.append('i');
    if (multiline_val.to_boolean())
        sb.append('m');

    result = EsValue::from_str(sb.string());
    return true;
}

ES_API_FUN(es_std_reg_exp)
{
    ES_API_PARAMETER(0, pattern);
    ES_API_PARAMETER(1, flags);

    if (flags.is_undefined() && pattern.is_object())
    {
        EsObject *o = pattern.as_object();
        if (o->class_name() == _USTR("RegExp"))
        {
            result = EsValue::from_obj(o);
            return true;
        }
    }

    return EsRegExp::default_constr()->constructT(argc, argv, result);
}
