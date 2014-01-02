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
#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include "exception.hh"
#include "stringbuilder.hh"
#include "unicode.hh"

StringBuilder::StringBuilder()
    : data_(NULL)
    , max_len_(0)
    , cur_len_(0)
{
    data_ = static_cast<uni_char *>(GC_MALLOC_ATOMIC((SB_DEFAULT_BUF_SIZE + 1) * sizeof(uni_char)));
    if (!data_)
        THROW(MemoryException);
    max_len_ = SB_DEFAULT_BUF_SIZE;
}

void StringBuilder::clear()
{
    cur_len_ = 0;
    data_[0] = 0;
}

void StringBuilder::grow(size_t count)
{
    size_t new_max_len = max_len_;
    while (new_max_len < count)
        new_max_len *= 2;

    if (new_max_len > max_len_)
    {
        uni_char *new_data = static_cast<uni_char *>(GC_MALLOC_ATOMIC((new_max_len + 1) * sizeof(uni_char)));
        if (!new_data)
            THROW(MemoryException);

        memcpy(new_data, data_, (cur_len_ + 1) * sizeof(uni_char));

        data_ = new_data;
        max_len_ = new_max_len;
    }

    assert(max_len_ >= count);
}

void StringBuilder::append(uni_char c)
{
    if (cur_len_ + 1 > max_len_)
        grow(cur_len_ + 1);

    data_[cur_len_++] = c;
    data_[cur_len_] = 0;
}

void StringBuilder::append(const char *str)
{
    append(str, strlen(str));
}

void StringBuilder::append(const char *str, size_t count)
{
    size_t utf8_count = utf8_len(reinterpret_cast<const byte *>(str), count);

    if (cur_len_ + utf8_count > max_len_)
        grow(cur_len_ + utf8_count);

    uni_char *dst = data_ + cur_len_;

    const byte *ptr = reinterpret_cast<const byte *>(str);
    for (size_t i = 0; i < utf8_count; i++)
        dst[i] = utf8_dec(ptr);

    cur_len_ += utf8_count;
    data_[cur_len_] = 0;
}

void StringBuilder::append(const uni_char *str)
{
    append(str, uni_strlen(str));
}

void StringBuilder::append(const uni_char *str, size_t count)
{
    if (cur_len_ + count > max_len_)
        grow(cur_len_ + count);

    memcpy(data_ + cur_len_, str, count * sizeof(uni_char));

    cur_len_ += count;
    data_[cur_len_] = 0;
}

void StringBuilder::append(const String &str)
{
    append(str.data(), str.length());
}

void StringBuilder::append_space(size_t count)
{
    static const char spaces[] = "                             ";

    while (count >= (int)sizeof(spaces) - 1 )
    {
        append(spaces, sizeof(spaces) - 1);
        count -= sizeof(spaces) - 1;
    }

    if (count > 0)
        append(spaces, count);
}

size_t StringBuilder::allocated() const
{
    return max_len_;
}

size_t StringBuilder::length() const
{
    return cur_len_;
}

String StringBuilder::string() const
{
    return cur_len_ == 0 ? String() : String(data_, cur_len_);
}

const StringBuilder::SprintfModifier StringBuilder::sprintf_mods_[] =
{
    { 'd', 10, 1, SB_SPRINTF_MOD_RADIX,     0,  0 },
    { 's',  0, 2, SB_SPRINTF_MOD_STRING,    0,  0 },
    { 'c',  0, 0, SB_SPRINTF_MOD_CHAR,      0,  0 },
    { 'o',  8, 0, SB_SPRINTF_MOD_RADIX,     0,  2 },
    { 'u', 10, 0, SB_SPRINTF_MOD_RADIX,     0,  0 },
    { 'x', 16, 0, SB_SPRINTF_MOD_RADIX,     16, 1 },
    { 'X', 16, 0, SB_SPRINTF_MOD_RADIX,     0,  4 },
    { 'i', 10, 1, SB_SPRINTF_MOD_RADIX,     0,  0 },
    { 'n',  0, 0, SB_SPRINTF_MOD_SIZE,      0,  0 },
    { '%',  0, 0, SB_SPRINTF_MOD_PERCENT,   0,  0 },
    { 'p', 16, 0, SB_SPRINTF_MOD_POINTER,   0,  1 },

    // The following modifiers are typically not present in standard sprintf.
    { 'S',  0, 2, SB_SPRINTF_MOD_UNI_STRING,0,  0 },
    { 'C',  0, 0, SB_SPRINTF_MOD_UNI_CHAR,  0,  0 }
};

String StringBuilder::vsprintf(const char *format, va_list vl)
{
    static const char digits[] = "0123456789ABCDEF0123456789abcdef";
    static const char prefixes[] = "-x0\000X0";

    StringBuilder strbld;

    char buf[SB_SPRINTF_BUF_SIZE];

    for (int c; (c = (*format)) != 0; ++format)
    {
        char *buf_ptr = NULL;     // Pointer to the conversion buffer.

        if (c != '%')
        {
            buf_ptr = (char *)format;
            int amt = 1;
            while ((c = (*++format)) != '%' && c != 0)
                amt++;
            strbld.append(buf_ptr, amt);
            if (c == 0)
                break;
        }

        if ((c = (*++format)) == 0)
        {
            strbld.append("%", 1);
            break;
        }

        // Find out what flags are present.
        bool flag_leftjustify = false;      // true if "-" flag is present.
        bool flag_plussign = false;         // true if "+" flag is present.
        bool flag_blanksign = false;        // true if " " flag is present.
        bool flag_alternateform = false;    // true if "#" flag is present.
        bool flag_zeropad = false;          // true if field width constant starts with zero.
        bool done = false;

        do
        {
            switch (c)
            {
                case '-': flag_leftjustify = true;   break;
                case '+': flag_plussign = true;      break;
                case ' ': flag_blanksign = true;     break;
                case '#': flag_alternateform = true; break;
                case '0': flag_zeropad = true;       break;
                default:  done = true;               break;
            }
        }
        while (!done && (c = (*++format)) != 0);

        // Get the field width.
        int width = 0;
        if (c == '*')
        {
            width = va_arg(vl, int);
            if (width < 0)
            {
                flag_leftjustify = true;
                width = -width;
            }
            c = *++format;
        }
        else
        {
            while (c >= '0' && c <= '9')
            {
                width = width * 10 + c - '0';
                c = *++format;
            }
        }

        // Get the precision.
        int precision;
        if (c == '.')
        {
            precision = 0;
            c = *++format;
            if (c == '*')
            {
                precision = va_arg(vl, int);
                if (precision < 0)
                    precision = -precision;
                c = *++format;
            }
            else
            {
                while (c >= '0' && c <= '9')
                {
                    precision = precision * 10 + c - '0';
                    c = *++format;
                }
            }
        }
        else
        {
            precision = -1;
        }

        // Get the conversion type modifier.
        bool flag_long = false;     // true if "l" flag is present.
        bool flag_longlong = false; // true if the "ll" flag is present.
        if (c == 'l')
        {
            flag_long = true;
            c = *++format;
            if (c == 'l')
            {
                flag_longlong = true;
                c = *++format;
            }
            else
            {
                flag_longlong = false;
            }
        }
        else
        {
            flag_long = flag_longlong = false;
        }

        // Fetch the info entry for the field.
        const SprintfModifier *infop = &sprintf_mods_[0];
        SprintfControlCode xtype = SB_SPRINTF_MOD_UNKNOWN;

        for (int idx = 0; idx < ((int)(sizeof(sprintf_mods_) / sizeof(sprintf_mods_[0]))); idx++)
        {
            if (c == sprintf_mods_[idx].letter)
            {
                infop = &sprintf_mods_[idx];
                xtype = infop->type;
                break;
            }
        }

        int length = 0;     // Field length.
        char prefix = '\0'; // Prefix character: "+" or "-" or " " or '\0'.

        switch (xtype)
        {
            case SB_SPRINTF_MOD_POINTER:
                flag_longlong = sizeof(char *) == sizeof(int64_t);
                flag_long = sizeof(char *) == sizeof(long int);
                // Fall through into the next case.
            case SB_SPRINTF_MOD_RADIX:
            {
                uint64_t longvalue = 0;
                if (infop->flags & SB_SPRINTF_FLAG_SIGNED)
                {
                    int64_t v;
                    if (flag_longlong)
                        v = va_arg(vl, int64_t);
                    else if (flag_long)
                        v = va_arg(vl, long int);
                    else
                        v = va_arg(vl, int);

                    if (v < 0)
                    {
                        if (v == std::numeric_limits<int64_t>::min())
                            longvalue = ((uint64_t)1) << 63;
                        else
                            longvalue = -v;

                        prefix = '-';
                    }
                    else
                    {
                        longvalue = v;
                        if (flag_plussign)
                            prefix = '+';
                        else if (flag_blanksign)
                            prefix = ' ';
                        else
                            prefix = 0;
                    }
                }
                else
                {
                    if (flag_longlong)
                        longvalue = va_arg(vl, uint64_t);
                    else if (flag_long)
                        longvalue = va_arg(vl, unsigned long int);
                    else
                        longvalue = va_arg(vl, unsigned int);
                    prefix = 0;
                }

                if (longvalue == 0)
                    flag_alternateform = false;
                if (flag_zeropad && precision < width - (prefix != 0))
                    precision = width - (prefix != 0);

                char *out_ptr = NULL;  // Rendering buffer.
                int out_len = 0;       // Size of rendering buffer.
                if (precision < SB_SPRINTF_BUF_SIZE - 10)
                {
                    out_len = SB_SPRINTF_BUF_SIZE;
                    out_ptr = buf;
                }
                else
                {
                    out_len = precision + 10;
                    out_ptr = static_cast<char *>(GC_MALLOC_ATOMIC(out_len));
                    if (!out_ptr)
                        THROW(MemoryException);
                }

                buf_ptr = &out_ptr[out_len - 1];

                {
                    register const char *cset = &digits[infop->charset];
                    register int base = infop->base;

                    // Convert to ASCII.
                    do
                    {
                        *(--buf_ptr) = cset[longvalue % base];
                        longvalue = longvalue / base;
                    }
                    while (longvalue > 0);
                }

                length = (int)(&out_ptr[out_len - 1] - buf_ptr);
                for (int idx = precision - length; idx > 0; idx--)
                    *(--buf_ptr) = '0';       // Zero pad.

                if (prefix)
                    *(--buf_ptr) = prefix;    // Add sign.

                // Add "0" or "0x".
                if (flag_alternateform && infop->prefix)
                {
                    const char *pre;
                    char x;
                    pre = &prefixes[infop->prefix];
                    for (; (x = (*pre)) != 0; pre++)
                        *(--buf_ptr) = x;
                }

                length = (int)(&out_ptr[out_len - 1] - buf_ptr);
                break;
            }

            case SB_SPRINTF_MOD_SIZE:
                *(va_arg(vl, int *)) = static_cast<int>(strbld.length());
                length = width = 0;
                break;

            case SB_SPRINTF_MOD_PERCENT:
                buf[0] = '%';
                buf_ptr = buf;
                length = 1;
                break;

            case SB_SPRINTF_MOD_CHAR:
                c = va_arg(vl, int);
                buf[0] = (char)c;
                if (precision >= 0)
                {
                    for (int idx = 1; idx < precision; idx++)
                        buf[idx] = (char)c;

                    length = precision;
                }
                else
                {
                    length = 1;
                }

                buf_ptr = buf;
                break;

            case SB_SPRINTF_MOD_STRING:
                buf_ptr = va_arg(vl, char *);
                if (buf_ptr == 0)
                    buf_ptr = const_cast<char *>("");

                if (precision >= 0)
                    for (length = 0; length < precision && buf_ptr[length]; length++) {}
                else
                    length = strlen(buf_ptr);
                break;

            case SB_SPRINTF_MOD_UNI_STRING:
            {
                uni_char *str = va_arg(vl, uni_char *);
                if (str)
                {
                    if (precision >= 0)
                        for (length = 0; length < precision && buf_ptr[length]; length++) {}
                    else
                        length = uni_strlen(str);

                    // The text of the conversion is pointed to by "buf_ptr" and is  "length"
                    // characters long. The field width is "width". Do the output.
                    if (!flag_leftjustify)
                    {
                        register int num_spaces = width - length;
                        if (num_spaces > 0)
                            strbld.append_space(num_spaces);
                    }

                    if (length > 0)
                        strbld.append(str, length);

                    if (flag_leftjustify)
                    {
                        register int num_spaces = width - length;
                        if (num_spaces > 0)
                            strbld.append_space(num_spaces);
                    }
                }

                length = 0;
                continue;
            }

            case SB_SPRINTF_MOD_UNI_CHAR:
            {
                c = va_arg(vl, int);
                uni_char uni_buf[SB_SPRINTF_BUF_SIZE];
                uni_buf[0] = (uni_char)c;
                if (precision >= 0)
                {
                    for (int idx = 1; idx < precision; idx++)
                        buf[idx] = (uni_char)c;

                    length = precision;
                }
                else
                {
                    length = 1;
                }

                if (!flag_leftjustify)
                {
                    register int num_spaces = width - length;
                    if (num_spaces > 0)
                        strbld.append_space(num_spaces);
                }

                if (length > 0)
                    strbld.append(uni_buf, length);

                if (flag_leftjustify)
                {
                    register int num_spaces = width - length;
                    if (num_spaces > 0)
                        strbld.append_space(num_spaces);
                }

                length = 0;
                continue;
            }

            default:
            {
                assert(xtype == SB_SPRINTF_MOD_UNKNOWN);
                return String();
            }
        }

        // The text of the conversion is pointed to by "buf_ptr" and is "length"
        // characters long. The field width is "width". Do the output.
        if (!flag_leftjustify)
        {
            register int num_spaces = width - length;
            if (num_spaces > 0)
                strbld.append_space(num_spaces);
        }

        if (length > 0)
            strbld.append(buf_ptr, length);

        if (flag_leftjustify)
        {
            register int num_spaces = width - length;
            if (num_spaces > 0)
                strbld.append_space(num_spaces);
        }
    }
    
    return strbld.string();
}

String StringBuilder::sprintf(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);
    String res = vsprintf(format, vl);
    va_end(vl);

    return res;
}

