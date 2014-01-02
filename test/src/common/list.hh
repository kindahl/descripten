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
#include "common/list.hh"
#include "../gc.hh"

class Node : public IntrusiveLinkedListNode<Node>
{
};

class ListTestSuite : public CxxTest::TestSuite
{
public:
    void test_empty_and_length()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;
        TS_ASSERT(list.empty());
        TS_ASSERT_EQUALS(list.length(), 0);

        list.push_back(new (GC)Node());
        TS_ASSERT(!list.empty());
        TS_ASSERT_EQUALS(list.length(), 1);
        list.push_back(new (GC)Node());
        TS_ASSERT(!list.empty());
        TS_ASSERT_EQUALS(list.length(), 2);
        list.push_back(new (GC)Node());
        TS_ASSERT(!list.empty());
        TS_ASSERT_EQUALS(list.length(), 3);
    }

    void test_default_node()
    {
        Gc::instance().init();

        Node *a = new (GC)Node();

        TS_ASSERT(a->previous() == NULL);
        TS_ASSERT(a->next() == NULL);
    }

    void test_push_back()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        Node *a = new (GC)Node();
        Node *b = new (GC)Node();
        Node *c = new (GC)Node();

        list.push_back(a);
        TS_ASSERT(&list.front() == a);
        TS_ASSERT(&list.back() == a);
        TS_ASSERT(a->previous() == NULL);
        TS_ASSERT(a->next() == NULL);

        list.push_back(b);
        TS_ASSERT(&list.front() == a);
        TS_ASSERT(&list.back() == b);
        TS_ASSERT(a->previous() == NULL);
        TS_ASSERT(a->next() == b);
        TS_ASSERT(b->previous() == a);
        TS_ASSERT(b->next() == NULL);

        list.push_back(c);
        TS_ASSERT(&list.front() == a);
        TS_ASSERT(&list.back() == c);
        TS_ASSERT(a->previous() == NULL);
        TS_ASSERT(a->next() == b);
        TS_ASSERT(b->previous() == a);
        TS_ASSERT(b->next() == c);
        TS_ASSERT(c->previous() == b);
        TS_ASSERT(c->next() == NULL);
    }

    void test_push_front()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        Node *a = new (GC)Node();
        Node *b = new (GC)Node();
        Node *c = new (GC)Node();

        list.push_front(a);
        TS_ASSERT(&list.front() == a);
        TS_ASSERT(&list.back() == a);
        TS_ASSERT(a->previous() == NULL);
        TS_ASSERT(a->next() == NULL);

        list.push_front(b);
        TS_ASSERT(&list.front() == b);
        TS_ASSERT(&list.back() == a);
        TS_ASSERT(a->previous() == b);
        TS_ASSERT(a->next() == NULL);
        TS_ASSERT(b->previous() == NULL);
        TS_ASSERT(b->next() == a);

        list.push_front(c);
        TS_ASSERT(&list.front() == c);
        TS_ASSERT(&list.back() == a);
        TS_ASSERT(a->previous() == b);
        TS_ASSERT(a->next() == NULL);
        TS_ASSERT(b->previous() == c);
        TS_ASSERT(b->next() == a);
        TS_ASSERT(c->previous() == NULL);
        TS_ASSERT(c->next() == b);
    }

    void test_iterators()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;
        IntrusiveLinkedList<Node>::Iterator it;
        for (it = list.begin(); it != list.end(); ++it)
        {
            TS_ASSERT(false);
        }

        Node *a = new (GC)Node();
        Node *b = new (GC)Node();
        Node *c = new (GC)Node();
        Node *nodes[] = { a, b, c };

        list.push_back(a);
        int count = 0;
        for (it = list.begin(); it != list.end(); it++)
        {
            TS_ASSERT_EQUALS(&(*it), nodes[count]);
            count++;
        }
        TS_ASSERT_EQUALS(count, 1);

        list.push_back(b);
        count = 0;
        for (it = list.begin(); it != list.end(); ++it)
        {
            TS_ASSERT_EQUALS(&(*it), nodes[count]);
            count++;
        }
        TS_ASSERT_EQUALS(count, 2);

        list.push_back(c);
        count = 0;
        for (it = list.begin(); it != list.end(); it++)
        {
            TS_ASSERT_EQUALS(&(*it), nodes[count]);
            count++;
        }
        TS_ASSERT_EQUALS(count, 3);
    }

    void test_erase_first()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        Node *a = new (GC)Node();
        Node *b = new (GC)Node();

        list.push_back(a);
        list.push_back(b);

        size_t i = 0;
        IntrusiveLinkedList<Node>::Iterator it;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), a);
                    break;
                case 1:
                    TS_ASSERT_EQUALS(&(*it), b);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }
            if (&(*it) == a)
                list.erase(it);
        }

        TS_ASSERT_EQUALS(list.length(), 1);

        i = 0;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), b);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }
        }
    }

    void test_erase_middle()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        Node *a = new (GC)Node();
        Node *b = new (GC)Node();
        Node *c = new (GC)Node();

        list.push_back(a);
        list.push_back(b);
        list.push_back(c);

        size_t i = 0;
        IntrusiveLinkedList<Node>::Iterator it;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), a);
                    break;
                case 1:
                    TS_ASSERT_EQUALS(&(*it), b);
                    break;
                case 2:
                    TS_ASSERT_EQUALS(&(*it), c);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }
            if (&(*it) == b)
                list.erase(it);
        }

        TS_ASSERT_EQUALS(list.length(), 2);

        i = 0;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), a);
                    break;
                case 1:
                    TS_ASSERT_EQUALS(&(*it), c);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }
        }
    }

    void test_erase_last()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        Node *a = new (GC)Node();
        Node *b = new (GC)Node();

        list.push_back(a);
        list.push_back(b);

        size_t i = 0;
        IntrusiveLinkedList<Node>::Iterator it;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), a);
                    break;
                case 1:
                    TS_ASSERT_EQUALS(&(*it), b);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }
            if (&(*it) == b)
                list.erase(it);
        }

        TS_ASSERT_EQUALS(list.length(), 1);

        i = 0;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), a);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }
        }
    }

    void test_erase_to_empty()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        Node *a = new (GC)Node();

        list.push_back(a);

        size_t i = 0;
        IntrusiveLinkedList<Node>::Iterator it;
        for (it = list.begin(); it != list.end(); ++it, ++i)
        {
            switch (i)
            {
                case 0:
                    TS_ASSERT_EQUALS(&(*it), a);
                    break;
                default:
                    TS_ASSERT(false);
                    break;
            }

            list.erase(it);
        }

        TS_ASSERT_EQUALS(list.length(), 0);
        TS_ASSERT(list.empty());

        for (it = list.begin(); it != list.end(); ++it)
        {
            TS_ASSERT(false);
        }
    }

    void test_erase_empty()
    {
        Gc::instance().init();

        IntrusiveLinkedList<Node> list;

        list.erase(list.begin());
        TS_ASSERT_EQUALS(list.length(), 0);
        TS_ASSERT(list.empty());

        list.erase(list.end());
        TS_ASSERT_EQUALS(list.length(), 0);
        TS_ASSERT(list.empty());
    }
};
