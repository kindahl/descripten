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

#pragma once
#include <vector>
#include <gc/gc_allocator.h>
#include "common/stringbuilder.hh"
#include "location.hh"
#include "stream.hh"
#include "token.hh"

namespace parser {

bool es_is_unicode_letter(uni_char c);
bool es_is_identifier_start(uni_char c);

class Lexer
{
private:
    /**
     * @brief Structure describing a single reserved word.
     */
    struct ReservedWord
    {
        const char *keyword_;
        Token::Type type_;
    };

    static ReservedWord reserved_words_[];  ///< Defines all reserved words.

private:
    UnicodeStream &stream_;     ///< Source input stream.

    StringBuilder sb_;          ///< Stateless string builder shared by many routines.

    TokenVector peek_;          ///< Peek stack.

    /**
     * Reads a hexadecimal number with the specified number of digits from the
     * Unicode input stream.
     * @note The maximum number of digits to parse is 4.
     * @param [in] num_digits The number of digits to read.
     * @return If successful the value of the hexadecimal digits, on failure -1
     *         is returned.
     */
    int32_t read_hex_number(int num_digits);

#ifdef ECMA262_EXT_STR_OCT_ESC
    /**
     * Reads an octal number with at most the specified number of digits from
     * the Unicode input stream.
     * @pre The Unicode stream contains at least one octal digit.
     * @param [in] num_digits The maximum number of digits to read.
     * @return If successful the value of the octal number, if no octal digits
     *         could be read from the stream the function returns zero.
     */
    int32_t read_oct_number(int num_digits);
#endif

    /**
     * Looks up a reserved word in the list of reserved words.
     * @param [in] keyword The keyword to look for.
     * @return If a matching keyword is found a pointer to it is returned. If
     *         no matching word is find the function returns NULL.
     */
    ReservedWord *find_reserved_word(const char *keyword);

    /**
     * Select a token depending on the next character in the input stream. If
     * the character test is present the accept token in returned and the
     * stream pointer advanced. If the character test is not present the reject
     * token is returned without advancing the stream pointer.
     * @param [in] test Character to look for.
     * @param [in] accept Token to return if test is found.
     * @param [in] reject Token to return if test is not found.
     * @return accept if test is found and reject otherwise.
     */
    inline Token select(uni_char test, Token accept, Token reject)
    {
        uni_char c = stream_.next();
        if (c == test)
            return accept;

        stream_.push(c);
        return reject;
    }

    /**
     * Select a token depending on the next character in the input stream. If
     * the character test1/test2 is present the accept1/accept2 token in
     * returned and the stream pointer advanced. If neither the test1 and test2
     * characters are present the reject token is returned without advancing
     * the stream pointer.
     * @param [in] test1 Character to look for.
     * @param [in] accept1 Token to return if test1 is found.
     * @param [in] test2 Character to look for.
     * @param [in] accept2 Token to return if test2 is found.
     * @param [in] reject Token to return if test1 and test2 is not found.
     * @return accept1/accept2 if test1/test2 is found and reject otherwise.
     */
    inline Token select(uni_char test1, Token accept1, uni_char test2, Token accept2, Token reject)
    {
        assert(test1 != test2);

        uni_char c = stream_.next();
        if (c == test1)
            return accept1;
        if (c == test2)
            return accept2;

        stream_.push(c);
        return reject;
    }

    /**
     * @return Token succeeding the line comment.
     * @param [in] skipped_line_term true if a line terminator separates the
     *                               comment from the previous token.
     */
    Token skip_line_comment(bool skipped_line_term);

    /**
     * @return Token succeeding the block comment.
     * @param [in] skipped_line_term true if a line terminator separates the
     *                               comment from the previous token.
     */
    Token skip_block_comment(bool skipped_line_term);

    /**
     * Lexes an identifier and returns the token.
     * @pre The next character in stream_ is an "IdentifierStart" character.
     * @param [in] skipped_line_term true if a line terminator separates the
     *                               next token from the previous one.
     * @return Identifier token.
     */
    Token lex_identifier_or_reserved_word(bool skipped_line_term);

    /**
     * Lexes a numeric literal and returns the token.
     * @param [in] skipped_line_term true if a line terminator separates the
     *                               next token from the previous one.
     * @param [in] parsed_period true if the period has already been parsed.
     * @return Numeric literal token.
     */
    Token lex_numeric_literal(bool skipped_line_term, bool parsed_period);

    /**
     * Lexes a string literal and returns the token.
     * @pre The next character in stream_ is either '"' or '\''.
     * @param [in] skipped_line_term true if a line terminator separates the
     *                               next token from the previous one.
     * @return String literal token.
     */
    Token lex_string_literal(bool skipped_line_term);

    /**
     * @return Next token in the token stream.
     * @param [in] skipped_line_term true if a line terminator separates the
     *                               next token from the previous one.
     */
    Token next(bool skipped_line_term);

public:
    /**
     * Constructs a lexer for lexing ECMA-262 code.
     * @param [in] stream Source input stream.
     */
    Lexer(UnicodeStream &stream);

    /**
     * @return Next token in the token stream.
     */
    Token next();

    /**
     * Regular expressions must be lexed differently than "ordinary" tokens.
     * Since the lexer doesn't know when it encounters a regular expression,
     * the parser must explicitly request a regular expression token when
     * appropriate.
     * @see ECMA-282 7.8.5.
     * @pre The next token is either Token::DIV or Token::ASSIGN_DIV.
     * @return Next token lexed as a regular expression token.
     */
    Token next_reg_exp();

    /**
     * @return Next token in the token stream without advancing the stream.
     */
    Token peek();

    /**
     * Push a token back to the lexer for later retrieval.
     * @param [in] tok Token to return.
     */
    void push(Token tok);

    /**
     * @return Current stream position.
     */
    size_t position() const;
};

}
