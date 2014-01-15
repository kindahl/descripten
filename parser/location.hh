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

namespace parser {

class Location
{
private:
    int beg_;
    int end_;

public:
    Location()
        : beg_(-1)
        , end_(-1)
    {
    }

    Location(int beg, int end)
        : beg_(beg)
        , end_(end)
    {
    }

    int begin() const
    {
        return beg_;
    }

    int end() const
    {
        return end_;
    }
};

}
