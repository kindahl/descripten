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
#include <string>
#include <stdint.h>

/**
 * @brief Class for generating unique names.
 */
class NameGenerator
{
private:
    uint64_t counter_;

    NameGenerator();
    NameGenerator(const NameGenerator &rhs);
    NameGenerator &operator=(const NameGenerator &rhs);
    
public:
    /**
     * Returns a reference to the one and only name generator instance.
     * @return Reference to the name generator.
     */
    static NameGenerator &instance();
    
    /**
     * Returns the next unique name provided by the name generator.
     * @return The next unique name.
     */
    std::string next();

    /**
     * Returns the next unique name provided by the name generator.
     * @param [in] prefix Name prefix.
     * @return Unique name based on prefix.
     */
    std::string next(const std::string &prefix);
};
