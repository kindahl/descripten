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

#include <cassert>
#include <cstdarg>
#include <stdio.h>
#include "messages.hh"

static const char *es_messages_[] =
{
    // RangeError.
    "invalid array length '%s'",
    "radix must be a value between 2 and 36.",
    "the number of fractional digits must be a value between 0 and 20.",
    "date number must be a finite number.",
    "precision must be a value between 1 and and 21",
    
    // ReferenceError.
    "'%s' is not defined.",
    "unresolvable reference to '%s.'",
    "expected reference, got something else.",
    
    // SyntaxError.
    "prefix increment/decrement may not have eval or arguments operand in strict mode.",
    "postfix increment/decrement may not have eval or arguments operand in strict mode.",
    "unqualified identifier cannot be deleted in strict mode.",
    "cannot delete property '%s' of '%s'.",
    "assignment to eval or arguments is not allowed in strict mode.",
    "could not compile regular expression at offset %d: %s.",
    "could not examine regular expression: /%s/.",
    "illegal flag '%c' in regular expression.",
    "duplicate flag '%c' in regular expression.",
    "illegal formal parameter list",
    
    // TypeError.
    "cannot convert null or undefined to an object.",
    "object cannot be converted to a primitive value.",
    "runtime error.",
    "cannot update immutable binding '%s'.",
    "cannot put property '%s'.",
    "cannot put property '%s', target property is an accessor without a setter.",
    "cannot delete property '%s'.",
    "cannot define property '%s', it is not extensible.",
    "cannot redefine property '%s'.",
    "caller property of an object cannot be accessed in strict mode.",
    "expected object in function instanceof check.",
    "cannot declare variable '%s'.",
    "object is not a function.",
    "element is not an object.",
    "expected %s value or object.",
    "cannot put value.",
    "cannot read default value.",
    "flags cannot be specified when specifying a RegExp object pattern.",
    "getter is not callable or undefined, cannot convert object to property.",
    "setter is not callable or undefined, cannot convert object to property.",
    "built-in objects cannot be used as constructors.",
    "specified parameter is not callable.",
    "object is not callable",
    "cannot reduce without an accumulator or initial value.",

    // UriError.
    "bad uri format.",
    "couldn't encode string in uri format."
};

String es_get_msg(EsMessage msg)
{
    return String(es_messages_[msg]);
}

String es_fmt_msg(EsMessage msg, ...)
{
    va_list vl;
    va_start(vl, msg);
    
    va_list vl_copy;
    va_copy(vl_copy, vl);
    
    std::string str;
    
    const int char_cnt = vsnprintf(NULL,0, es_get_msg(msg).utf8().c_str(), vl);
    if (char_cnt == 0)
        return String();
    
    str.resize(char_cnt + 1);
    if (char_cnt != vsnprintf(&str[0], char_cnt + 1, es_get_msg(msg).utf8().c_str(), vl_copy))
        assert(false);
    
    // Remove the null terminator, std::string will add its own if
    // necessary.
    str.resize(char_cnt);
    
    va_end(vl);
    
    return String(str.c_str(), char_cnt);
}
