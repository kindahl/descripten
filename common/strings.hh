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
#include "string.hh"

typedef uint32_t StringId;

/**
 * @brief Collection of interned strings.
 */
class Strings
{
private:
    typedef std::unordered_map<String, StringId, String::Hash,
                               std::equal_to<String>,
                               gc_allocator<std::pair<String, StringId> > > StringInternMap;
    StringInternMap interns_;
    StringId next_id_;

public:
    Strings();

    /**
     * Tests if a string is interned.
     * @return true if the string is interned, false if it's not.
     */
    bool is_interned(const String &str);

    /**
     * Interns a string by placing it into the global string set if it's not
     * already present there.
     * @param [in] str String to intern.
     * @return String identifier shared among all equal interned strings.
     */
    StringId intern(const String &str);

    /**
     * Interns a string to the specified identifier. This function should only
     * be used on strings that are known to not be interned. If the string is
     * already interned, its identifier will be replaced by the new identifier.
     * This will break all previsouly interned equal strings as the old
     * identifier will be rendered invalid.
     * @param [in] str String to intern.
     * @param [in] id String intern identifier.
     */
    void unsafe_intern(const String &str, StringId id);

    /**
     * Looks up the value of string through its identifier. The lookup is
     * performed using a linear search so care should be taken when using this
     * function.
     * @param [in] id Identifier of string to lookup.
     * @return String with identifier id.
     */
    const String &lookup(StringId id) const;
};
