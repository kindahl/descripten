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
#include <gc_cpp.h>
#include <limits>
#include "runtime/value.hh"

class ValueSuite : public CxxTest::TestSuite
{
public:
    void test_all_types()
    {
        Gc::instance().init();

        EsValue val;
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NOTHING);
        TS_ASSERT(val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_bool(true);
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_BOOLEAN);
        TS_ASSERT_EQUALS(val.as_boolean(), true);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_bool(false);
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_BOOLEAN);
        TS_ASSERT_EQUALS(val.as_boolean(), false);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_str(EsString::create_from_utf8("some text"));
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_STRING);
        TS_ASSERT(val.as_string()->equals(EsString::create_from_utf8("some text")));
        TS_ASSERT_EQUALS(val.as_string()->length(), 9);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(0.42);
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT_EQUALS(val.as_number(), 0.42);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(-0.123456789);
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT_EQUALS(val.as_number(), -0.123456789);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::quiet_NaN());
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT(std::isnan(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_obj(reinterpret_cast<EsObject *>(0xdeadbeef));
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_OBJECT);
        TS_ASSERT_EQUALS(val.as_object(), reinterpret_cast<EsObject *>(0xdeadbeef));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(val.is_object());

        EsValue val2 = EsValue::undefined;
        TS_ASSERT_EQUALS(val2.type(), EsValue::TYPE_UNDEFINED);
        TS_ASSERT(!val2.is_nothing());
        TS_ASSERT(val2.is_undefined());
        TS_ASSERT(!val2.is_null());
        TS_ASSERT(!val2.is_boolean());
        TS_ASSERT(!val2.is_number());
        TS_ASSERT(!val2.is_string());
        TS_ASSERT(!val2.is_object());

        val2 = EsValue::null;
        TS_ASSERT_EQUALS(val2.type(), EsValue::TYPE_NULL);
        TS_ASSERT(!val2.is_nothing());
        TS_ASSERT(!val2.is_undefined());
        TS_ASSERT(val2.is_null());
        TS_ASSERT(!val2.is_boolean());
        TS_ASSERT(!val2.is_number());
        TS_ASSERT(!val2.is_string());
        TS_ASSERT(!val2.is_object());

        val2 = EsValue::nothing;
        TS_ASSERT_EQUALS(val2.type(), EsValue::TYPE_NOTHING);
        TS_ASSERT(val2.is_nothing());
        TS_ASSERT(!val2.is_undefined());
        TS_ASSERT(!val2.is_null());
        TS_ASSERT(!val2.is_boolean());
        TS_ASSERT(!val2.is_number());
        TS_ASSERT(!val2.is_string());
        TS_ASSERT(!val2.is_object());
    }

    void test_number()
    {
        Gc::instance().init();

        EsValue val;

        val.set_num(0.0);
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT_EQUALS(val.as_number(), 0.0);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::quiet_NaN());
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT(std::isnan(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::signaling_NaN());
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT(std::isnan(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT(std::isinf(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(-std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(val.type(), EsValue::TYPE_NUMBER);
        TS_ASSERT(std::isinf(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());
    }
};
