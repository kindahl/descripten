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

#include <fstream>
#include "common/exception.hh"
#include "generator.hh"

Generator::~Generator()
{
}

std::string Generator::escape(const std::string &str)
{
    std::string res;

    for (size_t i = 0; i < str.length(); i++)
    {
        switch (str[i])
        {
            case '\0':
                res.append("\\0");
                break;
            case '\n':
                res.append("\\n");
                break;
            case '\t':
                res.append("\\t");
                break;
            case '\v':
                res.append("\\v");
                break;
            case '\b':
                res.append("\\b");
                break;
            case '\r':
                res.append("\\r");
                break;
            case '\f':
                res.append("\\f");
                break;
            case '\a':
                res.append("\\a");
                break;
            case '\\':
                res.append("\\\\");
                break;
            case '\?':
                res.append("\\?");
                break;
            case '\'':
                res.append("\\'");
                break;
            case '\"':
                res.append("\\\"");
                break;

            default:
                if (isascii(str[i]))
                {
                    res.push_back(str[i]);
                }
                else
                {
                    char buffer[16];
                    sprintf(buffer, "\"\"\\x%.2x\"\"", str[i] & 0xff);
                    res.append(buffer);
                }
                break;
        }
    }

    return res;
}

std::string Generator::escape(const String &str)
{
    std::string res;

    for (size_t i = 0; i < str.length(); i++)
    {
        switch (str[i])
        {
            case '\0':
                res.append("\\0");
                break;
            case '\n':
                res.append("\\n");
                break;
            case '\t':
                res.append("\\t");
                break;
            case '\v':
                res.append("\\v");
                break;
            case '\b':
                res.append("\\b");
                break;
            case '\r':
                res.append("\\r");
                break;
            case '\f':
                res.append("\\f");
                break;
            case '\a':
                res.append("\\a");
                break;
            case '\\':
                res.append("\\\\");
                break;
            case '\?':
                res.append("\\?");
                break;
            case '\'':
                res.append("\\'");
                break;
            case '\"':
                res.append("\\\"");
                break;

            default:
                if (isascii(str[i]))
                {
                    res.push_back(str[i]);
                }
                else
                {
                    char buffer[16];
                    sprintf(buffer, "\\U%.8x", str[i]);
                    res.append(buffer);
                }
                break;
        }
    }

    return res;
}

void Generator::write(const std::string &file_path)
{
    std::ofstream fos;
    fos.open(file_path.c_str());
    if (!fos.is_open())
    {
        std::stringstream msg;
        msg << "error: unable to open " << file_path << " for writing.";
        THROW(Exception, msg.str());
    }

    // Write regions.
    out_.write(fos);
}
