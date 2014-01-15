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

#include "token.hh"

namespace parser {

const char *Token::descriptions[] =
{
    "<end of input>",
    "<illegal>",

    // Punctuators 7.7
    "{",
    "}",
    "(",
    ")",
    "[",
    "]",
    ".",
    ";",
    ",",
    "<",
    ">",
    "<=",
    ">=",
    "==",
    "!=",
    "===",
    "!==",
    "+",
    "-",
    "*",
    "%",
    "++",
    "--",
    "<<",
    ">>",
    ">>>",
    "&",
    "|",
    "^",
    "!",
    "~",
    "&&",
    "||",
    "?",
    ":",
    "=",
    "+=",
    "-=",
    "*=",
    "%=",
    "<<=",
    ">>=",
    ">>>=",
    "&=",
    "|=",
    "^=",
    "/",
    "/=",

    // Literals 7.8
    "<identifier>",
    "<number>",
    "<string>",
    "null",
    "true",
    "false",
    "<regular expression>",

    // Keywords.
    "break",
    "case",
    "catch",
    "continue",
    "debugger",
    "default",
    "delete",
    "do",
    "else",
    "finally",
    "for",
    "function",
    "if",
    "in",
    "instanceof",
    "new",
    "return",
    "switch",
    "this",
    "throw",
    "try",
    "typeof",
    "var",
    "void",
    "while",
    "with",

    "<future reserved word>",
    "<future reserved strict mode word>"
};

}
