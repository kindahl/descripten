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
#include <cstring>
#include <gc.h>
#include "common/exception.hh"
#include "common/string.hh"
#include "common/unicode.hh"
#include "string.hh"

EsString::EsString(const uni_char *data, size_t len)
    : data_(data), len_(len), hash_(0)
{
}

EsString *EsString::alloc(size_t len)
{
    EsString *str = static_cast<EsString *>(
            GC_MALLOC_ATOMIC(sizeof(EsString) + (len + 1) * sizeof(uni_char)));
    if (!str)
        THROW(MemoryException);

    uni_char *data = reinterpret_cast<uni_char *>(
            reinterpret_cast<byte *>(str) + sizeof(EsString));
    new (str) EsString(data, len);

    return str;
}

const EsString *EsString::create()
{
    static const EsString *str = create_from_utf8("", 0);
    return str;
}

const EsString *EsString::create(uni_char c)
{
    EsString *str = alloc(1);

    uni_char *data = const_cast<uni_char *>(str->data_);
    data[0] = c;
    data[1] = 0;

    return str;
}

const EsString *EsString::create(const uni_char *ptr)
{
    return create(ptr, std::char_traits<uni_char>::length(ptr));
}

const EsString *EsString::create(const uni_char *ptr, size_t len)
{
    assert(ptr);

    EsString *str = alloc(len);

    uni_char *data = const_cast<uni_char *>(str->data_);
    memcpy(data, ptr, len * sizeof(uni_char));
    data[len] = 0;

    return str;
}

const EsString *EsString::create(const String &str)
{
    if (str.empty())
        return create();

    return create(str.data(), str.length());
}

const EsString *EsString::create_from_utf8(const char *str)
{
    return create_from_utf8(str, strlen(str));
}

const EsString *EsString::create_from_utf8(const char *raw, size_t size)
{
    assert(raw);
    const byte *ptr = reinterpret_cast<const byte *>(raw);

    size_t len = utf8_len(reinterpret_cast<const byte *>(raw), size);
    EsString *str = alloc(len);

    uni_char *data = const_cast<uni_char *>(str->data_);
    for (size_t i = 0; i < len; i++)
        data[i] = utf8_dec(ptr);
    data[len] = 0;

    return str;
}

const EsString *EsString::create_from_utf8(const std::string &str)
{
    return create_from_utf8(str.c_str(), str.size());
}

bool EsString::empty() const
{
    return len_ == 0;
}

bool EsString::contains(uni_char c) const
{
    for (size_t i = 0; i < len_; i++)
        if (data_[i] == c)
            return true;

    return false;
}

String EsString::str() const
{
    return String::wrap(data_, len_);
}

const EsString *EsString::take(size_t num) const
{
    size_t len = num > len_ ? len_ : num;
    if (len > 0)
        return EsString::create(data_, len);

    return create();
}

const EsString *EsString::skip(size_t num) const
{
    if (num >= len_)
        return create();

    size_t len = len_ - num;
    return EsString::create(data_ + num, len);
}

const EsString *EsString::substr(size_t start, size_t num) const
{
    if (start >= len_)
        return create();

    size_t len = len_ - start;
    if (len > num)
        len = num;

    return EsString::create(data_ + start, len);
}

const EsString *EsString::lower() const
{
    EsString *str = alloc(len_);

    uni_char *data = const_cast<uni_char *>(str->data());
    for (size_t i = 0; i < len_; i++)
        data[i] = tolower(data_[i]);
    data[len_] = 0;

    return str;
}

const EsString *EsString::upper() const
{
    EsString *str = alloc(len_);

    uni_char *data = const_cast<uni_char *>(str->data());
    for (size_t i = 0; i < len_; i++)
        data[i] = toupper(data_[i]);
    data[len_] = 0;

    return str;
}

const EsString *EsString::trim(bool (*filter)(uni_char c)) const
{
    if (len_ == 0)
        return create();

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

const EsString *EsString::concat(const EsString *other) const
{
    if (other->empty())
        return this;
    
    if (empty())
        return other;
    
    size_t len = len_ + other->len_;
    EsString *str = alloc(len);

    uni_char *data = const_cast<uni_char *>(str->data_);
    memcpy(data, data_, len_ * sizeof(uni_char));
    memcpy(data + len_, other->data_, other->len_ * sizeof(uni_char));
    data[len] = 0;

    return str;
}

ssize_t EsString::index_of(const EsString *str, size_t start) const
{
    if (empty() || str->empty())
        return -1;

    if (start + str->length() > len_)
        return -1;

    const uni_char *cur_ptr = data_ + start;
    const uni_char *end_ptr = data_ + len_;

    while (cur_ptr != end_ptr)
    {
        const uni_char *cmp_ptr = cur_ptr;
        const uni_char *str_cur = str->data_;
        const uni_char *str_end = str->data_ + str->len_;

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

ssize_t EsString::last_index_of(const EsString *str, size_t start) const
{
    if (empty() || str->empty())
        return -1;

    if (start + str->length() > len_)
        return -1;

    const uni_char *cur_ptr = data_ + start;
    const uni_char *end_ptr = data_ + len_;

    ssize_t res = -1;
    while (cur_ptr != end_ptr)
    {
        const uni_char *cmp_ptr = cur_ptr;
        const uni_char *str_cur = str->data_;
        const uni_char *str_end = str->data_ + str->len_;

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

bool EsString::equals(const EsString *other) const
{
    if (length() != other->length())
        return false;
    // Some (but not all) zero length otherings have one byte allocated for the
    // NULL terminator. Hence we need the check below before comparing pointer
    // availability.
    if (length() == 0)
        return true;
    if ((!data() || !other->data()) && data() != other->data())
        return false;
    
    return !memcmp(data(), other->data(), length() * sizeof(uni_char));
}

bool EsString::less(const EsString *other) const
{
    size_t min = std::min(length(), other->length());

    /*int res = memcmp(data_, other->data_, min * sizeof(uni_char));
    if (res != 0)
        return res < 0;
    
    return length() < other->length();*/

    for (size_t i = 0; i < min; ++i)
    {
        uni_char l = data_[i];
        uni_char r = other->data_[i];
        if (l != r)
            return l < r;
    }

    return length() < other->length();
}

int EsString::compare(const EsString *other) const
{
    size_t min = std::min(length(), other->length());
    return memcmp(data_, other->data_, min * sizeof(uni_char));
}

const std::string EsString::utf8() const
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

size_t EsString::hash() const
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
