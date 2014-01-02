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
#include <string>
#include "common/string.hh"
#include "rope.hh"

/**
 * @brief Generator base class.
 */
class Generator
{
protected:
    Rope out_;

protected:
    /**
     * Escapes a string into C-compatible string literal format.
     * @param [in] str String to escape.
     * @return Escaped string.
     */
    static std::string escape(const std::string &str);

    static std::string escape(const String &str);

    /**
     * Writes all regions to the specified file. If the file already exists it
     * will be overwritten.
     * @param [in] file_path Path to output file.
     */
    void write(const std::string &file_path);

public:
    virtual ~Generator();
};
