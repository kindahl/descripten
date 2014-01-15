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

/**
 * Expands to an API function header. Can be used for both definitions and
 * declarations.
 * @note es_self points to the function object of the function itself. es_this
 *       points to what ECMA-262 refers to as 'this'.
 */
#ifndef ES_API_FUN_HDR
#define ES_API_FUN_HDR(name) bool name(EsContext *ctx,\
                                       uint32_t argc,\
                                       EsValue *fp,\
                                       EsValue *vp)
#endif

/**
 * Expands to an API function pointer. The reason this functionality is used as
 * a macro and not a typedef is because we don't want to create an
 * Interdependency between this file and the source header where EsObject and
 * EsFunction is defined.
 * @note es_self points to the function object of the function itself. es_this
 *       points to what ECMA-262 refers to as 'this'.
 */
#ifndef ES_API_FUN_PTR
#define ES_API_FUN_PTR(name) bool (* name)(EsContext *ctx,\
                                           uint32_t argc,\
                                           EsValue *fp,\
                                           EsValue *vp)
#endif

/**
 * Convenience macro mapping to ES_API_FUN_HEADER. Should perhaps rename
 * ES_API_FUN_HEADER instead.
 */
#ifndef ES_API_FUN
#define ES_API_FUN ES_API_FUN_HDR
#endif

/**
 * Declares a function parameter.
 * @param [in] index Parameter index, first parameter has index 0.
 * @param [in] name Parameter name.
 */
#ifndef ES_API_PARAMETER
#define ES_API_PARAMETER(index, name) EsValue name = argc > (index) ? fp[(index)] : EsValue::undefined;
#endif
