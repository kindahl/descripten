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

#include "property_array.hh"
#include "property_storage.hh"

EsPropertyArray::EsPropertyArray()
    : compact_(true)
{
}

void EsPropertyArray::reserve_compact_storage(uint32_t count)
{
    if (!compact_)
        return;

    compact_storage_.reserve(count);
}

void EsPropertyArray::switch_to_sparse_storage()
{
    if (!compact_)
        return;

    assert(sparse_storage_.empty());
    for (const std::pair<uint32_t, EsProperty> entry : compact_storage_)
        sparse_storage_.set(entry.first, entry.second);

    compact_storage_.clear();
    assert(compact_storage_.empty());
    compact_ = false;
}

void EsPropertyArray::set(uint32_t index, const EsProperty &prop)
{
    if (compact_)
    {
        // If we'll get more than 10% holes in the compact array switch to a
        // sparse array. Arrays with fewer than 16 elements are excluded.
        uint32_t approx_holes = compact_storage_.approx_holes_if_setting(index);
        if (approx_holes > 16 &&
            (compact_storage_.empty() || 
             (static_cast<double>(approx_holes) / compact_storage_.count()) > 0.1f))
        {
            switch_to_sparse_storage();

            assert(!compact_);
            sparse_storage_.set(index, prop);
        }
        else
        {
            compact_storage_.set(index, prop);
        }
    }
    else
    {
        sparse_storage_.set(index, prop);
    }
}
