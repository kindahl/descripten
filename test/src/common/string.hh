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
#include "common/lexical.hh"
#include "common/string.hh"
#include "../gc.hh"

class StringTestSuite : public CxxTest::TestSuite
{
public:
    void test_es_strtou()
    {
        Gc::instance().init();

        String str00;
        String str01(" ");
        String str02(" 1");
        String str03("-1");
        String str04("+1");
        String str05("1");
        String str06("1 ");
        String str07("123");
        String str08("a123");
        String str09("123a");

        const uni_char * ptr = str00.data(), * end = str00.data();
        TS_ASSERT(std::isnan(es_strtou(ptr, NULL, 10)));
        TS_ASSERT(std::isnan(es_strtou(ptr, &end, 10)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str01.data(); end = str01.data();
        TS_ASSERT(std::isnan(es_strtou(ptr, NULL, 10)));
        TS_ASSERT(std::isnan(es_strtou(ptr, &end, 10)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str02.data(); end = str02.data();
        TS_ASSERT(std::isnan(es_strtou(ptr, NULL, 10)));
        TS_ASSERT(std::isnan(es_strtou(ptr, &end, 10)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str03.data(); end = str03.data();
        TS_ASSERT(std::isnan(es_strtou(ptr, NULL, 10)));
        TS_ASSERT(std::isnan(es_strtou(ptr, &end, 10)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str04.data(); end = str04.data();
        TS_ASSERT(std::isnan(es_strtou(ptr, NULL, 10)));
        TS_ASSERT(std::isnan(es_strtou(ptr, &end, 10)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str05.data(); end = str05.data();
        TS_ASSERT_EQUALS(es_strtou(ptr, NULL, 10), 1.0);
        TS_ASSERT_EQUALS(es_strtou(ptr, &end, 10), 1.0);
        TS_ASSERT_EQUALS(end - ptr, 1);

        ptr = str06.data(); end = str06.data();
        TS_ASSERT_EQUALS(es_strtou(ptr, NULL, 10), 1.0);
        TS_ASSERT_EQUALS(es_strtou(ptr, &end, 10), 1.0);
        TS_ASSERT_EQUALS(end - ptr, 1);

        ptr = str07.data(); end = str07.data();
        TS_ASSERT_EQUALS(es_strtou(ptr, NULL, 10), 123.0);
        TS_ASSERT_EQUALS(es_strtou(ptr, &end, 10), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 3);

        ptr = str08.data(); end = str08.data();
        TS_ASSERT(std::isnan(es_strtou(ptr, NULL, 10)));
        TS_ASSERT_EQUALS(es_strtou(ptr, NULL, 16), 41251.0);
        TS_ASSERT(std::isnan(es_strtou(ptr, &end, 10)));
        TS_ASSERT_EQUALS(end - ptr, 0);
        TS_ASSERT_EQUALS(es_strtou(ptr, &end, 16), 41251.0);
        TS_ASSERT_EQUALS(end - ptr, 4);

        ptr = str09.data(); end = str09.data();
        TS_ASSERT_EQUALS(es_strtou(ptr, NULL, 10), 123.0);
        TS_ASSERT_EQUALS(es_strtou(ptr, NULL, 16), 4666.0);
        TS_ASSERT_EQUALS(es_strtou(ptr, &end, 10), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 3);
        TS_ASSERT_EQUALS(es_strtou(ptr, &end, 16), 4666.0);
        TS_ASSERT_EQUALS(end - ptr, 4);
    }

    void test_es_strtod()
    {
        Gc::instance().init();

        String str00;
        String str01("a123");
        String str02("a123a");
        String str03("123a");
        String str04("123");
        String str05("   123");
        String str06("   -123");
        String str07("   +123");
        String str08("  - 123");
        String str09("  + 123 ");
        String str10("   123.");
        String str11("   123.0 ");
        String str12("   123.123");
        String str13("Infinity");
        String str14("   Infinity");
        String str15("   +Infinity ");
        String str16("   -Infinity ");
        String str17(" -  Infinity");
        String str18(" +  Infinity");

        const uni_char * ptr = str00.data(), * end = str00.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 0.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 0.0);
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str01.data(); end = str01.data();
        TS_ASSERT(std::isnan(es_strtod(ptr, NULL)));
        TS_ASSERT(std::isnan(es_strtod(ptr, &end)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str02.data(); end = str02.data();
        TS_ASSERT(std::isnan(es_strtod(ptr, NULL)));
        TS_ASSERT(std::isnan(es_strtod(ptr, &end)));
        TS_ASSERT_EQUALS(end - ptr, 0);

        ptr = str03.data(); end = str03.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 3);

        ptr = str04.data(); end = str04.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 3);

        ptr = str05.data(); end = str05.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 6);

        ptr = str06.data(); end = str06.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), -123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), -123.0);
        TS_ASSERT_EQUALS(end - ptr, 7);

        ptr = str07.data(); end = str07.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 7);

        ptr = str08.data(); end = str08.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), -123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), -123.0);
        TS_ASSERT_EQUALS(end - ptr, 7);

        ptr = str09.data(); end = str09.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 7);

        ptr = str10.data(); end = str10.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 7);

        ptr = str11.data(); end = str11.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.0);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.0);
        TS_ASSERT_EQUALS(end - ptr, 8);

        ptr = str12.data(); end = str12.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), 123.123);
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), 123.123);
        TS_ASSERT_EQUALS(end - ptr, 10);

        ptr = str13.data(); end = str13.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(end - ptr, 8);

        ptr = str14.data(); end = str14.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(end - ptr, 11);

        ptr = str15.data(); end = str15.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(end - ptr, 12);

        ptr = str16.data(); end = str16.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), -std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), -std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(end - ptr, 12);

        ptr = str17.data(); end = str17.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), -std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), -std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(end - ptr, 12);

        ptr = str18.data(); end = str18.data();
        TS_ASSERT_EQUALS(es_strtod(ptr, NULL), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(es_strtod(ptr, &end), std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(end - ptr, 12);
    }

    void test_string_contains()
    {
        Gc::instance().init();

        String str1;
        String str2 = String("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT(!str1.contains('a'));
        TS_ASSERT(!str1.contains('l'));
        TS_ASSERT(!str1.contains('z'));
        TS_ASSERT(!str1.contains('\0'));

        TS_ASSERT(str2.contains('a'));
        TS_ASSERT(str2.contains('l'));
        TS_ASSERT(str2.contains('z'));
        TS_ASSERT(!str2.contains('\0'));
    }

    void test_string_take()
    {
        Gc::instance().init();

        String str1;
        String str2 = String("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT_EQUALS(str1.take(0), String(""));
        TS_ASSERT_EQUALS(str1.take(32), String(""));

        TS_ASSERT_EQUALS(str2.take(0), String(""));
        TS_ASSERT_EQUALS(str2.take(1), String("a"));
        TS_ASSERT_EQUALS(str2.take(26), str2);
        TS_ASSERT_EQUALS(str2.take(32), str2);
    }

    void test_string_skip()
    {
        Gc::instance().init();

        String str1;
        String str2 = String("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT_EQUALS(str1.skip(0), String(""));
        TS_ASSERT_EQUALS(str1.skip(32), String(""));

        TS_ASSERT_EQUALS(str2.skip(0), str2);
        TS_ASSERT_EQUALS(str2.skip(1), String("bcdefghijklmnopqrstuvwxyz"));
        TS_ASSERT_EQUALS(str2.skip(26), String(""));
        TS_ASSERT_EQUALS(str2.skip(32), String(""));
    }

    void test_string_substr()
    {
        Gc::instance().init();

        String str1;
        String str2 = String("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT_EQUALS(str1.substr(0, 0), String(""));
        TS_ASSERT_EQUALS(str1.substr(0, 32), String(""));

        TS_ASSERT_EQUALS(str2.substr(0, 26), str2);
        TS_ASSERT_EQUALS(str2.substr(0, 32), str2);
        TS_ASSERT_EQUALS(str2.substr(1, 25), String("bcdefghijklmnopqrstuvwxyz"));
        TS_ASSERT_EQUALS(str2.substr(1, 32), String("bcdefghijklmnopqrstuvwxyz"));
        TS_ASSERT_EQUALS(str2.substr(1, 24), String("bcdefghijklmnopqrstuvwxy"));
        TS_ASSERT_EQUALS(str2.substr(26, 1), String(""));
        TS_ASSERT_EQUALS(str2.substr(25, 1), String("z"));
    }

    void test_string_index_of()
    {
        Gc::instance().init();

        String str1;
        TS_ASSERT_EQUALS(str1.index_of(String("")), -1);
        TS_ASSERT_EQUALS(str1.index_of(String("x")), -1);

        String str2 = String("abcdefghijklmnopqrstuvwxyz");
        TS_ASSERT_EQUALS(str2.index_of(String()), -1);
        TS_ASSERT_EQUALS(str2.index_of(String("x")), 23);
        TS_ASSERT_EQUALS(str2.index_of(String("xp")), -1);
        TS_ASSERT_EQUALS(str2.index_of(String("xy")), 23);
        TS_ASSERT_EQUALS(str2.index_of(String("xyz")), 23);
        TS_ASSERT_EQUALS(str2.index_of(String("xyz_")), -1);
        TS_ASSERT_EQUALS(str2.index_of(String("x"), 22), 23);
        TS_ASSERT_EQUALS(str2.index_of(String("x"), 23), 23);
        TS_ASSERT_EQUALS(str2.index_of(String("x"), 24), -1);
        TS_ASSERT_EQUALS(str2.index_of(String("abc")), 0);
        TS_ASSERT_EQUALS(str2.index_of(String("abc"),1), -1);

        String str3 = String("abcabcabcabcabc");
        TS_ASSERT_EQUALS(str3.index_of(String("")), -1);
        TS_ASSERT_EQUALS(str3.index_of(String("x")), -1);
        TS_ASSERT_EQUALS(str3.index_of(String("abc")), 0);
        TS_ASSERT_EQUALS(str3.index_of(String("abc"),1), 3);
        TS_ASSERT_EQUALS(str3.index_of(String("abc"),2), 3);
        TS_ASSERT_EQUALS(str3.index_of(String("abc"),3), 3);
        TS_ASSERT_EQUALS(str3.index_of(String("abc"),11), 12);
        TS_ASSERT_EQUALS(str3.index_of(String("abc"),12), 12);
        TS_ASSERT_EQUALS(str3.index_of(String("abc"),13), -1);
    }

    void test_string_last_index_of()
    {
        Gc::instance().init();

        String str1;
        TS_ASSERT_EQUALS(str1.last_index_of(String()), -1);
        TS_ASSERT_EQUALS(str1.last_index_of(String("x")), -1);

        String str2 = String("abcdefghijklmnopqrstuvwxyz");
        TS_ASSERT_EQUALS(str2.last_index_of(String("")), -1);
        TS_ASSERT_EQUALS(str2.last_index_of(String("x")), 23);
        TS_ASSERT_EQUALS(str2.last_index_of(String("xp")), -1);
        TS_ASSERT_EQUALS(str2.last_index_of(String("xy")), 23);
        TS_ASSERT_EQUALS(str2.last_index_of(String("xyz")), 23);
        TS_ASSERT_EQUALS(str2.last_index_of(String("xyz_")), -1);
        TS_ASSERT_EQUALS(str2.last_index_of(String("x"), 22), 23);
        TS_ASSERT_EQUALS(str2.last_index_of(String("x"), 23), 23);
        TS_ASSERT_EQUALS(str2.last_index_of(String("x"), 24), -1);
        TS_ASSERT_EQUALS(str2.last_index_of(String("abc")), 0);
        TS_ASSERT_EQUALS(str2.last_index_of(String("abc"),1), -1);

        String str3 = String("abcabcabcabcabc");
        TS_ASSERT_EQUALS(str3.last_index_of(String()), -1);
        TS_ASSERT_EQUALS(str3.last_index_of(String("x")), -1);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc")), 12);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc"),1), 12);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc"),2), 12);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc"),3), 12);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc"),11), 12);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc"),12), 12);
        TS_ASSERT_EQUALS(str3.last_index_of(String("abc"),13), -1);
    }
};
