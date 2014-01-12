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
#include "frame.hh"
#include "global.hh"
#include "json.hh"
#include "messages.hh"
#include "native.hh"
#include "operation.h"
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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (argc == 0)
        return true;

    const EsString *msg = frame.arg(0).to_stringT();
    if (!msg)
        return false;

    printf("%s\n", msg->utf8().c_str());
    return true;
}

ES_API_FUN(es_std_error)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (argc == 0)
        return true;
    
    const EsString *msg = frame.arg(0).to_stringT();
    if (!msg)
        return false;

    ES_THROW(EsError, _ESTR("test262 error: ")->concat(msg));
    return false;
}

ES_API_FUN(es_std_run_test_case)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (argc == 0 || !frame.arg(0).is_callable())
        ES_THROW(EsError, _ESTR("test262 error: runTestCase failed, no test function."));

    EsCallFrame new_frame = EsCallFrame::push_function(
        0, frame.arg(0).as_function(), frame.this_value());
    if (!frame.arg(0).as_function()->callT(new_frame))
        return false;

    if (!new_frame.result().to_boolean())
        ES_THROW(EsError, _ESTR("test262 error: runTestCase failed."));

    return true;
}

ES_API_FUN(es_std_fn_glob_obj)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    frame.set_result(EsValue::from_obj(es_global_obj()));
    return true;
}

ES_API_FUN(es_std_fn_exists)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, name_arg);

    const EsString *name = name_arg.to_stringT();
    if (!name)
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
                frame.set_result(EsValue::from_bool(true));
                return true;
            }
        }
        else
        {
            EsDeclarativeEnvironmentRecord *env =
                safe_cast<EsDeclarativeEnvironmentRecord *>(env_rec);

            if (env->has_binding(key))
            {
                frame.set_result(EsValue::from_bool(true));
                return true;
            }
        }
    }

    frame.set_result(EsValue::from_bool(false));
    return true;
}

struct CompareArraySortComparator
{
    bool operator() (const EsValue &e1, const EsValue &e2)
    {
        EsValue result;
        if (!esa_c_lt(e1, e2, &result))
            throw static_cast<int>(0);

        return result.to_boolean();
    }
};

ES_API_FUN(es_std_compare_array)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
        frame.set_result(EsValue::from_bool(false));
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
            frame.set_result(EsValue::from_bool(false));
            return true;
        }
    }

    frame.set_result(EsValue::from_bool(true));
    return true;
}

ES_API_FUN(es_std_array_contains)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
            frame.set_result(EsValue::from_bool(false));
            return true;
        }
    }

    frame.set_result(EsValue::from_bool(true));
    return true;
}

ES_API_FUN(es_std_decode_uri)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, encoded_uri);

    const EsString *encoded_uri_str = encoded_uri.to_stringT();
    if (!encoded_uri_str)
        return false;

    const EsString *decoded_str =
            es_uri_decode(encoded_uri_str, es_uri_reserved_predicate);
    if (!decoded_str)
        return false;

    frame.set_result(EsValue::from_str(decoded_str));
    return true;
}

ES_API_FUN(es_std_decode_uri_component)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, encoded_uri_component);

    const EsString *encoded_uri_component_str =
        encoded_uri_component.to_stringT();
    if (!encoded_uri_component_str)
        return false;

    const EsString *decoded_component =
            es_uri_decode(encoded_uri_component_str,
                          es_uri_component_reserved_predicate);
    if (!decoded_component)
        return false;

    frame.set_result(EsValue::from_str(decoded_component));
    return true;
}

ES_API_FUN(es_std_encode_uri)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, uri);

    const EsString *uri_str = uri.to_stringT();
    if (!uri_str)
        return false;

    const EsString *encoded_str =
            es_uri_encode(uri_str, es_uri_unescaped_predicate);
    if (!encoded_str)
        return false;

    frame.set_result(EsValue::from_str(encoded_str));
    return true;
}

ES_API_FUN(es_std_encode_uri_component)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, uri_component);

    const EsString *uri_component_str = uri_component.to_stringT();
    if (!uri_component_str)
        return false;

    const EsString *encoded_component =
            es_uri_encode(uri_component_str,
                          es_uri_component_unescaped_predicate);
    if (!encoded_component)
        return false;

    frame.set_result(EsValue::from_str(encoded_component));
    return true;
}

ES_API_FUN(es_std_eval)
{
    assert(false);
    return true;
}

ES_API_FUN(es_std_is_nan)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, number);
    
    double num = 0.0;
    if (!number.to_number(num))
        return false;

    frame.set_result(EsValue::from_bool(!!std::isnan(num)));
    return true;
}

ES_API_FUN(es_std_is_finite)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, number);
    
    double num = 0.0;
    if (!number.to_number(num))
        return false;

    frame.set_result(EsValue::from_bool(!!std::isfinite(num)));
    return true;
}

ES_API_FUN(es_std_parse_float)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, string);

    const EsString *input_str = string.to_stringT();
    if (!input_str)
        return false;

    const uni_char *input_str_ptr = input_str->data();  // WARNING: May return NULL for empty strings.
    es_str_skip_white_spaces(input_str_ptr);

    if (!input_str_ptr || !*input_str_ptr)
    {
        frame.set_result(
            EsValue::from_num(std::numeric_limits<double>::quiet_NaN()));
        return true;
    }

    const uni_char *trimmed_str = input_str_ptr;

    frame.set_result(EsValue::from_num(es_strtod(trimmed_str, &trimmed_str)));
    return true;
}

ES_API_FUN(es_std_parse_int)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, string);
    ES_API_PARAMETER(1, radix);

    const EsString *input_str = string.to_stringT();
    if (!input_str)
        return false;

    const uni_char *input_ptr = input_str->data();  // WARNING: May return NULL for empty strings.
    es_str_skip_white_spaces(input_ptr);

    if (!input_ptr || !*input_ptr)
    {
        frame.set_result(
            EsValue::from_num(std::numeric_limits<double>::quiet_NaN()));
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
            frame.set_result(
                EsValue::from_num(std::numeric_limits<double>::quiet_NaN()));
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
                frame.set_result(
                    EsValue::from_num(std::numeric_limits<double>::quiet_NaN()));
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
        frame.set_result(
            EsValue::from_num(std::numeric_limits<double>::quiet_NaN()));
    else
        frame.set_result(EsValue::from_num(math_int * sign));

    return true;
}

ES_API_FUN(es_std_arr_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *array = frame.this_value().to_objectT();
    if (!array)
        return false;

    EsValue fun;
    if (!array->getT(property_keys.join, fun))
        return false;

    if (!fun.is_callable())
        if (!es_proto_obj()->getT(property_keys.to_string, fun))
            return false;

    EsCallFrame fun_frame = EsCallFrame::push_function(
        0, fun.as_function(), EsValue::from_obj(array));
    if (!fun.as_function()->callT(fun_frame))
        return false;

    frame.set_result(fun_frame.result());
    return true;
}

ES_API_FUN(es_std_arr_proto_to_locale_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *array = frame.this_value().to_objectT();
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
        frame.set_result(EsValue::from_str(EsString::create()));
        return true;
    }

    const EsString *separator = _ESTR(","); // FIXME: Should be locale specific.

    const EsString *r = EsString::create();

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

        EsCallFrame fun_frame = EsCallFrame::push_function(
            0, fun.as_function(), EsValue::from_obj(elem_obj));
        if (!fun.as_function()->callT(fun_frame))
            return false;

        r = fun_frame.result().to_stringT();    // CUSTOM: to_stringT().
        if (!r)
            return false;
    }

    for (uint32_t k = 1; k < len; k++)
    {
        r = r->concat(separator);

        EsValue next_elem;
        if (!array->getT(EsPropertyKey::from_u32(k), next_elem))
            return false;

        const EsString *next = EsString::create();
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

            EsCallFrame next_frame = EsCallFrame::push_function(
                0, fun.as_function(), EsValue::from_obj(elem_obj));
            if (!fun.as_function()->callT(next_frame))
                return false;

            next = next_frame.result().to_stringT();    // CUSTOM: to_stringT().
            if (!next)
                return false;
        }

        r = r->concat(next);
    }

    frame.set_result(EsValue::from_str(r));
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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.4.4.4 explicitly tells to call ToObject(this), but 15.4.4.4-5-c-i-1.js
    // seems to disagree. Other implementations doesn't call ToObject(this) from
    // what I can tell.
    //EsObject *o = frame.this_value().to_object();
    EsValue o = frame.this_value();
    EsArray *a = EsArray::create_inst();

    uint32_t n = 0;
    if (!es_std_arr_proto_concat_value(a, o, n))
        return false;

    for (const EsValue &arg : frame.arguments())
    {
        if (!es_std_arr_proto_concat_value(a, arg, n))
            return false;
    }

    frame.set_result(EsValue::from_obj(a));

    // The standard does not specify this, but it seems to be necessary since
    // we only add properties that can be found (in array arguments). This
    // means that "nothing" items doesn't contribute to the overall length
    // of the array so we must update it manually.
    return a->putT(property_keys.length, EsValue::from_u32(n), false);
}

ES_API_FUN(es_std_arr_proto_join)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, separator);
    
    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;
    
    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    const EsString *sep = NULL;
    if (!separator.is_undefined())
    {
        sep = separator.to_stringT();
        if (!sep)
            return false;
    }
    else
    {
        sep = _ESTR(",");
    }

    if (len == 0)
    {
        frame.set_result(EsValue::from_str(EsString::create()));
        return true;
    }
    
    EsValue element0;
    if (!o->getT(EsPropertyKey::from_u32(0), element0))
        return false;

    const EsString *r = EsString::create();
    if (!element0.is_undefined() && !element0.is_null())
    {
        r = element0.to_stringT();
        if (!r)
            return false;
    }
    
    for (uint32_t k = 1; k < len; k++)
    {
        r = r->concat(sep);

        EsValue element;
        if (!o->getT(EsPropertyKey::from_u32(k), element))
            return false;

        const EsString *next = EsString::create();
        if (!element.is_undefined() && !element.is_null())
        {
            next = element.to_stringT();
            if (!next)
                return false;
        }

        r = r->concat(next);
    }
    
    frame.set_result(EsValue::from_str(r));
    return true;
}

ES_API_FUN(es_std_arr_proto_pop)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
        if (!len_val.to_uint32(len))
            return false;

    if (len == 0)
        return o->putT(property_keys.length, EsValue::from_u32(0), true);

    uint32_t indx = len - 1;

    EsValue elem;
    if (!o->getT(EsPropertyKey::from_u32(indx), elem))
        return false;

    frame.set_result(elem);

    if (!o->removeT(EsPropertyKey::from_u32(indx), true))
        return false;

    return o->putT(property_keys.length, EsValue::from_u32(indx), true);
}

ES_API_FUN(es_std_arr_proto_push)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    uint64_t n = len;   // Use uint64_t since we will add new items below, see S15.4.4.7_A4_T2.js.
    
    for (const EsValue &arg : frame.arguments())
    {
        // FIXME: No need to go through put, we know that the property does not exist.
        if (n > ES_ARRAY_INDEX_MAX)
        {
            // FIXME: This looks like a mess.
            if (!o->putT(EsPropertyKey::from_str(
                    EsString::create_from_utf8(
                            lexical_cast<const char *>(n++))), arg, true))
                return false;
        }
        else
        {
            if (!o->putT(EsPropertyKey::from_u32(static_cast<uint32_t>(n++)), arg, true))
                return false;
        }
    }
    
    frame.set_result(EsValue::from_u64(n));
    return o->putT(property_keys.length, frame.result(), true);
}

ES_API_FUN(es_std_arr_proto_reverse)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
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

    frame.set_result(EsValue::from_obj(o));
    return true;
}

ES_API_FUN(es_std_arr_proto_shift)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;

    EsValue len_val;
    if (!o->getT(property_keys.length, len_val))
        return false;

    uint32_t len = 0;
    if (!len_val.to_uint32(len))
        return false;

    if (len == 0)
        return o->putT(property_keys.length, EsValue::from_u32(0), true);

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

    frame.set_result(first);

    if (!o->removeT(EsPropertyKey::from_u32(len - 1), true))
        return false;

    return o->putT(property_keys.length, EsValue::from_u32(len - 1), true);
}

ES_API_FUN(es_std_arr_proto_slice)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, end);

    EsObject *o = frame.this_value().to_objectT();
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

    frame.set_result(EsValue::from_obj(a));
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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, comparefn);

    EsObject *obj = frame.this_value().to_objectT();
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

    frame.set_result(EsValue::from_obj(obj));
    return true;
}

ES_API_FUN(es_std_arr_proto_splice)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, del_count);

    EsObject *o = frame.this_value().to_objectT();
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
    for (uint32_t i = 2; i < argc; i++, k++)
    {
        if (!o->putT(EsPropertyKey::from_u32(k), frame.arg(i), true))
            return false;
    }

    frame.set_result(EsValue::from_obj(a));
    return o->putT(property_keys.length,
                   EsValue::from_i64(len - act_del_count + item_count), true);
}

ES_API_FUN(es_std_arr_proto_unshift)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
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

    uint32_t j = 0;
    for (const EsValue &arg : frame.arguments())
    {
        if (!o->putT(EsPropertyKey::from_u32(j++), arg, true))
            return false;
    }

    frame.set_result(EsValue::from_i64(static_cast<int64_t>(len) + arg_count));
    return o->putT(property_keys.length, frame.result(), true);
}

ES_API_FUN(es_std_arr_proto_index_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, search_element);

    EsObject *o = frame.this_value().to_objectT();
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
        frame.set_result(EsValue::from_i64(-1));
        return true;
    }

    int64_t n = 0;
    if (argc > 1 && !frame.arg(1).to_integer(n))
        return false;

    if (n >= len)
    {
        frame.set_result(EsValue::from_i64(-1));
        return true;
    }

    uint32_t k = 0;
    if (n >= 0)
        k = static_cast<uint32_t>(n);
    else
        k = len - static_cast<uint32_t>(std::abs(n));

    for (;k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue elem_k;
            if (!o->getT(EsPropertyKey::from_u32(k), elem_k))
                return false;

            if (algorithm::strict_eq_comp(search_element, elem_k))
            {
                frame.set_result(EsValue::from_i64(k));
                return true;
            }
        }
    }

    frame.set_result(EsValue::from_i64(-1));
    return true;
}

ES_API_FUN(es_std_arr_proto_last_index_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, search_element);

    EsObject *o = frame.this_value().to_objectT();
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
        frame.set_result(EsValue::from_i64(-1));
        return true;
    }

    int64_t n = len - 1;
    if (argc > 1 && !frame.arg(1).to_integer(n))
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
                frame.set_result(EsValue::from_i64(k));
                return true;
            }
        }
    }

    frame.set_result(EsValue::from_i64(-1));
    return true;
}

ES_API_FUN(es_std_arr_proto_every)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, comparefn);
    ES_API_PARAMETER(1, t);

    EsObject *o = frame.this_value().to_objectT();
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

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use the same call frame.
            EsCallFrame comparefn_frame = EsCallFrame::push_function(
                3, comparefn.as_function(), t);
            comparefn_frame.fp()[0] = k_val;
            comparefn_frame.fp()[1].set_i64(k);
            comparefn_frame.fp()[2].set_obj(o);

            if (!comparefn.as_function()->callT(comparefn_frame))
                return false;

            if (!comparefn_frame.result().to_boolean())
            {
                frame.set_result(EsValue::from_bool(false));
                return true;
            }
        }
    }

    frame.set_result(EsValue::from_bool(true));
    return true;
}

ES_API_FUN(es_std_arr_proto_some)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, comparefn);
    ES_API_PARAMETER(1, t);

    EsObject *o = frame.this_value().to_objectT();
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

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use the same call frame.
            EsCallFrame comparefn_frame = EsCallFrame::push_function(
                3, comparefn.as_function(), t);
            comparefn_frame.fp()[0] = k_val;
            comparefn_frame.fp()[1].set_i64(k);
            comparefn_frame.fp()[2].set_obj(o);

            if (!comparefn.as_function()->callT(comparefn_frame))
                return false;

            if (comparefn_frame.result().to_boolean())
            {
                frame.set_result(EsValue::from_bool(true));
                return true;
            }
        }
    }

    frame.set_result(EsValue::from_bool(false));
    return true;
}

ES_API_FUN(es_std_arr_proto_for_each)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, callbackfn);
    ES_API_PARAMETER(1, t);

    EsObject *o = frame.this_value().to_objectT();
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

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use the same call frame.
            EsCallFrame callback_frame = EsCallFrame::push_function(
                3, callbackfn.as_function(), t);
            callback_frame.fp()[0] = k_val;
            callback_frame.fp()[1].set_i64(k);
            callback_frame.fp()[2].set_obj(o);

            if (!callbackfn.as_function()->callT(callback_frame))
                return false;
        }
    }

    return true;
}

ES_API_FUN(es_std_arr_proto_map)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, callbackfn);
    ES_API_PARAMETER(1, t);

    EsObject *o = frame.this_value().to_objectT();
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

    for (uint32_t k = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use the same call frame.
            EsCallFrame callback_frame = EsCallFrame::push_function(
                3, callbackfn.as_function(), t);
            callback_frame.fp()[0] = k_val;
            callback_frame.fp()[1].set_i64(k);
            callback_frame.fp()[2].set_obj(o);

            if (!callbackfn.as_function()->callT(callback_frame))
                return false;

            if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(k),
                                 callback_frame.result(), true, true, true))
                return false;
        }
    }

    frame.set_result(EsValue::from_obj(a));
    return true;
}

ES_API_FUN(es_std_arr_proto_filter)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, callbackfn);
    ES_API_PARAMETER(1, t);

    EsObject *o = frame.this_value().to_objectT();
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

    for (uint32_t k = 0, to = 0; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use the same call frame.
            EsCallFrame callback_frame = EsCallFrame::push_function(
                3, callbackfn.as_function(), t);
            callback_frame.fp()[0] = k_val;
            callback_frame.fp()[1].set_i64(k);
            callback_frame.fp()[2].set_obj(o);

            if (!callbackfn.as_function()->callT(callback_frame))
                return false;

            if (callback_frame.result().to_boolean())
            {
                if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(to++), k_val, true, true, true))
                    return false;
            }
        }
    }

    frame.set_result(EsValue::from_obj(a));
    return true;
}

ES_API_FUN(es_std_arr_proto_reduce)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, callbackfn);

    EsObject *o = frame.this_value().to_objectT();
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
        accumulator = frame.arg(1);
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

    for (; k < len; k++)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use the same call frame for each call.
            EsCallFrame callback_frame = EsCallFrame::push_function(
                4, callbackfn.as_function(), EsValue::undefined);
            callback_frame.fp()[0] = accumulator;
            callback_frame.fp()[1] = k_val;
            callback_frame.fp()[2].set_i64(k);
            callback_frame.fp()[3].set_obj(o);

            if (!callbackfn.as_function()->callT(callback_frame))
                return false;

            accumulator = callback_frame.result();
        }
    }

    frame.set_result(accumulator);
    return true;
}

ES_API_FUN(es_std_arr_proto_reduce_right)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, callbackfn);

    EsObject *o = frame.this_value().to_objectT();
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
        accumulator = frame.arg(1);
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

    for (; k >= 0; k--)
    {
        if (o->has_property(EsPropertyKey::from_u32(k)))
        {
            EsValue k_val;
            if (!o->getT(EsPropertyKey::from_u32(k), k_val))
                return false;

            // FIXME: Re-use call frame for each call.
            EsCallFrame callback_frame = EsCallFrame::push_function(
                4, callbackfn.as_function(), EsValue::undefined);
            callback_frame.fp()[0] = accumulator;
            callback_frame.fp()[1] = k_val;
            callback_frame.fp()[2].set_i64(k);
            callback_frame.fp()[3] = EsValue::from_obj(o);

            if (!callbackfn.as_function()->callT(callback_frame))
                return false;

            accumulator = callback_frame.result();
        }
    }

    frame.set_result(accumulator);
    return true;
}

ES_API_FUN(es_std_arr_constr_is_arr)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, arg);
    
    if (!arg.is_object())
    {
        frame.set_result(EsValue::from_bool(false));
        return true;
    }

    EsObject *o = arg.as_object();
    
    frame.set_result(EsValue::from_bool(o->class_name() == _USTR("Array")));
    return true;
}

ES_API_FUN(es_std_arr)
{
    // 15.4.1
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsArray::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_bool_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    bool val = false;
    if (es_as_boolean(frame.this_value(), val))
    {
        frame.set_result(EsValue::from_str(val ? _ESTR("true") : _ESTR("false")));
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "boolean"));
    return false;
}

ES_API_FUN(es_std_bool_proto_val_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.6.4.3
    bool val = false;
    if (es_as_boolean(frame.this_value(), val))
    {
        frame.set_result(EsValue::from_bool(val));
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "boolean"));
    return false;
}

ES_API_FUN(es_std_bool)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, value);
    
    frame.set_result(EsValue::from_bool(value.to_boolean()));
    return true;
}

ES_API_FUN(es_std_date_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    time_t raw_time = this_date->date_value();

    struct tm *local_time = localtime(&raw_time);
    if (local_time)
        frame.set_result(EsValue::from_str(es_date_to_str(local_time)));
    else
        frame.set_result(EsValue::from_str(_ESTR("Invalid Date")));

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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }
    
    frame.set_result(EsValue::from_num(this_date->primitive_value()));
    return true;
}

ES_API_FUN(es_std_date_proto_get_time)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    frame.set_result(EsValue::from_num(this_date->primitive_value()));
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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_month_from_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_month)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_month_from_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_date)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_date_from_time(es_local_time(this_date->primitive_value()))));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_date)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_date_from_time(this_date->primitive_value())));

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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_hour_from_time(es_local_time(this_date->primitive_value()))));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_hours)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_hour_from_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_minutes)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_min_from_time(es_local_time(this_date->primitive_value()))));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_minutes)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_min_from_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_seconds)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_sec_from_time(es_local_time(this_date->primitive_value()))));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_seconds)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_sec_from_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_milliseconds)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_ms_from_time(es_local_time(this_date->primitive_value()))));

    return true;
}

ES_API_FUN(es_std_date_proto_get_utc_milliseconds)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    if (std::isnan(this_date->primitive_value()))
        frame.set_result(EsValue::from_num(this_date->primitive_value()));
    else
        frame.set_result(EsValue::from_i64(
            es_ms_from_time(this_date->primitive_value())));

    return true;
}

ES_API_FUN(es_std_date_proto_get_time_zone_off)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
    if (this_date == NULL)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "date"));
        return false;
    }

    double t = this_date->primitive_value();
    if (std::isnan(t))
    {
        frame.set_result(EsValue::from_num(t));
    }
    else
    {
        int64_t ms_per_minute = 60000;
        frame.set_result(EsValue::from_num((t - es_local_time(t)) / ms_per_minute));
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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsDate *this_date = es_as_date(frame.this_value());
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

    frame.set_result(EsValue::from_str(
        es_date_time_iso_str(this_date->primitive_value())));
    return true;
}

ES_API_FUN(es_std_date_proto_to_json)
{
    THROW(InternalException, "internal error: es_std_date_proto_to_json is not implemented.");
}

ES_API_FUN(es_std_date_constr_parse)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, string);

    const EsString *s = string.to_stringT();
    if (!s)
        return false;

    frame.set_result(EsValue::from_num(es_date_parse(s)));
    return true;
}

ES_API_FUN(es_std_date_constr_utc)
{
    THROW(InternalException, "internal error: es_std_date_constr_utc is not implemented.");
}

ES_API_FUN(es_std_date_constr_now)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    frame.set_result(EsValue::from_num(time_now()));
    return true;
}

ES_API_FUN(es_std_date)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.9.2
    time_t raw_time = static_cast<time_t>(time_now() / 1000.0);

    struct tm *local_time = localtime(&raw_time);
    assert(local_time);

    frame.set_result(EsValue::from_str(es_date_to_str(local_time)));
    return true;
}

ES_API_FUN(es_std_err_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_OBJ));
        return false;
    }
    
    EsObject *o = frame.this_value().as_object();

    EsValue name_val;
    if (!o->getT(property_keys.name, name_val))
        return false;

    const EsString *name = NULL;
    if (name_val.is_undefined())
    {
        name = _ESTR("Error");
    }
    else
    {
        name = name_val.to_stringT();
        if (!name)
            return false;
    }

    EsValue msg_val;
    if (!o->getT(property_keys.message, msg_val))
        return false;
    
    const EsString *msg = EsString::create();
    if (!msg_val.is_undefined())
    {
        msg = msg_val.to_stringT();
        if (!msg)
            return false;
    }
    
    if (name->empty())
    {
        frame.set_result(EsValue::from_str(msg));
        return true;
    }
    
    if (msg->empty())
    {
        frame.set_result(EsValue::from_str(name));
        return true;
    }
    
    const EsString *res = name;
    res = res->concat(_ESTR(": "));
    res = res->concat(msg);
    
    frame.set_result(EsValue::from_str(res));
    return true;
}

ES_API_FUN(es_std_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_fun_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }

    frame.set_result(EsValue::from_str(
        _ESTR("function Function() { [native code] }")));
    return true;
}

ES_API_FUN(es_std_fun_proto_apply)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, this_arg);
    ES_API_PARAMETER(1, arg_array);
    
    if (!frame.this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    EsFunction *fun = frame.this_value().as_function();
    if (arg_array.is_null() || arg_array.is_undefined())
    {
        EsCallFrame fun_frame = EsCallFrame::push_function(0, fun, this_arg);
        if (!fun->callT(fun_frame))
            return false;

        frame.set_result(fun_frame.result());
        return true;
    }

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

    EsCallFrame fun_frame = EsCallFrame::push_function(n, fun, this_arg);
    for (uint32_t i = 0; i < n; i++)
    {
        EsValue next_arg;
        if (!arg_array_obj->getT(EsPropertyKey::from_u32(i), next_arg))
            return false;

        fun_frame.fp()[i] = next_arg;
    }
    
    if (!fun->callT(fun_frame))
        return false;

    frame.set_result(fun_frame.result());
    return true;
}

ES_API_FUN(es_std_fun_proto_call)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, this_arg);
    
    if (!frame.this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    // FIXME: We can probably wrap the original frame + 1 value offset.
    EsCallFrame new_frame = EsCallFrame::push_function(
        argc > 1 ? argc - 1 : 0, frame.this_value().as_function(), this_arg);
    for (uint32_t i = 1; i < argc; i++)
        new_frame.fp()[i - 1] = fp[i];

    if (!frame.this_value().as_function()->callT(new_frame))
        return false;

    frame.set_result(new_frame.result());
    return true;
}

ES_API_FUN(es_std_fun_proto_bind)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, this_arg);
    
    if (!frame.this_value().is_callable())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_NO_FUN));
        return false;
    }
    
    EsValueVector a;
    if (argc > 1)
    {
        EsCallFrame::Arguments args = frame.arguments();
        for (EsValue *arg = args.begin() + 1; arg != args.end(); arg++)
            a.push_back(*arg);
    }

    frame.set_result(EsValue::from_obj(
        EsFunctionBind::create_inst(frame.this_value().as_function(),
                                    this_arg, a)));
    return true;
}

ES_API_FUN(es_std_fun)
{
    // 15.3.1
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsFunction::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_json_parse)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, text);
    ES_API_PARAMETER(1, reviver);

    const EsString *text_str = text.to_stringT();
    if (!text_str)
        return false;

    StringStream jtext(text_str->str());
    JsonParser parser(jtext);

    EsValue unfiltered;
    if (!parser.parse(unfiltered))
        return false;

    if (reviver.is_callable())
    {
        EsObject *root = EsObject::create_inst();
        if (!ES_DEF_PROPERTY(root, EsPropertyKey::from_str(
                EsString::create()), unfiltered, true, true, true))
            return false;

        EsValue result;
        if (!algorithm::json_walk(EsString::create(), root,
                                  reviver.as_function(), result))
            return false;

        frame.set_result(result);
        return true;
    }

    frame.set_result(unfiltered);
    return true;
}

ES_API_FUN(es_std_json_stringify)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
                        const EsString *v_str = v.to_stringT();
                        if (!v_str)
                            return false;

                        state.prop_list.push_back(v_str);
                    }
                }
            }
        }
    }

    // Calculate gap.
    double space_num = 0.0;
    const EsString *space_str = NULL;

    if (es_as_number(space, space_num))
    {
        int64_t space_int = 0;
        if (!space.to_integer(space_int))
            return false;

        EsStringBuilder sb;
        for (int64_t i = 0; i < std::min(static_cast<int64_t>(10), space_int); i++)
            sb.append(' ');

        state.gap = sb.string();
    }
    else if (es_as_string(space, space_str))
    {
        state.gap = space_str->length() <= 10 ? space_str : space_str->take(10);
    }

    EsObject *wrapper = EsObject::create_inst();
    if (!ES_DEF_PROPERTY(wrapper, EsPropertyKey::from_str(
            EsString::create()), value, true, true ,true))
        return false;

    EsValue result;
    if (!algorithm::json_str(EsString::create(), wrapper, state, result))
        return false;

    frame.set_result(result);
    return true;
}

ES_API_FUN(es_std_math_abs)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(fabs(x_num)));
    return true;
}

ES_API_FUN(es_std_math_acos)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);

    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(acos(x_num)));
    return true;
}

ES_API_FUN(es_std_math_asin)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(asin(x_num)));
    return true;
}

ES_API_FUN(es_std_math_atan)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(atan(x_num)));
    return true;
}

ES_API_FUN(es_std_math_atan2)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    ES_API_PARAMETER(1, y);

    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    double y_num = 0.0;
    if (!y.to_number(y_num))
        return false;

    frame.set_result(EsValue::from_num(atan2(x_num, y_num)));
    return true;
}

ES_API_FUN(es_std_math_ceil)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(ceil(x_num)));
    return true;
}

ES_API_FUN(es_std_math_cos)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(cos(x_num)));
    return true;
}

ES_API_FUN(es_std_math_log)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(log(x_num)));
    return true;
}

ES_API_FUN(es_std_math_max)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.8.2.11
    if (argc == 0)
    {
        frame.set_result(EsValue::from_num(
            -std::numeric_limits<double>::infinity()));
        return true;
    }

    EsCallFrame::Arguments args = frame.arguments();

    EsValue *arg = args.begin();

    double max = 0.0;
    if (!arg->to_number(max))
        return false;

    for (++arg ; arg != args.end(); arg++)
    {
        // CUSTOM: CK 2012-02-11
        // Should use algorithm in 11.8.5.
        double v = 0.0;
        if (!arg->to_number(v))
            return false;

        if (std::isnan(v))
        {
            frame.set_result(EsValue::from_num(v));
            return true;
        }

        if (v > max)
            max = v;
    }

    frame.set_result(EsValue::from_num(max));
    return true;
}

ES_API_FUN(es_std_math_min)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.8.2.12
    if (argc == 0)
    {
        frame.set_result(EsValue::from_num(
            std::numeric_limits<double>::infinity()));
        return true;
    }

    double min = std::numeric_limits<double>::max();

    for (const EsValue &arg : frame.arguments())
    {
        // CUSTOM: CK 2012-02-11
        // Should use algorithm in 11.8.5.
        double v = 0.0;
        if (!arg.to_number(v))
            return false;

        if (std::isnan(v))
        {
            frame.set_result(EsValue::from_num(v));
            return true;
        }

        if (v < min)
            min = v;
    }

    frame.set_result(EsValue::from_num(min));
    return true;
}

ES_API_FUN(es_std_math_exp)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(exp(x_num)));
    return true;
}

ES_API_FUN(es_std_math_floor)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(floor(x_num)));
    return true;
}

ES_API_FUN(es_std_math_pow)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
        frame.set_result(EsValue::from_num(
            std::numeric_limits<double>::quiet_NaN()));
        return true;
    }
    if (y_num == 0.0 || y_num == -0.0)
    {
        frame.set_result(EsValue::from_num(1.0));
        return true;
    }
    if (std::isnan(x_num) && y_num != 0.0)
    {
        frame.set_result(EsValue::from_num(
            std::numeric_limits<double>::quiet_NaN()));
        return true;
    }
    if (fabs(x_num) > 1.0)
    {
        if (y_num == std::numeric_limits<double>::infinity())
        {
            frame.set_result(EsValue::from_num(
                std::numeric_limits<double>::infinity()));
            return true;
        }
        else if (y_num == -std::numeric_limits<double>::infinity())
        {
            frame.set_result(
                EsValue::from_num(0.0));
            return true;
        }
    }
    if (fabs(x_num) == 1.0 && !std::isfinite(y_num))
    {
        frame.set_result(EsValue::from_num(
            std::numeric_limits<double>::quiet_NaN()));
        return true;
    }
    if (fabs(x_num) < 1.0 && !std::isfinite(y_num))
    {
        if (y_num == std::numeric_limits<double>::infinity())
        {
            frame.set_result(EsValue::from_num(0.0));
            return true;
        }
        else if (y_num == -std::numeric_limits<double>::infinity())
        {
            frame.set_result(EsValue::from_num(
                std::numeric_limits<double>::infinity()));
            return true;
        }
    }
    if (x_num == std::numeric_limits<double>::infinity())
    {
        if (y_num > 0.0)
        {
            frame.set_result(EsValue::from_num(
                std::numeric_limits<double>::infinity()));
            return true;
        }
        if (y_num < 0.0)
        {
            frame.set_result(EsValue::from_num(0.0));
            return true;
        }
    }
    if (x_num == -std::numeric_limits<double>::infinity())
    {
        if (y_num > 0.0)
        {
            // Test if odd integer.
            int64_t y_int = static_cast<int64_t>(y_num);
            frame.set_result(EsValue::from_num(
                static_cast<double>(y_num ==
                static_cast<double>(y_int) && (y_int & 1)
                ? -std::numeric_limits<double>::infinity()
                :  std::numeric_limits<double>::infinity())));
            return true;
        }
        if (y_num < 0.0)
        {
            // Test if odd integer.
            int64_t y_int = static_cast<int64_t>(y_num);
            frame.set_result(EsValue::from_num(
                y_num == static_cast<double>(y_int) && (y_int & 1)
                ? -0.0
                :  0.0));
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
                frame.set_result(EsValue::from_num(0.0));
                return true;
            }
            if (y_num < 0.0)
            {
                frame.set_result(EsValue::from_num(
                    std::numeric_limits<double>::infinity()));
                return true;
            }
        }
        else
        {
            if (y_num > 0.0)
            {
                // Test if odd integer.
                int64_t y_int = static_cast<int64_t>(y_num);
                frame.set_result(EsValue::from_num(
                    y_num == static_cast<double>(y_int) && (y_int & 1)
                    ? -0.0
                    :  0.0));
                return true;
            }
            if (y_num < 0.0)
            {
                // Test if odd integer.
                int64_t y_int = static_cast<int64_t>(y_num);
                frame.set_result(EsValue::from_num(
                    y_num == static_cast<double>(y_int) && (y_int & 1)
                    ? -std::numeric_limits<double>::infinity()
                    :  std::numeric_limits<double>::infinity()));
                return true;
            }
        }
    }
    bool y_is_int = y_num == static_cast<double>(static_cast<int64_t>(y_num));
    if (x_num < 0.0 && std::isfinite(x_num) && std::isfinite(y_num) && !y_is_int)
        frame.set_result(EsValue::from_num(
            std::numeric_limits<double>::quiet_NaN()));
    else
        frame.set_result(EsValue::from_num(pow(x_num, y_num)));
    return true;
}

ES_API_FUN(es_std_math_random)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    frame.set_result(EsValue::from_num(
        static_cast<double>(rand()) / static_cast<double>(RAND_MAX)));
    return true;
}

ES_API_FUN(es_std_math_round)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    if (x_num <= 0.0 && x_num > 0.5)
    {
        if ((x_num == 0.0 && copysign(1.0, x_num) < 0.0) ||
            (x_num < 0.0 && x_num > -0.5))
        {
            frame.set_result(EsValue::from_num(-0.0));
            return true;
        }
    }

    frame.set_result(EsValue::from_num(floor(x_num + 0.5)));
    return true;
}

ES_API_FUN(es_std_math_sin)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(sin(x_num)));
    return true;
}

ES_API_FUN(es_std_math_sqrt)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(sqrt(x_num)));
    return true;
}

ES_API_FUN(es_std_math_tan)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, x);
    
    double x_num = 0.0;
    if (!x.to_number(x_num))
        return false;

    frame.set_result(EsValue::from_num(tan(x_num)));
    return true;
}

ES_API_FUN(es_std_eval_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsEvalError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_range_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsRangeError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_ref_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsReferenceError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_syntax_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsSyntaxError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_type_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsTypeError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_uri_err)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);
    return EsUriError::default_constr()->constructT(frame);
}

ES_API_FUN(es_std_num_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    int32_t radix = 10;
    if (argc >= 1 &&
        !frame.arg(0).is_undefined() &&
        !frame.arg(0).to_int32(radix))
    {
        return false;
    }
    
    // NOTE: This might result in endless recursion which is dangerous, don't
    //       handle this special case but process base 10 as any other.
    //if (radix == 10)
    //    return frame.this_value().to_stringT();
    
    if (radix < 2 || radix > 36)
    {
        ES_THROW(EsRangeError, es_fmt_msg(ES_MSG_RANGE_RADIX));
        return false;
    }

    // Convert.
    char buffer[2048];  // Should be large enough to hold the largest possible value.

    double val = 0.0;
    if (es_as_number(frame.this_value(), val))
    {
        if (radix == 10)
        {
            frame.set_result(EsValue::from_str(es_num_to_str(val)));
        }
        else
        {
            double_to_cstring(val, radix, buffer);
            frame.set_result(EsValue::from_str(EsString::create_from_utf8(buffer)));
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
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    double val = 0.0;
    if (es_as_number(frame.this_value(), val))
    {
        frame.set_result(EsValue::from_num(val));
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));
    return false;
}

ES_API_FUN(es_std_num_proto_to_fixed)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
    if (!es_as_number(frame.this_value(), x))
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));        // CUSTOM: CK 2012-02-11
        return false;
    }

    // FIXME: Use StringBuilder to construct s.
    const EsString *s = es_num_to_str(x, static_cast<int>(f));

    if (x < 1e21)
    {
        // s will be rounded to the specified number of bits but we still have to
        // do padding.
        bool found_point = false;

        int pad_digits = static_cast<int>(f);
        for (size_t i = 0; i < s->length(); i++)
        {
            if (s->at(i) == '.')
                found_point = true;
            else if (found_point)
                pad_digits--;
        }

        if (!found_point && pad_digits > 0)
            s = s->concat(_ESTR("."));

        for (int i = 0; i < pad_digits; i++)
            s = s->concat(_ESTR("0"));
    }

    frame.set_result(EsValue::from_str(s));
    return true;
}

ES_API_FUN(es_std_num_proto_to_exp)
{
    THROW(InternalException, "internal error: es_std_num_proto_to_exp is not implemented.");
}

ES_API_FUN(es_std_num_proto_to_prec)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, precision);

    double x = 0.0;
    if (!es_as_number(frame.this_value(), x))
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "number"));        // CUSTOM: CK 2012-02-11
        return false;
    }

    if (precision.is_undefined())
    {
        frame.set_result(EsValue::from_str(es_num_to_str(x)));
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

    frame.set_result(EsValue::from_str(es_num_to_str(x, p)));
    return true;
}

ES_API_FUN(es_std_num)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, value);

    // FIXME: What's this!?
    if (argc == 0)
    {
        frame.set_result(EsValue::from_u32(0));
    }
    else
    {
        double num = 0.0;
        if (!value.to_number(num))
            return false;

        frame.set_result(EsValue::from_num(num));
    }
    
    return true;
}

ES_API_FUN(es_std_obj_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (frame.this_value().is_undefined())
    {
        frame.set_result(EsValue::from_str(_ESTR("[object Undefined]")));
        return true;
    }

    if (frame.this_value().is_null())
    {
        frame.set_result(EsValue::from_str(_ESTR("[object Null]")));
        return true;
    }
    
    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;
    
    const EsString *res = _ESTR("[object ")->concat(EsString::create(
            o->class_name()))->concat(_ESTR("]"));
    frame.set_result(EsValue::from_str(res));
    return true;
}

ES_API_FUN(es_std_obj_proto_to_loc_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
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
    
    EsCallFrame new_frame = EsCallFrame::push_function(
        0, to_string.as_function(), EsValue::from_obj(o));
    if (!to_string.as_function()->callT(new_frame))
        return false;

    frame.set_result(new_frame.result());
    return true;
}

ES_API_FUN(es_std_obj_proto_val_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;

    frame.set_result(EsValue::from_obj(o));
    return true;
}

ES_API_FUN(es_std_obj_proto_has_own_prop)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, v);

    const EsString *p = v.to_stringT();
    if (!p)
        return false;

    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;

    frame.set_result(EsValue::from_bool(
        o->get_own_property(EsPropertyKey::from_str(p))));
    return true;
}

ES_API_FUN(es_std_obj_proto_is_proto_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // FIXME: Can we use ES_API_PARAMETER_OBJ here?
    ES_API_PARAMETER(0, v);
    
    if (!v.is_object())
    {
        frame.set_result(EsValue::from_bool(false));
        return true;
    }
    
    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;

    EsObject *v_obj = v.as_object();

    while (true)
    {
        v_obj = v_obj->prototype();
        if (v_obj == NULL)
        {
            frame.set_result(EsValue::from_bool(false));
            return true;
        }
        
        if (v_obj == o)
        {
            frame.set_result(EsValue::from_bool(true));
            return true;
        }
    }

    assert(false);
    return true;
}

ES_API_FUN(es_std_obj_proto_prop_is_enum)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, v);

    const EsString *p = v.to_stringT();
    if (!p)
        return false;

    EsObject *o = frame.this_value().to_objectT();
    if (!o)
        return false;
    
    EsPropertyReference prop = o->get_own_property(EsPropertyKey::from_str(p));
    frame.set_result(EsValue::from_bool(prop && prop->is_enumerable()));
    return true;
}

ES_API_FUN(es_std_obj)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (argc == 0 ||
        frame.arg(0).is_null() ||
        frame.arg(0).is_undefined())
    {
        return EsObject::default_constr()->constructT(frame);
    }

    EsObject *o = frame.arg(0).to_objectT();
    if (!o)
        return false;

    frame.set_result(EsValue::from_obj(o));
    return true;
}

ES_API_FUN(es_std_obj_get_proto_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    frame.set_result(o_obj->prototype()
        ? EsValue::from_obj(o_obj->prototype())
        : EsValue::null);
    return true;
}

ES_API_FUN(es_std_obj_get_own_prop_desc)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, o);
    ES_API_PARAMETER(1, p);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    const EsString *name = p.to_stringT();
    if (!name)
        return false;

    frame.set_result(es_from_property_descriptor(
        o_obj->get_own_property(EsPropertyKey::from_str(name))));
    return true;
}

ES_API_FUN(es_std_obj_get_own_prop_names)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
        const EsString *val = str->primitive_value();
        for (size_t j = 0; j < val->length(); j++)
        {
            if (!ES_DEF_PROPERTY(array, EsPropertyKey::from_u32(i++),
                    EsValue::from_str(EsString::create(j)), true, true, true))
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

    frame.set_result(EsValue::from_obj(array));
    return true;
}

ES_API_FUN(es_std_obj_create)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
        // This is a little bit scary, but we're re-using the current stack frame.
        frame.fp()[0].set_obj(obj);

        es_std_obj_def_props(ctx, 2,
                             frame.fp(),
                             frame.vp());
    }

    frame.set_result(EsValue::from_obj(obj));   // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_def_prop)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, o);
    ES_API_PARAMETER(1, p);
    ES_API_PARAMETER(2, attributes);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    const EsString *name = p.to_stringT();
    if (!name)
        return false;

    EsPropertyDescriptor prop;
    if (!es_to_property_descriptor(attributes, prop))
        return false;

    frame.set_result(EsValue::from_obj(o_obj)); // FIXME: o?
    return o_obj->define_own_propertyT(EsPropertyKey::from_str(name), prop, true);
}

ES_API_FUN(es_std_obj_def_props)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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

    frame.set_result(EsValue::from_obj(o_obj)); // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_seal)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
    frame.set_result(EsValue::from_obj(o_obj)); // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_freeze)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
    frame.set_result(EsValue::from_obj(o_obj)); // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_prevent_exts)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    o_obj->set_extensible(false);
    frame.set_result(EsValue::from_obj(o_obj)); // FIXME: o?
    return true;
}

ES_API_FUN(es_std_obj_is_sealed)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
            frame.set_result(EsValue::from_bool(false));
            return true;
        }
    }

    frame.set_result(EsValue::from_bool(!o_obj->is_extensible()));
    return true;
}

ES_API_FUN(es_std_obj_is_frozen)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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
            frame.set_result(EsValue::from_bool(false));
            return true;
        }

        if (prop->is_configurable())
        {
            frame.set_result(EsValue::from_bool(false));
            return true;
        }
    }

    frame.set_result(EsValue::from_bool(!o_obj->is_extensible()));
    return true;
}

ES_API_FUN(es_std_obj_is_extensible)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, o);

    if (!o.is_object())
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "object"));
        return false;
    }

    EsObject *o_obj = o.as_object();

    frame.set_result(EsValue::from_bool(o_obj->is_extensible()));
    return true;
}

ES_API_FUN(es_std_obj_keys)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

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

    frame.set_result(EsValue::from_obj(array));
    return ES_DEF_PROPERTY(array, property_keys.length,
                           EsValue::from_u32(n), true, false, false);  // VERIFIED: 15.4.5.2
}

ES_API_FUN(es_std_str_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.5.4.2
    const EsString *val = NULL;
    if (es_as_string(frame.this_value(), val))
    {
        frame.set_result(EsValue::from_str(val));
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "string"));
    return false;
}

ES_API_FUN(es_std_str_proto_val_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    // 15.5.4.3
    const EsString *val = NULL;
    if (es_as_string(frame.this_value(), val))
    {
        frame.set_result(EsValue::from_str(val));
        return true;
    }
    
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "string"));
    return false;
}

ES_API_FUN(es_std_str_proto_char_at)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, pos);
    
    if (!frame.this_value().chk_obj_coercibleT())
        return false;
    
    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    int64_t position = 0;
    if (!pos.to_integer(position))
        return false;

    size_t size = s->length();
    
    if (position < 0 || position >= static_cast<int64_t>(size))
        frame.set_result(EsValue::from_str(EsString::create()));
    else
        frame.set_result(EsValue::from_str(EsString::create(s->at(position))));
    
    return true;
}

ES_API_FUN(es_std_str_proto_char_code_at)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, pos);
    
    if (!frame.this_value().chk_obj_coercibleT())
        return false;
    
    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    int64_t pos_int = 0;
    if (!pos.to_integer(pos_int))
        return false;
    
    if (pos_int < 0 || pos_int >= static_cast<int64_t>(s->length()))
        frame.set_result(EsValue::from_num(
            std::numeric_limits<double>::quiet_NaN()));
    else
        frame.set_result(EsValue::from_i32(s->at(pos_int)));

    return true;
}

ES_API_FUN(es_std_str_proto_concat)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *r = frame.this_value().to_stringT();
    if (!r)
        return false;

    for (const EsValue &arg : frame.arguments())
    {
        const EsString *str = arg.to_stringT();
        if (!str)
            return false;

        r = r->concat(str);
    }

    frame.set_result(EsValue::from_str(r));
    return true;
}

ES_API_FUN(es_std_str_proto_index_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, search_string);
    ES_API_PARAMETER(1, position);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    const EsString *search_str = search_string.to_stringT();
    if (!search_str)
        return false;

    int64_t pos = 0;
    if (!position.is_undefined() && !position.to_integer(pos))
        return false;

    int64_t len = static_cast<int64_t>(s->length());

    int64_t start = std::min(std::max(pos, static_cast<int64_t>(0)), len);
    frame.set_result(EsValue::from_i64(s->index_of(search_str, start)));
    return true;
}

ES_API_FUN(es_std_str_proto_last_index_of)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, search_string);
    ES_API_PARAMETER(1, position);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    const EsString *search_str = search_string.to_stringT();
    if (!search_str)
        return false;

    int64_t pos = 0;
    if (!position.to_integer(pos))  // 0xffffffff is not really positive infinity.
        return false;

    int64_t len = static_cast<int64_t>(s->length());

    int64_t start = std::min(std::max(pos, static_cast<int64_t>(0)), len);
    frame.set_result(EsValue::from_i64(s->last_index_of(search_str, start)));
    return true;
}

ES_API_FUN(es_std_str_proto_locale_compare)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, that);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;
    const EsString *t = that.to_stringT();
    if (!t)
        return false;

    // From ECMA-262 15.5.4.9:
    // If no language-sensitive comparison at all is available from the host
    // environment, this function may perform a bitwise comparison.
    frame.set_result(EsValue::from_i32(s->compare(t)));
    return true;
}

ES_API_FUN(es_std_str_proto_match)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, regexp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    EsRegExp *rx = es_as_reg_exp(regexp);
    if (!rx)
    {
        const EsString *regexp_str = regexp.to_stringT();
        if (!regexp_str)
            return false;

        rx = EsRegExp::create_inst(regexp.is_undefined()
            ? EsString::create()
            : regexp_str, EsString::create());
        if (rx == NULL)
            return false;
    }

    EsValue exec_val;
    if (!rx->getT(property_keys.exec, exec_val))
        return false;

    EsFunction *exec = exec_val.as_function();

    EsValue global_val;
    if (!rx->getT(property_keys.global, global_val))
        return false;

    EsCallFrame exec_frame = EsCallFrame::push_function(
        1, exec, EsValue::from_obj(rx));
    exec_frame.fp()[0] = EsValue::from_str(s);

    if (!global_val.to_boolean())
    {
        if (!exec->callT(exec_frame))
            return false;

        frame.set_result(exec_frame.result());
        return true;
    }

    if (!rx->putT(property_keys.last_index, EsValue::from_u32(0), true))
        return false;

    EsArray *a = EsArray::create_inst();
    int64_t prev_last_index = 0, n = 0;

    bool last_match = true;
    while (last_match)
    {
        // FIXME: This frame should be inlined in the previous frame.
        EsCallFrame exec_frame = EsCallFrame::push_function(
            1, exec, EsValue::from_obj(rx));
        exec_frame.fp()[0] = EsValue::from_str(s);

        if (!exec->callT(exec_frame))
            return false;

        EsValue exec_res = exec_frame.result();
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

    frame.set_result(n == 0 ? EsValue::null : EsValue::from_obj(a));
    return true;
}

ES_API_FUN(es_std_str_proto_replace)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, search_value);
    ES_API_PARAMETER(1, replace_value);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
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

            for (size_t i = 0; i <= s->length(); i++)
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

                if (last_index >= 0)
                {
                    for (size_t i = last_index; i <= s->length(); i++)
                    {
                        res = rx->match(s, i);
                        if (res != NULL)
                            break;
                    }
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
        const EsString *search_str = search_value.to_stringT();
        if (!search_str)
            return false;

        ssize_t i = s->index_of(search_str);
        if (i != -1)
        {
            EsRegExp::MatchResult::MatchStateVector tmp;
            tmp.push_back(EsRegExp::MatchState(i, search_str->length(), search_str));

            matches.push_back(tmp);
        }
    }

    EsStringBuilder sb;

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

        if (off + len <= static_cast<int>(s->length()))
            sb.append(s->data() + off, len);

        last_off = state.offset() + state.length();

        // Append the replaced text.
        if (replace_value.is_callable())
        {
            EsCallFrame fun_frame = EsCallFrame::push_function(
                using_reg_ex ? (*it).size() + 2 : 3,
                replace_value.as_function(), EsValue::undefined);
            EsValue *fun_fp = fun_frame.fp();

            if (using_reg_ex)
            {
                EsRegExp::MatchResult::MatchStateVector::const_iterator it_sub;
                for (it_sub = (*it).begin(); it_sub != (*it).end(); ++it_sub)
                    *fun_fp++ = EsValue::from_str((*it_sub).string());
            }
            else
            {
                *fun_fp++ = EsValue::from_str(state.string());
            }

            *fun_fp++ = EsValue::from_i32(state.offset());
            *fun_fp++ = EsValue::from_str(s);

            if (!replace_value.as_function()->callT(fun_frame))
                return false;

            const EsString *fun_res_str = fun_frame.result().to_stringT();
            if (!fun_res_str)
                return false;

            sb.append(fun_res_str);
        }
        else
        {
            const EsString *replace_str = replace_value.to_stringT();
            if (!replace_str)
                return false;

            if (replace_str->empty())
                continue;

            EsStringBuilder sb2;

            bool dollar_mode = false;
            for (size_t i = 0; i < replace_str->length(); i++)
            {
                uni_char c = replace_str->at(i);

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
                            sb2.append(s->substr(0, state.offset()));
                            break;
                        case '\'':
                        {
                            int off = state.offset() + state.length();
                            int len = s->length() - off;
                            sb2.append(s->substr(off, len));
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
                            if (j < static_cast<int>(replace_str->length()) &&
                                es_is_dec_digit(replace_str->at(j)) &&
                                (*it).size() > 9)   // Don't try to interpret two digits if one would be sufficient.
                            {
                                n = static_cast<size_t>(c - '0') * 10 +
                                        static_cast<size_t>(replace_str->at(j) - '0');
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
                sb2.append(replace_str->at(replace_str->length() - 1));
            }

            sb.append(sb2.string());
        }
    }

    if (last_off < static_cast<int>(s->length()))
        sb.append(s->data() + last_off, s->length() - last_off);

    frame.set_result(EsValue::from_str(sb.string()));
    return true;
}

ES_API_FUN(es_std_str_proto_search)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, regexp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    EsRegExp *rx = es_as_reg_exp(regexp);
    if (!rx)
    {
        const EsString *regexp_str = regexp.to_stringT();
        if (!regexp_str)
            return false;

        rx = EsRegExp::create_inst(regexp.is_undefined()
            ? EsString::create()
            : regexp_str, EsString::create());
        if (rx == NULL)
            return false;
    }

    for (size_t i = 0; i <= s->length(); i++)
    {
        EsRegExp::MatchResult *res = rx->match(s, i);
        if (res)
        {
            assert(res->length() > 0);
            frame.set_result(EsValue::from_i32(res->matches()[0].offset()));
            return true;
        }
    }

    frame.set_result(EsValue::from_i32(-1.0));
    return true;
}

ES_API_FUN(es_std_str_proto_slice)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, end);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    int64_t len = s->length();

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

    frame.set_result(EsValue::from_str(s->substr(from, span)));
    return true;
}

ES_API_FUN(es_std_str_proto_split)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, separator);
    ES_API_PARAMETER(1, limit);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    EsArray *a = EsArray::create_inst();
    uint32_t lim = 0xffffffff;
    if (!limit.is_undefined() && !limit.to_uint32(lim))
        return false;

    EsRegExp *r_reg = es_as_reg_exp(separator);

    const EsString *r_str = EsString::create();
    if (r_reg == NULL)
    {
        r_str = separator.to_stringT();
        if (!r_str)
            return false;
    }

    if (lim == 0)
    {
        frame.set_result(EsValue::from_obj(a));
        return true;
    }

    if (separator.is_undefined())
    {
        frame.set_result(EsValue::from_obj(a));
        return ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(0), EsValue::from_str(s), true, true, true);
    }

    if (s->empty())
    {
        algorithm::MatchResult *z = NULL;
        if (r_reg)
            z = algorithm::split_match(s, 0, r_reg);
        else
            z = algorithm::split_match(s, 0, r_str);

        if (z)
        {
            frame.set_result(EsValue::from_obj(a));
            return true;
        }

        frame.set_result(EsValue::from_obj(a));
        return ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(0), EsValue::from_str(s), true, true, true);
    }

    uint32_t length_a = 0, p = 0, q = 0;

    while (q != static_cast<uint32_t>(s->length()))
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
            EsStringVector &cap = z->cap_;

            if (e == p)
            {
                q++;
            }
            else
            {
                const EsString *t = s->substr(p, q - p);
                if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(length_a++), EsValue::from_str(t), true, true, true))
                    return false;

                if (length_a == lim)
                {
                    frame.set_result(EsValue::from_obj(a));
                    return true;
                }

                p = e;

                for (size_t i = 1; i < cap.size(); i++)
                {
                    if (!ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(length_a++), EsValue::from_str(cap[i]), true, true, true))
                        return false;

                    if (length_a == lim)
                    {
                        frame.set_result(EsValue::from_obj(a));
                        return true;
                    }
                }

                q = p;
            }
        }
    }

    frame.set_result(EsValue::from_obj(a));

    const EsString *t = s->substr(p, s->length() - p);
    return ES_DEF_PROPERTY(a, EsPropertyKey::from_u32(length_a),
                           EsValue::from_str(t), true, true, true);
}

ES_API_FUN(es_std_str_proto_substr)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, length);

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    int64_t s_len = static_cast<int64_t>(s->length());

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

    frame.set_result(EsValue::from_str(final_length <= 0
        ? EsString::create()
        : s->substr(final_start, final_length)));;
    return true;
}

ES_API_FUN(es_std_str_proto_substring)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, start);
    ES_API_PARAMETER(1, end);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    int64_t len = static_cast<int64_t>(s->length());

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

    frame.set_result(EsValue::from_str(s->substr(from, to - from)));
    return true;
}

ES_API_FUN(es_std_str_proto_to_lower_case)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    frame.set_result(EsValue::from_str(s->lower()));
    return true;
}

ES_API_FUN(es_std_str_proto_to_locale_lower_case)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    frame.set_result(EsValue::from_str(s->lower()));
    return true;
}

ES_API_FUN(es_std_str_proto_to_upper_case)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    frame.set_result(EsValue::from_str(s->upper()));
    return true;
}

ES_API_FUN(es_std_str_proto_to_locale_upper_case)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    frame.set_result(EsValue::from_str(s->upper()));
    return true;
}

bool es_is_white_space_or_line_term(uni_char c)
{
    return es_is_white_space(c) || es_is_line_terminator(c);
}

ES_API_FUN(es_std_str_proto_trim)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    if (!frame.this_value().chk_obj_coercibleT())
        return false;

    const EsString *s = frame.this_value().to_stringT();
    if (!s)
        return false;

    frame.set_result(EsValue::from_str(
            s->trim(es_is_white_space_or_line_term)));
    return true;
}

ES_API_FUN(es_std_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    const EsString *str = EsString::create();
    if (argc > 0)
    {
        str = frame.arg(0).to_stringT();
        if (!str)
            return false;
    }

    frame.set_result(EsValue::from_str(str));
    return true;
}

ES_API_FUN(es_std_str_from_char_code)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    byte *buf = new byte[argc * 6 + 1];     // One UTF-8 character may potentially be 6 bytes, not in this particular case but in general.
    byte *ptr = buf;
    
    for (uint32_t i = 0; i < argc; i++)
    {
        const EsValue &arg = frame.arg(i);
        
        double arg_num = 0.0;
        if (!arg.to_number(arg_num))
            return false;

        uint16_t num = es_to_uint16(arg_num);
        utf8_enc(ptr, static_cast<uni_char>(num));
    }
    
    *ptr = '\0';
    
    const EsString *str = EsString::create_from_utf8(
            reinterpret_cast<const char *>(buf), static_cast<int>(ptr - buf));
    
    delete [] buf;
    frame.set_result(EsValue::from_str(str));
    return true;
}

ES_API_FUN(es_std_reg_exp_proto_exec)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, string);

    EsRegExp *r = es_as_reg_exp(frame.this_value());
    if (!r)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "regular expression"));
        return false;
    }

    const EsString *s = string.to_stringT();
    if (!s)
        return false;

    size_t length = s->length();

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
            frame.set_result(EsValue::null);
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

    frame.set_result(EsValue::from_obj(a));
    return true;
}

ES_API_FUN(es_std_reg_exp_proto_test)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, string);

    EsRegExp *r = es_as_reg_exp(frame.this_value());
    if (!r)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "regular expression"));
        return false;
    }

    const EsString *s = string.to_stringT();
    if (!s)
        return false;

    int64_t length = s->length();

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
            frame.set_result(EsValue::from_bool(false));
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

    frame.set_result(EsValue::from_bool(true));
    return true;
}

ES_API_FUN(es_std_reg_exp_proto_to_str)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    EsRegExp *r = es_as_reg_exp(frame.this_value());
    if (!r)
    {
        ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_WRONG_TYPE, "regular expression"));
        return false;
    }

    EsStringBuilder sb;
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

    frame.set_result(EsValue::from_str(sb.string()));
    return true;
}

ES_API_FUN(es_std_reg_exp)
{
    EsCallFrame frame = EsCallFrame::wrap(argc, fp, vp);

    ES_API_PARAMETER(0, pattern);
    ES_API_PARAMETER(1, flags);

    if (flags.is_undefined() && pattern.is_object())
    {
        EsObject *o = pattern.as_object();
        if (o->class_name() == _USTR("RegExp"))
        {
            frame.set_result(EsValue::from_obj(o));
            return true;
        }
    }

    return EsRegExp::default_constr()->constructT(frame);
}
