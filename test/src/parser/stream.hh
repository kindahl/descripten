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

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <limits>
#include <stdarg.h>
#include <string.h>
#include "parser/stream.hh"

// FIXME: Move to common library.

using parser::UnicodeStream;
using parser::Utf8Stream;

bool is_line_break(uni_char c)
{
    // From Wikipedial:
    // LF:    Line Feed, U+000A
    // VT:    Vertical Tab, U+000B
    // FF:    Form Feed, U+000C
    // CR:    Carriage Return, U+000D
    // CR+LF: CR (U+000D) followed by LF (U+000A)
    // NEL:   Next Line, U+0085
    // LS:    Line Separator, U+2028
    // PS:    Paragraph Separator, U+2029

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

std::string read_file(const char * file_path)
{
    std::ifstream file(file_path);

    file.seekg(0, std::ios::end);   
    std::string str;
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign(std::istreambuf_iterator<char>(file),
               std::istreambuf_iterator<char>());

    return str;
}

std::vector<uni_char> get_word(UnicodeStream & str)
{
    std::vector<uni_char> res;

    uni_char c = str.next();
    while (c == ' ' || is_line_break(c))
        c = str.next();

    while (c && c != ' ' && !is_line_break(c))
    {
        res.push_back(c);
        c = str.next();
    }

    str.push(c);
    return res;
}

std::vector<uni_char> get_line(UnicodeStream & str)
{
    std::vector<uni_char> res;

    uni_char c = str.next();
    while (is_line_break(c))
        c = str.next();

    while (c && !is_line_break(c))
    {
        res.push_back(c);
        c = str.next();
    }

    str.push(c);
    return res;
}

std::vector<uni_char> to_unicode(const char * str)
{
    std::vector<uni_char> res;
    for (size_t i = 0; i < strlen(str); i++)
        res.push_back(str[i]);

    return res;
}

std::vector<uni_char> to_unicode(int len, ...)
{
    va_list ap;

    va_start(ap, len);

    std::vector<uni_char> res;
    for (size_t i = 0; i < len; i++)
        res.push_back(va_arg(ap, unsigned int));

    va_end(ap);
    return res;
}

void skip_words(UnicodeStream & str, int count)
{
    for (int i = 0; i < count; i++)
        get_word(str);
}

void skip_lines(UnicodeStream & str, int count)
{
    for (int i = 0; i < count; i++)
        get_line(str);
}

class StreamTestSuite : public CxxTest::TestSuite
{
public:
    void test_utf8_stream()
    {
        Utf8Stream str(read_file("data/UTF-8-test.txt"));

        TS_ASSERT_EQUALS(str.position(), 0);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("UTF-8"));
        TS_ASSERT_EQUALS(str.position(), 5);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("decoder"));
        TS_ASSERT_EQUALS(str.position(), 13);
        TS_ASSERT_EQUALS(str.skip(11), 11);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("and"));
        TS_ASSERT_EQUALS(str.position(), 28);
        str.push('a'); str.push('n'); str.push('d');
        TS_ASSERT_EQUALS(str.position(), 25);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("dna"));
        TS_ASSERT_EQUALS(str.position(), 28);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("stress"));
        TS_ASSERT_EQUALS(str.position(), 35);
        skip_lines(str, 60 - 9);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("Here"));
        skip_lines(str, 4);
        TS_ASSERT_EQUALS(get_word(str), to_unicode("You"));
        skip_words(str, 6);
        TS_ASSERT_EQUALS(get_word(str), to_unicode(7, '"', 0x03BA, 0x1F79, 0x03C3, 0x03BC, 0x03B5, '"'));
    }
};
