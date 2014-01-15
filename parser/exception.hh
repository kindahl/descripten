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
#include "common/exception.hh"
#include "common/string.hh"

namespace parser {

/**
 * @brief Exception thrown on file errors.
 */
class FileException : public Exception
{
public:
    DECLARE_CONSTRUCTOR(FileException, const String &what);
};

/**
 * @brief Exception thrown on parse errors.
 */
class ParseException : public Exception
{
public:
    enum Kind
    {
        KIND_SYNTAX,
        KIND_REFERENCE
    };

private:
    Kind kind_;

public:
    DECLARE_CONSTRUCTOR(ParseException, const String &what, Kind kind = KIND_SYNTAX);

    Kind kind() const
    {
        return kind_;
    }
};

}
