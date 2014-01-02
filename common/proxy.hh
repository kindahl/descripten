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
#include <memory>

template <typename T>
class ProxySource;

/**
 * @brief Proxy value for a pure value declared elsewhere.
 */
template <typename T>
class Proxy
{
private:
    friend class ProxySource<T>;

    std::weak_ptr<T> val_;

    Proxy(const std::shared_ptr<T> &val)
        : val_(val) {}

public:
    const T &get() const
    {
        std::shared_ptr<T> val = val_.lock();
        assert(val);

        return *val;
    }

    operator const T &() const
    {
        return get();
    }
};

/**
 * @brief Source for a pure value accessed by any number of proxies.
 */
template <typename T>
class ProxySource
{
private:
    std::shared_ptr<T> val_;

public:
    ProxySource()
        : val_(std::make_shared<T>(T())) {}

    Proxy<T> get_proxy() const
    {
        return Proxy<T>(val_);
    }

    void set_value(const T &val)
    {
        *val_ = val;
    }
};
