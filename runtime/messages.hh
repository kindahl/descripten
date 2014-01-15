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

enum EsMessage
{
    // RangeError.
    ES_MSG_RANGE_INVALID_ARRAY,
    ES_MSG_RANGE_RADIX,
    ES_MSG_RANGE_FRAC_DIGITS,
    ES_MSG_RANGE_INFINITE_DATE,
    ES_MSG_RANGE_PRECISION,

    // ReferenceError.
    ES_MSG_REF_NOT_DEFINED,
    ES_MSG_REF_UNRESOLVABLE,
    ES_MSG_REF_IS_NOT,

    // SyntaxError.
    ES_MSG_SYNTAX_PREFIX,
    ES_MSG_SYNTAX_POSTFIX,
    ES_MSG_SYNTAX_DELETE_UNRESOLVABLE,
    ES_MSG_SYNTAX_DELETE_PROP,
    ES_MSG_SYNTAX_ASSIGN,
    ES_MSG_SYNTAX_REGEXP_COMPILE,
    ES_MSG_SYNTAX_REGEXP_EXAMINE,
    ES_MSG_SYNTAX_REGEXP_ILLEGAL_FLAG,
    ES_MSG_SYNTAX_REGEXP_DUPLICATE_FLAG,
    ES_MSG_SYNTAX_FUN_PARAM,

    // TypeError.
    ES_MSG_TYPE_NULL_UNDEF_TO_OBJ,
    ES_MSG_TYPE_OBJ_TO_PRIMITIVE,
    ES_MSG_TYPE_RUNTIME_ERR,
    ES_MSG_TYPE_NONMUTABLE,
    ES_MSG_TYPE_PROP_PUT,
    ES_MSG_TYPE_PROP_PUT_NO_SETTER,
    ES_MSG_TYPE_PROP_DELETE,
    ES_MSG_TYPE_PROP_DEF_NO_EXT,
    ES_MSG_TYPE_PROP_DEF,
    ES_MSG_TYPE_PROP_CALLER,
    ES_MSG_TYPE_INST_OBJ,
    ES_MSG_TYPE_DECL,
    ES_MSG_TYPE_NO_FUN,
    ES_MSG_TYPE_NO_OBJ,
    ES_MSG_TYPE_WRONG_TYPE,
    ES_MSG_TYPE_VAL_PUT,
    ES_MSG_TYPE_VAL_DEFAULT,
    ES_MSG_TYPE_REGEXP_FLAGS,
    ES_MSG_TYPE_PROP_CONV_GETTER,
    ES_MSG_TYPE_PROP_CONV_SETTER,
    ES_MSG_TYPE_BUILTIN_CONSTRUCT,
    ES_MSG_TYPE_PARAM_CALLABLE,
    ES_MSG_TYPE_CALLABLE,
    ES_MSG_TYPE_REDUCE_INIT_VAL,

    // UriError.
    ES_MSG_URI_BAD_FORMAT,
    ES_MSG_URI_ENC_FAIL
};

String es_get_msg(EsMessage msg);
String es_fmt_msg(EsMessage msg, ...);
