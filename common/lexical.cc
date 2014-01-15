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

#include <cassert>
#include <cmath>
#include <limits>
#include <errno.h>
#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include "lexical.hh"
#include "unicode.hh"

/** strtod in dtoa.c */
extern "C" double _strtod(const char *s00, char **se);

bool es_is_white_space(uni_char c)
{
    // A.1; 7.2
    // WhiteSpace :: <TAB> <VT> <FF> <SP> <NBSP> <BOM> <USP>
    
    static uni_char ws[] = 
    {
        0x0009, /* TAB */
        0x000b, /* VT */
        0x000c, /* FF */
        0x0020, /* SP */
        0x00a0, /* NBSP */
        0xfeff, /* BOM*/
        
        // Other white space characters recognized by the Unicode standard.
        0x000a, /* LF */
        0x000d, /* CR */
        0x0085, /* NEL */
        0x1680,
        0x180e,
        0x2000,
        0x2001,
        0x2002,
        0x2003,
        0x2004,
        0x2005,
        0x2006,
        0x2007,
        0x2008,
        0x2009,
        0x200a,
        0x2028, /* LS */
        0x2029, /* PS */
        0x202f,
        0x205f,
        0x3000
    };
    
    for (size_t i = 0; i < sizeof(ws)/sizeof(uni_char); i++)
    {
        if (c == ws[i])
            return true;
    }
    
    return false;
}

bool es_is_line_terminator(uni_char c)
{
    // A.1; 7.3
    // LineTerminator :: <LF> <CR> <LS> <PS>
    
    static uni_char lt[] = 
    {
        0x000a, /* LF */
        0x000d, /* CR */
        0x2028, /* LS */
        0x2029  /* PS */
    };
    
    for (size_t i = 0; i < sizeof(lt)/sizeof(uni_char); i++)
    {
        if (c == lt[i])
            return true;
    }
    
    return false;
}

bool es_is_carriage_return(uni_char c)
{
    return c == 0x000d;
}

bool es_is_line_feed(uni_char c)
{
    return c == 0x000a;
}

bool es_is_oct_digit(uni_char c)
{
    return (c >= '0' && c <= '7');
}

bool es_is_dec_digit(uni_char c)
{
    // 7.8.3
    return (c >= '0' && c <= '9');
}

bool es_is_hex_digit(uni_char c)
{
    // 7.8.3
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool es_is_dec_number(const uni_char *str, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (!es_is_dec_digit(str[i]))
            return false;
    }

    return true;
}

uint8_t es_as_oct_digit(uni_char c)
{
    assert(es_is_oct_digit(c));

    if (c >= '0' && c <= '7')
        return static_cast<uint8_t>(c - '0');

    return 0;
}

uint8_t es_as_dec_digit(uni_char c)
{
    assert(es_is_hex_digit(c));

    if (c >= '0' && c <= '9')
        return static_cast<uint8_t>(c - '0');

    return 0;
}

uint8_t es_as_hex_digit(uni_char c)
{
    assert(es_is_hex_digit(c));

    if (c >= '0' && c <= '9')
        return static_cast<uint8_t>(c - '0');

    if (c >= 'a' && c <= 'f')
        return static_cast<uint8_t>(c - 'a') + 10;

    if (c >= 'A' && c <= 'F')
        return static_cast<uint8_t>(c - 'A') + 10;

    return 0;
}

double es_strtou(const uni_char *nptr, const uni_char **endptr, int radix)
{
    if (!nptr)
        return std::numeric_limits<double>::quiet_NaN();
    
    assert(radix >= 2 && radix <= 32);
    
    int num_d_chars = radix < 10 ? radix : 10;
    int num_l_chars = radix > 10 ? radix - 10 : 0;
    
    double res = 0.0;
    
    const uni_char *ptr = nptr;
    while (*ptr)
    {
        uint32_t digit = 0;
        uni_char c = *ptr;
        if (c >= static_cast<uni_char>('0') &&
            c <= static_cast<uni_char>('0' + num_d_chars - 1))
        {
            digit = static_cast<int>(c - '0');
        }
        else if (num_l_chars > 0 &&
                 c >= 'a' && c <= static_cast<uni_char>('a' + num_l_chars - 1))
        {
            digit = static_cast<int>(c - 'a') + 10;
        }
        else if (num_l_chars > 0 &&
                 c >= static_cast<uni_char>('A') &&
                 c <= static_cast<uni_char>('A' + num_l_chars - 1))
        {
            digit = static_cast<int>(c - 'A') + 10;
        }
        else
        {
            // Check if failure on first try.
            if (ptr == nptr)
                return std::numeric_limits<double>::quiet_NaN();
            break;
        }
        
        res = res * radix + digit;
        ptr++;
    }
    
    if (endptr)
        *endptr = ptr;
    
    // Return if we're sure that the integer can be properly represented  using
    // a double.
    uint64_t dbl_int_limit = static_cast<uint64_t>(1) << 53;
    if (res < static_cast<double>(dbl_int_limit))
        return res;
    
    // Let es_strtod to the conversion, it may do a better job.
    if (radix == 10)
        return es_strtod(nptr, endptr);
    
    return res;
}

double es_strtod(const uni_char *nptr, const uni_char **endptr)
{
    if (!nptr)
        return 0.0;
    
    const uni_char *ustr = nptr;
    es_str_skip_white_spaces(ustr);
    if (!ustr || !*ustr)
        return 0.0;
    
    size_t len = uni_strlen(ustr);
    
    // Create a C-string in order to use standard functions.
    char cbuf[32 * 6];
    char *cstr = cbuf;
    
    if (len > (sizeof(cbuf) / 6) - 1)
    {
        cstr = static_cast<char *>(GC_MALLOC_ATOMIC((len + 1) * 6));
        assert(cstr);
    }
    
    // Encode string as UTF-8.
    byte *tmp = reinterpret_cast<byte *>(cstr);
    for (size_t i = 0; i < len; i++)
        utf8_enc(tmp, ustr[i]);
    *tmp = 0;
    
    // Look for a sign.
    char *istr = cstr;
    bool neg = *istr == '-';
    if (neg || *istr == '+')
        istr++;

    // Look for more white spaces.
    while (*istr && *istr == ' ')
        istr++;
    
    // Do the parsing.
    double res = 0.0;

    char *estr = NULL;
    if (!strncmp(istr,"Infinity",8))
    {
        estr = istr + 8;
        res = std::numeric_limits<double>::infinity();
    }
    else
    {
        res = _strtod(istr, &estr);
        
        if (errno == ERANGE)
        {
            if (res == HUGE_VAL)
                res = std::numeric_limits<double>::infinity();
            else if (res == -HUGE_VAL)
                res = -std::numeric_limits<double>::infinity();
        }
    }

    assert(estr);

    if (istr == estr)
        res = std::numeric_limits<double>::quiet_NaN();

    size_t diff = estr - cstr;  // Number of bytes parsed by strtod or custom parsing above.
    if (endptr)
    {
        size_t skip = utf8_len(reinterpret_cast<const byte *>(cstr), diff);
        *endptr = ustr + skip;  // ustr points to the first non-white-space.
    }
    
    return neg ? -res : res;
}
