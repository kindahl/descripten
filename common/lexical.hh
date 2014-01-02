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
#include <string>
#include <stddef.h>
#include "types.hh"

/**
 * Checks if the specified character is a white space.
 * @param [in] c Character to test.
 * @return true if c is a white space and false otherwise.
 */
bool es_is_white_space(uni_char c);

/**
 * Checks if the specified character is a line terminator.
 * @param [in] c Character to test.
 * @return true if c is a line terminator and false otherwise.
 */
bool es_is_line_terminator(uni_char c);

/**
 * Checks if the specified character is a carriage return.
 * @param [in] c Character to test.
 * @return true if c is a carriage return terminator and false otherwise.
 */
bool es_is_carriage_return(uni_char c);

/**
 * Checks if the specified character is a line feed.
 * @param [in] c Character to test.
 * @return true if c is a line feed and false otherwise.
 */
bool es_is_line_feed(uni_char c);

/**
 * Checks if the specified character is an octal digit [0-7].
 * @param [in] c Character to test.
 * @return true if the character is an octal digit and false otherwise.
 */
bool es_is_oct_digit(uni_char c);

/**
 * Checks if the specified character is a decimal digit [0-9].
 * @param [in] c Character to test.
 * @return true if the character is a decimal digit and false otherwise.
 */
bool es_is_dec_digit(uni_char c);

/**
 * Checks if the specified character is a hexadecimal digit [0-9a-fA-F].
 * @param [in] c Character to test.
 * @return true if the character is a hexadecimal digit and false otherwise.
 */
bool es_is_hex_digit(uni_char c);

/**
 * Tests if the letters in the specified buffer can be interpreted as a decimal
 * number.
 * @param [in] str Pointer to letters to test.
 * @param [in] len Number of letters to test in str.
 * @return true if the letters can be interpreted as a decimal number and false
 *         otherwise.
 */
bool es_is_dec_number(const uni_char *str, size_t len);

/**
 * Interprets the specified character [0-7] as an octal number.
 * @pre c is an octal character.
 * @param [in] c Character to convert.
 * @return c interpreted as an octal number.
 */
uint8_t es_as_oct_digit(uni_char c);

/**
 * Interprets the specified character [0-9] as a decimal number.
 * @pre c is a decimal character.
 * @param [in] c Character to convert.
 * @return c interpreted as a decimal number.
 */
uint8_t es_as_dec_digit(uni_char c);

/**
 * Interprets the specified character [0-9a-fA-F] as a hexadecimal number.
 * @pre c is a hexadecimal character.
 * @param [in] c Character to convert.
 * @return c interpreted as a hexadecimal number.
 */
uint8_t es_as_hex_digit(uni_char c);

/**
 * Moves the specified pointer to the first non-white-space character.
 * @param [in] ptr Pointer to update.
 * @return The number of skipped white spaces.
 */
inline size_t es_str_skip_white_spaces(const uni_char *&ptr)
{
    size_t skip = 0;
    while (ptr && es_is_white_space(*ptr))
    {
        ptr++; skip++;
    }
    
    return skip;
}

/**
 * Unsigned integer variant of the es_strtod() function. It assumes an unsigned
 * int and is thus faster on these types than es_strtod().
 * @param [in] nptr Pointer to start of buffer.
 * @param [out] endptr Optional pointer that will be updated to point at the
 *                     character after the last parsed character.
 * @param [in] radix Base of number to be parsed from string.
 * @return Value parsed from string or NaN on failure.
 */
double es_strtou(const uni_char *nptr, const uni_char **endptr, int radix);

/**
 * ECMAScript compatible implementation of the system function strtod.
 * @param [in] nptr Pointer to start of buffer.
 * @param [out] endptr Optional pointer that will be updated to point at the
 *                     character after the last parsed character.
 * @return Value parsed from string.
 */
double es_strtod(const uni_char *nptr, const uni_char **endptr);
