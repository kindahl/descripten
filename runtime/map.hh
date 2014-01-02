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
#include "common/string.hh"
#include "container.hh"
#include "property_key.hh"
#include "property_reference.hh"

class EsObject;
class EsProperty;
class EsShape;

/**
 * @brief Maps property names to properties.
 */
class EsMap
{
public:
#ifdef UNITTEST
    friend class MapTestSuite;
#endif

    typedef uintptr_t Id;

private:
    /** Maximum number of properties to maintain before creating a hash map. */
    static const size_t MAX_NUM_NON_MAPPED = 10;

private:
    typedef std::unordered_map<EsPropertyKey, size_t, EsPropertyKey::Hash> EsKeySlotMap;

private:
    /** Base object owning the map. */
    EsObject *base_;

    /** Previously allocated slots that have been freed up by removing
     * properties. These slots should be re-used before allocating new
     * slots. */
    std::vector<size_t> free_slots_;

    /** Last added shape, or EsShape::root() if the map is empty. */
    EsShape *last_shape_;

    /** Property array. */
    EsPropertyVector props_;

    /** When the number of properties becomes too large we'll create a hash map
     * for faster property lookup.
     * @see MAX_NUM_NON_MAPPED */
    EsKeySlotMap *map_;

public:
    explicit EsMap(EsObject *base);

    /**
     * @return Identifier that's shared by all maps sharing the same structure.
     */
    Id id() const;

    /**
     * @return Number of properties in map.
     */
    size_t size() const;

    /**
     * @return List of property keys in the order they were added.
     */
    std::vector<EsPropertyKey> keys() const;

    /**
     * Adds a new property to the map.
     * @param [in] key Property key.
     * @param [in] prop Property.
     * @pre No property with the key key exist in the map.
     */
    void add(const EsPropertyKey &p, const EsProperty &prop);

    /**
     * Removes a property from the map.
     * @param [in] key Property key.
     */
    void remove(const EsPropertyKey &key);

    /**
     * Searches the map for a property matching the given key.
     * @param [in] key Property key to lookup.
     * @return Reference to property object or an invalid reference if no
     *         matching property was found.
     */
    EsPropertyReference lookup(const EsPropertyKey &key);

    EsPropertyReference from_cached(const EsPropertyReference &cached);

    /**
     * Compares two maps for equality.
     * @param [in] rhs Right-hand-side map to compare against.
     * @return true if rhs is considered equal to this map.
     */
    bool operator==(const EsMap &rhs) const;

    /**
     * Compares two maps for inequality.
     * @param [in] rhs Right-hand-side map to compare against.
     * @return true if rhs is not considered equal to this map.
     */
    bool operator!=(const EsMap &rhs) const;

#ifdef DEBUG
    size_t capacity() const;
    size_t slot(const EsPropertyKey &key) const;
#endif
};
