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
#include <stddef.h>
#include "types.hh"

///< Maximum value a single byte UTF-8 character can hold.
#define UTF8_MAX_1_BYTE_CHAR        0x7f

///< Maximum value a two byte UTF-8 character can hold.
#define UTF8_MAX_2_BYTE_CHAR        0x7ff

///< Maximum value a three byte UTF-8 character can hold.
#define UTF8_MAX_3_BYTE_CHAR        0xffff

///< Maximum value a four byte UTF-8 character can hold.
#define UTF8_MAX_4_BYTE_CHAR        0x1fffff

///< Maximum value a five byte UTF-8 character can hold.
#define UTF8_MAX_5_BYTE_CHAR        0x3ffffff

///< Maximum value a six byte UTF-8 character can hold.
#define UTF8_MAX_6_BYTE_CHAR        0x7fffffff

///< Maximum valid value for a Unicode code point.
#define UNI_CODE_POINT_MAX          0x0010ffff

///< Minimum value for Unicode code points used to encode leading surrogate pairs.
#define UNI_SURROGATE_LEAD_MIN      0xd800

///< Maximum value for Unicode code points used to encode leading surrogate pairs.
#define UNI_SURROGATE_LEAD_MAX      0xdbff

///< Minimum for Unicode code points used to encode tailing surrogate pairs.
#define UNI_SURROGATE_TAIL_MIN      0xdc00

///< Maximum for Unicode code points used to encode tailing surrogate pairs.
#define UNI_SURROGATE_TAIL_MAX      0xdfff

/**
 * @brief Defines Unicode category typs.
 * @see http://www.unicode.org/Public/UNIDATA/UnicodeData.html
 */
enum UnicodeCategory
{
    UC_GENERAL_OTHER_TYPES      = 0,
    UC_UPPERCASE_LETTER         = 1,  ///< Lu.
    UC_LOWERCASE_LETTER         = 2,  ///< Ll.
    UC_TITLECASE_LETTER         = 3,  ///< Lt.
    UC_MODIFIER_LETTER          = 4,  ///< Lm.
    UC_OTHER_LETTER             = 5,  ///< Lo.
    UC_NON_SPACING_MARK         = 6,  ///< Mn.
    UC_ENCLOSING_MARK           = 7,  ///< Me.
    UC_COMBINING_SPACING_MARK   = 8,  ///< Mc.
    UC_DECIMAL_DIGIT_NUMBER     = 9,  ///< Nd.
    UC_LETTER_NUMBER            = 10, ///< Nl.
    UC_OTHER_NUMBER             = 11, ///< No.
    UC_SPACE_SEPARATOR          = 12, ///< Zs.
    UC_LINE_SEPARATOR           = 13, ///< Zl.
    UC_PARAGRAPH_SEPARATOR      = 14, ///< Zp.
    UC_CONTROL_CHAR             = 15, ///< Cc.
    UC_FORMAT_CHAR              = 16, ///< Cf.
    UC_PRIVATE_USE_CHAR         = 17, ///< Co.
    UC_SURROGATE                = 18, ///< Cs.
    UC_DASH_PUNCTUATION         = 19, ///< Pd.
    UC_START_PUNCTUATION        = 20, ///< Ps.
    UC_END_PUNCTUATION          = 21, ///< Pe.
    UC_CONNECTOR_PUNCTUATION    = 22, ///< Pc.
    UC_OTHER_PUNCTUATION        = 23, ///< Po.
    UC_MATH_SYMBOL              = 24, ///< Sm.
    UC_CURRENCY_SYMBOL          = 25, ///< Sc.
    UC_MODIFIER_SYMBOL          = 26, ///< Sk.
    UC_OTHER_SYMBOL             = 27, ///< So.
    UC_INITIAL_PUNCTUATION      = 28, ///< Pi.
    UC_FINAL_PUNCTUATION        = 29, ///< Pf.
};

/**
 * Computes the length of a NULL-terminated UTF-8 encoded string.
 * @param [in] ptr Pointer to beginning of UTF-8 encoded string.
 * @return Number of characters in string.
 */
size_t utf8_len(const byte *ptr);

/**
 * Computes the length of a non NULL-terminated UTF-8 encoded string.
 * @param [in] ptr Pointer to beginning of UTF-8 encoded string.
 * @param [in] size Number of bytes in input buffer.
 * @return Number of characters in string.
 */
size_t utf8_len(const byte *ptr, size_t size);

/**
 * Computes the byte offset to the character at position index.
 * @param [in] ptr Pointer to beginning of UTF-8 encoded string.
 * @param [in] size Number of bytes in input string.
 * @param [in] index Index of character to compute offset for.
 * @return If index refers to a character in the string the byte
 *         offset to that character is returned. If index is out
 *         of range, index is returned.
 */
size_t utf8_off(const byte *ptr, size_t size, size_t index);

/**
 * Tests if the specified byte sequence can be decoded a UTF-8 character.
 * @param [in,out] ptr Buffer to read from, it must be at least 6 bytes
 *                     large.
 * @return true if the data can be interpreted as a UTF-8 character, if not
 *         false is returned.
 */
bool utf8_test(const byte *ptr);

/**
 * Decodes a UTF-8 character value.
 * @param [in,out] ptr UTF-8 buffer to read from, the pointer will be
 *                     updated to point to the byte after the last decoded
 *                     byte. The buffer must contain at least 6 bytes.
 * @return Unicode character value.
 */
uni_char utf8_dec(const byte *&ptr);

/**
 * Encodes a character value in UTF-8 format.
 * @param [out] ptr Output pointer to write UTF-8 data to, must be able to
 *                  hold at least 6 bytes. The pointer will be updated to point
 *                  to the byte after the last encoded byte.
 * @param [in] val Value to encode in UTF-8.
 * @return The number of bytes used to store the character value.
 */
byte utf8_enc(byte *&ptr, uni_char val);

/**
 * Computes the length of a NULL-terminated UTF-16 little endian encoded
 * string.
 * @param [in] ptr Pointer to beginning of UTF-16 encoded string.
 * @return Number of characters in string.
 */
size_t utf16le_len(const byte *ptr);

/**
 * Computes the length of a non NULL-terminated UTF-16 little endian encoded
 * string.
 * @param [in] ptr Pointer to beginning of UTF-16 encoded string.
 * @param [in] size Number of bytes in input buffer.
 * @return Number of characters in string.
 */
size_t utf16le_len(const byte *ptr, size_t size);

/**
 * Decodes a UTF-16 little endian character value.
 * @param [in,out] ptr UTF-16 buffer to read from, the pointer will be
 *                     updated to point to the byte after the last decoded
 *                     byte.
 * @return Unicode character value.
 */
uni_char utf16le_dec(const byte *&ptr);

/**
 * Encodes a character value in UTF-16 little endian format.
 * @param [out] ptr Output pointer to write UTF-16 data to, must be able to
 *                  hold at least 4 bytes.
 * @param [in] val Value to encode in UTF-16.
 * @return The number of bytes used to store the character value.
 */
byte utf16le_enc(byte *&ptr, uni_char val);

/**
 * Unicode implementation of the standard strcmp() function.
 * @param [in] s1 First string.
 * @param [in] s2 Second string.
 * @return The result of the comparison.
 */
int uni_strcmp(const uni_char *s1, const uni_char *s2);

/**
 * Unicode implementation of the standard strlen() function.
 * @param [in] s String to get length of.
 * @return Number of characters in the string.
 */
size_t uni_strlen(const uni_char *s);

/**
 * Returns the Unicode category for a given character.
 * @param [in] c Character to get category for.
 * @return Category for character c.
 */
UnicodeCategory uni_get_category(uni_char c);

/**
 * Tests if a character is a valid Unicode code point.
 * @param [in] c Character to test.
 * @return true if c is a valid Unicode code point, false otherwise.
 */
bool uni_is_valid_char(uni_char c);

/**
 * Tests if a character is within the range of characters used by UTF-16 to
 * encode surrogate pairs.
 * @param [in] c Character to test.
 * @return true if the character is within the surrogate code point range,
 *         false otherwise.
 */
bool uni_is_surrogate(uni_char c);
