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
#include <map>
#include <vector>
#include <stdint.h>
#include <gc_cpp.h>             // 3rd party.
#include <gc/gc_allocator.h>    // 3rd party.
#include "common/core.hh"
#include "common/maybe.hh"
#include "property.hh"

/**
 * @brief Compact property storage
 */
class EsCompactPropertyStorage
{
private:
    typedef std::vector<Maybe<EsProperty>,
                        gc_allocator<Maybe<EsProperty> > > PropertyVector;
    PropertyVector properties_;

    uint32_t holes_;    ///< Number of holes in the array.

public:
    /**
     * @brief Storage property iterator.
     */
    class Iterator
    {
    private:
        const PropertyVector &vec_;
        size_t pos_;    ///< Current position in vector.
        
        Iterator(const PropertyVector &vec, size_t pos)
            : vec_(vec), pos_(pos) {}

    public:
        inline static Iterator begin(const PropertyVector &vec)
        {
            for (size_t i = 0; i < vec.size(); i++)
            {
                if (vec[i])
                    return Iterator(vec, i);
            }

            return Iterator(vec, vec.size());
        }

        inline static Iterator end(const PropertyVector &vec)
        {
            return Iterator(vec, vec.size());
        }

        const std::pair<uint32_t, EsProperty> operator*() const
        {
            assert(pos_ < vec_.size());

            const Maybe<EsProperty> &item = vec_[pos_];
            assert(item);
            return std::make_pair(static_cast<uint32_t>(pos_), *item);
        }

        const Iterator &operator++()
        {
            ++pos_;

            if (pos_ >= vec_.size())
            {
                pos_ = vec_.size();
                return *this;
            }

            while (!vec_[pos_])
            {
                ++pos_;
                if (pos_ == vec_.size())
                    break;
            }

            return *this;
        }

        bool operator!=(const Iterator &rhs) const
        {
            assert(pos_ <= vec_.size());
            return pos_ != rhs.pos_;
        }
    };

public:
    EsCompactPropertyStorage()
        : holes_(0) {}

    /**
     * Reserves memory for storing @p count number of properties.
     * @param [in] count How many properties to reserve memory for.
     */
    inline void reserve(uint32_t count)
    {
        properties_.reserve(count);
    }

    /**
     * @return true if the storage is empty.
     */
    inline bool empty() const
    {
        return properties_.empty();
    }

    /**
     * Clears the storage and releases the memory.
     */
    inline void clear()
    {
        PropertyVector tmp;
        properties_.swap(tmp);
    }

    /**
     * @return Number of holes in storage.
     */
    inline uint32_t holes() const
    {
        return holes_;
    }

    /**
     * Computes the approximate number of holes the storage would contain if
     * setting the property at the given index.
     * @oaram [in] index Property index.
     * @return Approximate number of holes after setting index.
     */
    inline uint32_t approx_holes_if_setting(uint32_t index) const
    {
        if (index >= static_cast<uint32_t>(properties_.size()))
            return holes_ + index - properties_.size();

        // This may not be 100% accurate since we might fill a hole at the
        // given index.
        return holes_;
    }

    /**
     * @return Number of elements in storage.
     */
    inline uint32_t count() const
    {
        return static_cast<uint32_t>(properties_.size()) - holes_;
    }

    /**
     * Gets a property at a given index.
     * @param [in] index Property index.
     * @return Property at index, or NULL if no property exist at index.
     */
    inline EsProperty *get(uint32_t index)
    {
        if (index >= static_cast<uint32_t>(properties_.size()))
            return NULL;

        Maybe<EsProperty> &item = properties_[index];
        return item ? &(*item) : NULL;
    }

    /**
     * Sets a property at a given index.
     * @param [in] index Property index.
     * @param [in] prop Property to set at index.
     */
    inline void set(uint32_t index, const EsProperty &prop)
    {
        // Pad the array if necessary.
        while (index >= static_cast<uint32_t>(properties_.size()))
        {
            properties_.push_back(Maybe<EsProperty>());
            holes_++;
        }

        Maybe<EsProperty> &slot = properties_[index];
        if (!slot)
            holes_--;
        slot = prop;
    }

    /**
     * Removes a property at the given index.
     * @param [in] index Property index.
     */
    inline void remove(uint32_t index)
    {
        if (index >= static_cast<uint32_t>(properties_.size()))
            return;

        Maybe<EsProperty> &slot = properties_[index];
        if (slot)
        {
            slot.clear();
            holes_++;
        }
    }

    inline Iterator begin()
    {
        return Iterator::begin(properties_);
    }

    inline Iterator end()
    {
        return Iterator::end(properties_);
    }

    inline const Iterator begin() const
    {
        return Iterator::begin(properties_);
    }

    inline const Iterator end() const
    {
        return Iterator::end(properties_);
    }
};

/**
 * @brief Sparse property storage.
 */
class EsSparsePropertyStorage
{
private:
    typedef std::map<uint32_t, EsProperty,
                     std::less<uint32_t>,
                     gc_allocator<std::pair<uint32_t, EsProperty> > > PropertyMap;
    PropertyMap properties_;

public:
    typedef PropertyMap::iterator Iterator;
    typedef PropertyMap::const_iterator ConstIterator;

public:
    /**
     * @return true if the storage is empty.
     */
    inline bool empty() const
    {
        return properties_.empty();
    }

    /**
     * @return Number of elements in storage.
     */
    inline uint32_t count() const
    {
        return static_cast<uint32_t>(properties_.size());
    }

    /**
     * Gets a property at a given index.
     * @param [in] index Property index.
     * @return Property at index, or NULL if no property exist at index.
     */
    inline EsProperty *get(uint32_t index)
    {
        PropertyMap::iterator it = properties_.find(index);
        if (it == properties_.end())
            return NULL;

        return &it->second;
    }

    /**
     * Sets a property at a given index.
     * @param [in] index Property index.
     * @param [in] prop Property to set at index.
     */
    inline void set(uint32_t index, const EsProperty &prop)
    {
        properties_.insert(std::make_pair(index, prop));
    }

    /**
     * Removes a property at the given index.
     * @param [in] index Property index.
     */
    inline void remove(uint32_t index)
    {
        PropertyMap::iterator it = properties_.find(index);
        if (it == properties_.end())
            return;

        properties_.erase(it);
    }

    inline Iterator begin()
    {
        return properties_.begin();
    }

    inline ConstIterator begin() const
    {
        return properties_.begin();
    }

    inline Iterator end()
    {
        return properties_.end();
    }

    inline ConstIterator end() const
    {
        return properties_.end();
    }
};
