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

#pragma once
#include "property_storage.hh"

class EsPropertyStorage;

/**
 * @brief Property array.
 */
class EsPropertyArray
{
private:
    EsCompactPropertyStorage compact_storage_;
    EsSparsePropertyStorage sparse_storage_;

    bool compact_;  ///< true when using compact storage.

    /**
     * Switches the property array storage model from compact mode to sparse
     * mode.
     */
    void switch_to_sparse_storage();

public:
    /**
     * @brief Array iterator.
     */
    class Iterator
    {
    private:
        Maybe<EsCompactPropertyStorage::Iterator> compact_it_;
        Maybe<EsSparsePropertyStorage::ConstIterator> sparse_it_;
        bool compact_;
        
    public:
        Iterator(EsCompactPropertyStorage::Iterator compact_it)
            : compact_it_(compact_it), compact_(true) {}

        Iterator(EsSparsePropertyStorage::ConstIterator sparse_it)
            : sparse_it_(sparse_it), compact_(false) {}

        const std::pair<uint32_t, EsProperty> operator*() const
        {
            if (compact_)
                return **compact_it_;
            else
                return **sparse_it_;
        }

        const Iterator &operator++()
        {
            if (compact_)
                ++(*compact_it_);
            else if (sparse_it_)
                ++(*sparse_it_);

            return *this;
        }

        bool operator!=(const Iterator &rhs) const
        {
            if (compact_ != rhs.compact_)
                return true;

            if (compact_)
                return *compact_it_ != *rhs.compact_it_;
            else
                return *sparse_it_ != *rhs.sparse_it_;
        }
    };

public:
    EsPropertyArray();

    void reserve_compact_storage(uint32_t count);

    /**
     * @return true if the array is in compact storage mode.
     */
    inline bool is_compact() const
    {
        return compact_;
    }

    /**
     * @return true if the array is empty.
     */
    inline bool empty() const
    {
        return compact_ ? compact_storage_.empty() : sparse_storage_.empty();
    }

    /**
     * @return Number of elements in array.
     */
    inline uint32_t count() const
    {
        return compact_ ? compact_storage_.count() : sparse_storage_.count();
    }

    /**
     * Gets a property at a given index.
     * @param [in] index Property index.
     * @return Property at index, or NULL if no property exist at index.
     */
    inline EsProperty *get(uint32_t index)
    {
        return compact_ ? compact_storage_.get(index) : sparse_storage_.get(index);
    }

    /**
     * Sets a property at a given index.
     * @param [in] index Property index.
     * @param [in] prop Property to set at index.
     */
    void set(uint32_t index, const EsProperty &prop);

    /**
     * Removes a property at the given index.
     * @param [in] index Property index.
     */
    inline void remove(uint32_t index)
    {
        return compact_ ? compact_storage_.remove(index) : sparse_storage_.remove(index);
    }

    inline EsProperty *operator[](size_t index)
    {
        return get(index);
    }

    inline Iterator begin()
    {
        if (compact_)
            return Iterator(compact_storage_.begin());
        else
            return Iterator(sparse_storage_.begin());
    }

    inline Iterator end()
    {
        if (compact_)
            return Iterator(compact_storage_.end());
        else
            return Iterator(sparse_storage_.end());
    }

    inline const Iterator begin() const
    {
        if (compact_)
            return Iterator(compact_storage_.begin());
        else
            return Iterator(sparse_storage_.begin());
    }

    inline const Iterator end() const
    {
        if (compact_)
            return Iterator(compact_storage_.end());
        else
            return Iterator(sparse_storage_.end());
    }
};
