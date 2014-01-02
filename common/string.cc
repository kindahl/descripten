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

#include <cassert>
#include <limits>
#include <errno.h>
#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include "exception.hh"
#include "string.hh"
#include "unicode.hh"

String::String()
    : data_(NULL), len_(0), hash_(0)
{
}

String::String(const char *str)
    : data_(NULL), len_(0), hash_(0)
{
    set(str);
}

String::String(const char *str, size_t size)
    : data_(NULL), len_(0), hash_(0)
{
    set(str, size);
}

String::String(uni_char c)
    : data_(NULL), len_(0), hash_(0)
{
    set(c);
}

String::String(const uni_char *data)
    : data_(NULL), len_(0), hash_(0)
{
    set(data, std::char_traits<uni_char>::length(data));
}

String::String(const uni_char *data, size_t len)
    : data_(NULL), len_(0), hash_(0)
{
    set(data, len);
}

String String::wrap(const uni_char *data, size_t len)
{
    String res;
    res.data_ = data;
    res.len_ = len;
    return res;
}

void String::clear()
{
    data_ = NULL;
    len_ = 0;
}

void String::set(const char *str)
{
    set(str, strlen(str));
}

void String::set(const char *str, size_t size)
{
    assert(str);
    
    data_ = NULL;
    len_ = 0;

    size_t len = utf8_len(reinterpret_cast<const byte *>(str), size);
    if (len > 0)
    {
        uni_char *data = static_cast<uni_char *>(GC_MALLOC_ATOMIC((len + 1) * sizeof(uni_char)));
        if (!data)
            THROW(MemoryException);
        
        const byte *ptr = reinterpret_cast<const byte *>(str);
        for (size_t i = 0; i < len; i++)
            data[i] = utf8_dec(ptr);

        data[len] = 0;

        data_ = data;
        len_ = len;
    }
}

void String::set(uni_char c)
{
    data_ = NULL;
    len_ = 0;
    
    uni_char *data = static_cast<uni_char *>(GC_MALLOC_ATOMIC(2 * sizeof(uni_char)));
    if (!data)
        THROW(MemoryException);
    
    data[0] = c;
    data[1] = 0;

    data_ = data;
    len_ = 1;
}

void String::set(const uni_char *str, size_t len)
{
    assert(str);

    data_ = NULL;
    len_ = 0;

    if (len > 0)
    {
        uni_char *data = static_cast<uni_char *>(GC_MALLOC_ATOMIC((len + 1) * sizeof(uni_char)));
        if (!data)
            THROW(MemoryException);

        memcpy(data, str, len * sizeof(uni_char));
        data[len] = 0;

        data_ = data;
        len_ = len;
    }
}

bool String::empty() const
{
    return len_ == 0;
}

bool String::contains(uni_char c) const
{
    for (size_t i = 0; i < len_; i++)
        if (data_[i] == c)
            return true;

    return false;
}

String String::take(size_t num) const
{
    size_t len = num > len_ ? len_ : num;
    if (len > 0)
        return String(data_, len);

    return String();
}

String String::skip(size_t num) const
{
    if (num >= len_)
        return String();

    size_t len = len_ - num;
    return String(data_ + num, len);
}

String String::substr(size_t start, size_t num) const
{
    if (start >= len_)
        return String();

    size_t len = len_ - start;
    if (len > num)
        len = num;

    return String(data_ + start, len);
}

String String::lower() const
{
    uni_char *data = static_cast<uni_char *>(GC_MALLOC_ATOMIC((len_ + 1) * sizeof(uni_char)));
    if (!data)
        THROW(MemoryException);

    for (size_t i = 0; i < len_; i++)
        data[i] = tolower(data_[i]);

    data[len_] = 0;

    return String::wrap(data, len_);
}

String String::upper() const
{
    uni_char *data = static_cast<uni_char *>(GC_MALLOC_ATOMIC((len_ + 1) * sizeof(uni_char)));
    if (!data)
        THROW(MemoryException);

    for (size_t i = 0; i < len_; i++)
        data[i] = toupper(data_[i]);

    data[len_] = 0;

    return String::wrap(data, len_);
}

#ifdef UNTESTED
String String::trim(const char *chars) const
{
    assert(chars);
    size_t num_chars = strlen(chars);

    // Find start of trimmed string.
    size_t start = 0;
    for (; start < len_; start++)
    {
        bool found_char = false;
        for (size_t j = 0; j < num_chars; j++)
        {
            if (data_[start] == static_cast<uni_char>(chars[j]))
            {
                found_char = true;
                break;
            }
        }

        if (!found_char)
            break;
    }

    // Find end of trimmed string.
    size_t end = len_ - 1;
    for (; end >= 0; end--)
    {
        bool found_char = false;
        for (size_t j = 0; j < num_chars; j++)
        {
            if (data_[end] == static_cast<uni_char>(chars[j]))
            {
                found_char = true;
                break;
            }
        }

        if (!found_char)
            break;
    }

    assert(start <= end);
    return substr(start, end - start + 1);
}
#endif

String String::trim(bool (*filter)(uni_char c)) const
{
    if (len_ == 0)
        return String();

    // Find start of trimmed string.
    size_t start = 0;
    for (; start < len_; start++)
    {
        if (!filter(data_[start]))
            break;
    }

    // Find end of trimmed string.
    size_t end = len_;
    for (; end-- > 0;)
    {
        if (!filter(data_[end]))
            break;
    }

    assert(start <= end);
    return substr(start, end - start + 1);
}

ssize_t String::index_of(const String &str, size_t start)
{
    if (empty() || str.empty())
        return -1;

    if (start + str.length() > len_)
        return -1;

    const uni_char *cur_ptr = data_ + start;
    const uni_char *end_ptr = data_ + len_;

    while (cur_ptr != end_ptr)
    {
        const uni_char *cmp_ptr = cur_ptr;
        const uni_char *str_cur = str.data_;
        const uni_char *str_end = str.data_ + str.len_;

        while (*cmp_ptr == *str_cur &&
               str_cur != str_end)
        {
            cmp_ptr++;
            str_cur++;
        }

        if (str_cur == str_end)
            return cur_ptr - data_;

        cur_ptr++;
    }

    return -1;
}

ssize_t String::last_index_of(const String &str, size_t start)
{
    if (empty() || str.empty())
        return -1;

    if (start + str.length() > len_)
        return -1;

    const uni_char *cur_ptr = data_ + start;
    const uni_char *end_ptr = data_ + len_;

    ssize_t res = -1;
    while (cur_ptr != end_ptr)
    {
        const uni_char *cmp_ptr = cur_ptr;
        const uni_char *str_cur = str.data_;
        const uni_char *str_end = str.data_ + str.len_;

        while (*cmp_ptr == *str_cur &&
               str_cur != str_end)
        {
            cmp_ptr++;
            str_cur++;
        }

        if (str_cur == str_end)
            res = cur_ptr - data_;

        cur_ptr++;
    }

    return res;
}

int String::compare(const String &str) const
{
    size_t min = std::min(length(), str.length());
    return memcmp(data_, str.data_, min * sizeof(uni_char));
}

const std::string String::utf8() const
{
    byte buffer[6];
    
    std::string res(len_ * 6,' ');
    
    size_t j = 0;
    for (size_t i = 0; i < len_; i++)
    {
        byte *ptr = buffer;
        size_t bytes = utf8_enc(ptr, data_[i]);
        
        for (size_t k = 0; k < bytes; k++)
            res[j++] = static_cast<char>(buffer[k]);
    }
    
    res.resize(j);
    return res;
}

size_t String::hash() const
{
    if (hash_ == 0)
    {
        hash_ = 5381;

        uni_char c = 0;
        const uni_char *ptr = data_;
        if (!ptr)
            return hash_;

        while ((c = *ptr++))
            hash_ = ((hash_ << 5) + hash_) + c; // hash_ * 33 + c.
    }

    return hash_;
}

bool String::operator==(const String &rhs) const
{
    if (length() != rhs.length())
        return false;
    // Some (but not all) zero length strings have one byte allocated for the
    // NULL terminator. Hence we need the check below before comparing pointer
    // availability.
    if (length() == 0)
        return true;
    if ((!data() || !rhs.data()) && data() != rhs.data())
        return false;
    
    return !memcmp(data(), rhs.data(), length() * sizeof(uni_char));
}

bool String::operator!=(const String &rhs) const
{
    return !(*this == rhs);
}

bool String::operator<(const String &rhs) const
{
    size_t min = std::min(length(), rhs.length());

    /*int res = memcmp(data_, rhs.data_, min * sizeof(uni_char));
    if (res != 0)
        return res < 0;
    
    return length() < rhs.length();*/

    for (size_t i = 0; i < min; ++i)
    {
        uni_char l = data_[i];
        uni_char r = rhs.data_[i];
        if (l != r)
            return l < r;
    }

    return length() < rhs.length();
}

String String::operator+(const String &rhs) const
{
    if (rhs.empty())
        return *this;
    
    if (empty())
        return rhs;
    
    size_t len = len_ + rhs.len_;
    uni_char *data = static_cast<uni_char *>(GC_MALLOC_ATOMIC((len + 1) * sizeof(uni_char)));

    if (!data)
        THROW(MemoryException);
    
    memcpy(data, data_, len_ * sizeof(uni_char));
    memcpy(data + len_, rhs.data_, rhs.len_ * sizeof(uni_char));
    data[len] = '\0';

    return String::wrap(data, len);
}

uni_char String::operator[](size_t index) const
{
    assert(index < length());
    return data_[index];
}
