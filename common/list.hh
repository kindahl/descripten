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

#pragma once
#include <cassert>

template <typename T>
class IntrusiveLinkedList;

/**
 * @brief Base class for providing previous and next accessors.
 */
template <typename T>
class IntrusiveLinkedListNode
{
public:
    friend class IntrusiveLinkedList<T>;

private:
    T *prev_;
    T *next_;

protected:
    /**
     * Constructs a new node with no previous or next element.
     */
    IntrusiveLinkedListNode()
        : prev_(NULL)
        , next_(NULL) {}

public:
    /**
     * @return Previous node, or NULL for head node.
     */
    T *previous()
    {
        // Check for sentinel.
        if (!prev_ || !prev_->prev_)
            return NULL;

        return prev_;
    }

    /**
     * @return Previous node, or NULL for head node.
     */
    const T *previous() const
    {
        // Check for sentinel.
        if (!prev_ || !prev_->prev_)
            return NULL;

        return prev_;
    }

    /**
     * @return Next node, or NULL for tail node.
     */
    T *next()
    {
        // Check for sentinel.
        if (!next_ || !next_->next_)
            return NULL;

        return next_;
    }

    /**
     * @return Next node, or NULL for tail node.
     */
    const T *next() const
    {
        // Check for sentinel.
        if (!next_ || !next_->next_)
            return NULL;

        return next_;
    }
};


/**
 * @brief Intrusive linked list traits.
 */
template <typename T>
struct IntrusiveLinkedListTraits
{
    /**
     * @return New sentinel object.
     */
    static T *create_sentinel()
    {
        return new T();
    }

    /**
     * Destroys sentinel object.
     * @param [in] sentinel Sentinel to destroy.
     */
    static void destroy_sentinel(T *sentinel)
    {
        delete sentinel;
    }
};

/**
 * @brief Intrusive linked list.
 */
template <typename T>
class IntrusiveLinkedList final
{
public:
    /**
     * @brief Intrusive linked list iterator.
     */
    template <typename U>
    class BasicIterator
    {
    private:
        U *pos_;

    public:
        /**
         * Creates a new iterator pointing to nothing.
         */
        BasicIterator()
            : pos_(NULL) {}

        /**
         * Creates a new iterator pointing to the specified element.
         * @param [in] pos Pointer to element, or NULL to create an end
         *                 iterator.
         */
        BasicIterator(U *pos)
            : pos_(pos) {}

        /**
         * @return Raw element pointer, without checking for validity.
         */
        U *raw_pointer() const
        {
            return pos_;
        }

        /**
         * @return Reference to the element the at current position.
         * @pre Iterator points at a valid element.
         */
        U &operator*() const
        {
            assert(pos_);
            return *pos_;
        }

        /**
         * @return Point to the element at the current position.
         * @pre Iterator points at a valid element.
         */
        U *operator->() const
        {
            return pos_;
        }

        /**
         * Increments iterator (pre-increment).
         * @return Current iterator.
         */
        BasicIterator &operator++()
        {
            if (pos_)
                pos_ = pos_->next_;
            return *this;
        }

        /**
         * Increments iterator (post-increment).
         * @return Current iterator.
         */
        BasicIterator operator++(int)
        {
            BasicIterator res = *this;
            ++*this;
            return res;
        }

        /**
         * @return true if both iterators refer to the same element in the
         *         list, false if not.
         */
        bool operator==(const BasicIterator &rhs) const
        {
            return pos_ == rhs.pos_;
        }

        /**
         * @return true if both iterators does not refer to the same element
         *         in the list, false if not.
         */
        bool operator!=(const BasicIterator &rhs) const
        {
            return pos_ != rhs.pos_;
        }
    };

    typedef BasicIterator<T> Iterator;
    typedef BasicIterator<const T> ConstIterator;

private:
    T *head_;   ///< List head.
    T *tail_;   ///< List tail.

public:
    /**
     * Constructs a new empty list.
     */
    IntrusiveLinkedList()
        : head_(IntrusiveLinkedListTraits<T>::create_sentinel())
        , tail_(IntrusiveLinkedListTraits<T>::create_sentinel())
    {
        assert(head_);
        assert(tail_);

        head_->next_ = tail_;
        tail_->prev_ = head_;
    }

    /**
     * Destructs the list.
     */
    ~IntrusiveLinkedList()
    {
        T *next = head_->next_;
        T *prev = tail_->prev_;

        next->prev_ = NULL;
        prev->next_ = NULL;

        IntrusiveLinkedListTraits<T>::destroy_sentinel(head_);
        IntrusiveLinkedListTraits<T>::destroy_sentinel(tail_);
    }

    /**
     * @return true if the list is empty, false if it's not.
     */
    bool empty() const
    {
        return head_->next_ == tail_;
    }

    /**
     * @return Number of elements in list.
     */
    size_t length() const
    {
        size_t size = 0;

        for (ConstIterator it = begin(); it != end(); ++it)
            size++;

        return size;
    }

    /**
     * @return First element in the list.
     * @pre The list is not empty.
     */
    const T &front() const
    {
        assert(!empty());
        return *head_->next_;
    }

    /**
     * @return First element in the list.
     * @pre The list is not empty.
     */
    T &front()
    {
        assert(!empty());
        return *head_->next_;
    }

    /**
     * @return Last element in the list.
     * @pre The list is not empty.
     */
    const T &back() const
    {
        assert(!empty());
        return *tail_->prev_;
    }

    /**
     * @return Last element in the list.
     * @pre The list is not empty.
     */
    T &back()
    {
        assert(!empty());
        return *tail_->prev_;
    }

    /**
     * @return Constant iterator to beginning of list.
     */
    ConstIterator begin() const
    {
        return ConstIterator(head_->next_);
    }

    /**
     * @return Iterator to beginning of list.
     */
    Iterator begin()
    {
        return Iterator(head_->next_);
    }

    /**
     * @return Constant iterator to end of list.
     */
    ConstIterator end() const
    {
        return ConstIterator(tail_);
    }

    /**
     * @return Iterator to end of list.
     */
    Iterator end()
    {
        return Iterator(tail_);
    }

    /**
     * Inserts a value before the element specified by the iterator.
     * @param [in] where Position to insert element before.
     * @param [in] val Value to insert into the list.
     * @return Iterator to the newly inserted element.
     */
    Iterator insert(Iterator where, T *val)
    {
        assert(val);

        T *next = where.raw_pointer();
        assert(next);

        T *prev = next->prev_;
        assert(prev);

        val->next_ = next;
        val->prev_ = prev;
        prev->next_ = val;
        next->prev_ = val;

        return Iterator(val);
    }

    /**
     * Erases an element from the list.
     * @param [in] where Position of element to erase.
     * @return Iterator to element proceeding the erased element.
     * @note Memory of any erased element will not be released.
     */
    Iterator erase(Iterator where)
    {
        T *elem = where.raw_pointer();
        if (!elem || elem == tail_)
            return end();

        T *next = elem->next_;
        assert(next);
        T *prev = elem->prev_;
        assert(prev);

        prev->next_ = next;
        next->prev_ = prev;

        return Iterator(next);
    }

    /**
     * Insert an element into the front of the list.
     * @param [in] val Value to insert into the list.
     */
    void push_front(T *val)
    {
        insert(begin(), val);
    }

    /**
     * Insert an element into the back of the list.
     * @param [in] val Value to insert into the list.
     */
    void push_back(T *val)
    {
        insert(end(), val);
    }
};
