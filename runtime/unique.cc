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

#include "common/exception.hh"
#include "api.hh"
#include "error.hh"
#include "global.hh"
#include "messages.hh"
#include "object.hh"
#include "utility.hh"
#include "unique.hh"

EsFunction *es_throw_type_err_ = NULL;

ES_API_FUN(es_throw_type_err_fun)
{
    ES_THROW(EsTypeError, es_fmt_msg(ES_MSG_TYPE_RUNTIME_ERR));
    return false;
}

EsFunction *es_throw_type_err()
{
    if (es_throw_type_err_ == NULL)
    {
        es_throw_type_err_ = EsBuiltinFunction::create_inst(es_global_env(), es_throw_type_err_fun, 0);
        es_throw_type_err_->set_extensible(false);
    }
    
    return es_throw_type_err_;
}
