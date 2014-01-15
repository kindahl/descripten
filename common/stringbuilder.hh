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
#include <stdarg.h>
#include "string.hh"

/**
 * @brief Class for efficient string constructing.
 */
class StringBuilder
{
private:
    /**
     * @brief Defines internal control parameters.
     */
    enum
    {
        SB_DEFAULT_BUF_SIZE = 32,   ///< Default buffer size used by string builder.
        SB_SPRINTF_BUF_SIZE = 70    ///< Size of internal buffer used by sprintf.
    };

    /**
     * @brief Defines available sprintf control codes.
     */
    enum SprintfControlCode
    {
        SB_SPRINTF_MOD_UNKNOWN,     ///< Any unrecognized modifier.
        SB_SPRINTF_MOD_RADIX,       ///< Integer modifiers: %d, %x, %o etc.
        SB_SPRINTF_MOD_SIZE,        ///< Size modifier: %n.
        SB_SPRINTF_MOD_STRING,      ///< String modifier: %s.
        SB_SPRINTF_MOD_PERCENT,     ///< Percent symbol: %%.
        SB_SPRINTF_MOD_CHAR,        ///< Character modifier: %c.
        SB_SPRINTF_MOD_POINTER,     ///< Pointer modifier: %p.

        // The follownig control codes are typically not present in standard sprintf.
        SB_SPRINTF_MOD_UNI_STRING,  ///< Unicode string modifier: %S.
        SB_SPRINTF_MOD_UNI_CHAR     ///< Unicode character modifier: %C.
    };

    /**
     * @brief Defines sprintf conversion modifier flags.
     */
    enum SprintfFlags
    {
        SB_SPRINTF_FLAG_SIGNED = 1, ///< Set if the value to convert is signed.
        SB_SPRINTF_FLAG_STRING = 2, ///< Used to allow infinity precision.
    };

    /**
     * @brief Holds informationm about each available sprintf format field.
     *
     * Each builtin conversion character (ex: the 'd' in "%d") is described by
     * an instance of the following structure.
     */
    typedef struct
    {
        char letter;        ///< The format field code letter.
        byte base;          ///< The base for radix conversion.
        byte flags;         ///< One or more of FLAG_ constants below.
        SprintfControlCode type;    ///< Conversion paradigm.
        byte charset;       ///< Offset into digits[] of the digits string.
        byte prefix;        ///< Offset into prefix[] of the prefix string.
    } SprintfModifier;

    static const SprintfModifier sprintf_mods_[];

private:
    uni_char *data_;    ///< Buffer used for constructing string.
    size_t max_len_;    ///< Number of character the buffer can hold (excluding the terminating NULL character).
    size_t cur_len_;    ///< Number of character used in the buffer.

    /**
     * Grow the buffer so that it can hold at least the specified number of
     * characters. An extra character will always be reserved to store the
     * terminating NULL-character.
     * @param [in] count Number of characters the buffer should be able to hold.
     * @throw MemoryException if out of memory.
     */
    void grow(size_t count);

public:
    StringBuilder();

    void clear();

    void append(uni_char c);
    void append(const char *str);
    void append(const char *str, size_t count);
    void append(const uni_char *str);
    void append(const uni_char *str, size_t count);
    void append(const String &str);
    void append_space(size_t count);

    size_t allocated() const;
    size_t length() const;
    String string() const;

    static String vsprintf(const char *format, va_list vl);
    static String sprintf(const char *format, ...);
};
