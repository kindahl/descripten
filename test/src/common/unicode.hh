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
#include <string.h>
#include "common/unicode.hh"

class UnicodeTestSuite : public CxxTest::TestSuite
{
public:
    void test_utf8_enc()
    {
        byte buf[6];
        byte cmp[6];
        memset(buf, 0, sizeof(buf));
        memset(cmp, 0, sizeof(cmp));

        byte * ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, 0x0000), 1);
        TS_ASSERT_EQUALS(ptr - buf, 1);
        cmp[0] = 0x00; cmp[1] = 0x00; cmp[2] = 0x00; cmp[3] = 0x00; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_1_BYTE_CHAR), 1);
        TS_ASSERT_EQUALS(ptr - buf, 1);
        cmp[0] = 0x7f; cmp[1] = 0x00; cmp[2] = 0x00; cmp[3] = 0x00; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_1_BYTE_CHAR + 1), 2);
        TS_ASSERT_EQUALS(ptr - buf, 2);
        cmp[0] = 0xc2; cmp[1] = 0x80; cmp[2] = 0x00; cmp[3] = 0x00; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_2_BYTE_CHAR), 2);
        TS_ASSERT_EQUALS(ptr - buf, 2);
        cmp[0] = 0xdf; cmp[1] = 0xbf; cmp[2] = 0x00; cmp[3] = 0x00; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_2_BYTE_CHAR + 1), 3);
        TS_ASSERT_EQUALS(ptr - buf, 3);
        cmp[0] = 0xe0; cmp[1] = 0xa0; cmp[2] = 0x80; cmp[3] = 0x00; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_3_BYTE_CHAR), 3);
        TS_ASSERT_EQUALS(ptr - buf, 3);
        cmp[0] = 0xef; cmp[1] = 0xbf; cmp[2] = 0xbf; cmp[3] = 0x00; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_3_BYTE_CHAR + 1), 4);
        TS_ASSERT_EQUALS(ptr - buf, 4);
        cmp[0] = 0xf0; cmp[1] = 0x90; cmp[2] = 0x80; cmp[3] = 0x80; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_4_BYTE_CHAR), 4);
        TS_ASSERT_EQUALS(ptr - buf, 4);
        cmp[0] = 0xf7; cmp[1] = 0xbf; cmp[2] = 0xbf; cmp[3] = 0xbf; cmp[4] = 0x00; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_4_BYTE_CHAR + 1), 5);
        TS_ASSERT_EQUALS(ptr - buf, 5);
        cmp[0] = 0xf8; cmp[1] = 0x88; cmp[2] = 0x80; cmp[3] = 0x80; cmp[4] = 0x80; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_5_BYTE_CHAR), 5);
        TS_ASSERT_EQUALS(ptr - buf, 5);
        cmp[0] = 0xfb; cmp[1] = 0xbf; cmp[2] = 0xbf; cmp[3] = 0xbf; cmp[4] = 0xbf; cmp[5] = 0x00;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_5_BYTE_CHAR + 1), 6);
        TS_ASSERT_EQUALS(ptr - buf, 6);
        cmp[0] = 0xfc; cmp[1] = 0x84; cmp[2] = 0x80; cmp[3] = 0x80; cmp[4] = 0x80; cmp[5] = 0x80;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);

        ptr = buf;
        TS_ASSERT_EQUALS(utf8_enc(ptr, UTF8_MAX_6_BYTE_CHAR), 6);
        TS_ASSERT_EQUALS(ptr - buf, 6);
        cmp[0] = 0xfd; cmp[1] = 0xbf; cmp[2] = 0xbf; cmp[3] = 0xbf; cmp[4] = 0xbf; cmp[5] = 0xbf;
        TS_ASSERT_SAME_DATA(buf, cmp, 6);
    }

    void test_utf8_dec()
    {
        byte buf[6];
        memset(buf, 0, sizeof(buf));

        const byte * ptr = buf;
        buf[0] = 0x00; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), 0x0000);
        TS_ASSERT_EQUALS(ptr - buf, 1);

        ptr = buf;
        buf[0] = 0x7f; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_1_BYTE_CHAR);
        TS_ASSERT_EQUALS(ptr - buf, 1);

        ptr = buf;
        buf[0] = 0xc2; buf[1] = 0x80; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_1_BYTE_CHAR + 1);
        TS_ASSERT_EQUALS(ptr - buf, 2);

        ptr = buf;
        buf[0] = 0xdf; buf[1] = 0xbf; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_2_BYTE_CHAR);
        TS_ASSERT_EQUALS(ptr - buf, 2);

        ptr = buf;
        buf[0] = 0xe0; buf[1] = 0xa0; buf[2] = 0x80; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_2_BYTE_CHAR + 1);
        TS_ASSERT_EQUALS(ptr - buf, 3);

        ptr = buf;
        buf[0] = 0xef; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_3_BYTE_CHAR);
        TS_ASSERT_EQUALS(ptr - buf, 3);

        ptr = buf;
        buf[0] = 0xf0; buf[1] = 0x90; buf[2] = 0x80; buf[3] = 0x80; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_3_BYTE_CHAR + 1);
        TS_ASSERT_EQUALS(ptr - buf, 4);

        ptr = buf;
        buf[0] = 0xf7; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0xbf; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_4_BYTE_CHAR);
        TS_ASSERT_EQUALS(ptr - buf, 4);

        ptr = buf;
        buf[0] = 0xf8; buf[1] = 0x88; buf[2] = 0x80; buf[3] = 0x80; buf[4] = 0x80; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_4_BYTE_CHAR + 1);
        TS_ASSERT_EQUALS(ptr - buf, 5);

        ptr = buf;
        buf[0] = 0xfb; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0xbf; buf[4] = 0xbf; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_5_BYTE_CHAR);
        TS_ASSERT_EQUALS(ptr - buf, 5);

        ptr = buf;
        buf[0] = 0xfc; buf[1] = 0x84; buf[2] = 0x80; buf[3] = 0x80; buf[4] = 0x80; buf[5] = 0x80;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_5_BYTE_CHAR + 1);
        TS_ASSERT_EQUALS(ptr - buf, 6);

        ptr = buf;
        buf[0] = 0xfd; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0xbf; buf[4] = 0xbf; buf[5] = 0xbf;
        TS_ASSERT_EQUALS(utf8_dec(ptr), UTF8_MAX_6_BYTE_CHAR);
        TS_ASSERT_EQUALS(ptr - buf, 6);
    }

    void test_utf8_len()
    {
        byte buf[7];
        memset(buf, 0, sizeof(buf));

        buf[0] = 0x00; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 0);

        buf[0] = 0x7f; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xc2; buf[1] = 0x80; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xdf; buf[1] = 0xbf; buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xe0; buf[1] = 0xa0; buf[2] = 0x80; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xef; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xf0; buf[1] = 0x90; buf[2] = 0x80; buf[3] = 0x80; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xf7; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0xbf; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xf8; buf[1] = 0x88; buf[2] = 0x80; buf[3] = 0x80; buf[4] = 0x80; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xfb; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0xbf; buf[4] = 0xbf; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xfc; buf[1] = 0x84; buf[2] = 0x80; buf[3] = 0x80; buf[4] = 0x80; buf[5] = 0x80;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);

        buf[0] = 0xfd; buf[1] = 0xbf; buf[2] = 0xbf; buf[3] = 0xbf; buf[4] = 0xbf; buf[5] = 0xbf;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);
    }

    void test_utf8_len_2()
    {
        byte buf[7];
        memset(buf, 0, sizeof(buf));

        buf[0] = 0xc2; buf[1] = 0x80; buf[2] = 0xc2; buf[3] = 0x80; buf[4] = 0x00; buf[5] = 0x00;
        TS_ASSERT_EQUALS(utf8_len(buf), 2);
        TS_ASSERT_EQUALS(utf8_len(buf,1), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,2), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,3), 2);
        TS_ASSERT_EQUALS(utf8_len(buf,4), 2);
        TS_ASSERT_EQUALS(utf8_len(buf,5), 3);
        TS_ASSERT_EQUALS(utf8_len(buf,6), 4);
        TS_ASSERT_EQUALS(utf8_len(buf,7), 5);

        buf[0] = 0xc2; buf[1] = 0x80; buf[2] = 0xc2; buf[3] = 0x80; buf[4] = 0xc2; buf[5] = 0x80;
        TS_ASSERT_EQUALS(utf8_len(buf), 3);
        TS_ASSERT_EQUALS(utf8_len(buf,1), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,2), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,3), 2);
        TS_ASSERT_EQUALS(utf8_len(buf,4), 2);
        TS_ASSERT_EQUALS(utf8_len(buf,5), 3);
        TS_ASSERT_EQUALS(utf8_len(buf,6), 3);
        TS_ASSERT_EQUALS(utf8_len(buf,7), 4);

        buf[0] = 0xc2; buf[1] = 0x80; buf[2] = 0x00; buf[3] = 0xc2; buf[4] = 0x80; buf[5] = 0xc2;
        TS_ASSERT_EQUALS(utf8_len(buf), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,1), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,2), 1);
        TS_ASSERT_EQUALS(utf8_len(buf,3), 2);
        TS_ASSERT_EQUALS(utf8_len(buf,4), 3);
        TS_ASSERT_EQUALS(utf8_len(buf,5), 3);
        TS_ASSERT_EQUALS(utf8_len(buf,6), 4);
        TS_ASSERT_EQUALS(utf8_len(buf,7), 5);
    }

    void test_uni_strlen()
    {
        Gc::instance().init();

        String str1;
        String str2 = String("a");
        String str3 = String("abcdefghijklmnopqrstuvwxyz");

        TS_ASSERT_EQUALS(uni_strlen(NULL), 0);
        TS_ASSERT_EQUALS(uni_strlen(str1.data()), 0);
        TS_ASSERT_EQUALS(uni_strlen(str2.data()), 1);
        TS_ASSERT_EQUALS(uni_strlen(str3.data()), 26);
    }
};
