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
#include <stdarg.h>
#include "api.hh"

/*
 * Generated Functions for Test Harness
 */
ES_API_FUN(es_std_data_prop_attr_are_correct);
ES_API_FUN(es_std_accessor_prop_attr_are_correct);

EsValue es_new_std_data_prop_attr_are_correct_function(EsLexicalEnvironment *scope);
EsValue es_new_std_accessor_prop_attr_are_correct_function(EsLexicalEnvironment *scope);
