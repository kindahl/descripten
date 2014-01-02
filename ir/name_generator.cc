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
#include "name_generator.hh"

NameGenerator::NameGenerator()
    : counter_(0)
{
}

NameGenerator &NameGenerator::instance()
{
    static NameGenerator instance;
    return instance;
}

std::string NameGenerator::next()
{
    std::stringstream name;
    name << "_" << counter_++;
    return name.str();
}

std::string NameGenerator::next(const std::string &prefix)
{
    bool use_prefix = !prefix.empty();
    for (size_t i = 0; i < prefix.size(); i++)
    {
        if (prefix[i] >= 'a' && prefix[i] <= 'z')
            continue;
        if (prefix[i] >= 'A' && prefix[i] <= 'Z')
            continue;
        if (prefix[i] >= '0' && prefix[i] <= '9' && i != 0)
            continue;
        if (prefix[i] == '_')
            continue;

        use_prefix = false;
        break;
    }

    std::stringstream name;
    name << (use_prefix ? prefix : "_") << counter_++;
    return name.str();
}
