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
#include <type_traits>

/**
 * @brief Container for a value that might not be.
 *
 * This class does some trickery in order to not call the default constructor
 * on the maybe object at initialization. If storing the maybe object as an
 * ordinary member it would have to be initialized in the default constructor
 * when creating an empty maybe object. We don't want to do this because some
 * maybe objects might not have a default constructor at all.
 */
template <typename T>
class Maybe
{
private:
    typename std::aligned_storage<
        sizeof(T), std::alignment_of<T>::value>::type data_;
    bool has_data_;

    inline T *object()
    {
        return reinterpret_cast<T *>(&data_);
    }

    inline const T *object() const
    {
        return reinterpret_cast<const T *>(&data_);
    }

public:
    /**
     * Construct value-less object.
     */
    inline Maybe()
        : has_data_(false) {}

    /**
     * Construct object containing a value.
     * @param [in] val Value.
     */
    inline Maybe(const T &val)
        : has_data_(true)
    {
        new (object()) T(val);
    }

    /**
     * Sets new container value.
     * @param [in] val New value.
     */
    inline void set(const T &val)
    {
        new (object()) T(val);
        has_data_ = true;
    }

    /**
     * Empties the container.
     */
    inline void clear()
    {
        has_data_ = false;
    }

    /**
     * @return Container value.
     */
    inline T &operator*()
    {
        assert(has_data_);
        return *object();
    }
    /**
     * @return Container value.
     */
    inline const T &operator*() const
    {
        assert(has_data_);
        return *object();
    }

    /**
     * @return true if there is a value, false if there's not.
     */
    inline operator bool() const
    {
        return has_data_;
    }

    /**
     * @return true if there is no value, false if there is.
     */
    inline bool operator!() const
    {
        return !has_data_;
    }

    /**
     * Assigns another maybe container.
     * @param [in] rhs Right hand side container.
     * @return Reference to this container.
     */
    inline Maybe<T> &operator=(const Maybe<T> &rhs)
    {
        if (rhs)
        {
            new (object()) T(*rhs);
            has_data_ = true;
        }
        else
        {
            has_data_ = false;
        }

        return *this;
    }

    /**
     * Assigns a new container value.
     * @param [in] val New value.
     */
    inline Maybe<T> &operator=(const T &val)
    {
        set(val);
        return *this;
    }
};
