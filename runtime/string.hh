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
#include <cassert>
#include <set>
#include <string>
#include <vector>
#include <gc/gc_allocator.h>
#include "common/types.hh"

class String;

/**
 * Macro for wrapping a string literal into a Unicode string object.
 */
#ifndef _ESTR
#define _ESTR(str)  EsString::create(_U(str), sizeof(str) - 1)
#endif

/**
 * @brief String type.
 *
 * String instances cannot be created using the 'new' operator. For efficiency
 * reasons we want to allocate all object members in the same memory area like
 * this:
 *  | EsString | data_ |
 */
class EsString
{
public:
    /**
     * String hash comparator.
     */
    struct Hash
    {
        inline size_t operator()(const EsString *str) const
        {
            return str->hash();
        }
    };

    /**
     * String equal comparator.
     */
    struct EqualTo
    {
        bool operator()(const EsString *x, const EsString *y) const
        {
            return x->equals(y);
        }
    };

    /**
     * String less than comparator.
     */
    struct LessThan
    {
        bool operator()(const EsString *x, const EsString *y) const
        {
            return x->less(y);
        }
    };

private:
    const uni_char *data_;
    size_t len_;
    mutable size_t hash_;   ///< String hash value, computed lazilly by hash().

    EsString(const uni_char *data, size_t len);
    EsString(const EsString &rhs);
    EsString &operator=(const EsString &rhs);

    static EsString *alloc(size_t len);

public:
    static const EsString *create();
    static const EsString *create(uni_char c);
    static const EsString *create(const uni_char *ptr);
    static const EsString *create(const uni_char *ptr, size_t len);
    static const EsString *create(const String &str);
    static const EsString *create_from_utf8(const char *str);
    static const EsString *create_from_utf8(const char *raw, size_t size);

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
     * @return String in non-ECMAScript representation.
     */
    String str() const;
    
    /**
     * @return Character at @a index.
     */
    inline uni_char at(size_t index) const
    {
        assert(index < len_);
        return data_[index];
    }

    /**
     * @return The first num character of the string.
     */
    const EsString *take(size_t num) const;

    /**
     * @return Discards num characters from the beginning of the string and
     *         returns the result.
     */
    const EsString *skip(size_t num) const;

    /**
     * @param [in] start Zero based start index.
     * @param [in] num Number of characters to take.
     * @return Specified substring.
     */
    const EsString *substr(size_t start, size_t num) const;
    
    /**
     * @return String in lower case.
     */
    const EsString *lower() const;

    /**
     * @return String in upper case.
     */
    const EsString *upper() const;

    /**
     * Derives a new string by trimming the beginning and end of this string.
     * @param [in] filter Filter function, returning true for characters that
     *                    should be trimmed.
     * @return Trimmed string.
     */
    const EsString *trim(bool (*filter)(uni_char c)) const;

    /**
     * Concatinates this string with another string.
     * @param [in] other String to append after this string.
     * @return New string with the result of concatinating this string with
     *         @a other.
     */
    const EsString *concat(const EsString *other) const;

    /**
     * Returns the index of str in the string, occurring after the offset
     * start. If str occurs multiple times the index of the first occurrence
     * is returned.
     * @param [in] str String to look for.
     * @param [in] start Search start offset.
     * @return If str is found the string, its position is returned, if str is
     *         not found -1 is returned.
     */
    ssize_t index_of(const EsString *str, size_t start = 0) const;

    /**
     * Returns the index of str in the string, occurring after the offset
     * start. If str occurs multiple times the index of the last occurrence
     * is returned.
     * @param [in] str String to look for.
     * @param [in] start Search start offset.
     * @return If str is found the string, its position is returned, if str is
     *         not found -1 is returned.
     */
    ssize_t last_index_of(const EsString *str, size_t start = 0) const;

    /**
     * @return true if this string is considered equal to @a other.
     */
    bool equals(const EsString *other) const;

    /**
     * @return true if this string is considered less than @a other.
     */
    bool less(const EsString *other) const;

    /**
     * Compares the current string with another string.
     * @param [in] str String to compare with.
     * @return An integer less than, equal to, or greater than zero if this
     *         string is found to be less than, equal, or greater than str.
     */
    int compare(const EsString *other) const;

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
};

/**
 * Vector of strings.
 */
typedef std::vector<const EsString *,
                    gc_allocator<const EsString *> > EsStringVector;

/**
 * Set of strings.
 */
typedef std::set<const EsString *, EsString::LessThan,
                 gc_allocator<const EsString *> > EsStringSet;
