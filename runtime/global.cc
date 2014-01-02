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

#include <cmath>
#include <limits>
#include <gc/gc_cpp.h>
#include "common/exception.hh"
#include "environment.hh"
#include "error.hh"
#include "eval.hh"
#include "global.hh"
#include "math.hh"
#include "messages.hh"
#include "object.hh"
#include "property.hh"
#include "prototype.hh"
#include "standard.hh"
#include "test.hh"
#include "utility.hh"

#ifndef ES_DEF_GLOBAL_PROPERTY
#define ES_DEF_GLOBAL_PROPERTY(obj, p, v) \
    obj->define_new_own_property(p, EsPropertyDescriptor(false, true, true, v));
#endif

#ifndef ES_DEF_GLOBAL_PROPERTY_RD_ONLY
#define ES_DEF_GLOBAL_PROPERTY_RD_ONLY(obj, p, v) \
    obj->define_new_own_property(p, EsPropertyDescriptor(false, false, false, v));
#endif

#ifndef ES_DEF_GLOBAL_PROPERTY_FUN
#define ES_DEF_GLOBAL_PROPERTY_FUN(obj, p, fun_ptr, fun_len) \
    obj->define_new_own_property(p, EsPropertyDescriptor(false, true, true, EsValue::from_obj(EsBuiltinFunction::create_inst(global_env, fun_ptr, fun_len, false))));
#endif

EsLexicalEnvironment *global_env = NULL;
EsObject *global_obj = NULL;

void es_global_create()
{
    // 10.2.3
    assert(global_env == NULL);
    assert(global_obj == NULL);

    global_obj = EsObject::create_raw();
    global_env = es_new_obj_env(global_obj, NULL, false);
}

void es_global_init()
{
    assert(global_env);
    assert(global_obj);
    
    global_obj->make_inst();

    // 8.5
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(global_obj, EsPropertyKey::from_str(_USTR("Infinity")), EsValue::from_num(std::numeric_limits<double>::infinity()));
    
    // Add global properties.
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(global_obj, property_keys.nan, EsValue::from_num(std::numeric_limits<double>::quiet_NaN()));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(global_obj, property_keys.infinity, EsValue::from_num(std::numeric_limits<double>::infinity()));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(global_obj, property_keys.undefined, EsValue::undefined);
    
    // Add global functions.
    ES_DEF_GLOBAL_PROPERTY(global_obj, property_keys.eval, EsValue::from_obj(EsEvalFunction::create_inst()));
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("$PRINT")), es_std_print, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("PRINT")), es_std_print, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("print")), es_std_print, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("$ERROR")), es_std_error, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("ERROR")), es_std_error, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("$FAIL")), es_std_error, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("runTestCase")), es_std_run_test_case, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("fnGlobalObject")), es_std_fn_glob_obj, 0);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("fnExists")), es_std_fn_exists, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("compareArray")), es_std_compare_array, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, EsPropertyKey::from_str(_USTR("arrayContains")), es_std_array_contains, 2);
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("dataPropertyAttributesAreCorrect")), es_new_std_data_prop_attr_are_correct_function(global_env));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("accessorPropertyAttributesAreCorrect")), es_new_std_accessor_prop_attr_are_correct_function(global_env));

    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.encode_uri, es_std_encode_uri, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.encode_uri_component, es_std_encode_uri_component, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.decode_uri, es_std_decode_uri, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.decode_uri_component, es_std_decode_uri_component, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.is_nan, es_std_is_nan, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.is_finite, es_std_is_finite, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.parse_float, es_std_parse_float, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(global_obj, property_keys.parse_int, es_std_parse_int, 2);
    
    // Add boolean object.
    EsObject *array = EsArray::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Array")), EsValue::from_obj(array));
    
    // Add boolean object.
    EsObject *boolean = EsBooleanObject::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Boolean")), EsValue::from_obj(boolean));
    
    // Add date object.
    EsObject *date = EsDate::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Date")), EsValue::from_obj(date));
    
    // Add function object.
    EsObject *fun = EsFunction::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Function")), EsValue::from_obj(fun));
    
    // Add math object.
    EsObject *math = EsObject::create_inst_with_class(_USTR("Math"));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Math")), EsValue::from_obj(math));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.e, EsValue::from_num(ES_MATH_E));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.ln10, EsValue::from_num(ES_MATH_LN10));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.ln2, EsValue::from_num(ES_MATH_LN2));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.log10e, EsValue::from_num(ES_MATH_LOG10E));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.log2e, EsValue::from_num(ES_MATH_LOG2E));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.pi, EsValue::from_num(ES_MATH_PI));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.sqrt1_2, EsValue::from_num(ES_MATH_SQRT1_2));
    ES_DEF_GLOBAL_PROPERTY_RD_ONLY(math, property_keys.sqrt2, EsValue::from_num(ES_MATH_SQRT2));
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.abs, es_std_math_abs, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.acos, es_std_math_acos, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.asin, es_std_math_asin, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.atan, es_std_math_atan, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.atan2, es_std_math_atan2, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.ceil, es_std_math_ceil, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.cos, es_std_math_cos, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.exp, es_std_math_exp, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.floor, es_std_math_floor, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.log, es_std_math_log, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.max, es_std_math_max, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.min, es_std_math_min, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.pow, es_std_math_pow, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.random, es_std_math_random, 0);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.round, es_std_math_round, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.sin, es_std_math_sin, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.sqrt, es_std_math_sqrt, 1);
    ES_DEF_GLOBAL_PROPERTY_FUN(math, property_keys.tan, es_std_math_tan, 1);

    // Add JSON object.
    EsObject *json = EsObject::create_inst_with_class(_USTR("JSON"));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("JSON")), EsValue::from_obj(json));
    ES_DEF_GLOBAL_PROPERTY_FUN(json, property_keys.parse, es_std_json_parse, 2);
    ES_DEF_GLOBAL_PROPERTY_FUN(json, property_keys.stringify, es_std_json_stringify, 3);

    // Add number object.
    EsFunction *num = EsNumberObject::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Number")), EsValue::from_obj(num));
    
    // Add object object.
    EsObject *obj = EsObject::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Object")), EsValue::from_obj(obj));
    
    // Add string object.
    EsFunction *str = EsStringObject::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("String")), EsValue::from_obj(str));

    // Add regular expression object.
    EsFunction *reg_exp = EsRegExp::default_constr();
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("RegExp")), EsValue::from_obj(reg_exp));
    
    // Add error objects.
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("Error")), EsValue::from_obj(EsError::default_constr()));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("EvalError")), EsValue::from_obj(EsEvalError::default_constr()));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("RangeError")), EsValue::from_obj(EsRangeError::default_constr()));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("ReferenceError")), EsValue::from_obj(EsReferenceError::default_constr()));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("SyntaxError")), EsValue::from_obj(EsSyntaxError::default_constr()));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("TypeError")), EsValue::from_obj(EsTypeError::default_constr()));
    ES_DEF_GLOBAL_PROPERTY(global_obj, EsPropertyKey::from_str(_USTR("URIError")), EsValue::from_obj(EsUriError::default_constr()));
}

EsLexicalEnvironment *es_global_env()
{
    assert(global_env);
    return global_env;
}

EsObject *es_global_obj()
{
    assert(global_obj);
    return global_obj;
}
