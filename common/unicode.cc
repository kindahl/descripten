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

/*
 * Portions of this file uses code from the ICU project. The ICU license
 * follows below:
 *
 * ICU License - ICU 1.8.1 and later
 *
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright (c) 1995-2011 International Business Machines Corporation and others 
 *
 * All rights reserved. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, provided that the above copyright notice(s) and this permission
 * notice appear in all copies of the Software and that both the above
 * copyright notice(s) and this permission notice appear in supporting
 * documentation. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE
 * LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 *
 * Except as contained in this notice, the name of a copyright holder shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization of the
 * copyright holder.
 */

#include <cassert>
#include "unicode.hh"

/** Build-time trie structure. */
struct UNewTrie2;
typedef struct UNewTrie2 UNewTrie2;

/*
 * Trie structure definition.
 *
 * Either the data table is 16 bits wide and accessed via the index
 * pointer, with each index item increased by indexLength;
 * in this case, data32==NULL, and data16 is used for direct ASCII access.
 *
 * Or the data table is 32 bits wide and accessed via the data32 pointer.
 */
struct UTrie2 {
    /* protected: used by macros and functions for reading values */
    const uint16_t *index;
    const uint16_t *data16;     /* for fast UTF-8 ASCII access, if 16b data */
    const uint32_t *data32;     /* NULL if 16b data is used via index */

    int32_t indexLength, dataLength;
    uint16_t index2NullOffset;  /* 0xffff if there is no dedicated index-2 null block */
    uint16_t dataNullOffset;
    uint32_t initialValue;
    /** Value returned for out-of-range code points and illegal UTF-8. */
    uint32_t errorValue;

    /* Start of the last range which ends at U+10ffff, and its value. */
    uni_char highStart;
    int32_t highValueIndex;

    /* private: used by builder and unserialization functions */
    void *memory;           /* serialized bytes; NULL if not frozen yet */
    int32_t length;         /* number of serialized bytes at memory; 0 if not frozen yet */
    int isMemoryOwned;     /* TRUE if the trie owns the memory */
    int padding1;
    int16_t padding2;
    UNewTrie2 *newTrie;     /* builder object; NULL when frozen */
};

/** An ICU version consists of up to 4 numbers from 0..255.
 *  @stable ICU 2.4
 */
#define U_MAX_VERSION_LENGTH 4

/** The binary form of a version on ICU APIs is an array of 4 uint8_t.
 *  To compare two versions, use memcmp(v1,v2,sizeof(UVersionInfo)).
 *  @stable ICU 2.4
 */
typedef uint8_t UVersionInfo[U_MAX_VERSION_LENGTH];

#define FALSE false
#define TRUE true 

/* indexes[] entries */
enum {
    UPROPS_INDEX_COUNT=16
};

#include "uchar_props_data.c"

/**
 * Trie constants, defining shift widths, index array lengths, etc.
 *
 * These are needed for the runtime macros but users can treat these as
 * implementation details and skip to the actual public API further below.
 */
enum {
    /** Shift size for getting the index-1 table offset. */
    UTRIE2_SHIFT_1=6+5,

    /** Shift size for getting the index-2 table offset. */
    UTRIE2_SHIFT_2=5,

    /**
     * Difference between the two shift sizes,
     * for getting an index-1 offset from an index-2 offset. 6=11-5
     */
    UTRIE2_SHIFT_1_2=UTRIE2_SHIFT_1-UTRIE2_SHIFT_2,

    /** Number of entries in a data block. 32=0x20 */
    UTRIE2_DATA_BLOCK_LENGTH=1<<UTRIE2_SHIFT_2,

    /** Mask for getting the lower bits for the in-data-block offset. */
    UTRIE2_DATA_MASK=UTRIE2_DATA_BLOCK_LENGTH-1,

    /**
     * Shift size for shifting left the index array values.
     * Increases possible data size with 16-bit index values at the cost
     * of compactability.
     * This requires data blocks to be aligned by UTRIE2_DATA_GRANULARITY.
     */
    UTRIE2_INDEX_SHIFT=2,

    /**
     * The part of the index-2 table for U+D800..U+DBFF stores values for
     * lead surrogate code _units_ not code _points_.
     * Values for lead surrogate code _points_ are indexed with this portion of the table.
     * Length=32=0x20=0x400>>UTRIE2_SHIFT_2. (There are 1024=0x400 lead surrogates.)
     */
    UTRIE2_LSCP_INDEX_2_OFFSET=0x10000>>UTRIE2_SHIFT_2,
    UTRIE2_LSCP_INDEX_2_LENGTH=0x400>>UTRIE2_SHIFT_2,

    /** Count the lengths of both BMP pieces. 2080=0x820 */
    UTRIE2_INDEX_2_BMP_LENGTH=UTRIE2_LSCP_INDEX_2_OFFSET+UTRIE2_LSCP_INDEX_2_LENGTH,

    /**
     * The illegal-UTF-8 data block follows the ASCII block, at offset 128=0x80.
     * Used with linear access for single bytes 0..0xbf for simple error handling.
     * Length 64=0x40, not UTRIE2_DATA_BLOCK_LENGTH.
     */
    UTRIE2_BAD_UTF8_DATA_OFFSET=0x80,

    /**
     * The 2-byte UTF-8 version of the index-2 table follows at offset 2080=0x820.
     * Length 32=0x20 for lead bytes C0..DF, regardless of UTRIE2_SHIFT_2.
     */
    UTRIE2_UTF8_2B_INDEX_2_OFFSET=UTRIE2_INDEX_2_BMP_LENGTH,
    UTRIE2_UTF8_2B_INDEX_2_LENGTH=0x800>>6,  /* U+0800 is the first code point after 2-byte UTF-8 */

    /**
     * The index-1 table, only used for supplementary code points, at offset 2112=0x840.
     * Variable length, for code points up to highStart, where the last single-value range starts.
     * Maximum length 512=0x200=0x100000>>UTRIE2_SHIFT_1.
     * (For 0x100000 supplementary code points U+10000..U+10ffff.)
     *
     * The part of the index-2 table for supplementary code points starts
     * after this index-1 table.
     *
     * Both the index-1 table and the following part of the index-2 table
     * are omitted completely if there is only BMP data.
     */
    UTRIE2_INDEX_1_OFFSET=UTRIE2_UTF8_2B_INDEX_2_OFFSET+UTRIE2_UTF8_2B_INDEX_2_LENGTH,
    UTRIE2_MAX_INDEX_1_LENGTH=0x100000>>UTRIE2_SHIFT_1,

    /**
     * Number of index-1 entries for the BMP. 32=0x20
     * This part of the index-1 table is omitted from the serialized form.
     */
    UTRIE2_OMITTED_BMP_INDEX_1_LENGTH=0x10000>>UTRIE2_SHIFT_1,

    /** Number of entries in an index-2 block. 64=0x40 */
    UTRIE2_INDEX_2_BLOCK_LENGTH=1<<UTRIE2_SHIFT_1_2,

    /** Mask for getting the lower bits for the in-index-2-block offset. */
    UTRIE2_INDEX_2_MASK=UTRIE2_INDEX_2_BLOCK_LENGTH-1,
};

/** Internal low-level trie getter. Returns a data index. */
#define _UTRIE2_INDEX_RAW(offset, trieIndex, c) \
    (((int32_t)((trieIndex)[(offset)+((c)>>UTRIE2_SHIFT_2)]) \
    <<UTRIE2_INDEX_SHIFT)+ \
    ((c)&UTRIE2_DATA_MASK))

/** Internal trie getter from a supplementary code point below highStart. Returns the data index. */
#define _UTRIE2_INDEX_FROM_SUPP(trieIndex, c) \
    (((int32_t)((trieIndex)[ \
        (trieIndex)[(UTRIE2_INDEX_1_OFFSET-UTRIE2_OMITTED_BMP_INDEX_1_LENGTH)+ \
                      ((c)>>UTRIE2_SHIFT_1)]+ \
        (((c)>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK)]) \
    <<UTRIE2_INDEX_SHIFT)+ \
    ((c)&UTRIE2_DATA_MASK))

/**
 * Internal trie getter from a code point, with checking that c is in 0..10FFFF.
 * Returns the data index.
 */
#define _UTRIE2_INDEX_FROM_CP(trie, asciiOffset, c) \
    ((uint32_t)(c)<0xd800 ? \
        _UTRIE2_INDEX_RAW(0, (trie)->index, c) : \
        (uint32_t)(c)<=0xffff ? \
            _UTRIE2_INDEX_RAW( \
                (c)<=0xdbff ? UTRIE2_LSCP_INDEX_2_OFFSET-(0xd800>>UTRIE2_SHIFT_2) : 0, \
                (trie)->index, c) : \
            (uint32_t)(c)>0x10ffff ? \
                (asciiOffset)+UTRIE2_BAD_UTF8_DATA_OFFSET : \
                (c)>=(trie)->highStart ? \
                    (trie)->highValueIndex : \
                    _UTRIE2_INDEX_FROM_SUPP((trie)->index, c))

/**
 * Internal trie getter from a code point, with checking that c is in 0..10FFFF.
 * Returns the data.
 */
#define _UTRIE2_GET(trie, data, asciiOffset, c) \
    (trie)->data[_UTRIE2_INDEX_FROM_CP(trie, asciiOffset, c)]

/**
 * Return a 16-bit trie value from a code point, with range checking.
 * Returns trie->errorValue if c is not in the range 0..U+10ffff.
 *
 * @param trie (const UTrie2 *, in) a frozen trie
 * @param c (UChar32, in) the input code point
 * @return (uint16_t) The code point's trie value.
 */
#define UTRIE2_GET16(trie, c) _UTRIE2_GET((trie), index, (trie)->indexLength, (c))

#define GET_PROPS(c, result) ((result)=UTRIE2_GET16(&propsTrie, c));

#define GET_CATEGORY(props) ((props)&0x1f)

/* Gets the Unicode character's general category.*/
int8_t u_charType(uni_char c)
{
    uint32_t props;
    GET_PROPS(c, props);
    return (int8_t)GET_CATEGORY(props);
}

size_t utf8_len(const byte *ptr)
{
    size_t i = 0, j = 0;
    while (ptr[i])
    {
        if ((ptr[i] & 0xc0) != 0x80)
            j++;
        i++;
    }
    
    return j;
}

size_t utf8_len(const byte *ptr, size_t size)
{
    size_t j = 0;
    for (size_t i = 0; i < size; i++)
    {
        if ((ptr[i] & 0xc0) != 0x80)
            j++;
    }
    
    return j;
}

size_t utf8_off(const byte *ptr, size_t size, size_t index)
{
    size_t j = 0;
    for (size_t i = 0; i < size; i++)
    {
        if ((ptr[i] & 0xc0) != 0x80)
        {
            if (j == index)
                return i;

            j++;
        }
    }

    return index;
}

bool utf8_test(const byte *ptr)
{
    byte bytes = 0;

    byte b1 = *ptr++;
    if (b1 <= UTF8_MAX_1_BYTE_CHAR)
        return true;
    else if ((b1 & 0xe0) == 0xc0)
        bytes = 2;
    else if ((b1 & 0xf0) == 0xe0)
        bytes = 3;
    else if ((b1 & 0xf8) == 0xf0)
        bytes = 4;
    else if ((b1 & 0xfc) == 0xf8)
        bytes = 5;
    if ((b1 & 0xfe) == 0xfc)
        bytes = 6;

    for (byte i = 1; i < bytes; i++)
    {
        byte b = *ptr++;

        if (!b || (b & 0xc0) != 0x80)
            return false;
    }

    return true;
}

uni_char utf8_dec(const byte *&ptr)
{
    uni_char val = 0;
    byte bytes = 0;
    
    byte b1 = *ptr++;
    if (b1 <= UTF8_MAX_1_BYTE_CHAR)
    {
        return static_cast<uni_char>(b1);
    }
    else if ((b1 & 0xe0) == 0xc0)
    {
        bytes = 2;
        val = b1 & 0x1f;
    }
    else if ((b1 & 0xf0) == 0xe0)
    {
        bytes = 3;
        val = b1 & 0x0f;
    }
    else if ((b1 & 0xf8) == 0xf0)
    {
        bytes = 4;
        val = b1 & 0x07;
    }
    else if ((b1 & 0xfc) == 0xf8)
    {
        bytes = 5;
        val = b1 & 0x03;
    }
    if ((b1 & 0xfe) == 0xfc)
    {
        bytes = 6;
        val = b1 & 0x1;
    }
    
    for (byte i = 1; i < bytes; i++)
    {
        byte b = *ptr++;
        
        if (!b || (b & 0xc0) != 0x80)
        {
            assert(false);
            break;
        }
        
        val = (val << 6) | (b & 0x3f);
    }
    
    return val;
}

byte utf8_enc(byte *&ptr, uni_char val)
{
    //assert(uni_is_valid_char(val));

    if (val <= UTF8_MAX_1_BYTE_CHAR)        // 1-byte.
    {
        *ptr++ = val;
        return 1;
    }
    else if (val <= UTF8_MAX_2_BYTE_CHAR)   // 2-bytes.
    {
        *ptr++ = 0xc0 | (val >> 6);
        *ptr++ = 0x80 | (val & 0x3f);
        return 2;
    }
    else if (val <= UTF8_MAX_3_BYTE_CHAR)   // 3-bytes.
    {
        *ptr++ = 0xe0 | (val >> 12);
        *ptr++ = 0x80 | ((val >> 6) & 0x3f);
        *ptr++ = 0x80 | (val & 0x3f);
        return 3;
    }
    else if (val <= UTF8_MAX_4_BYTE_CHAR)   // 4-bytes.
    {
        *ptr++ = 0xf0 | (val >> 18);
        *ptr++ = 0x80 | ((val >> 12) & 0x3f);
        *ptr++ = 0x80 | ((val >> 6) & 0x3f);
        *ptr++ = 0x80 | (val & 0x3f);
        return 4;
    }
    else if (val <= UTF8_MAX_5_BYTE_CHAR)   // 5-bytes.
    {
        *ptr++ = 0xf8 | (val >> 24);
        *ptr++ = 0x80 | ((val >> 18) & 0x3f);
        *ptr++ = 0x80 | ((val >> 12) & 0x3f);
        *ptr++ = 0x80 | ((val >> 6) & 0x3f);
        *ptr++ = 0x80 | (val & 0x3f);
        return 5;
    }
    else                                    // 6-bytes.
    {
        *ptr++ = 0xfc | (val >> 30);
        *ptr++ = 0x80 | ((val >> 24) & 0x3f);
        *ptr++ = 0x80 | ((val >> 18) & 0x3f);
        *ptr++ = 0x80 | ((val >> 12) & 0x3f);
        *ptr++ = 0x80 | ((val >> 6) & 0x3f);
        *ptr++ = 0x80 | (val & 0x3f);
        return 6;
    }
}

size_t utf16le_len(const byte *ptr)
{
    size_t i = 0, j = 0;
    while (ptr[i])
    {
        //uint32_t c0 = (static_cast<uint16_t>(ptr[i]) << 8) | ptr[i + 1];
        uint32_t c0 = (static_cast<uint16_t>(ptr[i + 1]) << 8) | ptr[i];

        if (c0 < 0xd800 || c0 > 0xdfff)
        {
            j++;
            i += 2;
            continue;
        }

        //uint32_t c1 = (static_cast<uint16_t>(ptr[i + 2]) << 8) | ptr[i + 3];
        uint32_t c1 = (static_cast<uint16_t>(ptr[i + 3]) << 8) | ptr[i + 2];

        if (c0 >= 0xd800 && c0 <= 0xdbff &&
            c1 >= 0xdc00 && c1 <= 0xdfff)
        {
            j++;
            i += 2;
        }

        i += 2;
    }
    
    return j;
}

size_t utf16le_len(const byte *ptr, size_t size)
{
    size_t j = 0;
    for (size_t i = 0; i < size; i+= 2)
    {
        //uint32_t c0 = (static_cast<uint16_t>(ptr[i]) << 8) | ptr[i + 1];
        uint32_t c0 = (static_cast<uint16_t>(ptr[i + 1]) << 8) | ptr[i];

        if (c0 < 0xd800 || c0 > 0xdfff)
        {
            j++;
            continue;
        }

        assert(i + 3 < size);
        //uint32_t c1 = (static_cast<uint16_t>(ptr[i + 2]) << 8) | ptr[i + 3];
        uint32_t c1 = (static_cast<uint16_t>(ptr[i + 3]) << 8) | ptr[i + 2];

        if (c0 >= 0xd800 && c0 <= 0xdbff &&
            c1 >= 0xdc00 && c1 <= 0xdfff)
        {
            j++;
            i += 2;
        }
    }
    
    return j;
}

uni_char utf16le_dec(const byte *&ptr)
{
    //uint16_t code0 = (static_cast<uint16_t>(ptr[0]) << 8) | ptr[1];
    uint16_t code0 = (static_cast<uint16_t>(ptr[1]) << 8) | ptr[0];

    if (code0 < 0xd800 || code0 > 0xdfff)
    {
        ptr += 2;
        return code0;
    }

    //uint16_t code1 = (static_cast<uint16_t>(ptr[2]) << 8) | ptr[3];
    uint16_t code1 = (static_cast<uint16_t>(ptr[3]) << 8) | ptr[2];

    if (code0 >= 0xd800 && code0 <= 0xdbff &&
        code1 >= 0xdc00 && code1 <= 0xdfff)
    {
        ptr += 4;
        return (((code0 & 0x03ffL) << 10) |
                ((code1 & 0x03ffL) <<  0)) + 0x00010000L;
    }

    assert(false);
    return 0;
}

#ifdef UNTESTED
byte utf16le_enc(byte *&ptr, uni_char val)
{
    assert(uni_is_valid_char(val));

    if ((val >= 0x0000 && val <= 0xd7ff) ||
        (val >= 0xe000 && val <= 0xffff))
    {
        ptr[0] =  val       & 0xff;
        ptr[1] = (val >> 8) & 0xff;
        return 2;
    }

    if (val >= 0x10000 && val <= 0x10ffff)
    {
        uni_char tmp  = val - 0x10000;
        uint16_t hi = 0xd800 | (tmp >> 10);
        uint16_t lo = 0xdc00 | (tmp & 0x3ff);

        ptr[0] =  hi       & 0xff;
        ptr[1] = (hi >> 8) & 0xff;
        ptr[2] =  lo       & 0xff;
        ptr[3] = (lo >> 8) & 0xff;
        return 4;
    }

    assert(false);
    return 0;
}
#endif

int uni_strcmp(const uni_char *s1, const uni_char *s2)
{
    for (; *s1 == *s2; ++s1, ++s2)
    {
        if (*s1 == 0)
            return 0;
    }
    
    return *s1 < *s2 ? -1 : 1;
}

size_t uni_strlen(const uni_char *s)
{
    if (!s)
        return 0;

    const uni_char *str = s;
    for (; *str; ++str);
    return str - s;
}

UnicodeCategory uni_get_category(uni_char c)
{
    return static_cast<UnicodeCategory>(u_charType(c));
}

bool uni_is_valid_char(uni_char c)
{
    return c <= UNI_CODE_POINT_MAX && !uni_is_surrogate(c);
}

bool uni_is_lead_surrogate(uni_char c)
{
    return c >= UNI_SURROGATE_LEAD_MIN && c <= UNI_SURROGATE_LEAD_MAX;
}

bool uni_is_tail_surrogate(uni_char c)
{
    return c >= UNI_SURROGATE_TAIL_MIN && c <= UNI_SURROGATE_TAIL_MAX;
}

bool uni_is_surrogate(uni_char c)
{
    return c >= UNI_SURROGATE_LEAD_MIN && c <= UNI_SURROGATE_TAIL_MAX;
}
