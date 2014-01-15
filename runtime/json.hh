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
#include <gc/gc_allocator.h>
#include "common/stringbuilder.hh"
#include "parser/stream.hh"

bool json_is_white_space(uni_char c);

using parser::UnicodeStream;

/**
 * @brief JSON parser.
 */
class JsonParser
{
private:
    UnicodeStream &stream_;     ///< Source input stream.

    StringBuilder sb_;          ///< Stateless string builder shared by many routines.

    bool expect(const char *text);

    void skip_white_space();

    // FIXME: Copied from lexer.
    int32_t read_hex_number(int num_digits);

    bool parse_object(EsValue &result);
    bool parse_array(EsValue &result);
    bool parse_string(EsValue &result);
    bool parse_number(EsValue &result);
    bool parse_value(EsValue &result);

public:
    /**
     * Constructs a JSON parser for parsing JSON code.
     * @param [in] stream Source input stream.
     */
    JsonParser(UnicodeStream &stream);

    bool parse(EsValue &result);
};
