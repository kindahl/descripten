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
#include "runtime/value_boxed_base.hh"

namespace
{

class TestValue : public EsValueBoxedBase
{
private:
    TestValue(Type type)
        : EsValueBoxedBase(type) {}

public:
    static TestValue nothing()
    {
        return TestValue(TYPE_NOTHING);
    }

    static TestValue undefined()
    {
        return TestValue(TYPE_UNDEFINED);
    }

    static TestValue null()
    {
        return TestValue(TYPE_NULL);
    }
};

}

class ValueBoxedBaseTestSuite : public CxxTest::TestSuite
{
public:
    void test_all_types()
    {
        Gc::instance().init();

        EsValueBoxedBase val;
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NOTHING);
        TS_ASSERT(val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_bool(true);
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_BOOLEAN);
        TS_ASSERT_EQUALS(val.as_boolean(), true);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_bool(false);
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_BOOLEAN);
        TS_ASSERT_EQUALS(val.as_boolean(), false);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_str(String("some text"));
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_STRING);
        TS_ASSERT_EQUALS(val.as_string(), String("some text"));
        TS_ASSERT_EQUALS(val.as_string().length(), 9);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(0.42);
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT_EQUALS(val.as_number(), 0.42);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(-0.123456789);
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT_EQUALS(val.as_number(), -0.123456789);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::quiet_NaN());
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT(std::isnan(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_obj(reinterpret_cast<EsObject *>(0xdeadbeef));
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_OBJECT);
        TS_ASSERT_EQUALS(val.as_object(), reinterpret_cast<EsObject *>(0xdeadbeef));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(!val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(val.is_object());

        TestValue val2 = TestValue::undefined();
        TS_ASSERT_EQUALS(val2.type(), EsValueBoxedBase::TYPE_UNDEFINED);
        TS_ASSERT(!val2.is_nothing());
        TS_ASSERT(val2.is_undefined());
        TS_ASSERT(!val2.is_null());
        TS_ASSERT(!val2.is_boolean());
        TS_ASSERT(!val2.is_number());
        TS_ASSERT(!val2.is_string());
        TS_ASSERT(!val2.is_object());

        val2 = TestValue::null();
        TS_ASSERT_EQUALS(val2.type(), EsValueBoxedBase::TYPE_NULL);
        TS_ASSERT(!val2.is_nothing());
        TS_ASSERT(!val2.is_undefined());
        TS_ASSERT(val2.is_null());
        TS_ASSERT(!val2.is_boolean());
        TS_ASSERT(!val2.is_number());
        TS_ASSERT(!val2.is_string());
        TS_ASSERT(!val2.is_object());

        val2 = TestValue::nothing();
        TS_ASSERT_EQUALS(val2.type(), EsValueBoxedBase::TYPE_NOTHING);
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

        EsValueBoxedBase val;

        val.set_num(0.0);
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT_EQUALS(val.as_number(), 0.0);
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::quiet_NaN());
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT(std::isnan(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::signaling_NaN());
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT(std::isnan(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
        TS_ASSERT(std::isinf(val.as_number()));
        TS_ASSERT(!val.is_nothing());
        TS_ASSERT(!val.is_undefined());
        TS_ASSERT(!val.is_null());
        TS_ASSERT(!val.is_boolean());
        TS_ASSERT(val.is_number());
        TS_ASSERT(!val.is_string());
        TS_ASSERT(!val.is_object());

        val.set_num(-std::numeric_limits<double>::infinity());
        TS_ASSERT_EQUALS(val.type(), EsValueBoxedBase::TYPE_NUMBER);
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
