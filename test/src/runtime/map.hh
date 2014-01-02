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
#include "runtime/map.hh"
#include "runtime/property.hh"
#include "../gc.hh"

class MapTestSuite : public CxxTest::TestSuite
{
public:
    void test_add_non_mapped()
    {
        Gc::instance().init();

        EsMap map0(NULL);
        TS_ASSERT(!map0.lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT_EQUALS(map0.size(), 0);
        TS_ASSERT_EQUALS(map0.props_.size(), 0);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop0 = map0.lookup(EsPropertyKey::from_str(String("0")));
        TS_ASSERT(prop0);
        TS_ASSERT_EQUALS(map0.size(), 1);
        TS_ASSERT_EQUALS(map0.props_.size(), 1);

        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop1 = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1);
        TS_ASSERT(prop1 != prop0);
        TS_ASSERT_EQUALS(map0.size(), 2);
        TS_ASSERT_EQUALS(map0.props_.size(), 2);

        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop2 = map0.lookup(EsPropertyKey::from_str(String("2")));
        TS_ASSERT(prop2);
        TS_ASSERT(prop2 != prop0);
        TS_ASSERT(prop2 != prop1);
        TS_ASSERT_EQUALS(map0.size(), 3);
        TS_ASSERT_EQUALS(map0.props_.size(), 3);
    }

    void test_add_mapped()
    {
        Gc::instance().init();

        EsMap map0(NULL);
        TS_ASSERT(!map0.map_);
        for (size_t i = 0; i < EsMap::MAX_NUM_NON_MAPPED; i++)
        {
            TS_ASSERT(!map0.map_);
            map0.add(EsPropertyKey::from_str(String(lexical_cast<const char *>(i))), EsProperty(false, false, false, Maybe<EsValue>()));
        }
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED);

        map0.add(EsPropertyKey::from_str(String("_0")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0.map_);
        EsPropertyReference prop0 = map0.lookup(EsPropertyKey::from_str(String("_0")));
        TS_ASSERT(prop0);
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 1);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 1);

        map0.add(EsPropertyKey::from_str(String("_1")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop1 = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1);
        TS_ASSERT(prop1 != prop0);
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 2);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 2);

        map0.add(EsPropertyKey::from_str(String("_2")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop2 = map0.lookup(EsPropertyKey::from_str(String("_2")));
        TS_ASSERT(prop2);
        TS_ASSERT(prop2 != prop0);
        TS_ASSERT(prop2 != prop1);
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 3);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 3);

        for (size_t i = 0; i < EsMap::MAX_NUM_NON_MAPPED; i++)
        {
            TS_ASSERT(map0.lookup(EsPropertyKey::from_str(String(lexical_cast<const char *>(i)))));
        }
    }

    void test_remove_non_mapped()
    {
        Gc::instance().init();

        EsMap map0(NULL);
        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        EsPropertyReference prop1 = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1);
        TS_ASSERT_EQUALS(map0.size(), 3);
        TS_ASSERT_EQUALS(map0.props_.size(), 3);

        map0.remove(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(!map0.lookup(EsPropertyKey::from_str(String("1"))));
        TS_ASSERT_EQUALS(map0.size(), 2);
        TS_ASSERT_EQUALS(map0.props_.size(), 3);

        // Add new property "3".
        map0.add(EsPropertyKey::from_str(String("3")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(!map0.lookup(EsPropertyKey::from_str(String("1"))));
        EsPropertyReference prop3 = map0.lookup(EsPropertyKey::from_str(String("3")));
        TS_ASSERT(prop3);
        TS_ASSERT_EQUALS(map0.size(), 3);       // Re-using slot.
        TS_ASSERT_EQUALS(map0.props_.size(), 3);
        TS_ASSERT_EQUALS(prop1, prop3);         // Re-using slot.

        // Re-add "1".
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop1_ = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1);
        TS_ASSERT_EQUALS(map0.size(), 4);
        TS_ASSERT_EQUALS(map0.props_.size(), 4);
        TS_ASSERT_EQUALS(prop1, prop3);         // Re-used slot.
        TS_ASSERT(prop1_ != prop1);
        TS_ASSERT(prop1_ != prop3);
    }

    void test_remove_mapped()
    {
        Gc::instance().init();

        EsMap map0(NULL);
        TS_ASSERT(!map0.map_);
        for (size_t i = 0; i < EsMap::MAX_NUM_NON_MAPPED; i++)
        {
            TS_ASSERT(!map0.map_);
            map0.add(EsPropertyKey::from_str(String(lexical_cast<const char *>(i))),
                     EsProperty(false, false, false, Maybe<EsValue>()));
        }
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED);

        map0.add(EsPropertyKey::from_str(String("_0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("_1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("_2")), EsProperty(false, false, false, Maybe<EsValue>()));

        EsPropertyReference prop1 = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1);
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 3);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 3);

        map0.remove(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(!map0.lookup(EsPropertyKey::from_str(String("_1"))));
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 2);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 3);

        // Add new property "_3".
        map0.add(EsPropertyKey::from_str(String("_3")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(!map0.lookup(EsPropertyKey::from_str(String("_1"))));
        EsPropertyReference prop3 = map0.lookup(EsPropertyKey::from_str(String("_3")));
        TS_ASSERT(prop3);
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 3);   // Re-using slot.
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 3);
        TS_ASSERT_EQUALS(prop1, prop3);         // Re-using slot.

        // Re-add "_1".
        map0.add(EsPropertyKey::from_str(String("_1")), EsProperty(false, false, false, Maybe<EsValue>()));
        EsPropertyReference prop1_ = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1);
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED + 4);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED + 4);
        TS_ASSERT_EQUALS(prop1, prop3);         // Re-used slot.
        TS_ASSERT(prop1_ != prop1);
        TS_ASSERT(prop1_ != prop3);
    }

    void test_compare_ordered()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        TS_ASSERT_EQUALS(map0, map1);
        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);
        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT_EQUALS(map0, map1);

        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);

        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT_EQUALS(map0, map1);
    }

    void test_compare_unordered()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));

        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);

        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);
    }

    void test_compare_deleted_last()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.remove(EsPropertyKey::from_str(String("0")));
        TS_ASSERT_EQUALS(map0, map1);

        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);

        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT_EQUALS(map0, map1);
    }

    void test_compare_deleted_middle()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));

        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        TS_ASSERT(map0 != map1);
        map0.remove(EsPropertyKey::from_str(String("1")));

        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));
        //TS_ASSERT_EQUALS(map0, map1);
        TS_ASSERT(map0 != map1);
    }

    void test_compare_deleted_middle_unordered()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 != map1);
        map0.remove(EsPropertyKey::from_str(String("1")));
        //TS_ASSERT_EQUALS(map0, map1);
        TS_ASSERT(map0 != map1);
    }

    void test_compare_delete_1()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 == map1);
        map0.remove(EsPropertyKey::from_str(String("0")));
        TS_ASSERT(map0 != map1);
    }

    void test_compare_delete_2()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 == map1);
        map1.remove(EsPropertyKey::from_str(String("0")));
        TS_ASSERT(map0 != map1);
    }

    void test_compare_delete_3()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 == map1);
        map0.remove(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(map0 != map1);
    }

    void test_compare_delete_4()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 == map1);
        map1.remove(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(map0 != map1);
    }

    void test_compare_delete_5()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 == map1);
        map0.remove(EsPropertyKey::from_str(String("2")));
        TS_ASSERT(map0 != map1);
    }

    void test_compare_delete_6()
    {
        Gc::instance().init();
        EsMap map0(NULL), map1(NULL);

        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        map1.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map1.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        TS_ASSERT(map0 == map1);
        map1.remove(EsPropertyKey::from_str(String("2")));
        TS_ASSERT(map0 != map1);
    }

    void test_modify_non_mapped()
    {
        Gc::instance().init();

        EsMap map0(NULL);
        map0.add(EsPropertyKey::from_str(String("0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("2")), EsProperty(false, false, false, Maybe<EsValue>()));

        EsPropertyReference prop1 = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("1"))), 1);
        TS_ASSERT_EQUALS(prop1->is_enumerable(), false);
        prop1->set_enumerable(true);
        TS_ASSERT_EQUALS(prop1->is_enumerable(), true);

        EsPropertyReference prop1_ = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1_);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("1"))), 1);
        TS_ASSERT_EQUALS(prop1_->is_enumerable(), true);

        map0.add(EsPropertyKey::from_str(String("3")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("4")), EsProperty(false, false, false, Maybe<EsValue>()));

        prop1_ = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1_);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("1"))), 1);
        TS_ASSERT_EQUALS(prop1_->is_enumerable(), true);

        map0.remove(EsPropertyKey::from_str(String("0")));

        prop1_ = map0.lookup(EsPropertyKey::from_str(String("1")));
        TS_ASSERT(prop1_);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("1"))), 1);
        TS_ASSERT_EQUALS(prop1_->is_enumerable(), true);
    }

    void test_modify_mapped()
    {
        Gc::instance().init();

        EsMap map0(NULL);
        TS_ASSERT(!map0.map_);
        for (size_t i = 0; i < EsMap::MAX_NUM_NON_MAPPED; i++)
        {
            TS_ASSERT(!map0.map_);
            map0.add(EsPropertyKey::from_str(String(lexical_cast<const char *>(i))),
                     EsProperty(false, false, false, Maybe<EsValue>()));
        }
        TS_ASSERT_EQUALS(map0.size(), EsMap::MAX_NUM_NON_MAPPED);
        TS_ASSERT_EQUALS(map0.props_.size(), EsMap::MAX_NUM_NON_MAPPED);

        map0.add(EsPropertyKey::from_str(String("_0")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("_1")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("_2")), EsProperty(false, false, false, Maybe<EsValue>()));

        EsPropertyReference prop1 = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("_1"))), EsMap::MAX_NUM_NON_MAPPED + 1);
        TS_ASSERT_EQUALS(prop1->is_enumerable(), false);
        prop1->set_enumerable(true);
        TS_ASSERT_EQUALS(prop1->is_enumerable(), true);

        EsPropertyReference prop1_ = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1_);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("_1"))), EsMap::MAX_NUM_NON_MAPPED + 1);
        TS_ASSERT_EQUALS(prop1_->is_enumerable(), true);

        map0.add(EsPropertyKey::from_str(String("_3")), EsProperty(false, false, false, Maybe<EsValue>()));
        map0.add(EsPropertyKey::from_str(String("_4")), EsProperty(false, false, false, Maybe<EsValue>()));

        prop1_ = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1_);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("_1"))), EsMap::MAX_NUM_NON_MAPPED + 1);
        TS_ASSERT_EQUALS(prop1_->is_enumerable(), true);

        map0.remove(EsPropertyKey::from_str(String("_0")));

        prop1_ = map0.lookup(EsPropertyKey::from_str(String("_1")));
        TS_ASSERT(prop1_);
        TS_ASSERT_EQUALS(map0.slot(EsPropertyKey::from_str(String("_1"))), EsMap::MAX_NUM_NON_MAPPED + 1);
        TS_ASSERT_EQUALS(prop1_->is_enumerable(), true);
    }
};
