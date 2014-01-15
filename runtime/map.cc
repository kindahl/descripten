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

#include <cassert>
#include <gc_cpp.h>
#include "map.hh"
#include "shape.hh"

EsMap::EsMap(EsObject *base)
    : base_(base)
    , last_shape_(EsShape::root())
    , map_(NULL)
{
}

EsMap::Id EsMap::id() const
{
    return reinterpret_cast<EsMap::Id>(last_shape_);
}

size_t EsMap::size() const
{
    return last_shape_->depth();
}

std::vector<EsPropertyKey> EsMap::keys() const
{
    size_t prop_count = last_shape_->depth();
    std::vector<EsPropertyKey> prop_keys(prop_count);

    EsShape *shape = last_shape_;
    for (size_t i = prop_count - 1; shape != EsShape::root(); shape = shape->parent(), i--)
        prop_keys[i] = shape->key();

    return prop_keys;
}

void EsMap::add(const EsPropertyKey &key, const EsProperty &prop)
{
    size_t slot = EsShape::INVALID_SLOT;
    if (free_slots_.empty())
    {
        slot = props_.size();
        props_.push_back(prop);
    }
    else
    {
        slot = free_slots_.back();
        free_slots_.pop_back();
        props_[slot] = prop;
    }

    last_shape_ = last_shape_->add(key, slot);

    // If the property chain grows too large, allocate a hash map for fast
    // property lookup.
    if (!map_ && last_shape_->depth() > MAX_NUM_NON_MAPPED)
    {
        map_ = new (GC)EsKeySlotMap();

        // Add all existing properties.
        EsShape *shape = last_shape_->parent();
        for (; shape != EsShape::root(); shape = shape->parent())
            map_->insert(std::make_pair(shape->key(), shape->slot()));
    }

    if (map_)
        map_->insert(std::make_pair(key, slot));
}

void EsMap::remove(const EsPropertyKey &key)
{
    const EsShape *to_remove = last_shape_->lookup(key);
    if (!to_remove)
        return;

    last_shape_ = last_shape_->remove(key);
    assert(last_shape_);

    free_slots_.push_back(to_remove->slot());

    if (map_)
        map_->erase(key);
}

EsPropertyReference EsMap::lookup(const EsPropertyKey &key)
{
    if (map_)
    {
        EsKeySlotMap::iterator it = map_->find(key);
        if (it == map_->end())
            return EsPropertyReference();

        assert(it->second < props_.size());
        return EsPropertyReference(base_, &props_, it->second);
    }

    const EsShape *shape = last_shape_->lookup(key);
    if (shape)
    {
        assert(shape->slot() < props_.size());
        return EsPropertyReference(base_, &props_, shape->slot());
    }

    return EsPropertyReference();
}

EsPropertyReference EsMap::from_cached(const EsPropertyReference &cached)
{
    return cached.rebase(base_, &props_);
}

bool EsMap::operator==(const EsMap &rhs) const
{
    // If the last shape pointers refers to the same shape we know that they
    // have followed the same transitions.
    return last_shape_ == rhs.last_shape_;
}

bool EsMap::operator!=(const EsMap &rhs) const
{
    return !(*this == rhs);
}

#ifdef DEBUG
size_t EsMap::capacity() const
{
    return props_.capacity();
}

size_t EsMap::slot(const EsPropertyKey &key) const
{
    if (map_)
    {
        EsKeySlotMap::iterator it = map_->find(key);
        if (it == map_->end())
            return EsShape::INVALID_SLOT;

        return it->second;
    }

    const EsShape *shape = last_shape_->lookup(key);
    if (shape)
        return shape->slot();

    return EsShape::INVALID_SLOT;
}
#endif
