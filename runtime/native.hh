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
#include <cmath>
#include <stdint.h>

/**
 * Converts a double into a 32-bit signed integer in accordance with 9.5.
 * @param [in] value Double value to convert.
 * @return Double value converted into a 32-bit signed integer.
 */
static inline int32_t es_to_int32(double value)
{
    int32_t i = static_cast<int32_t>(value);
    if (static_cast<double>(i) == value)
        return i;
    
    static const double two32 = 4294967296.0;
    static const double two31 = 2147483648.0;
    if (!std::isfinite(value) || value == 0)
        return 0;
    
    if (value < 0 || value >= two32)
        value = fmod(value, two32);
    
    value = (value >= 0) ? floor(value) : ceil(value) + two32;
    return (int32_t)((value >= two31) ? value - two32 : value);
}

/**
 * Converts a double into an 32-bit unsigned integer in accordance with 9.6.
 * @param [in] value Double value to convert.
 * @return Double value converted into an 32-bit unsigned integer.
 */
static inline uint32_t es_to_uint32(double value)
{
    return static_cast<uint32_t>(es_to_int32(value));
}

/**
 * Converts a double into an 16-bit unsigned integer in accordance with 9.7.
 * @param [in] value Double value to convert.
 * @return Double value converted into an 16-bit unsigned integer.
 */
static inline uint16_t es_to_uint16(double value)
{
    int16_t i = static_cast<int16_t>(value);
    if (static_cast<double>(i) == value)
        return i;
    
    static const double two16 = 65536.0f;
    static const double two15 = 32768.0f;
    if (!std::isfinite(value) || value == 0)
        return 0;
    
    if (value < 0 || value >= two16)
        value = fmod(value, two16);
    
    value = (value >= 0) ? floor(value) : ceil(value) + two16;
    return (uint16_t)((value >= two15) ? value - two16 : value);
}

/**
 * Converts a double into a string.
 * @param [in] value Double value to convert.
 * @param [in] radix Value base radix.
 * @param [out] buffer Buffer to write string to.
 */
void double_to_cstring(double value, int32_t radix, char *buffer);
