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
#include "runtime/algorithm.hh"
#include "runtime/property_array.hh"

class PropertyArrayTestSuite : public CxxTest::TestSuite
{
public:
    void test_compact()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 0);

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(false);
        }

        array.set(3, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(3.0))));
        TS_ASSERT_EQUALS(array.count(), 1);
        TS_ASSERT(array.get(0) == NULL);
        TS_ASSERT(array.get(1) == NULL);
        TS_ASSERT(array.get(2) == NULL);
        TS_ASSERT(array.get(3) != NULL);
        TS_ASSERT(algorithm::same_value(array.get(3)->value_or_undefined(), EsValue::from_num(3.0)));

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT_EQUALS(entry.first, 3);
            TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(3.0)));
        }

        array.set(5, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(5.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 2);
        TS_ASSERT(array.get(5) != NULL);
        TS_ASSERT(algorithm::same_value(array.get(5)->value_or_undefined(), EsValue::from_num(5.0)));

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(entry.first == 3 || entry.first == 5);

            if (entry.first == 3)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(3.0)));
            if (entry.first == 5)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(5.0)));
        }

        // Plug a hole.
        array.set(2, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(2.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 3);
        TS_ASSERT(array.get(2) != NULL);
        TS_ASSERT(algorithm::same_value(array.get(2)->value_or_undefined(), EsValue::from_num(2.0)));

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(entry.first == 2 || entry.first == 3 || entry.first == 5);

            if (entry.first == 2)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(2.0)));
            if (entry.first == 3)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(3.0)));
            if (entry.first == 5)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(5.0)));
        }
    }

    void test_compact_with_incremental_add()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 0);

        for (int64_t i = 0; i < 16; i++)
        {
            array.set(i, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(static_cast<double>(i)))));
            TS_ASSERT(array.is_compact());
            TS_ASSERT_EQUALS(array.count(), i + 1);
        }

        array.set(16, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(16.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 17);

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(static_cast<double>(entry.first))));
        }
    }

    void test_compact_remove()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 0);

        for (int64_t i = 0; i < 32; i++)
        {
            array.set(i, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(static_cast<double>(i)))));
            TS_ASSERT(array.is_compact());
            TS_ASSERT_EQUALS(array.count(), i + 1);
        }

        array.remove(3);
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 31);

        array.remove(15);
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 30);

        array.remove(7);
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 29);

        array.remove(0);
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 28);

        array.remove(1);
        array.remove(2);
        array.remove(4);
        array.remove(5);
        array.remove(6);
        array.remove(8);
        array.remove(9);
        array.remove(10);
        array.remove(11);
        array.remove(12);
        array.remove(13);
        array.remove(14);
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 16);

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(static_cast<double>(entry.first))));
        }

        for (int64_t i = 16; i < 32; i++)
        {
            TS_ASSERT(array.get(i));
            TS_ASSERT(algorithm::same_value(array.get(i)->value_or_undefined(), EsValue::from_num(static_cast<double>(i))));
        }
    }

    void test_compact_remove_high()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 0);

        array.remove(1024);
        TS_ASSERT_EQUALS(array.count(), 0);
    }

    void test_compact_iterator()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 0);

        array.set(3, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(3.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 1);

        array.set(15, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(15.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 2);

        array.set(7, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(7.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 3);

        array.set(0, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(0.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 4);

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(entry.first == 0 || entry.first == 3 || entry.first == 7 || entry.first == 15);

            if (entry.first == 0)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(0.0)));
            if (entry.first == 3)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(3.0)));
            if (entry.first == 7)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(7.0)));
            if (entry.first == 15)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(15.0)));
        }

        for (int64_t i = 0; i < 32; i++)
        {
            if (array.get(i))
            {
                TS_ASSERT(i == 0 || i == 3 || i == 7 || i == 15);
                TS_ASSERT(algorithm::same_value(array.get(i)->value_or_undefined(), EsValue::from_num(static_cast<double>(i))));
            }
        }
    }

    void test_compact_remove_double()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 0);

        array.set(4, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(4.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 1);

        array.remove(4);
        TS_ASSERT_EQUALS(array.count(), 0);

        array.remove(4);
        TS_ASSERT_EQUALS(array.count(), 0);

        array.set(4, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(4.0))));
        TS_ASSERT_EQUALS(array.count(), 1);
    }

    void test_switch_to_sparse_from_empty()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT_EQUALS(array.count(), 0);

        array.set(1024, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(1024.0))));
        TS_ASSERT(!array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 1);
        TS_ASSERT(array.get(0) == NULL);
        TS_ASSERT(array.get(1023) == NULL);
        TS_ASSERT(array.get(1024) != NULL);
        TS_ASSERT(algorithm::same_value(array.get(1024)->value_or_undefined(), EsValue::from_num(1024.0)));

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT_EQUALS(entry.first, 1024);
            TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(1024.0)));
        }
    }

    void test_switch_to_sparse_from_minimal()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT_EQUALS(array.count(), 0);

        array.set(0, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(0.0))));
        TS_ASSERT(array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 1);
        TS_ASSERT(array.get(0) != NULL);
        TS_ASSERT(algorithm::same_value(array.get(0)->value_or_undefined(), EsValue::from_num(0.0)));

        array.set(1024, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(1024.0))));
        TS_ASSERT(!array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 2);
        TS_ASSERT(array.get(0) != NULL);
        TS_ASSERT(array.get(1024) != NULL);
        TS_ASSERT(algorithm::same_value(array.get(0)->value_or_undefined(), EsValue::from_num(0.0)));
        TS_ASSERT(algorithm::same_value(array.get(1024)->value_or_undefined(), EsValue::from_num(1024.0)));

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(entry.first == 0 || entry.first == 1024);

            if (entry.first == 0)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(0.0)));
            if (entry.first == 1024)
                TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(1024.0)));
        }
    }

    void test_switch_to_sparse_from_non_empty()
    {
        Gc::instance().init();

        EsPropertyArray array;
        TS_ASSERT(array.empty());
        TS_ASSERT_EQUALS(array.count(), 0);

        for (int64_t i = 0; i < 32; i++)
        {
            array.set(i, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(static_cast<double>(i)))));
            TS_ASSERT(array.is_compact());
            TS_ASSERT_EQUALS(array.count(), i + 1);
        }

        array.set(64, EsProperty(false, false, false, Maybe<EsValue>(EsValue::from_num(64.0))));
        TS_ASSERT(!array.is_compact());
        TS_ASSERT_EQUALS(array.count(), 33);

        for (const std::pair<int64_t, EsProperty> &entry : array)
        {
            TS_ASSERT(algorithm::same_value(entry.second.value_or_undefined(), EsValue::from_num(static_cast<double>(entry.first))));
        }

        for (int64_t i = 0; i < 32; i++)
        {
            TS_ASSERT(array.get(i));
            TS_ASSERT(algorithm::same_value(array.get(i)->value_or_undefined(), EsValue::from_num(static_cast<double>(i))));
        }

        for (int64_t i = 32; i < 64; i++)
        {
            TS_ASSERT(!array.get(i));
        }

        TS_ASSERT(array.get(64));
        TS_ASSERT(algorithm::same_value(array.get(64)->value_or_undefined(), EsValue::from_num(64.0)));
    }
};
