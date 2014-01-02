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
#include "common/string.hh"
#include "container.hh"

class EsDate;
class EsValue;
class EsRegExp;

/**
 * Tries to interpret a value as a boolean. If the value is a boolean object
 * or a boolean value, it will be written to boolean. If the value can not be
 * interpreted as a boolean the function will return false without updating
 * boolean.
 * @param [in] val Value to interpret.
 * @param [out] boolean Will be updated to contain the boolean value on success.
 * @return true if the value could be interpreted as a boolean and false
 *         otherwise.
 */
bool es_as_boolean(const EsValue &val, bool &boolean);

/**
 * Tries to interpret a value as a number. If the value is a number object
 * or a number value, it will be written to number. If the value can not be
 * interpreted as a number the function will return false without updating
 * number.
 * @param [in] val Value to interpret.
 * @param [out] number Will be updated to contain the number value on success.
 * @return true if the value could be interpreted as a number and false
 *         otherwise.
 */
bool es_as_number(const EsValue &val, double &number);

/**
 * Tries to interpret a value as a string. If the value is a string object
 * or a string value, it will be written to str. If the value can not be
 * interpreted as a string the function will return false without updating
 * str.
 * @param [in] val Value to interpret.
 * @param [out] str Will be updated to contain the string value on success.
 * @return true if the value could be interpreted as a string and false
 *         otherwise.
 */
bool es_as_string(const EsValue &val, String &str);

/**
 * Tries to interpret a value as an object. If the value is an object and
 * it's of the specified class type, it will be written to object. If the
 * value is not an object the function will return false without updating
 * object.
 * @param [in] val Value to interpret.
 * @param [out] object Will be updated to point to the object on success.
 * @param [in] class_name Non-NULL to specify object class name condition. If
 *                        NULL any object will be accepted.
 * @return true if the value could be interpreted as an object of the
 *         specified class and false otherwise.
 */
bool es_as_object(const EsValue &val, EsObject *&object, const char *class_name = NULL);

/**
 * Tries to interpret a value as a date object. If the value is a date
 * object, a pointer to it will be returned. If the value is not a date object
 * the function will return NULL.
 * @param [in] val Value to interpret.
 * @return Pointer to date object on success and NULL otherwise.
 */
EsDate *es_as_date(const EsValue &val);

/**
 * Tries to interpret a value as a regular expression object. If the value
 * is a regular expression object, a pointer to it will be returned. If the
 * value is not a regular expression object the function will return NULL.
 * @param [in] val Value to interpret.
 * @return Pointer to regular expression object on success and NULL otherwise.
 */
EsRegExp *es_as_reg_exp(const EsValue &val);
