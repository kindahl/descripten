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
#include <string.h>
#include "native.hh"

void double_to_cstring(double value, int32_t radix, char *buffer)
{
    assert(radix >= 2 && radix <= 36);

    if (std::isnan(value))
    {
        strcpy(buffer, "NaN");
        return;
    }

    if (std::isinf(value))
    {
        strcpy(buffer, value < 0.0 ? "-Infinity" : "Infinity");
        return;
    }

    // Character array used for conversion.
    static const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    
    // Buffer for the integer part of the result. 1024 chars is enough for max
    // integer value in radix 2.  We need room for a sign too.
    static const int buffer_size = 1100;
    char integer_buffer[buffer_size];
    integer_buffer[buffer_size - 1] = '\0';
    
    // Buffer for the decimal part of the result. We only generate up to
    // kBufferSize - 1 chars for the decimal part.
    char decimal_buffer[buffer_size];
    decimal_buffer[buffer_size - 1] = '\0';
    
    // Make sure the value is positive.
    bool is_negative = value < 0.0;
    if (is_negative)
        value = -value;
    
    // Get the integer part and the decimal part.
    double integer_part = floor(value);
    double decimal_part = value - integer_part;
    
    // Convert the integer part starting from the back. Always generate at least
    // one digit.
    int integer_pos = buffer_size - 2;
    do
    {
        integer_buffer[integer_pos--] = chars[static_cast<int>(fmod(integer_part, radix))];
        integer_part /= radix;
    } while (integer_part >= 1.0);
    
    // Sanity check.
    assert(integer_pos > 0);
    
    // Add sign if needed.
    if (is_negative)
        integer_buffer[integer_pos--] = '-';
    
    // Convert the decimal part. Repeatedly multiply by the radix to generate
    // the next char. Never generate more than kBufferSize - 1 chars.
    int decimal_pos = 0;
    while ((decimal_part > 0.0) && (decimal_pos < buffer_size - 1))
    {
        decimal_part *= radix;
        decimal_buffer[decimal_pos++] =
        chars[static_cast<int>(floor(decimal_part))];
        decimal_part -= floor(decimal_part);
    }
    decimal_buffer[decimal_pos] = '\0';
    
    // Compute the result size.
    int integer_part_size = buffer_size - 2 - integer_pos;
    
    // Make room for zero termination.
    unsigned result_size = integer_part_size + decimal_pos;
    
    // If the number has a decimal part, leave room for the period.
    if (decimal_pos > 0)
        result_size++;
    
    // Allocate result and fill in the parts.
    memcpy(buffer, integer_buffer + integer_pos + 1, integer_part_size);
    int buffer_pos = integer_part_size;
    
    if (decimal_pos > 0)
    {
        memcpy(buffer + buffer_pos, ".", 1);
        buffer_pos++;
    }
    
    memcpy(buffer + buffer_pos, decimal_buffer, decimal_pos);
    buffer_pos += decimal_pos;
    memcpy(buffer + buffer_pos, "\0", 1);
}
