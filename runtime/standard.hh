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

#pragma once
#include <stdarg.h>
#include "api.hh"

class EsFunctionContext;

/*
 * Extra
 */
ES_API_FUN(es_std_print);
ES_API_FUN(es_std_error);
ES_API_FUN(es_std_run_test_case);
ES_API_FUN(es_std_fn_glob_obj);
ES_API_FUN(es_std_fn_exists);
ES_API_FUN(es_std_compare_array);
ES_API_FUN(es_std_array_contains);

/*
 * Global Object
 */
ES_API_FUN(es_std_decode_uri);
ES_API_FUN(es_std_decode_uri_component);
ES_API_FUN(es_std_encode_uri);
ES_API_FUN(es_std_encode_uri_component);
ES_API_FUN(es_std_eval);
ES_API_FUN(es_std_is_nan);
ES_API_FUN(es_std_is_finite);
ES_API_FUN(es_std_parse_float);
ES_API_FUN(es_std_parse_int);

/*
 * Array Prototype
 */
ES_API_FUN(es_std_arr_proto_to_str);
ES_API_FUN(es_std_arr_proto_to_locale_str);
ES_API_FUN(es_std_arr_proto_concat);
ES_API_FUN(es_std_arr_proto_join);
ES_API_FUN(es_std_arr_proto_pop);
ES_API_FUN(es_std_arr_proto_push);
ES_API_FUN(es_std_arr_proto_reverse);
ES_API_FUN(es_std_arr_proto_shift);
ES_API_FUN(es_std_arr_proto_slice);
ES_API_FUN(es_std_arr_proto_sort);
ES_API_FUN(es_std_arr_proto_splice);
ES_API_FUN(es_std_arr_proto_unshift);
ES_API_FUN(es_std_arr_proto_index_of);
ES_API_FUN(es_std_arr_proto_last_index_of);
ES_API_FUN(es_std_arr_proto_every);
ES_API_FUN(es_std_arr_proto_some);
ES_API_FUN(es_std_arr_proto_for_each);
ES_API_FUN(es_std_arr_proto_map);
ES_API_FUN(es_std_arr_proto_filter);
ES_API_FUN(es_std_arr_proto_reduce);
ES_API_FUN(es_std_arr_proto_reduce_right);

/*
 * Array Constructor
 */
ES_API_FUN(es_std_arr_constr_is_arr);

/*
 * Array Object
 */
ES_API_FUN(es_std_arr);       // Constructor called as function.

/*
 * Boolean Prototype
 */
ES_API_FUN(es_std_bool_proto_to_str);
ES_API_FUN(es_std_bool_proto_val_of);

/*
 * Boolean Object
 */
ES_API_FUN(es_std_bool);      // Constructor called as function.

/*
 * Date Prototype
 */
ES_API_FUN(es_std_date_proto_to_str);
ES_API_FUN(es_std_date_proto_to_date_str);
ES_API_FUN(es_std_date_proto_to_time_str);
ES_API_FUN(es_std_date_proto_to_locale_str);
ES_API_FUN(es_std_date_proto_to_locale_date_str);
ES_API_FUN(es_std_date_proto_to_locale_time_str);
ES_API_FUN(es_std_date_proto_val_of);
ES_API_FUN(es_std_date_proto_get_time);
ES_API_FUN(es_std_date_proto_get_full_year);
ES_API_FUN(es_std_date_proto_get_utc_full_year);
ES_API_FUN(es_std_date_proto_get_month);
ES_API_FUN(es_std_date_proto_get_utc_month);
ES_API_FUN(es_std_date_proto_get_date);
ES_API_FUN(es_std_date_proto_get_utc_date);
ES_API_FUN(es_std_date_proto_get_day);
ES_API_FUN(es_std_date_proto_get_utc_day);
ES_API_FUN(es_std_date_proto_get_hours);
ES_API_FUN(es_std_date_proto_get_utc_hours);
ES_API_FUN(es_std_date_proto_get_minutes);
ES_API_FUN(es_std_date_proto_get_utc_minutes);
ES_API_FUN(es_std_date_proto_get_seconds);
ES_API_FUN(es_std_date_proto_get_utc_seconds);
ES_API_FUN(es_std_date_proto_get_milliseconds);
ES_API_FUN(es_std_date_proto_get_utc_milliseconds);
ES_API_FUN(es_std_date_proto_get_time_zone_off);
ES_API_FUN(es_std_date_proto_set_time);
ES_API_FUN(es_std_date_proto_set_milliseconds);
ES_API_FUN(es_std_date_proto_set_utc_milliseconds);
ES_API_FUN(es_std_date_proto_set_seconds);
ES_API_FUN(es_std_date_proto_set_utc_seconds);
ES_API_FUN(es_std_date_proto_set_minutes);
ES_API_FUN(es_std_date_proto_set_utc_minutes);
ES_API_FUN(es_std_date_proto_set_hours);
ES_API_FUN(es_std_date_proto_set_utc_hours);
ES_API_FUN(es_std_date_proto_set_date);
ES_API_FUN(es_std_date_proto_set_utc_date);
ES_API_FUN(es_std_date_proto_set_month);
ES_API_FUN(es_std_date_proto_set_utc_month);
ES_API_FUN(es_std_date_proto_set_full_year);
ES_API_FUN(es_std_date_proto_set_utc_full_year);
ES_API_FUN(es_std_date_proto_to_utc_str);
ES_API_FUN(es_std_date_proto_to_iso_str);
ES_API_FUN(es_std_date_proto_to_json);

/*
 * Date Constructor
 */
ES_API_FUN(es_std_date_constr_parse);
ES_API_FUN(es_std_date_constr_utc);
ES_API_FUN(es_std_date_constr_now);

/*
 * Date Object
 */
ES_API_FUN(es_std_date);      // Constructor called as function.

/*
 * Error Prototype
 */
ES_API_FUN(es_std_err_proto_to_str);

/*
 * Error Object
 */
ES_API_FUN(es_std_err);       // Constructor called as function.

/*
 * Function Prototype
 */
ES_API_FUN(es_std_fun_proto_to_str);
ES_API_FUN(es_std_fun_proto_apply);
ES_API_FUN(es_std_fun_proto_call);
ES_API_FUN(es_std_fun_proto_bind);

/*
 * Function Object
 */
ES_API_FUN(es_std_fun);

/*
 * JSON Object
 */
ES_API_FUN(es_std_json_parse);
ES_API_FUN(es_std_json_stringify);

/*
 * Math Object
 */
ES_API_FUN(es_std_math_abs);
ES_API_FUN(es_std_math_acos);
ES_API_FUN(es_std_math_asin);
ES_API_FUN(es_std_math_atan);
ES_API_FUN(es_std_math_atan2);
ES_API_FUN(es_std_math_ceil);
ES_API_FUN(es_std_math_cos);
ES_API_FUN(es_std_math_exp);
ES_API_FUN(es_std_math_floor);
ES_API_FUN(es_std_math_log);
ES_API_FUN(es_std_math_max);
ES_API_FUN(es_std_math_min);
ES_API_FUN(es_std_math_pow);
ES_API_FUN(es_std_math_random);
ES_API_FUN(es_std_math_round);
ES_API_FUN(es_std_math_sin);
ES_API_FUN(es_std_math_sqrt);
ES_API_FUN(es_std_math_tan);

/*
 * Native Error Objects
 */
ES_API_FUN(es_std_eval_err);      // Constructor called as function.
ES_API_FUN(es_std_range_err);     // Constructor called as function.
ES_API_FUN(es_std_ref_err);       // Constructor called as function.
ES_API_FUN(es_std_syntax_err);    // Constructor called as function.
ES_API_FUN(es_std_type_err);      // Constructor called as function.
ES_API_FUN(es_std_uri_err);       // Constructor called as function.

/*
 * Number Prototype
 */
ES_API_FUN(es_std_num_proto_to_str);
ES_API_FUN(es_std_num_proto_to_locale_str);
ES_API_FUN(es_std_num_proto_val_of);
ES_API_FUN(es_std_num_proto_to_fixed);
ES_API_FUN(es_std_num_proto_to_exp);
ES_API_FUN(es_std_num_proto_to_prec);

/*
 * Number Object
 */
ES_API_FUN(es_std_num);       // Constructor called as function.

/*
 * Object Prototype
 */
ES_API_FUN(es_std_obj_proto_to_str);
ES_API_FUN(es_std_obj_proto_to_loc_str);
ES_API_FUN(es_std_obj_proto_val_of);
ES_API_FUN(es_std_obj_proto_has_own_prop);
ES_API_FUN(es_std_obj_proto_is_proto_of);
ES_API_FUN(es_std_obj_proto_prop_is_enum);

/*
 * Object Object
 */
ES_API_FUN(es_std_obj);       // Constructor called as function.
ES_API_FUN(es_std_obj_get_proto_of);
ES_API_FUN(es_std_obj_get_own_prop_desc);
ES_API_FUN(es_std_obj_get_own_prop_names);
ES_API_FUN(es_std_obj_create);
ES_API_FUN(es_std_obj_def_prop);
ES_API_FUN(es_std_obj_def_props);
ES_API_FUN(es_std_obj_seal);
ES_API_FUN(es_std_obj_freeze);
ES_API_FUN(es_std_obj_prevent_exts);
ES_API_FUN(es_std_obj_is_sealed);
ES_API_FUN(es_std_obj_is_frozen);
ES_API_FUN(es_std_obj_is_extensible);
ES_API_FUN(es_std_obj_keys);

/*
 * String Prototype
 */
ES_API_FUN(es_std_str_proto_to_str);
ES_API_FUN(es_std_str_proto_val_of);
ES_API_FUN(es_std_str_proto_char_at);
ES_API_FUN(es_std_str_proto_char_code_at);
ES_API_FUN(es_std_str_proto_concat);
ES_API_FUN(es_std_str_proto_index_of);
ES_API_FUN(es_std_str_proto_last_index_of);
ES_API_FUN(es_std_str_proto_locale_compare);
ES_API_FUN(es_std_str_proto_match);
ES_API_FUN(es_std_str_proto_replace);
ES_API_FUN(es_std_str_proto_search);
ES_API_FUN(es_std_str_proto_slice);
ES_API_FUN(es_std_str_proto_split);
ES_API_FUN(es_std_str_proto_substr);
ES_API_FUN(es_std_str_proto_substring);
ES_API_FUN(es_std_str_proto_to_lower_case);
ES_API_FUN(es_std_str_proto_to_locale_lower_case);
ES_API_FUN(es_std_str_proto_to_upper_case);
ES_API_FUN(es_std_str_proto_to_locale_upper_case);
ES_API_FUN(es_std_str_proto_trim);

/*
 * String Object
 */
ES_API_FUN(es_std_str);    // Constructor called as function.
ES_API_FUN(es_std_str_from_char_code);

/*
 * RegExp Prototype
 */
ES_API_FUN(es_std_reg_exp_proto_exec);
ES_API_FUN(es_std_reg_exp_proto_test);
ES_API_FUN(es_std_reg_exp_proto_to_str);

/*
 * RegExp Object
 */
ES_API_FUN(es_std_reg_exp);   // Constructor called as a function.
