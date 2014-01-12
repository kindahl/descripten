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

#include "value_data.h"

// Make sure that we link one implementation of the inline functions. This is
// necessary if a program that uses this library decides that it cannot inline
// for some reason. In that case we must provide an implementation or the
// program will not link.
//
// See:
// http://gustedt.wordpress.com/2010/11/29/myth-and-reality-about-inline-in-c99/
extern inline EsValueData es_value_create(uint32_t type);
extern inline EsValueData es_value_nothing();
extern inline EsValueData es_value_null();
extern inline EsValueData es_value_undefined();
extern inline EsValueData es_value_true();
extern inline EsValueData es_value_false();
extern inline EsValueData es_value_from_boolean(int val);
extern inline EsValueData es_value_from_number(double val);
extern inline EsValueData es_value_from_i64(int64_t val);
extern inline EsValueData es_value_from_string(const struct EsString *str);
extern inline EsValueData es_value_from_object(struct EsObject *obj);
extern inline int es_value_is_nothing(const EsValueData value);
extern inline int es_value_is_undefined(const EsValueData value);
extern inline int es_value_is_null(const EsValueData value);
extern inline int es_value_is_boolean(const EsValueData value);
extern inline int es_value_is_number(const EsValueData value);
extern inline int es_value_is_string(const EsValueData value);
extern inline int es_value_is_object(const EsValueData value);
extern inline int es_value_as_boolean(const EsValueData value);
extern inline double es_value_as_number(const EsValueData value);
extern inline struct EsString *es_value_as_string(const EsValueData value);
extern inline struct EsObject *es_value_as_object(const EsValueData value);
extern inline uint32_t es_value_type(const EsValueData value);
