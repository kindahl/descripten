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

#include <sstream>
#include <stddef.h>
#include "algorithm.hh"
#include "property.hh"
#include "utility.hh"

bool EsProperty::described_by(const EsPropertyDescriptor &desc) const
{
    // First, check presence of fields.
    if ((desc.writable_ && !writable_) ||
        (desc.value_ && !value_) ||
        (desc.setter_ && !setter_) ||
        (desc.getter_ && !getter_))
        return false;

    // We now know that all fields in the descriptor is present in this
    // property so compare the contents.
    if (desc.enumerable_ && *desc.enumerable_ != enumerable_)
        return false;
    if (desc.configurable_ && *desc.configurable_ != configurable_)
        return false;
    if (desc.writable_ && desc.writable_ != writable_)
        return false;
    if (desc.value_ && !algorithm::same_value(*value_, *desc.value_))
        return false;
    if (desc.setter_ && !algorithm::same_value(*setter_, *desc.setter_))
        return false;
    if (desc.getter_ && !algorithm::same_value(*getter_, *desc.getter_))
        return false;

    return true;
}

void EsProperty::copy_from(const EsPropertyDescriptor &desc)
{
    if (desc.enumerable_)
        enumerable_ = *desc.enumerable_;
    if (desc.configurable_)
        configurable_ = *desc.configurable_;
    if (desc.writable_)
        writable_ = *desc.writable_;

    if (desc.value_)
        value_ = desc.value_;
    if (desc.setter_)
        setter_ = desc.setter_;
    if (desc.getter_)
        getter_ = desc.getter_;
}
