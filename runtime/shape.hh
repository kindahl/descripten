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
#include <unordered_map>
#include <vector>
#include <gc/gc_allocator.h>    // NOTE: 3rd party.
#include "container.hh"
#include "property_key.hh"

/**
 * @brief Shape used to dynamically classify objects.
 */
class EsShape
{
#ifdef UNITTEST
public:
    friend class ShapeTestSuite;
#endif

public:
    /** Unallocated slot, or used to signal that a lookup failed. */
    static const size_t INVALID_SLOT = -1;

private:
    /**
     * @brief Class transition.
     */
    struct Transition
    {
        EsShape *shape_;    ///< Transitioned to shape.
        int count_;         ///< Number of transitions to shape_.

        Transition(EsShape *shape)
            : shape_(shape)
            , count_(0) {}
    };

    typedef std::unordered_map<EsPropertyKey, Transition, EsPropertyKey::Hash,
                               std::equal_to<EsPropertyKey>,
                               gc_allocator<std::pair<EsPropertyKey, Transition> > > TransitionMap;

    void add_transition(const EsPropertyKey &key, EsShape *shape);
    void remove_transition(const EsPropertyKey &key);
    void clear_transitions();

private:
    typedef std::vector<EsShape *, gc_allocator<EsShape *> > EsShapeVector;

private:
    EsShape *parent_;   ///< Parent shape.

    EsPropertyKey key_; ///< Shape key.
    size_t slot_;       ///< Slot index.
    size_t depth_;      ///< Class depth.

    TransitionMap transitions_; ///< List of property transitions.

    /**
     * Constructs a new root shape.
     */
    EsShape();

    /**
     * Constructs a new shape.
     * @param [in] parent Parent shape.
     * @param [in] key Shape key.
     * @param [in] slot Shape slot.
     */
    EsShape(EsShape *parent, const EsPropertyKey &key, size_t slot);

public:
    /**
     * @return Poiner to root shape object.
     */
    static EsShape *root();

    /**
     * @return Parent shape.
     */
    EsShape *parent() const;

    /**
     * @return Shape key.
     */
    const EsPropertyKey &key() const;

    /**
     * @return Slot for primary property.
     */
    size_t slot() const;

    /**
     * @return Shape depth in shape hierarchy.
     */
    size_t depth() const;

    /**
     * Adds a shape to the hierarchy.
     * @param [in] key Shape key.
     * @return Pointer to new shape object.
     */
    EsShape *add(const EsPropertyKey &key, size_t slot);

    /**
     * Removes a shape from the hierarchy.
     * @param [in] key Shape key.
     * @return Pointer to new shape object.
     */
    EsShape *remove(const EsPropertyKey &key);

    /**
     * Searches the shape hierarchy for a shape matching the given key.
     * @param [in] key Shape key to lookup.
     * @return Pointer to shape object or NULL if no matching shape was found.
     */
    const EsShape *lookup(const EsPropertyKey &key) const;
};
