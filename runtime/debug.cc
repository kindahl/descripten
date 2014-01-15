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

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.hh"

void print_stack_trace()
{
    void *callstack[128];
    int i, frames = backtrace(callstack, 128);

    char ** strs = backtrace_symbols(callstack, frames);
    
    for (i = 0; i < frames; ++i)
        printf("%s\n", strs[i]);

    free(strs);
}
