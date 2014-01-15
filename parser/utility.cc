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

#include "utility.hh"

namespace parser {

void merge_asts(FunctionLiteral *dst, FunctionLiteral *src)
{
    assert(src->parameters().size() == 0);

    StatementVector::iterator it_stmt;
    for (it_stmt = src->body().begin(); it_stmt != src->body().end(); ++it_stmt)
        dst->push_back(*it_stmt);

    DeclarationVector::iterator it_decl;
    for (it_decl = src->declarations().begin(); it_decl != src->declarations().end(); ++it_decl)
        dst->push_decl(*it_decl);
}

}
