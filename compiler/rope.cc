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

#include "common/exception.hh"
#include "rope.hh"

Rope::Rope()
{
}

Rope::~Rope()
{
    clear();
}

void Rope::clear()
{
    std::vector<Rope *>::iterator it;
    for (it = forks_.begin(); it != forks_.end(); ++it)
        delete *it;

    forks_.clear();

    stream_.str("");
}

Rope *Rope::fork()
{
    Rope *new_fork = new Rope();
    forks_.push_back(new_fork);
    return new_fork;
}

std::stringstream &Rope::stream()
{
    return stream_;
}

void Rope::write(std::ostream &stream) const
{
    std::string str = stream_.str();
    stream.write(str.c_str(), str.size());

    if (!stream.good())
        THROW(Exception, "error: unable to write output file.");

    std::vector<Rope *>::const_iterator it;
    for (it = forks_.begin(); it != forks_.end(); ++it)
        (*it)->write(stream);
}
