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

#include <cassert>
#include <gc_cpp.h>
#include "shape.hh"

/** Minimum number of slots in the transition map. */
#define DEFAULT_TRANSITION_MAP_SIZE    1

EsShape::EsShape()
    : parent_(NULL)
    , slot_(INVALID_SLOT)
    , depth_(0)
    , transitions_(DEFAULT_TRANSITION_MAP_SIZE)
{
}

EsShape::EsShape(EsShape *parent, const EsPropertyKey &key, size_t slot)
    : parent_(parent)
    , key_(key)
    , slot_(slot)
    , depth_(parent->depth() + 1)
    , transitions_(DEFAULT_TRANSITION_MAP_SIZE)
{
}

EsShape *EsShape::root()
{
    static EsShape *root = new (GC)EsShape();
    return root;
}

EsShape *EsShape::parent() const
{
    return parent_;
}

const EsPropertyKey &EsShape::key() const
{
    return key_;
}

size_t EsShape::slot() const
{
    return slot_;
}

size_t EsShape::depth() const
{
    return depth_;
}

void EsShape::add_transition(const EsPropertyKey &key, EsShape *shape)
{
    TransitionMap::iterator it = transitions_.find(key);
    if (it == transitions_.end())
        it = transitions_.insert(std::make_pair(key, shape)).first;

    assert(it != transitions_.end());
    it->second.count_++;
}

void EsShape::remove_transition(const EsPropertyKey &key)
{
    TransitionMap::iterator it = transitions_.find(key);
    if (it == transitions_.end())       // ANOMALY
        return;

    assert(it->second.count_ > 0);
    if (--it->second.count_ == 0)
        transitions_.erase(it);
}

void EsShape::clear_transitions()
{
    transitions_.clear();
}

EsShape *EsShape::add(const EsPropertyKey &key, size_t slot)
{
    TransitionMap::iterator it = transitions_.find(key);
    if (it != transitions_.end() && it->second.shape_->slot_ == slot)   // ANOMALY
    {
        it->second.count_++;
        return it->second.shape_;
    }

    EsShape *new_shape = new (GC)EsShape(this, key, slot);
    add_transition(key, new_shape);
    return new_shape;
}

EsShape *EsShape::remove(const EsPropertyKey &key)
{
    // We cannot delete anything from root.
    if (this == EsShape::root())
        return this;

    if (key == key_)
    {
        // Update the parent's transition map or we might end up in a situation
        // where this object will never be garbage collected.
        parent_->remove_transition(key);
        return parent_;
    }

    EsShapeVector classes_to_clone;

    EsShape *shape = this;
    for (; shape && shape != EsShape::root(); shape = shape->parent_)
    {
        if (shape->key_ == key)
            break;

        classes_to_clone.push_back(shape);
    }

    // We still have the same class if we didn't find the property.
    if (!shape)
        return this;

    // Update the parents transition map.
    shape = shape->parent_;
    shape->remove_transition(key);

    if (!shape)
        shape = root();

    // Clone the properties, creating a new hierarchy.
    EsShapeVector::reverse_iterator it;
    for (it = classes_to_clone.rbegin(); it != classes_to_clone.rend(); ++it)
        shape = shape->add((*it)->key_, (*it)->slot_);

    return shape;
}

const EsShape *EsShape::lookup(const EsPropertyKey &key) const
{
    for (const EsShape *shape = this; shape && shape != EsShape::root();
        shape = shape->parent_)
    {
        if (shape->key_ == key)
            return shape;
    }

    return NULL;
}
