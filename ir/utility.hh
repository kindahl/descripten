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

/**
 * @brief Assigns a value to a variable for the duration of a scope.
 */
template <typename T>
class ScopedValue
{
private:
    T &var_;    ///< Variable where the value is stored.
    T val_; ///< Original value stored in variable.

public:
    ScopedValue(T &var, T val)
        : var_(var)
    {
        val_ = var;
        var_ = val;
    }
    ~ScopedValue()
    {
        var_ = val_;
    }

    T was() const
    {
        return val_;
    }
};

/**
 * @brief Pushes a value to a vector for the duration of a scope.
 */
template <typename T>
class ScopedVectorValue
{
private:
    std::vector<T *, gc_allocator<T *> > &vec_;
    T *val_;

public:
    ScopedVectorValue(std::vector<T *, gc_allocator<T *> > &vec, T *val)
        : vec_(vec)
        , val_(val)
    {
        vec_.push_back(val);
    }

    ~ScopedVectorValue()
    {
        vec_.pop_back();
    }

    inline T *operator->()
    {
        return val_;
    }
};
