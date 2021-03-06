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
#include <map>
#include <vector>
#include <gc_cpp.h>             // 3rd party.
#include <gc/gc_allocator.h>    // 3rd party.
#include "common/string.hh"
#include "property.hh"
#include "value.hh"

/**
 * Vector containing values.
 */
typedef std::vector<EsValue, gc_allocator<EsValue> > EsValueVector;

/**
 * Vector of property names and properties.
 */
typedef std::vector<EsProperty,
                    gc_allocator<EsProperty> > EsPropertyVector;
