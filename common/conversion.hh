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
#include <stdint.h>

class String;

/**
 * Tries to parse the contents of a string as an ECMA-262 array index.
 * @param [in] str String to parse into an index.
 * @param [out] index Index.
 * @return true if str was successfully parsed as an index, false if it was
 *         not.
 */
bool es_str_to_index(const String &str, uint32_t &index);

/**
 * Checks if the double can be represented as an ECMA-262 array index.
 * @param [in] num Number to convert into an index.
 * @param [out] index Index.
 * @return true if the double can be used as an index, false if it cannot.
 */
inline bool es_num_to_index(double num, uint32_t &index)
{
    index = static_cast<uint32_t>(num);
    return static_cast<double>(index) == num;
}
