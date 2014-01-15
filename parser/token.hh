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
#include <limits>
#include "common/string.hh"
#include "location.hh"

namespace parser {

class Token
{
public:
    enum Type
    {
        EOI,            ///< Signals end of input.
        ILLEGAL,        ///< Illegal token.

        // Punctuators 7.7
        LBRACE,         ///< Left curly brace "{".
        RBRACE,         ///< Right curly brace "}".
        LPAREN,         ///< Left parenthesis "(".
        RPAREN,         ///< Right parenthesis ")".
        LBRACK,         ///< Left bracket "[".
        RBRACK,         ///< Right bracket "]".
        DOT,            ///< Dot ".".
        SEMI,           ///< Semicolon ";".
        COMMA,          ///< Comma ",".
        LT,             ///< Less than "<".
        GT,             ///< Greater than ">".
        LTE,            ///< Less than equals "<=".
        GTE,            ///< Greater than equals ">=".
        EQ,             ///< Equals "==".
        NEQ,            ///< Not equals "!=".
        STRICT_EQ,      ///< Strict equals "===".
        STRICT_NEQ,     ///< Strict not equals "!==".
        ADD,            ///< Add "+".
        SUB,            ///< Subtract "-".
        MUL,            ///< Multiply "*".
        MOD,            ///< Modulo "%".
        INC,            ///< Increment "++".
        DEC,            ///< Decrement "--".
        LS,             ///< Left shift "<<".
        RSS,            ///< Right signed shift ">>".
        RUS,            ///< Right unsigned shift ">>>".
        BIT_AND,        ///< Bitwise AND "&".
        BIT_OR,         ///< Bitwise OR "|".
        BIT_XOR,        ///< Bitwise XOR "^".
        LOG_NOT,        ///< Not "!".
        BIT_NOT,        ///< Bitwise NOT "~".
        LOG_AND,        ///< Logical AND "&&".
        LOG_OR,         ///< Logical OR "||".
        COND,           ///< Conditional "?".
        COLON,          ///< Colon ":".
        ASSIGN,         ///< Assign "=".
        ASSIGN_ADD,     ///< Add and assign "+=".
        ASSIGN_SUB,     ///< Subtract and assign "-=".
        ASSIGN_MUL,     ///< Multiply and assign "*=".
        ASSIGN_MOD,     ///< Modulo and assign "%=".
        ASSIGN_LS,      ///< Left shift and assign "<<=".
        ASSIGN_RSS,     ///< Right signed shift and assign ">>=".
        ASSIGN_RUS,     ///< Right unsigned shift and assign ">>>=".
        ASSIGN_BIT_AND, ///< Bit AND and assign "&=".
        ASSIGN_BIT_OR,  ///< Bit OR and assign "|=".
        ASSIGN_BIT_XOR, ///< Bit XOR and assign "^=".
        DIV,            ///< Divide "/".
        ASSIGN_DIV,     ///< Divide and assign "/=".

        // Literals 7.8
        LIT_IDENTIFIER, ///< Identifier.
        LIT_NUMBER,     ///< Number literal.
        LIT_STRING,     ///< String literal.
        LIT_NULL,       ///< Null literal.
        LIT_TRUE,       ///< Boolean TRUE literal.
        LIT_FALSE,      ///< Boolean FALSE literal.
        _LIT_REGEXP,    ///< Regular expression literal. NOTE: requires special lexing.

        // Keywords.
        BREAK,          ///< "break" keyword.
        CASE,           ///< "case" keyword.
        CATCH,          ///< "catch" keyword.
        CONTINUE,       ///< "continue" keyword.
        DEBUGGER,       ///< "debugger" keyword.
        DEFAULT,        ///< "default" keyword.
        DELETE,         ///< "delete" keyword.
        DO,             ///< "do" keyword.
        ELSE,           ///< "else" keyword.
        FINALLY,        ///< "finally" keyword.
        FOR,            ///< "for" keyword.
        FUNCTION,       ///< "function" keyword.
        IF,             ///< "if" keyword.
        IN,             ///< "in" keyword.
        INSTANCEOF,     ///< "instanceof" keyword.
        NEW,            ///< "new" keyword.
        RETURN,         ///< "return" keyword.
        SWITCH,         ///< "switch" keyword.
        THIS,           ///< "this" keyword.
        THROW,          ///< "throw" keyword.
        TRY,            ///< "try" keyword.
        TYPEOF,         ///< "typeof" keyword.
        VAR,            ///< "var" keyword.
        VOID,           ///< "void" keyword.
        WHILE,          ///< "while" keyword.
        WITH,           ///< "with" keyword.

        FUTURE_RESERVED_WORD,           ///< Keywords reserved for the future.
        FUTURE_STRICT_RESERVED_WORD,    ///< Strict mode keywords reserved for the future.
    };

    static const char *descriptions[];

private:
    bool line_term_;    ///< Is the token separated from the previous token by a line terminator?
    bool cont_esc_;     ///< Does the token contain a line continuation or escape sequence, applies to string literals.
    Type type_;
    String string_;
    Location loc_;

public:
    Token()
        : line_term_(false)
        , cont_esc_(false)
        , type_(ILLEGAL)
    {
    }

    Token(Type type, const String &string, Location loc, bool line_term, bool cont_esc = false)
        : line_term_(line_term)
        , cont_esc_(cont_esc)
        , type_(type)
        , string_(string)
        , loc_(loc)
    {
    }

    Type type() const
    {
        return type_;
    }

    String string() const
    {
        return string_;
    }

    Location location() const
    {
        return loc_;
    }

    bool contains_esc_seq() const
    {
        return cont_esc_;
    }

    bool is_separated_by_line_term() const
    {
        return line_term_;
    }

    bool is_assignment() const
    {
        // AssignmentOperator (A.3) + '='.
        switch (type_)
        {
            case ASSIGN:
            case ASSIGN_ADD:
            case ASSIGN_SUB:
            case ASSIGN_MUL:
            case ASSIGN_MOD:
            case ASSIGN_LS:
            case ASSIGN_RSS:
            case ASSIGN_RUS:
            case ASSIGN_BIT_AND:
            case ASSIGN_BIT_OR:
            case ASSIGN_BIT_XOR:
            case ASSIGN_DIV:
                return true;
        }

        return false;
    }

    bool is_unary() const
    {
        // AssignmentOperator (A.3) + '='.
        switch (type_)
        {
            case DELETE:
            case VOID:
            case TYPEOF:
            case INC:
            case DEC:
            case ADD:
            case SUB:
            case BIT_NOT:
            case LOG_NOT:
                return true;
        }

        return false;
    }

    /**
     * @return true if the token is a reserved keyword and false otherwise.
     */
    bool is_keyword() const
    {
        switch (type_)
        {
            case BREAK:
            case CASE:
            case CATCH:
            case CONTINUE:
            case DEBUGGER:
            case DEFAULT:
            case DELETE:
            case DO:
            case ELSE:
            case FINALLY:
            case FOR:
            case FUNCTION:
            case IF:
            case IN:
            case INSTANCEOF:
            case NEW:
            case RETURN:
            case SWITCH:
            case THIS:
            case THROW:
            case TRY:
            case TYPEOF:
            case VAR:
            case VOID:
            case WHILE:
            case WITH:

            case LIT_NULL:
            case LIT_TRUE:
            case LIT_FALSE:
                return true;
        }

        return false;
    }

    /**
     * @return true if the token is a reserved keyword (for the future) and
     *         false otherwise.
     */
    bool is_future_reserved_keyword() const
    {
        return type_ == FUTURE_RESERVED_WORD;
    }

    /**
     * @return true if the token is a reserved strict mode keyword (for the
     *         future) and false otherwise.
     */
    bool is_future_strict_reserved_keyword() const
    {
        return type_ == FUTURE_STRICT_RESERVED_WORD;
    }

    /**
     * @return true if the token is an octal number literal.
     */
    bool is_octal() const
    {
        if (type_ != LIT_NUMBER)
            return false;

        if (string_.length() < 2 || string_[0] != '0')
            return false;

        for (size_t i = 1; i < string_.length(); i++)
        {
            if (string_[i] < '0' || string_[i] > '7')
                return false;
        }

        return true;
    }

    /**
     * Returns  the precedence for token operator.
     * @param [in] no_in Set to true to disable (maximum precedence) the IN operator.
     * @return Precedence for binary operators.
     */
    int precedence(bool no_in = false) const
    {
        return precedence(type_, no_in);
    }

    /**
     * @return Token type.
     */
    operator Token::Type()
    {
        return type_;
    }

    /**
     * Returns  the precedence for binary operators.
     * @param [in] type Token type to get precedence for.
     * @param [in] no_in Set to true to disable (maximum precedence) the IN operator.
     * @return Precedence for binary operators.
     */
    static int precedence(Token::Type type, bool no_in = false)
    {
        if (type == IN && no_in)
            return std::numeric_limits<int>::max();

        // Values given by:
        // https://developer.mozilla.org/en/JavaScript/Reference/Operators/Operator_Precedence
        switch (type)
        {
            case MUL:
            case DIV:
            case MOD:
                return 5;

            case ADD:
            case SUB:
                return 6;

            case LS:
            case RSS:
            case RUS:
                return 7;

            case LT:
            case GT:
            case LTE:
            case GTE:
            case IN:
            case INSTANCEOF:
                return 8;

            case EQ:
            case NEQ:
            case STRICT_EQ:
            case STRICT_NEQ:
                return 9;

            case BIT_AND:
                return 10;

            case BIT_XOR:
                return 11;

            case BIT_OR:
                return 12;

            case LOG_AND:
                return 13;

            case LOG_OR:
                return 14;
        }

        return std::numeric_limits<int>::max();     // Weakest binding possible.
    }

    static String description(Token::Type type)
    {
        // WARNING: This is dangerous.
        return String(descriptions[type]);
    }
};

/**
 * Vector of tokens.
 */
typedef std::vector<Token, gc_allocator<Token> > TokenVector;

}
