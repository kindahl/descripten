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

#ifdef DEBUG
#  ifndef THROW
#    define THROW(type,...) throw type(__FILE__, __LINE__, ##__VA_ARGS__)
#  endif
#  ifndef DECLARE_CONSTRUCTOR
#    define DECLARE_CONSTRUCTOR(type,...) type(const char *file, int line, ##__VA_ARGS__)
#  endif
#  ifndef DEFINE_CONSTRUCTOR
#    define DEFINE_CONSTRUCTOR(type,...) type::type(const char *file, int line, ##__VA_ARGS__)
#  endif
#  ifndef PASS_INITIALIZER
#    define PASS_INITIALIZER(type,...) type(file, line, ##__VA_ARGS__)
#  endif
#else
#  ifndef THROW
#    define THROW(type,...) throw type(__VA_ARGS__)
#  endif
#  ifndef DECLARE_CONSTRUCTOR
#    define DECLARE_CONSTRUCTOR(type,...) type(__VA_ARGS__)
#  endif
#  ifndef DEFINE_CONSTRUCTOR
#    define DEFINE_CONSTRUCTOR(type,...) type::type(__VA_ARGS__)
#  endif
#  ifndef PASS_INITIALIZER
#    define PASS_INITIALIZER(type,...) type(__VA_ARGS__)
#  endif
#endif

/**
 * @brief Exception base class.
 */
class Exception
{
protected:
    std::string what_;
    
public:
    DECLARE_CONSTRUCTOR(Exception, const char *what);
    DECLARE_CONSTRUCTOR(Exception, const std::string &what);
    
    const std::string &what() const;
};

/**
 * @brief Exception class thrown when an internal error has occurred.
 */
class InternalException : public Exception
{
public:
    DECLARE_CONSTRUCTOR(InternalException, const char *what);
};

/**
 * @brief Exception class thrown when on memory allocation failures.
 */
class MemoryException : public Exception
{
public:
    DECLARE_CONSTRUCTOR(MemoryException);
};
