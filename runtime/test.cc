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

#include <cassert>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <gc_cpp.h>
#include "common/cast.hh"
#include "common/exception.hh"
#include "common/lexical.hh"
#include "common/unicode.hh"
#include "parser/parser.hh"
#include "algorithm.hh"
#include "context.hh"
#include "conversion.hh"
#include "date.hh"
#include "error.hh"
#include "global.hh"
#include "messages.hh"
#include "native.hh"
#include "operation.h"
#include "property.hh"
#include "prototype.hh"
#include "test.hh"
#include "utility.hh"
#include "unique.hh"
#include "uri.hh"

/*
 * Compiled from:
 * test262/external/contributions/Microsoft/ietestcenter_build_2011/harness/sta.js
 */

ES_API_FUN(es_std_data_prop_attr_are_correct)
{
    // FIXME: Regenerate.
    return true;
}

ES_API_FUN(es_std_accessor_prop_attr_are_correct)
{
    // FIXME: Regenerate.
    return true;
}

EsValue es_new_std_data_prop_attr_are_correct_function(EsLexicalEnvironment *scope)
{
    EsFunction *obj = EsFunction::create_inst(scope, es_std_data_prop_attr_are_correct, false, 6);
    if (!obj)
        THROW(MemoryException);

    return EsValue::from_obj(obj);
}

EsValue es_new_std_accessor_prop_attr_are_correct_function(EsLexicalEnvironment *scope)
{
    EsFunction *obj = EsFunction::create_inst(scope, es_std_accessor_prop_attr_are_correct, false, 7);
    if (!obj)
        THROW(MemoryException);

    return EsValue::from_obj(obj);
}
