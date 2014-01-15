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

class EsString;

/**
 * Defines the URI predicate function for encoding and decoding URIs.
 * @param [in] c Character to test.
 * @return true if c is included in the set and false otherwise.
 */
typedef bool (*EsUriSetPredicate)(uni_char c);

/**
 * Predicate function for including all reserved URI characters (A.6) plus the
 * additional '#' character.
 * @param [in] c Character to test.
 * @return true if c is included in the set and false otherwise.
 */
bool es_uri_reserved_predicate(uni_char c);

/**
 * Predicate function for including all reserved URI component characters (A.6)
 * plus the additional '#' character.
 * @param [in] c Character to test.
 * @return true if c is included in the set and false otherwise.
 */
bool es_uri_component_reserved_predicate(uni_char c);

/**
 * Predicate function for including all unescaped URI characters (A.6) plus the
 * additional '#' character.
 * @param [in] c Character to test.
 * @return true if c is included in the set and false otherwise.
 */
bool es_uri_unescaped_predicate(uni_char c);

/**
 * Predicate function for including all unescaped URI component characters
 * (A.6) plus the additional '#' character.
 * @param [in] c Character to test.
 * @return true if c is included in the set and false otherwise.
 */
bool es_uri_component_unescaped_predicate(uni_char c);

/**
 * Encodes an URI string according to 15.1.3.
 * @param [in] str String to encode.
 * @param [in] unescaped Set of characters that shouldn't be escaped.
 * @return Pointer to encoded string on normal return, a NULL pointer if an
 *         exception was thrown.
 */
const EsString *es_uri_encode(const EsString *str, EsUriSetPredicate pred);

/**
 * Decodes an URU string according to 15.1.3.
 * @param [in] str String to decode.
 * @param [in] reserve Set of reserved character that shouldn't be decoded.
 * @return Pointer to decoded string on normal return, a NULL pointer if an
 *         exception was thrown.
 */
const EsString *es_uri_decode(const EsString *str, EsUriSetPredicate pred);
