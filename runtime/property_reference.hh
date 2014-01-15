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
#include <cassert>
#include <vector>
#include "common/string.hh"
#include "container.hh"

class EsObject;
class EsProperty;

/**
 * @brief Reference to a property.
 */
class EsPropertyReference
{
private:
    /**
     * @brief Reference types.
     */
    enum
    {
        /** Invalid property reference, may not be accessed. */
        INVALID,

        /** Property is stored in a slot in a property array. */
        SLOTTED,

        /** Property is owned by the reference. */
        IMMEDIATE
    } kind_;

    union
    {
        struct
        {
            EsPropertyVector *storage_;
            size_t slot_;
        } slotted;

        struct
        {
            EsProperty *property_;
        } immediate;
    };

    EsObject *base_;    ///< Base object.

public:
    /**
     * Constructs a new invalid reference.
     */
    inline EsPropertyReference()
        : kind_(INVALID)
        , base_(NULL)
    {
        slotted.storage_ = NULL;
        slotted.slot_ = 0;
    }

    /**
     * Constructs a new slotted reference.
     * @param [in] base Base object.
     * @param [in] storage Slots array.
     * @param [in] slot Property slot number.
     */
    inline EsPropertyReference(EsObject *base, EsPropertyVector *storage,
                               size_t slot)
        : kind_(SLOTTED)
        , base_(base)
    {
        slotted.storage_ = storage;
        slotted.slot_ = slot;
    }

    /**
     * Constructs a new immediate reference.
     * @param [in] base Base object.
     * @param [in] property Pointer to property.
     */
    inline EsPropertyReference(EsObject *base, EsProperty *property)
        : kind_(IMMEDIATE)
        , base_(base)
    {
        immediate.property_ = property;
    }

    /**
     * @return true if the property reference can be cached and false if it
     *         cannot be cached.
     */
    bool is_cachable() const
    {
        return kind_ == SLOTTED;
    }

    bool is_slotted() const
    {
        return kind_ == SLOTTED;
    }

    size_t slot() const
    {
        return slotted.slot_;
    }

    /**
     * @return Reference base object.
     */
    inline EsObject *base() const
    {
        assert(base_);
        return base_;
    }

    inline EsPropertyReference rebase(EsObject *base,
                                      EsPropertyVector *storage) const
    {
        assert(base);
        assert(storage);

        switch (kind_)
        {
            case SLOTTED:
                assert(storage->size() > slotted.slot_);
                return EsPropertyReference(base, storage, slotted.slot_);
            case IMMEDIATE:
                return EsPropertyReference(base, immediate.property_);
            default:
                return *this;
        }

        return *this;
    }

    /**
     * @return Pointer to property.
     */
    inline EsProperty *operator->()
    {
        switch (kind_)
        {
            case SLOTTED:
                return &(*slotted.storage_)[slotted.slot_];
            case IMMEDIATE:
                return immediate.property_;
            default:
                assert(false);
                return NULL;
        }
    }

    /**
     * @return Pointer to property.
     */
    inline const EsProperty *operator->() const
    {
        switch (kind_)
        {
            case SLOTTED:
                return &(*slotted.storage_)[slotted.slot_];
            case IMMEDIATE:
                return immediate.property_;
            default:
                assert(false);
                return NULL;
        }
    }

    /**
     * @return Reference to property.
     */
    inline EsProperty &operator*()
    {
        switch (kind_)
        {
            case SLOTTED:
                return (*slotted.storage_)[slotted.slot_];
            case IMMEDIATE:
                return *immediate.property_;
            default:
                assert(false);
                return *immediate.property_;
        }
    }

    /**
     * @return Reference to property.
     */
    inline const EsProperty &operator*() const
    {
        switch (kind_)
        {
            case SLOTTED:
                return (*slotted.storage_)[slotted.slot_];
            case IMMEDIATE:
                return *immediate.property_;
            default:
                assert(false);
                return *immediate.property_;
        }
    }

    /**
     * @return true if the reference is valid, false if it's not.
     */
    inline operator bool() const
    {
        return kind_ != INVALID;
    }

    /**
     * Compares two references for equality.
     * @param [in] rhs Right-hand-side reference to compare against.
     * @return true if rhs is considered equal to this reference.
     */
    inline bool operator==(const EsPropertyReference &rhs) const
    {
        if (kind_ != rhs.kind_)
            return false;

        switch (kind_)
        {
            case INVALID:
                return true;
            case SLOTTED:
                return slotted.storage_ == rhs.slotted.storage_ &&
                       slotted.slot_ == rhs.slotted.slot_;
            case IMMEDIATE:
                return immediate.property_ == rhs.immediate.property_;
            default:
                assert(false);
                return false;
        }
    }

    /**
     * Compares two references for inequality.
     * @param [in] rhs Right-hand-side reference to compare against.
     * @return true if rhs is not considered equal to this reference.
     */
    inline bool operator!=(const EsPropertyReference &rhs) const
    {
        return !(*this == rhs);
    }
};
