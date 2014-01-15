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
#include <limits.h>
#include <stdint.h>
#include "common/string.hh"
#include "object.hh"
#include "value.hh"

class EsPropertyDescriptor;

/**
 * Converts a string into a number value according to 9.3.1.
 * @param [in] str String to convert.
 * @return str converted to a number value.
 */
double es_str_to_num(const String &str);

/**
 * Converts a double value to a string value.
 * @param [in] m Value to convert.
 * @param [in] num_digits Maximum number of decimal digits, INT_MIN means use
 *                        as many as possible.
 * @return m converted to a string value.
 */
String es_num_to_str(double m, int num_digits = INT_MIN);

/**
 * Converts a time stamp into a human readable date string.
 * @param [in] timeinfo Pointer to tm structure, may not be NULL.
 * @return timeinfo converted to a human readable date string.
 */
String es_date_to_str(struct tm *timeinfo);

/**
 * Converts the specified property into an object.
 * @param [in] prop Property to convert.
 * @return prop converted to an object.
 * @throw TypeError if conversion failed.
 */
EsValue es_from_property_descriptor(const EsPropertyReference &prop);

/**
 * Converts the specified value into a property descriptor.
 * @param [in] val Value to convert.
 * @param [out] prop val converted to a property descriptor.
 * @return true if successful, false if a type error occurred.
 */
bool es_to_property_descriptor(const EsValue &val, EsPropertyDescriptor &desc);
