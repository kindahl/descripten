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
#include <cassert>
#include <cinttypes>
#include <sstream>
#include <typeinfo>
#include <stdio.h>
#include "exception.hh"

#ifdef PLATFORM_DARWIN
extern char lex_cast_buf_[64];      // FIXME: Enable thread local storage on Darwin.
#else
extern __thread char lex_cast_buf_[64];
#endif

template <typename T, typename S>
inline T lexical_cast(S val)
{
    std::stringstream ss;
    ss << val;

    T res = T();
    if (ss >> res)
        return res;

    std::stringstream msg;
    msg << "lexical cast failure, casting \"" << typeid(S).name()
        << "\" to type " << typeid(T).name() << ".";
    THROW(Exception, msg.str().c_str());
}

template <>
inline const char *lexical_cast(int val)
{
    sprintf(lex_cast_buf_, "%d", val);
    return lex_cast_buf_;
}

template <>
inline const char *lexical_cast(unsigned int val)
{
    sprintf(lex_cast_buf_, "%u", val);
    return lex_cast_buf_;
}

template <>
inline const char *lexical_cast(uint64_t val)
{
    sprintf(lex_cast_buf_, "%" PRIu64, val);
    return lex_cast_buf_;
}

template <>
inline const char *lexical_cast(ssize_t val)
{
    sprintf(lex_cast_buf_, "%zd", val);
    return lex_cast_buf_;
}

template <>
inline const char *lexical_cast(size_t val)
{
    sprintf(lex_cast_buf_, "%zd", val);
    return lex_cast_buf_;
}

template <typename T, typename S>
inline T safe_cast(S val)
{
#ifdef DEBUG
    T res = dynamic_cast<T>(val);
    assert(res);
    return res;
#else
    return static_cast<T>(val);
#endif
}
