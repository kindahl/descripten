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

#include "conversion.hh"
#include "string.hh"

bool es_str_to_index(const String &str, uint32_t &index)
{
    if (str.empty())
        return false;

    // Check the first character.
    uint32_t c = str[0] - _U('0');
    if (c > 9)
        return false;

    if (!c && str.length() > 1)
        return false;

    index = 0;
    for (size_t i = 0; i < str.length(); i++)
    {
        uint32_t c = str[i] - _U('0');
        if (c > 9)
            return false;

        // Updated new index and check for overflow.
        uint32_t new_index = c + index * 10;
        if (new_index < index)
            return false;

        index = new_index;
    }

    return true;
}

