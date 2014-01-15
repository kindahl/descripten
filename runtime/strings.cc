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

#include <algorithm>
#include <cassert>
#include "strings.hh"

EsStrings::EsStrings()
    : next_id_(0)
{
}

bool EsStrings::is_interned(const EsString *str)
{
    return interns_.find(str) != interns_.end();
}

StringId EsStrings::intern(const EsString *str)
{
    StringInternMap::iterator it = interns_.find(str);
    if (it != interns_.end())
        return it->second;

    interns_.insert(std::make_pair(str, next_id_));
    return next_id_++;
}

void EsStrings::unsafe_intern(const EsString *str, StringId id)
{
#ifdef DEBUG
    StringInternMap::iterator it = interns_.find(str);
    assert(it == interns_.end());
#endif

    // FIXME: Limit next_id_ depending on id.
    interns_.insert(std::make_pair(str, id));
}

const EsString *EsStrings::lookup(StringId id) const
{
    StringInternMap::const_iterator it;
    it = std::find_if(interns_.begin(), interns_.end(),
                      [&id](const StringInternMap::value_type &v)
        {
            return v.second == id;
        });
    assert(it != interns_.end());
    return it->first;
}
