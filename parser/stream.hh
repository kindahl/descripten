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
#include <string>
#include <stddef.h>
#include "common/string.hh"
#include "types.hh"

namespace parser {

class UnicodeStream
{
protected:
    static const uni_char EOI = -1;

protected:
    size_t pos_;
    uni_char *cur_;
    uni_char *end_;

    virtual bool internal_fetch() = 0;
    virtual size_t internal_skip(size_t count) = 0;

public:
    UnicodeStream()
        : pos_(0)
        , cur_(NULL)
        , end_(NULL) {}

    virtual ~UnicodeStream() {}

    inline uni_char next()
    {
        if (cur_ < end_ || internal_fetch())
        {
            pos_++;
            return *(cur_++);
        }

        // Allow reading past the actual data.
        pos_++;
        return EOI;
    }

    inline size_t position() const
    {
        return pos_;
    }

    inline size_t skip(size_t count)
    {
        size_t buf_data = static_cast<size_t>(end_ - cur_);
        if (count <= buf_data)
        {
            cur_ += count;
            pos_ += count;
            return count;
        }

        return internal_skip(count);
    }

    virtual void push(uni_char c) = 0;
};

template <size_t S>
class BufferedUnicodeStream : public UnicodeStream
{
protected:
    uni_char buf_[S];
    uni_char *push_limit_;

    virtual bool internal_fetch() override
    {
        cur_ = buf_;

        if (push_limit_ != NULL)
        {
            end_ = push_limit_;
            push_limit_ = NULL;

            if (cur_ < end_)
                return true;
        }

        size_t len = buffer_fill(pos_, S);
        end_ = buf_ + len;
        return len > 0;
    }

    virtual size_t internal_skip(size_t count) override
    {
        push_limit_ = NULL;
        return buffer_skip(count);
    }

    virtual size_t buffer_fill(size_t pos, size_t len) = 0;
    virtual size_t buffer_skip(size_t count) = 0;

public:
    BufferedUnicodeStream()
        : push_limit_(NULL)
    {
        cur_ = buf_;
        end_ = buf_;
    }

    virtual void push(uni_char c) override
    {
        // Since we allow reading past the buffer we must allow putting
        // non-existing items back.
        if (c == EOI)
        {
            pos_--;
            return;
        }

        if (push_limit_ == NULL && cur_ > buf_)
        {
            *(--cur_) = c;
            pos_--;
        }
        else
        {
            if (push_limit_ == NULL)
            {
                push_limit_ = end_;
                end_ = buf_ + S;
                cur_ = end_;
            }

            assert(cur_ > buf_);
            assert(pos_ > 0);

            *(--cur_) = c;

            if (cur_ == buf_)
                push_limit_ = NULL;
            else if (cur_ < push_limit_)
                push_limit_ = cur_;

            pos_--;
        }
    }
};

class StringStream : public BufferedUnicodeStream<1024>
{
private:
    const String data_;

protected:
    virtual size_t buffer_fill(size_t pos, size_t len) override;
    virtual size_t buffer_skip(size_t count) override;

public:
    StringStream(const String &data);
};

class Utf8Stream : public BufferedUnicodeStream<1024>
{
private:
    const std::string data_;
    size_t data_len_;

    const byte *next_ptr_;  ///< Pointer to next chunk of unread data.
    size_t next_pos_;       ///< Character index of first character in next unread data.

    const byte *find_char_at_pos(size_t pos);

protected:
    virtual size_t buffer_fill(size_t pos, size_t len) override;
    virtual size_t buffer_skip(size_t count) override;

public:
    Utf8Stream(const std::string &data);
};

class Utf16Stream : public BufferedUnicodeStream<1024>
{
public:
    enum Endianness
    {
        ENDIAN_LITTLE,      // LITTLE_ENDIAN and BIT_ENDIAN seems to conflict.
        ENDIAN_BIG
    };

private:
    Endianness endianness_;
    const std::string data_;
    size_t data_len_;

    const byte *next_ptr_;  ///< Pointer to next chunk of unread data.
    size_t next_pos_;       ///< Character index of first character in next unread data.

    const byte *find_char_at_pos(size_t pos);

protected:
    virtual size_t buffer_fill(size_t pos, size_t len) override;
    virtual size_t buffer_skip(size_t count) override;

public:
    Utf16Stream(Endianness endianness, const std::string &data);
};

/**
 * @brief Factory for creating streams.
 */
class StreamFactory
{
public:
    /**
     * Creates a Unicode stream from a file.
     * @param [in] file_path Path to source file.
     * @return Pointer to Unicode stream.
     * @throw FileException if unable to open and read the file.
     */
    static UnicodeStream *from_file(const char *file_path);
};

}
