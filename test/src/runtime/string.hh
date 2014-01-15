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
#include "runtime/string.hh"
#include "../gc.hh"

class StringTestSuite : public CxxTest::TestSuite
{
public:
    void test_string_contains()
    {
        Gc::instance().init();

        const EsString *str1 = EsString::create();
        const EsString *str2 = EsString::create_from_utf8("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT(!str1->contains('a'));
        TS_ASSERT(!str1->contains('l'));
        TS_ASSERT(!str1->contains('z'));
        TS_ASSERT(!str1->contains('\0'));

        TS_ASSERT(str2->contains('a'));
        TS_ASSERT(str2->contains('l'));
        TS_ASSERT(str2->contains('z'));
        TS_ASSERT(!str2->contains('\0'));
    }

    void test_string_take()
    {
        Gc::instance().init();

        const EsString *str1 = EsString::create();
        const EsString *str2 = EsString::create_from_utf8("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT(str1->take(0)->equals(EsString::create()));
        TS_ASSERT(str1->take(32)->equals(EsString::create()));

        TS_ASSERT(str2->take(0)->equals(EsString::create()));
        TS_ASSERT(str2->take(1)->equals(EsString::create_from_utf8("a")));
        TS_ASSERT(str2->take(26)->equals(str2));
        TS_ASSERT(str2->take(32)->equals(str2));
    }

    void test_string_skip()
    {
        Gc::instance().init();

        const EsString *str1 = EsString::create();
        const EsString *str2 = EsString::create_from_utf8("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT(str1->skip(0)->equals(EsString::create()));
        TS_ASSERT(str1->skip(32)->equals(EsString::create()));

        TS_ASSERT(str2->skip(0)->equals(str2));
        TS_ASSERT(str2->skip(1)->equals(EsString::create_from_utf8("bcdefghijklmnopqrstuvwxyz")));
        TS_ASSERT(str2->skip(26)->equals(EsString::create()));
        TS_ASSERT(str2->skip(32)->equals(EsString::create()));
    }

    void test_string_substr()
    {
        Gc::instance().init();

        const EsString *str1 = EsString::create();
        const EsString *str2 = EsString::create_from_utf8("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT(str1->substr(0, 0)->equals(EsString::create()));
        TS_ASSERT(str1->substr(0, 32)->equals(EsString::create()));

        TS_ASSERT(str2->substr(0, 26)->equals(str2));
        TS_ASSERT(str2->substr(0, 32)->equals(str2));
        TS_ASSERT(str2->substr(1, 25)->equals(EsString::create_from_utf8("bcdefghijklmnopqrstuvwxyz")));
        TS_ASSERT(str2->substr(1, 32)->equals(EsString::create_from_utf8("bcdefghijklmnopqrstuvwxyz")));
        TS_ASSERT(str2->substr(1, 24)->equals(EsString::create_from_utf8("bcdefghijklmnopqrstuvwxy")));
        TS_ASSERT(str2->substr(26, 1)->equals(EsString::create()));
        TS_ASSERT(str2->substr(25, 1)->equals(EsString::create_from_utf8("z")));
    }

    void test_string_index_of()
    {
        Gc::instance().init();

        const EsString *str1 = EsString::create();
        TS_ASSERT_EQUALS(str1->index_of(EsString::create()), -1);
        TS_ASSERT_EQUALS(str1->index_of(EsString::create_from_utf8("x")), -1);

        const EsString *str2 = EsString::create_from_utf8("abcdefghijklmnopqrstuvwxyz");
        TS_ASSERT_EQUALS(str2->index_of(EsString::create()), -1);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("x")), 23);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("xp")), -1);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("xy")), 23);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("xyz")), 23);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("xyz_")), -1);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("x"), 22), 23);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("x"), 23), 23);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("x"), 24), -1);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("abc")), 0);
        TS_ASSERT_EQUALS(str2->index_of(EsString::create_from_utf8("abc"),1), -1);

        const EsString *str3 = EsString::create_from_utf8("abcabcabcabcabc");
        TS_ASSERT_EQUALS(str3->index_of(EsString::create()), -1);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("x")), -1);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc")), 0);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc"),1), 3);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc"),2), 3);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc"),3), 3);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc"),11), 12);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc"),12), 12);
        TS_ASSERT_EQUALS(str3->index_of(EsString::create_from_utf8("abc"),13), -1);
    }

    void test_string_last_index_of()
    {
        Gc::instance().init();

        const EsString *str1 = EsString::create();
        TS_ASSERT_EQUALS(str1->last_index_of(EsString::create()), -1);
        TS_ASSERT_EQUALS(str1->last_index_of(EsString::create_from_utf8("x")), -1);

        const EsString *str2 = EsString::create_from_utf8("abcdefghijklmnopqrstuvwxyz");
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create()), -1);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("x")), 23);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("xp")), -1);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("xy")), 23);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("xyz")), 23);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("xyz_")), -1);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("x"), 22), 23);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("x"), 23), 23);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("x"), 24), -1);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("abc")), 0);
        TS_ASSERT_EQUALS(str2->last_index_of(EsString::create_from_utf8("abc"),1), -1);

        const EsString *str3 = EsString::create_from_utf8("abcabcabcabcabc");
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create()), -1);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("x")), -1);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc")), 12);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc"),1), 12);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc"),2), 12);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc"),3), 12);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc"),11), 12);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc"),12), 12);
        TS_ASSERT_EQUALS(str3->last_index_of(EsString::create_from_utf8("abc"),13), -1);
    }
};
