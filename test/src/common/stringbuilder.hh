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
#include <limits>
#include "common/stringbuilder.hh"
#include "../gc.hh"

class StringBuilderTestSuite : public CxxTest::TestSuite
{
public:
    void test_string_builder_grow()
    {
        Gc::instance().init();

        StringBuilder sb1, sb2;

        TS_ASSERT_EQUALS(sb1.allocated(), 32);
        TS_ASSERT_EQUALS(sb1.length(), 0);
        TS_ASSERT_EQUALS(sb1.string(), String());

        sb1.append("0123456789012345678901234567890123456789");
        TS_ASSERT_EQUALS(sb1.allocated(), 64);
        TS_ASSERT_EQUALS(sb1.length(), 40);
        TS_ASSERT_EQUALS(sb1.string(), String("0123456789012345678901234567890123456789"));

        TS_ASSERT_EQUALS(sb2.allocated(), 32);
        TS_ASSERT_EQUALS(sb2.length(), 0);
        TS_ASSERT_EQUALS(sb2.string(), String());

        sb2.append("01234567890123456789012345678901234567890123456789"
                   "01234567890123456789012345678901234567890123456789"
                   "01234567890123456789012345678901234567890123456789");
        TS_ASSERT_EQUALS(sb2.allocated(), 256);
        TS_ASSERT_EQUALS(sb2.length(), 150);
        TS_ASSERT_EQUALS(sb2.string(), String("01234567890123456789012345678901234567890123456789"
                                              "01234567890123456789012345678901234567890123456789"
                                              "01234567890123456789012345678901234567890123456789"));
    }

    void test_string_builder_append()
    {
        Gc::instance().init();

        StringBuilder sb;

        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 0);
        TS_ASSERT_EQUALS(sb.string(), String());

        sb.append("");
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 0);
        TS_ASSERT_EQUALS(sb.string(), String());

        sb.append("abc");
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 3);
        TS_ASSERT_EQUALS(sb.string(), String("abc"));

        sb.append("def");
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 6);
        TS_ASSERT_EQUALS(sb.string(), String("abcdef"));

        String str = String("ghi");
        sb.append(str.data());
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 9);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghi"));

        sb.append(str.data(), 2);
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 11);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghigh"));

        sb.append("jkl", 2);
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 13);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghighjk"));

        sb.append("jkl", 0);
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 13);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghighjk"));

        sb.append("0123456789012345678", 19);
        TS_ASSERT_EQUALS(sb.allocated(), 32);
        TS_ASSERT_EQUALS(sb.length(), 32);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghighjk0123456789012345678"));

        sb.append("0", 1);
        TS_ASSERT_EQUALS(sb.allocated(), 64);
        TS_ASSERT_EQUALS(sb.length(), 33);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghighjk01234567890123456780"));

        sb.append('q');
        TS_ASSERT_EQUALS(sb.allocated(), 64);
        TS_ASSERT_EQUALS(sb.length(), 34);
        TS_ASSERT_EQUALS(sb.string(), String("abcdefghighjk01234567890123456780q"));
    }

    void test_string_builder_sprintf()
    {
        // Radix.
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%db", 0), String("a0b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%db", -128), String("a-128b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%db", 128), String("a128b"));

        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%ib", 0), String("a0b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%ib", -128), String("a-128b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%ib", 128), String("a128b"));

        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%ob", 0), String("a0b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%ob", 128), String("a200b"));

        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%xb", 0), String("a0b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%xb", 128), String("a80b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%xb", 255), String("affb"));

        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%Xb", 0), String("a0b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%Xb", 128), String("a80b"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%Xb", 255), String("aFFb"));
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%.6Xb", 255), String("a0000FFb"));

        // Size.
        int len1 = 0, len2 = 0, len3 = 0;
        TS_ASSERT_EQUALS(StringBuilder::sprintf("%na%nb01234%n", &len1, &len2, &len3), String("ab01234"));
        TS_ASSERT_EQUALS(len1, 0);
        TS_ASSERT_EQUALS(len2, 1);
        TS_ASSERT_EQUALS(len3, 7);

        // String.
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%sb", "foo"), String("afoob"));

        // Percent.
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%%%d%%b", 42), String("a%42%b"));

        // Character.
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%cb", 'c'), String("acb"));

        // Unicode string.
        String str = String("foo");
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%Sb", str.data()), String("afoob"));

        // Unicode character.
        TS_ASSERT_EQUALS(StringBuilder::sprintf("a%Cb", str.data()[1]), String("aob"));
    }
};
