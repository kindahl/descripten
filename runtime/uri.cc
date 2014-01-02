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

#include "common/lexical.hh"
#include "common/stringbuilder.hh"
#include "common/unicode.hh"
#include "error.hh"
#include "messages.hh"
#include "uri.hh"

bool es_uri_reserved_predicate(uni_char c)
{
    switch (c)
    {
        case ';':
        case '/':
        case '?':
        case ':':
        case '@':
        case '&':
        case '=':
        case '+':
        case '$':
        case ',':
        case '#':   // Additional.
            return true;
    }

    return false;
}

bool es_uri_component_reserved_predicate(uni_char c)
{
    return false;
}

bool es_uri_unescaped_predicate(uni_char c)
{
    if (c >= '0' && c <= '9')
        return true;
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;

    switch (c)
    {
        case '-':
        case '_':
        case '.':
        case '!':
        case '~':
        case '*':
        case '\'':
        case '(':
        case ')':

        case ';':
        case '/':
        case '?':
        case ':':
        case '@':
        case '&':
        case '=':
        case '+':
        case '$':
        case ',':

        case '#':   // Additional.
            return true;
    }

    return false;
}

bool es_uri_component_unescaped_predicate(uni_char c)
{
    if (c >= '0' && c <= '9')
        return true;
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;

    switch (c)
    {
        case '-':
        case '_':
        case '.':
        case '!':
        case '~':
        case '*':
        case '\'':
        case '(':
        case ')':
            return true;
    }

    return false;
}

bool es_uri_encode(String str, EsUriSetPredicate pred, String &result)
{
    // 15.1.3.
    size_t str_len = str.length();

    StringBuilder r;

    for (size_t k = 0; k < str_len; k++)
    {
        uni_char c = str[k];
        if (pred(c))
        {
            r.append(c);
        }
        else
        {
            if (c >= 0xdc00 && c <= 0xdfff)
            {
                ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_ENC_FAIL));
                return false;
            }

            uni_char v = 0;
            if (c < 0xd800 || c > 0xdbff)
            {
                v = c;
            }
            else
            {
                k++;
                if (k == str_len)
                {
                    ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_ENC_FAIL));
                    return false;
                }

                uni_char kchar = str[k];
                if (kchar < 0xdc00 || kchar > 0xdfff)
                {
                    ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_ENC_FAIL));
                    return false;
                }

                v = (c - 0xd800) * 0x400 + (kchar - 0xdc00) + 0x10000;
            }

            byte octets[6];
            byte *ptr = octets;
            byte l = utf8_enc(ptr, v);

            for (byte j = 0; j < l; j++)
            {
                byte joctet = octets[j];
                r.append(StringBuilder::sprintf("%%%.2X", joctet));
            }
        }
    }

    result =  r.string();
    return true;
}

bool es_uri_decode(String str, EsUriSetPredicate pred, String &result)
{
    // 15.1.3.
    size_t str_len = str.length();

    StringBuilder r;

    for (size_t k = 0; k < str_len; k++)
    {
        uni_char c = str[k];

        String s;
        if (c != '%')
        {
            // FIXME: Remove s and append to r directly.
            s = String(c);
        }
        else
        {
            int start = k;
            if (k + 2 >= str_len)
            {
                ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                return false;
            }

            if (!es_is_hex_digit(str[k + 1]) || !es_is_hex_digit(str[k + 2]))
            {
                ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                return false;
            }

            uint8_t b = es_as_hex_digit(str[k + 1]) * 16 + es_as_hex_digit(str[k + 2]);

            k += 2;

            if (!(b & 0x80))
            {
                uni_char c = static_cast<uni_char>(b);
                if (!pred(c))
                    s = String(c);
                else
                    s = str.substr(start, k - start + 1);
            }
            else
            {
                int n = 1;
                for (; n < 6; n++)
                {
                    if (!((b << n) & 0x80))
                        break;
                }

                if (n == 1 || n > 4)
                {
                    ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                    return false;
                }

                byte octets[6];     // Even though we only use 4 bytes, utf8_dec expects 6.
                octets[0] = b;
                octets[4] = 0;      // To capture decode errors in utf8_test/utf8_dec.
                octets[5] = 0;      // To capture decode errors in utf8_test/utf8_dec.

                if (k + 3 * (n - 1) >= str_len)
                {
                    ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                    return false;
                }

                for (int j = 1; j < n; j++)
                {
                    k++;
                    if (str[k] != '%')
                    {
                        ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                        return false;
                    }

                    if (!es_is_hex_digit(str[k + 1]) || !es_is_hex_digit(str[k + 2]))
                    {
                        ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                        return false;
                    }

                    b = es_as_hex_digit(str[k + 1]) * 16 + es_as_hex_digit(str[k + 2]);
                    if ((b & 0xc0) != 0x80)
                    {
                        ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                        return false;
                    }

                    k += 2;

                    octets[j] = b;
                }

                if (!utf8_test(octets))
                {
                    ES_THROW(EsUriError, es_fmt_msg(ES_MSG_URI_BAD_FORMAT));
                    return false;
                }

                const byte *ptr = octets;
                c = utf8_dec(ptr);
                if (c < 0x10000)
                {
                    if (!pred(c))
                        s = String(c);
                    else
                        s = str.substr(start, k - start + 1);
                }
                else
                {
                    uni_char l = ((c - 0x10000) & 0x3ff) + 0xdc00;
                    uni_char h = (((c - 0x10000) >> 10) & 0x3ff) + 0xd800;

                    s = String(h);
                    s = s + String(l);
                }
            }
        }

        r.append(s);
    }

    result = r.string();
    return true;
}
