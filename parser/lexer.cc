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

#include <string.h>
#include "common/lexical.hh"
#include "common/unicode.hh"
#include "lexer.hh"

namespace parser {

bool es_is_unicode_letter(uni_char c)
{
    // 7.6
    switch (uni_get_category(c))
    {
        case UC_UPPERCASE_LETTER:
        case UC_LOWERCASE_LETTER:
        case UC_TITLECASE_LETTER:
        case UC_MODIFIER_LETTER:
        case UC_OTHER_LETTER:
        case UC_LETTER_NUMBER:
            return true;
    }

    return false;
}

bool es_is_unicode_combining_mark(uni_char c)
{
    // 7.6
    switch (uni_get_category(c))
    {
        case UC_NON_SPACING_MARK:
        case UC_COMBINING_SPACING_MARK:
            return true;
    }

    return false;
}

bool es_is_unicode_digit(uni_char c)
{
    return uni_get_category(c) == UC_DECIMAL_DIGIT_NUMBER;
}

bool es_is_unicode_connector_punctuation(uni_char c)
{
    return uni_get_category(c) == UC_CONNECTOR_PUNCTUATION;
}

bool es_is_identifier_start(uni_char c)
{
    // 7.6
    // IdentifierStart :: UnicodeLetter $ _ \UnicodeEscapeSequence
    return c == '$' || c == '_' || c == '\\' || es_is_unicode_letter(c);
}

bool es_is_identifier_part(uni_char c)
{
    // 7.6
    // IdentifierPart :: IdentifierStart UnicodeCombiningMark UnicodeDigit
    //                   UnicodeConnectorPunctuation <ZWNJ> <ZWJ>

    static uni_char zwnj = 0x200c;
    static uni_char zwj = 0x200d;

    return es_is_identifier_start(c) || es_is_unicode_combining_mark(c) ||
           es_is_unicode_digit(c) || es_is_unicode_connector_punctuation(c) ||
           c == zwnj || c == zwj;
}

Lexer::Lexer(UnicodeStream &stream)
    : stream_(stream)
{
}

Lexer::ReservedWord Lexer::reserved_words_[] =
{
    // Keywords (7.6.1.1)
    { "break", Token::BREAK },
    { "case", Token::CASE },
    { "catch", Token::CATCH },
    { "continue", Token::CONTINUE },
    { "debugger", Token::DEBUGGER },
    { "default", Token::DEFAULT },
    { "delete", Token::DELETE },
    { "do", Token::DO },
    { "else", Token::ELSE },
    { "finally", Token::FINALLY },
    { "for", Token::FOR },
    { "function", Token::FUNCTION },
    { "if", Token::IF },
    { "in", Token::IN },
    { "instanceof", Token::INSTANCEOF },
    { "new", Token::NEW },
    { "return", Token::RETURN },
    { "switch", Token::SWITCH },
    { "this", Token::THIS },
    { "throw", Token::THROW },
    { "try", Token::TRY },
    { "typeof", Token::TYPEOF },
    { "var", Token::VAR },
    { "void", Token::VOID },
    { "while", Token::WHILE },
    { "with", Token::WITH },

    // Future reserved words (7.6.1.2)
    { "class", Token::FUTURE_RESERVED_WORD },
    { "const", Token::FUTURE_RESERVED_WORD },
    { "enum", Token::FUTURE_RESERVED_WORD },
    { "export", Token::FUTURE_RESERVED_WORD },
    { "extends", Token::FUTURE_RESERVED_WORD },
    { "import", Token::FUTURE_RESERVED_WORD },
    { "super", Token::FUTURE_RESERVED_WORD },

    // Future reserved strict mode words (7.6.1.2)
    { "implements", Token::FUTURE_STRICT_RESERVED_WORD },
    { "interface", Token::FUTURE_STRICT_RESERVED_WORD },
    { "let", Token::FUTURE_STRICT_RESERVED_WORD },
    { "package", Token::FUTURE_STRICT_RESERVED_WORD },
    { "private", Token::FUTURE_STRICT_RESERVED_WORD },
    { "protected", Token::FUTURE_STRICT_RESERVED_WORD },
    { "public", Token::FUTURE_STRICT_RESERVED_WORD },
    { "static", Token::FUTURE_STRICT_RESERVED_WORD },
    { "yield", Token::FUTURE_STRICT_RESERVED_WORD },

    // Null literal (7.8.1)
    { "null", Token::LIT_NULL },

    // Boolean literals (7.8.2)
    { "true", Token::LIT_TRUE },
    { "false", Token::LIT_FALSE }
};

int32_t Lexer::read_hex_number(int num_digits)
{
    uni_char digits[4];
    assert(num_digits <= 4);    // Assert to detect when debugging.
    if (num_digits > 4)
        return -1;

    int32_t res = 0;
    for (int i = 0; i < num_digits; i++)
    {
        uni_char c = stream_.next();
        digits[i] = c;

        if (!es_is_hex_digit(c))
        {
            for (int j = i; j >= 0; j--)
                stream_.push(digits[j]);

            return -1;
        }

        res = res * 16 + es_as_hex_digit(c);
    }

    return res;
}

#ifdef ECMA262_EXT_STR_OCT_ESC
int32_t Lexer::read_oct_number(int num_digits)
{
    int32_t res = 0;
    for (int i = 0; i < num_digits; i++)
    {
        uni_char c = stream_.next();
        if (!es_is_oct_digit(c))
        {
            stream_.push(c);
            break;
        }

        res = res * 8 + es_as_oct_digit(c);
    }

    return res;
}
#endif

Lexer::ReservedWord *Lexer::find_reserved_word(const char *keyword)
{
    size_t num_res_words = sizeof(reserved_words_)/sizeof(ReservedWord);
    assert(num_res_words == 45);    // FIXME: Remove after confirmation.

    for (size_t i = 0; i < num_res_words; i++)
    {
        if (!strcmp(reserved_words_[i].keyword_, keyword))
            return &reserved_words_[i];
    }

    return NULL;
}

Token Lexer::skip_line_comment(bool skipped_line_term)
{
    uni_char c = stream_.next();
    while (c != static_cast<uni_char>(-1) && !es_is_line_terminator(c))
        c = stream_.next();

    // According to 7.4 the line terminator itself doesn't belong to the line comment.
    stream_.push(c);

    return next(skipped_line_term);
}

Token Lexer::skip_block_comment(bool skipped_line_term)
{
    uni_char c = stream_.next();
    while (c != static_cast<uni_char>(-1))
    {
        if (es_is_line_terminator(c))
            skipped_line_term = true;

        if (c == '*')
        {
            c = stream_.next();
            if (c == static_cast<uni_char>(-1))
                return Token(Token::ILLEGAL, String(),
                             Location(stream_.position() - 1, stream_.position()),
                             skipped_line_term);
            if (c == '/')
                return next(skipped_line_term);

            stream_.push(c);
        }

        c = stream_.next();
    }

    return Token(Token::ILLEGAL, String(),
                 Location(stream_.position() - 1, stream_.position()),
                 skipped_line_term);
}

Token Lexer::lex_identifier_or_reserved_word(bool skipped_line_term)
{
    int beg_pos = stream_.position();

    sb_.clear();

    uni_char c = stream_.next();
    while (es_is_identifier_part(c))
    {
        if (c == '\\')
        {
            // Scan escape code.
            uni_char c1 = stream_.next();
            if (c1 != 'u')
                return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

            int32_t val = read_hex_number(4);
            if (val == -1)
                return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

            if (!es_is_identifier_part(static_cast<uni_char>(val)))
                return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

            sb_.append(static_cast<uni_char>(val));
        }
        else
        {
            sb_.append(c);
        }

        c = stream_.next();
    }

    stream_.push(c);

    // Check if reserved word.
    ReservedWord *res_word = find_reserved_word(sb_.string().utf8().c_str());
    if (res_word)
        return Token(res_word->type_, sb_.string(), Location(beg_pos, stream_.position()), skipped_line_term);
    else
        return Token(Token::LIT_IDENTIFIER, sb_.string(), Location(beg_pos, stream_.position()), skipped_line_term);
}

Token Lexer::lex_numeric_literal(bool skipped_line_term, bool parsed_period)
{
    int beg_pos = stream_.position();

    sb_.clear();

    if (parsed_period)
    {
        sb_.append('.');
        beg_pos--;

        uni_char c = stream_.next();
        while (es_is_dec_digit(c))
        {
            sb_.append(c);
            c = stream_.next();
        }

        // Return the non-digit.
        stream_.push(c);
    }
    else
    {
        // Check if hexadecimal.
        uni_char c0 = stream_.next();
        uni_char c1 = stream_.next();
        if (c0 == '0' && (c1 == 'x' || c1 == 'X'))
        {
            sb_.append(c0);
            sb_.append(c1);

            // We must have at least one valid hexadecimal digit after 0x/0X to produce a valid token.
            uni_char c = stream_.next();
            if (!es_is_hex_digit(c))
                return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

            while (es_is_hex_digit(c))
            {
                sb_.append(c);
                c = stream_.next();
            }

            // Return the non-digit.
            stream_.push(c);
        }
        else
        {
            stream_.push(c1);
            stream_.push(c0);

            // Parse the decimal number.
            uni_char c = stream_.next();
            while (es_is_dec_digit(c))
            {
                sb_.append(c);
                c = stream_.next();
            }

            if (c == '.')
            {
                sb_.append(c);

                c = stream_.next();
                while (es_is_dec_digit(c))
                {
                    sb_.append(c);
                    c = stream_.next();
                }

                // Return the non-digit.
                stream_.push(c);
            }
            else
            {
                // Return the non-dot.
                stream_.push(c);
            }
        }
    }

    // Scan exponent.
    uni_char c = stream_.next();
    if (c == 'e' || c == 'E')
    {
        sb_.append(c);

        c = stream_.next();
        if (c == '+' || c == '-')
        {
            sb_.append(c);
            c = stream_.next();
        }

        if (!es_is_dec_digit(c))
            return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

        while (es_is_dec_digit(c))
        {
            sb_.append(c);
            c = stream_.next();
        }

        // Return the non-digit.
        stream_.push(c);
    }
    else
    {
        // Return non 'e' or 'E' letter.
        stream_.push(c);
    }

    // According to 7.8.3 "The source character immediately following a
    // NumericLiteral must not be an IdentifierStart or DecimalDigit."
    c = stream_.next();
    if (es_is_identifier_start(c) || es_is_dec_digit(c))
        return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

    stream_.push(c);
    return Token(Token::LIT_NUMBER, sb_.string(), Location(beg_pos, stream_.position()), skipped_line_term);
}

Token Lexer::lex_string_literal(bool skipped_line_term)
{
    sb_.clear();
    uni_char quote = stream_.next();    // We know that this will be a quote.

    // NOTE: The surrounding quote is not included in the literal.
    int beg_pos = stream_.position();

    bool contains_esc_seq = false;

    uni_char c = stream_.next();
    while (c != static_cast<uni_char>(-1) && c != quote && !es_is_line_terminator(c))
    {
        if (c == '\\')
        {
            contains_esc_seq = true;

            // Scan escape code.
            uni_char c1 = stream_.next();
            if (es_is_line_terminator(c1))
            {
                // According to 7.8.4 "The SV of
                // LineContinuation :: \ LineTerminatorSequence is the empty
                // character sequence."

                // Allow CR+LF and LF+CR.
                if (es_is_carriage_return(c1) || es_is_line_feed(c1))
                {
                    uni_char c2 = stream_.next();
                    if (!es_is_carriage_return(c2) && !es_is_line_feed(c2))
                        stream_.push(c2);
                }
            }
            else
            {
                switch (c1)
                {
                    // Single escape character.
                    case '\'':
                    case '"':
                    case '\\':
                        sb_.append(c1);
                        break;
                    case 'b':
                        sb_.append('\b');
                        break;
                    case 'f':
                        sb_.append('\f');
                        break;
                    case 'n':
                        sb_.append('\n');
                        break;
                    case 'r':
                        sb_.append('\r');
                        break;
                    case 't':
                        sb_.append('\t');
                        break;
                    case 'v':
                        sb_.append('\v');
                        break;

                    // <NUL> character.
                    case '0':
                    {
                        uni_char c2 = stream_.next();
#ifdef ECMA262_EXT_STR_OCT_ESC
                        if (es_is_oct_digit(c2))
                        {
                            stream_.push(c2);
                            stream_.push(c1);

                            sb_.append(static_cast<uni_char>(read_oct_number(2)));
                            break;
                        }
                        else
#endif
                        if (es_is_dec_digit(c2))
                            return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);
                        stream_.push(c2);
                        sb_.append(_U('\0'));
                        break;
                    }
#ifdef ECMA262_EXT_STR_OCT_ESC
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    {
                        stream_.push(c1);
                        int32_t val = read_oct_number(2);
                        if (val == -1)
                            return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);
                        else
                            sb_.append(static_cast<uni_char>(val));
                        break;
                    }
#endif

                    // Hex escape sequence.
                    case 'x':
                    {
                        int32_t val = read_hex_number(2);
                        if (val == -1)
                            return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);
                        else
                            sb_.append(static_cast<uni_char>(val));
                        break;
                    }

                    // Unicode escape sequence.
                    case 'u':
                    {
                        int32_t val = read_hex_number(4);
                        if (val == -1)
                            return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);
                        else
                            sb_.append(static_cast<uni_char>(val));
                        break;
                    }

                    // Non-escape character.
                    default:
                    {
                        // NonEscapeCharacter ::
                        //      SourceCharacter but not one of EscapeCharacter or LineTerminator
                        //
                        // EscapeCharacter ::
                        //      SingleEscapeCharacter
                        //      DecimalDigit
                        //      x
                        //      u

                        if (es_is_dec_digit(c1) || es_is_line_terminator(c1))
                            return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);
                        sb_.append(c1);
                        break;
                    }
                }
            }
        }
        else
        {
            sb_.append(c);
        }

        c = stream_.next();
    }

    if (c != quote)
        return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);

    // NOTE: The surrounding quote is not included in the literal.
    return Token(Token::LIT_STRING, sb_.string(), Location(beg_pos, stream_.position() - 1),
                 skipped_line_term, contains_esc_seq);
}

Token Lexer::next(bool skipped_line_term)
{
    // Check if we have already fetched the next token.
    if (!peek_.empty())
    {
        Token tok = peek_.back();
        peek_.pop_back();
        return tok;
    }

    int beg_pos = stream_.position();

    uni_char c0 = stream_.next();
    while (c0 != static_cast<uni_char>(-1) && es_is_white_space(c0))
    {
        if (es_is_line_terminator(c0))
            skipped_line_term = true;

        c0 = stream_.next();
    }

    switch (c0)
    {
        case static_cast<uni_char>(-1):
            return Token(Token::EOI   , String() , Location(beg_pos, stream_.position()), skipped_line_term);
        case '{':
            return Token(Token::LBRACE, _USTR("{"), Location(beg_pos, stream_.position()), skipped_line_term);
        case '}':
            return Token(Token::RBRACE, _USTR("}"), Location(beg_pos, stream_.position()), skipped_line_term);
        case '(':
            return Token(Token::LPAREN, _USTR("("), Location(beg_pos, stream_.position()), skipped_line_term);
        case ')':
            return Token(Token::RPAREN, _USTR(")"), Location(beg_pos, stream_.position()), skipped_line_term);
        case '[':
            return Token(Token::LBRACK, _USTR("["), Location(beg_pos, stream_.position()), skipped_line_term);
        case ']':
            return Token(Token::RBRACK, _USTR("]"), Location(beg_pos, stream_.position()), skipped_line_term);
        case '.':
        {
            uni_char c1 = stream_.next();
            if (es_is_dec_digit(c1))
            {
                stream_.push(c1);
                return lex_numeric_literal(skipped_line_term, true);
            }

            stream_.push(c1);
            return Token(Token::DOT   , _USTR("."), Location(beg_pos, stream_.position()), skipped_line_term);
        }
        case ';':
            return Token(Token::SEMI  , _USTR(";"), Location(beg_pos, stream_.position()), skipped_line_term);
        case ',':
            return Token(Token::COMMA , _USTR(","), Location(beg_pos, stream_.position()), skipped_line_term);
        case '<':
        {
            // <, <=, <<, <<=
            uni_char c1 = stream_.next();
            if (c1 == '=')
                return Token(Token::LTE, _USTR("<="), Location(beg_pos, stream_.position()), skipped_line_term);
            if (c1 == '<')
                return select('=', Token(Token::ASSIGN_LS, _USTR("<<="), Location(beg_pos, stream_.position()), skipped_line_term),
                                   Token(Token::LS       , _USTR("<<") , Location(beg_pos, stream_.position()), skipped_line_term));

            stream_.push(c1);
            return Token(Token::LT, _USTR("<"), Location(beg_pos, stream_.position()), skipped_line_term);
        }
        case '>':
        {
            // >, >=, >>, >>=, >>>, >>>=
            uni_char c1 = stream_.next();
            if (c1 == '=')
                return Token(Token::GTE, _USTR(">="), Location(beg_pos, stream_.position()), skipped_line_term);
            if (c1 == '>')
            {
                uni_char c2 = stream_.next();
                if (c2 == '=')
                    return Token(Token::ASSIGN_RSS, _USTR(">>="), Location(beg_pos, stream_.position()), skipped_line_term);

                if (c2 == '>')
                    return select('=', Token(Token::ASSIGN_RUS, _USTR(">>>="), Location(beg_pos, stream_.position()), skipped_line_term),
                                       Token(Token::RUS       , _USTR(">>>") , Location(beg_pos, stream_.position()), skipped_line_term));

                stream_.push(c2);
                return Token(Token::RSS, _USTR(">>"), Location(beg_pos, stream_.position()), skipped_line_term);
            }

            stream_.push(c1);
            return Token(Token::GT, _USTR(">"), Location(beg_pos, stream_.position()), skipped_line_term);
        }
        case '=':
        {
            // =, ==, ===
            uni_char c1 = stream_.next();
            if (c1 == '=')
                return select('=', Token(Token::STRICT_EQ, _USTR("==="), Location(beg_pos, stream_.position()), skipped_line_term),
                                   Token(Token::EQ       , _USTR("==") , Location(beg_pos, stream_.position()), skipped_line_term));

            stream_.push(c1);
            return Token(Token::ASSIGN, _USTR("="), Location(beg_pos, stream_.position()) , skipped_line_term);
        }
        case '!':
        {
            // !, !=, !==
            uni_char c1 = stream_.next();
            if (c1 == '=')
                return select('=', Token(Token::STRICT_NEQ, _USTR("!=="), Location(beg_pos, stream_.position()), skipped_line_term),
                                   Token(Token::NEQ       , _USTR("!=") , Location(beg_pos, stream_.position()), skipped_line_term));

            stream_.push(c1);
            return Token(Token::LOG_NOT, _USTR("!"), Location(beg_pos, stream_.position()), skipped_line_term);
        }
        case '+':
        {
            // +, ++, +=
            return select('+', Token(Token::INC       , _USTR("++"), Location(beg_pos, stream_.position()), skipped_line_term),
                          '=', Token(Token::ASSIGN_ADD, _USTR("+="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::ADD       , _USTR("+") , Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '-':
        {
            // -, --, -=
            return select('-', Token(Token::DEC       , _USTR("--"), Location(beg_pos, stream_.position()), skipped_line_term),
                          '=', Token(Token::ASSIGN_SUB, _USTR("-="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::SUB       , _USTR("-" ), Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '*':
        {
            // *, *=
            return select('=', Token(Token::ASSIGN_MUL, _USTR("*="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::MUL       , _USTR("*") , Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '%':
        {
            // %, %=
            return select('=', Token(Token::ASSIGN_MOD, _USTR("%="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::MOD       , _USTR("%") , Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '&':
        {
            // &, &&, &=
            return select('&', Token(Token::LOG_AND       , _USTR("&&"), Location(beg_pos, stream_.position()), skipped_line_term),
                          '=', Token(Token::ASSIGN_BIT_AND, _USTR("&="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::BIT_AND       , _USTR("&") , Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '|':
        {
            // |, ||, |=
            return select('|', Token(Token::LOG_OR       , _USTR("||"), Location(beg_pos, stream_.position()), skipped_line_term),
                          '=', Token(Token::ASSIGN_BIT_OR, _USTR("|="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::BIT_OR       , _USTR("|") , Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '^':
        {
            // ^, ^=
            return select('=', Token(Token::ASSIGN_BIT_XOR, _USTR("^="), Location(beg_pos, stream_.position()), skipped_line_term),
                               Token(Token::BIT_XOR       , _USTR("^" ), Location(beg_pos, stream_.position()), skipped_line_term));
        }
        case '~':
            return Token(Token::BIT_NOT, _USTR("~"), Location(beg_pos, stream_.position()), skipped_line_term);
        case '?':
            return Token(Token::COND, _USTR("?"), Location(beg_pos, stream_.position()), skipped_line_term);
        case ':':
            return Token(Token::COLON, _USTR(":"), Location(beg_pos, stream_.position()), skipped_line_term);
        case '/':
        {
            // /, /=, //, /*
            uni_char c1 = stream_.next();
            if (c1 == '=')
                return Token(Token::ASSIGN_DIV, _USTR("/="), Location(beg_pos, stream_.position()), skipped_line_term);
            if (c1 == '/')
                return skip_line_comment(skipped_line_term);
            if (c1 == '*')
                return skip_block_comment(skipped_line_term);

            stream_.push(c1);
            return Token(Token::DIV, _USTR("/"), Location(beg_pos, stream_.position()), skipped_line_term);
        }
        default:
        {
            stream_.push(c0);
            if (es_is_identifier_start(c0))
                return lex_identifier_or_reserved_word(skipped_line_term);
            if (es_is_dec_digit(c0))
                return lex_numeric_literal(skipped_line_term, false);
            if (c0 == '"' || c0 == '\'')
                return lex_string_literal(skipped_line_term);
            break;
        }
    };

    return Token(Token::ILLEGAL, String(), Location(beg_pos, stream_.position()), skipped_line_term);
}

Token Lexer::next()
{
    return next(false);
}

Token Lexer::next_reg_exp()
{
    // Lex body.
    Token tok = next();
    if (tok != Token::DIV && tok != Token::ASSIGN_DIV)
        return Token(Token::ILLEGAL, String(), tok.location(), tok.is_separated_by_line_term());

    sb_.clear();
    sb_.append(tok.string());

    bool in_char_class = false;

    uni_char c = stream_.next();
    while (c != '/' || in_char_class)
    {
        if (c == static_cast<uni_char>(-1) || es_is_line_terminator(c))
        {
            return Token(Token::ILLEGAL, String(),
                         Location(tok.location().begin(), stream_.position()),
                         tok.is_separated_by_line_term());
        }

        if (c == '\\')
        {
            sb_.append(c);

            c = stream_.next();
            if (c == static_cast<uni_char>(-1) || es_is_line_terminator(c))
            {
                return Token(Token::ILLEGAL, String(),
                             Location(tok.location().begin(), stream_.position()),
                             tok.is_separated_by_line_term());
            }
        }
        else
        {
            if (c == '[')
                in_char_class = true;
            if (c == ']')
                in_char_class = false;
        }

        sb_.append(c);
        c = stream_.next();
    }

    sb_.append(c);  // Include trailing '/'.

    // Lex flags.
    c = stream_.next();
    while (es_is_identifier_part(c))
    {
        // FIXME: similar code as in lex_identifier_or_reserved_word.
        if (c == '\\')
        {
            // Scan escape code.
            uni_char c1 = stream_.next();
            if (c1 != 'u')
            {
                stream_.push(c1);
                break;
            }

            int32_t val = read_hex_number(4);
            if (val == -1)
            {
                stream_.push(c1);
                break;
            }

            sb_.append(static_cast<uni_char>(val));
        }
        else
        {
            sb_.append(c);
        }

        c = stream_.next();
    }
    stream_.push(c);

    return Token(Token::_LIT_REGEXP, sb_.string(),
                 Location(tok.location().begin(), stream_.position()),
                 tok.is_separated_by_line_term());
}

Token Lexer::peek()
{
    // We can only peek one element.
    if (peek_.empty())
        peek_.push_back(next());

    return peek_.back();
}

void Lexer::push(Token tok)
{
    peek_.push_back(tok);
}

size_t Lexer::position() const
{
    if (!peek_.empty())
        return peek_.back().location().begin();

    return stream_.position();
}

}
