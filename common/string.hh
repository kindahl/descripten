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
#include <set>
#include <string>
#include <vector>
#include <gc/gc_allocator.h>
#include "types.hh"

/**
 * Macro for wrapping a string literal into a Unicode string object.
 */
#ifndef _USTR
#define _USTR(str)  String::wrap(_U(str), sizeof(str) - 1)
#endif

namespace parser {
class Token;
}

/**
 * @brief Immutable string class for storing garbage collectible strings.
 */
class String
{
public:
    /**
     * String hash comparator.
     */
    struct Hash
    {
        inline size_t operator()(const String &str) const
        {
            return str.hash();
        }
    };

private:
    const uni_char *data_;
    size_t len_;
    mutable size_t hash_;   ///< String hash value, computed lazilly by hash().

    void clear();
    void set(const char *str);
    void set(const char *str, size_t size);
    void set(uni_char c);
    void set(const uni_char *str, size_t len);

public:
    String();
    explicit String(const char *str);
    explicit String(const char *str, size_t size);
    explicit String(uni_char c);
    explicit String(const uni_char *data);
    explicit String(const uni_char *data, size_t len);

    /**
     * Creates a string by wrapping the specified pointer and length directly,
     * without copying the string data into a new buffer.
     * @param [in] data String data.
     * @param [in] len String length.
     * @return String object wrapping data and len.
     */
    static String wrap(const uni_char *data, size_t len);

    /**
     * @return true if the string is empty and false otherwise.
     */
    bool empty() const;

    /**
     * Checks if the specified character is present in the string.
     * @param [in] c Character to look for.
     * @return true of the string contains the specified character.
     */
    bool contains(uni_char c) const;
    
    /**
     * @return The number of character in the string.
     */
    inline size_t length() const { return len_; }
    
    /**
     * @return Pointer to character data.
     */
    inline const uni_char *data() const { return data_; }

    /**
     * @return The first num character of the string.
     */
    String take(size_t num) const;

    /**
     * @return Discards num characters from the beginning of the string and
     *         returns the result.
     */
    String skip(size_t num) const;

    /**
     * @param [in] start Zero based start index.
     * @param [in] num Number of characters to take.
     * @return Specified substring.
     */
    String substr(size_t start, size_t num) const;
    
    /**
     * @return String in lower case.
     */
    String lower() const;

    /**
     * @return String in upper case.
     */
    String upper() const;

#ifdef UNTESTED
    /**
     * Derives a new string by trimming the beginning and end of this string.
     * @param [in] chars String of characters to trim.
     * @return Trimmed string.
     */
    String trim(const char *chars) const;
#endif

    /**
     * Derives a new string by trimming the beginning and end of this string.
     * @param [in] filter Filter function, returning true for characters that
     *                    should be trimmed.
     * @return Trimmed string.
     */
    String trim(bool (*filter)(uni_char c)) const;

    /**
     * Returns the index of str in the string, occurring after the offset
     * start. If str occurs multiple times the index of the first occurrence
     * is returned.
     * @param [in] str String to look for.
     * @param [in] start Search start offset.
     * @return If str is found the string, its position is returned, if str is
     *         not found -1 is returned.
     */
    ssize_t index_of(const String &str, size_t start = 0) const;

    /**
     * Returns the index of str in the string, occurring after the offset
     * start. If str occurs multiple times the index of the last occurrence
     * is returned.
     * @param [in] str String to look for.
     * @param [in] start Search start offset.
     * @return If str is found the string, its position is returned, if str is
     *         not found -1 is returned.
     */
    ssize_t last_index_of(const String &str, size_t start = 0) const;

    /**
     * Compares the current string with another string.
     * @param [in] str String to compare with.
     * @return An integer less than, equal to, or greater than zero if this
     *         string is found to be less than, equal, or greater than str.
     */
    int compare(const String &str) const;

    /**
     * @return String encoded in UTF-8 encoding.
     */
    const std::string utf8() const;

    /**
     * Computes a hash from the string. The hash is computed using the
     * Dan Bernstein (djb2) hash algorithm. The hash will be cached inside the
     * string object. Since the string is immutable the hash will need to be
     * computed only once.
     * @return String hash value.
     */
    size_t hash() const;
    
    bool operator==(const String &rhs) const;
    bool operator!=(const String &rhs) const;
    bool operator<(const String &rhs) const;
    String operator+(const String &rhs) const;
    uni_char operator[](size_t index) const;
};

/**
 * Vector of strings.
 */
typedef std::vector<String, gc_allocator<String> > StringVector;

/**
 * Set of strings.
 */
typedef std::set<String, std::less<String>, gc_allocator<String > > StringSet;
