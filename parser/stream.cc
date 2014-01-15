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

#include <algorithm>
#include <fstream>
#include "common/stringbuilder.hh"
#include "common/unicode.hh"
#include "exception.hh"
#include "stream.hh"

namespace parser {

StringStream::StringStream(const String &data)
    : data_(data)
{
}

size_t StringStream::buffer_fill(size_t pos, size_t len)
{
    if (pos >= data_.length())
        return 0;

    size_t to_read = pos + len > data_.length() ? data_.length() - pos : len;

    for (size_t i = 0; i < to_read; i++)
        buf_[i] = data_[pos + i];

    return to_read;
}

size_t StringStream::buffer_skip(size_t count)
{
    size_t to_skip = pos_ + count > data_.length() ? data_.length() - pos_ : count;
    pos_ += to_skip;
    internal_fetch();
    return to_skip;
}

Utf8Stream::Utf8Stream(const std::string &data)
    : data_(data)
    , data_len_(0)
    , next_ptr_(NULL)
    , next_pos_(0)
{
    next_ptr_ = reinterpret_cast<const byte *>(&data_[0]);
    data_len_ = utf8_len(next_ptr_);
}

const byte *Utf8Stream::find_char_at_pos(size_t pos)
{
    assert(pos < data_len_);

    const byte *ptr = reinterpret_cast<const byte *>(&data_[0]);

    for (size_t i = 0; i < pos; i++)
        utf8_dec(ptr);

    return ptr;
}

size_t Utf8Stream::buffer_fill(size_t pos, size_t len)
{
    if (pos >= data_len_)
        return 0;

    const byte *ptr = NULL;
    if (pos == next_pos_)
        ptr = next_ptr_;
    else
        ptr = find_char_at_pos(pos);

    size_t to_read = pos + len > data_len_ ? data_len_ - pos : len;

    for (size_t i = 0; i < to_read; i++)
        buf_[i] = utf8_dec(ptr);

    next_ptr_ = ptr;
    next_pos_ = pos + to_read;
    return to_read;
}

size_t Utf8Stream::buffer_skip(size_t count)
{
    size_t to_skip = pos_ + count > data_len_ ? data_len_ - pos_ : count;
    pos_ += to_skip;
    internal_fetch();
    return to_skip;
}

Utf16Stream::Utf16Stream(Endianness endianness, const std::string &data)
    : endianness_(endianness)
    , data_(data)
    , data_len_(0)
    , next_ptr_(NULL)
    , next_pos_(0)
{
    next_ptr_ = reinterpret_cast<const byte *>(&data_[0]);
    data_len_ = endianness_ == ENDIAN_LITTLE ? utf16le_len(next_ptr_) : utf16le_len(next_ptr_); // FIXME:
}

const byte *Utf16Stream::find_char_at_pos(size_t pos)
{
    assert(pos < data_len_);

    const byte *ptr = reinterpret_cast<const byte *>(&data_[0]);

    for (size_t i = 0; i < pos; i++)
    {
        if (endianness_ == ENDIAN_LITTLE)
            utf16le_dec(ptr);
        else
            utf16le_dec(ptr);   // FIXME:
    }

    return ptr;
}

size_t Utf16Stream::buffer_fill(size_t pos, size_t len)
{
    if (pos >= data_len_)
        return 0;

    const byte *ptr = NULL;
    if (pos == next_pos_)
        ptr = next_ptr_;
    else
        ptr = find_char_at_pos(pos);

    size_t to_read = pos + len > data_len_ ? data_len_ - pos : len;

    for (size_t i = 0; i < to_read; i++)
    {
        if (endianness_ == ENDIAN_LITTLE)
            buf_[i] = utf16le_dec(ptr);
        else
            buf_[i] = utf16le_dec(ptr);     // FIXME:
    }

    next_ptr_ = ptr;
    next_pos_ = pos + to_read;
    return to_read;
}

size_t Utf16Stream::buffer_skip(size_t count)
{
    size_t to_skip = pos_ + count > data_len_ ? data_len_ - pos_ : count;
    pos_ += to_skip;
    internal_fetch();
    return to_skip;
}

UnicodeStream *StreamFactory::from_file(const char *file_path)
{
    // Read file contents.
    std::ifstream file(file_path);
    if (!file.is_open())
        THROW(FileException, StringBuilder::sprintf("unable to open source file '%s' for reading.", file_path));

    file.seekg(0, std::ios::end);   
    std::string str;
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign(std::istreambuf_iterator<char>(file),
               std::istreambuf_iterator<char>());

    byte bom[4] = { 0x00,0x00,0x00,0x00 };
    for (int i = 0; i < std::min(4, static_cast<int>(str.size())); i++)
        bom[i] = static_cast<byte>(str[i]);

    // Look for BOM.
    if (bom[0] == 0xfe && bom[1] == 0xff)                           // UTF-16 (BE)
        return new Utf16Stream(Utf16Stream::ENDIAN_BIG, str.substr(2));
    else if (bom[0] == 0xff && bom[1] == 0xfe)                      // UTF-16 (LE)
        return new Utf16Stream(Utf16Stream::ENDIAN_LITTLE, str.substr(2));
    else if (bom[0] == 0xef && bom[1] == 0xbb && bom[2] == 0xbf)    // UTF-8
        return new Utf8Stream(str.substr(3));

    return new Utf8Stream(str);
}

}
