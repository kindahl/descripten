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

#include <cxxtest/TestSuite.h>
#include <gc_cpp.h>
#include "runtime/shape.hh"
#include "../gc.hh"

class ShapeTestSuite : public CxxTest::TestSuite
{
public:
    void test_add()
    {
        Gc::instance().init();
        EsShape::root()->clear_transitions();

        EsShape *shape0 = EsShape::root()->add(EsPropertyKey::from_str(String("0")), 0);
        TS_ASSERT_EQUALS(shape0->parent_, EsShape::root());
        TS_ASSERT_EQUALS(shape0->key_, EsPropertyKey::from_str(String("0")));
        TS_ASSERT_EQUALS(shape0->slot_, 0);
        TS_ASSERT_EQUALS(shape0->depth_, 1);

        EsShape *shape1 = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        TS_ASSERT_EQUALS(shape1->parent_, shape0);
        TS_ASSERT_EQUALS(shape1->key_, EsPropertyKey::from_str(String("1")));
        TS_ASSERT_EQUALS(shape1->slot_, 1);
        TS_ASSERT_EQUALS(shape1->depth_, 2);

        EsShape *shape2 = shape1->add(EsPropertyKey::from_str(String("2")), 2);
        TS_ASSERT_EQUALS(shape2->parent_, shape1);
        TS_ASSERT_EQUALS(shape2->key_, EsPropertyKey::from_str(String("2")));
        TS_ASSERT_EQUALS(shape2->slot_, 2);
        TS_ASSERT_EQUALS(shape2->depth_, 3);

        EsShape *shape3 = shape2->add(EsPropertyKey::from_str(String("3")), 3);
        TS_ASSERT_EQUALS(shape3->parent_, shape2);
        TS_ASSERT_EQUALS(shape3->key_, EsPropertyKey::from_str(String("3")));
        TS_ASSERT_EQUALS(shape3->slot_, 3);
        TS_ASSERT_EQUALS(shape3->depth_, 4);

        EsShape *shape4 = shape3->add(EsPropertyKey::from_str(String("4")), 4);
        TS_ASSERT_EQUALS(shape4->parent_, shape3);
        TS_ASSERT_EQUALS(shape4->key_, EsPropertyKey::from_str(String("4")));
        TS_ASSERT_EQUALS(shape4->slot_, 4);
        TS_ASSERT_EQUALS(shape4->depth_, 5);

        EsShape *shape5 = shape2->add(EsPropertyKey::from_str(String("5")), 5);
        TS_ASSERT_EQUALS(shape5->parent_, shape2);
        TS_ASSERT_EQUALS(shape5->key_, EsPropertyKey::from_str(String("5")));
        TS_ASSERT_EQUALS(shape5->slot_, 5);
        TS_ASSERT_EQUALS(shape5->depth_, 4);

        EsShape *shape6 = shape5->add(EsPropertyKey::from_str(String("6")), 6);
        TS_ASSERT_EQUALS(shape6->parent_, shape5);
        TS_ASSERT_EQUALS(shape6->key_, EsPropertyKey::from_str(String("6")));
        TS_ASSERT_EQUALS(shape6->slot_, 6);
        TS_ASSERT_EQUALS(shape6->depth_, 5);
    }

    void test_remove_middle()
    {
        Gc::instance().init();
        EsShape::root()->clear_transitions();

        EsShape *shape0 = EsShape::root()->add(EsPropertyKey::from_str(String("0")), 0);
        EsShape *shape1 = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        EsShape *shape2 = shape1->add(EsPropertyKey::from_str(String("2")), 2);
        EsShape *shape3 = shape2->add(EsPropertyKey::from_str(String("3")), 3);
        EsShape *shape4 = shape3->add(EsPropertyKey::from_str(String("4")), 4);
        EsShape *shape5 = shape2->add(EsPropertyKey::from_str(String("5")), 5);
        EsShape *shape6 = shape5->add(EsPropertyKey::from_str(String("6")), 6);

        // Branch 0.
        shape4 = shape4->remove(EsPropertyKey::from_str(String("2")));
        TS_ASSERT(shape4);
        TS_ASSERT_EQUALS(shape4->key_, EsPropertyKey::from_str(String("4")));
        TS_ASSERT_EQUALS(shape4->slot_, 4);
        TS_ASSERT_EQUALS(shape4->depth_, 4);

        shape3 = shape4->parent_;
        TS_ASSERT_EQUALS(shape3->key_, EsPropertyKey::from_str(String("3")));
        TS_ASSERT_EQUALS(shape3->slot_, 3);
        TS_ASSERT_EQUALS(shape3->depth_, 3);

        shape1 = shape3->parent_;
        TS_ASSERT_EQUALS(shape1->key_, EsPropertyKey::from_str(String("1")));
        TS_ASSERT_EQUALS(shape1->slot_, 1);
        TS_ASSERT_EQUALS(shape1->depth_, 2);

        shape0 = shape1->parent_;
        TS_ASSERT_EQUALS(shape0->parent_, EsShape::root());
        TS_ASSERT_EQUALS(shape0->key_, EsPropertyKey::from_str(String("0")));
        TS_ASSERT_EQUALS(shape0->slot_, 0);
        TS_ASSERT_EQUALS(shape0->depth_, 1);

        // Branch 1.
        TS_ASSERT_EQUALS(shape6->key_, EsPropertyKey::from_str(String("6")));
        TS_ASSERT_EQUALS(shape6->slot_, 6);
        TS_ASSERT_EQUALS(shape6->depth_, 5);

        shape5 = shape6->parent_;
        TS_ASSERT_EQUALS(shape5->key_, EsPropertyKey::from_str(String("5")));
        TS_ASSERT_EQUALS(shape5->slot_, 5);
        TS_ASSERT_EQUALS(shape5->depth_, 4);

        shape2 = shape5->parent_;
        TS_ASSERT_EQUALS(shape2->key_, EsPropertyKey::from_str(String("2")));
        TS_ASSERT_EQUALS(shape2->slot_, 2);
        TS_ASSERT_EQUALS(shape2->depth_, 3);

        shape1 = shape2->parent_;
        TS_ASSERT_EQUALS(shape1->key_, EsPropertyKey::from_str(String("1")));
        TS_ASSERT_EQUALS(shape1->slot_, 1);
        TS_ASSERT_EQUALS(shape1->depth_, 2);

        shape0 = shape1->parent_;
        TS_ASSERT_EQUALS(shape0->parent_, EsShape::root());
        TS_ASSERT_EQUALS(shape0->key_, EsPropertyKey::from_str(String("0")));
        TS_ASSERT_EQUALS(shape0->slot_, 0);
        TS_ASSERT_EQUALS(shape0->depth_, 1);
    }

    void test_remove_first()
    {
        Gc::instance().init();
        EsShape::root()->clear_transitions();

        EsShape *shape0 = EsShape::root()->add(EsPropertyKey::from_str(String("0")), 0);
        EsShape *shape1 = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        EsShape *shape2 = shape1->add(EsPropertyKey::from_str(String("2")), 2);
        EsShape *shape3 = shape2->add(EsPropertyKey::from_str(String("3")), 3);
        EsShape *shape4 = shape3->add(EsPropertyKey::from_str(String("4")), 4);

        TS_ASSERT_EQUALS(shape0->slot(), 0);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        TS_ASSERT_EQUALS(shape3->slot(), 3);
        TS_ASSERT_EQUALS(shape4->slot(), 4);

        shape4 = shape4->remove(EsPropertyKey::from_str(String("0")));
        TS_ASSERT_EQUALS(shape4->slot(), 4);

        shape3 = shape4->parent();
        TS_ASSERT_EQUALS(shape3->slot(), 3);

        shape2 = shape3->parent();
        TS_ASSERT_EQUALS(shape2->slot(), 2);

        shape1 = shape2->parent();
        TS_ASSERT_EQUALS(shape1->slot(), 1);

        TS_ASSERT_EQUALS(shape1->parent(), EsShape::root());
    }

    void test_lookup()
    {
        Gc::instance().init();
        EsShape::root()->clear_transitions();

        EsShape *shape0 = EsShape::root()->add(EsPropertyKey::from_str(String("0")), 0);
        EsShape *shape1 = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        EsShape *shape2 = shape1->add(EsPropertyKey::from_str(String("2")), 2);
        EsShape *shape3 = shape2->add(EsPropertyKey::from_str(String("3")), 3);
        EsShape *shape4 = shape3->add(EsPropertyKey::from_str(String("4")), 4);
        TS_ASSERT_EQUALS(shape0->slot(), 0);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        TS_ASSERT_EQUALS(shape3->slot(), 3);
        TS_ASSERT_EQUALS(shape4->slot(), 4);

        shape4 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("4"))));
        TS_ASSERT(shape4);
        TS_ASSERT_EQUALS(shape4->slot(), 4);
        shape3 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("3"))));
        TS_ASSERT(shape3);
        TS_ASSERT_EQUALS(shape3->slot(), 3);
        shape2 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("2"))));
        TS_ASSERT(shape2);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        shape1 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("1"))));
        TS_ASSERT(shape1);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        shape0 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT(shape0);
        TS_ASSERT_EQUALS(shape0->slot(), 0);

        shape3 = const_cast<EsShape *>(shape3->lookup(EsPropertyKey::from_str(String("3"))));
        TS_ASSERT(shape3);
        TS_ASSERT_EQUALS(shape3->slot(), 3);
        shape2 = const_cast<EsShape *>(shape3->lookup(EsPropertyKey::from_str(String("2"))));
        TS_ASSERT(shape2);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        shape1 = const_cast<EsShape *>(shape3->lookup(EsPropertyKey::from_str(String("1"))));
        TS_ASSERT(shape1);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        shape0 = const_cast<EsShape *>(shape3->lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT(shape0);
        TS_ASSERT_EQUALS(shape0->slot(), 0);

        shape2 = const_cast<EsShape *>(shape2->lookup(EsPropertyKey::from_str(String("2"))));
        TS_ASSERT(shape2);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        shape1 = const_cast<EsShape *>(shape2->lookup(EsPropertyKey::from_str(String("1"))));
        TS_ASSERT(shape1);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        shape0 = const_cast<EsShape *>(shape2->lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT(shape0);
        TS_ASSERT_EQUALS(shape0->slot(), 0);

        shape1 = const_cast<EsShape *>(shape1->lookup(EsPropertyKey::from_str(String("1"))));
        TS_ASSERT(shape1);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        shape0 = const_cast<EsShape *>(shape1->lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT(shape0);
        TS_ASSERT_EQUALS(shape0->slot(), 0);

        shape0 = const_cast<EsShape *>(shape0->lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT(shape0);
        TS_ASSERT_EQUALS(shape0->slot(), 0);
    }

    void test_lookup_remove()
    {
        Gc::instance().init();
        EsShape::root()->clear_transitions();

        EsShape *shape0 = EsShape::root()->add(EsPropertyKey::from_str(String("0")), 0);
        EsShape *shape1 = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        EsShape *shape2 = shape1->add(EsPropertyKey::from_str(String("2")), 2);
        EsShape *shape3 = shape2->add(EsPropertyKey::from_str(String("3")), 3);
        EsShape *shape4 = shape3->add(EsPropertyKey::from_str(String("4")), 4);
        TS_ASSERT_EQUALS(shape0->slot(), 0);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        TS_ASSERT_EQUALS(shape3->slot(), 3);
        TS_ASSERT_EQUALS(shape4->slot(), 4);

        shape4 = shape4->remove(EsPropertyKey::from_str(String("0")));

        shape4 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("4"))));
        TS_ASSERT(shape4);
        TS_ASSERT_EQUALS(shape4->slot(), 4);
        shape3 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("3"))));
        TS_ASSERT(shape3);
        TS_ASSERT_EQUALS(shape3->slot(), 3);
        shape2 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("2"))));
        TS_ASSERT(shape2);
        TS_ASSERT_EQUALS(shape2->slot(), 2);
        shape1 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("1"))));
        TS_ASSERT(shape1);
        TS_ASSERT_EQUALS(shape1->slot(), 1);
        shape0 = const_cast<EsShape *>(shape4->lookup(EsPropertyKey::from_str(String("0"))));
        TS_ASSERT(!shape0);
    }

    void test_add_transition()
    {
        Gc::instance().init();
        EsShape::root()->clear_transitions();

        EsShape *shape0 = EsShape::root()->add(EsPropertyKey::from_str(String("0")), 0);
        TS_ASSERT(shape0->transitions_.empty());

        EsShape *shape1 = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        TS_ASSERT_EQUALS(shape0->transitions_.size(), 1);
        TS_ASSERT_EQUALS(shape1->transitions_.size(), 0);

        EsShape *shape2 = shape0->add(EsPropertyKey::from_str(String("2")), 2);
        TS_ASSERT_EQUALS(shape0->transitions_.size(), 2);
        TS_ASSERT_EQUALS(shape2->transitions_.size(), 0);

        EsShape *shape1_ = shape0->add(EsPropertyKey::from_str(String("1")), 1);
        TS_ASSERT_EQUALS(shape0->transitions_.size(), 2);
        TS_ASSERT_EQUALS(shape1_->transitions_.size(), 0);
        TS_ASSERT_EQUALS(shape1, shape1_);

        EsShape *shape2_ = shape0->add(EsPropertyKey::from_str(String("2")), 2);
        TS_ASSERT_EQUALS(shape0->transitions_.size(), 2);
        TS_ASSERT_EQUALS(shape2_->transitions_.size(), 0);
        TS_ASSERT_EQUALS(shape2, shape2_);
    }
};
