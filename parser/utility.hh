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
#include "ast.hh"

namespace parser {

/**
 * Merges two ASTs, all declarations and body statements will be merged into
 * the destination tree. Function parameters will not be merged due to the
 * assumption that the program (root) function doesn't have any.
 * @param [in,out] dst The first and destination AST.
 * @param [in] src The second AST.
 */
void merge_asts(FunctionLiteral *dst, FunctionLiteral *src);

}
