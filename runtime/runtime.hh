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
#include "common/string.hh"
#include "operation.hh"
#include "value.hh"

class EsContext;

namespace runtime
{
    typedef void (*tglobal_data)();   // FIXME: No need to typedef here.
    typedef bool (*tglobal_main)(EsContext *ctx, EsFunction *callee,
                                 int argc, EsValue argv[], EsValue &result);   // FIXME: No need to typedef here.
    
    bool init(tglobal_data global_data);
    bool shutdown();
    bool run(tglobal_main global_main);
    const char *error();
}
