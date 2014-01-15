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
#include <gc_cpp.h>
#include "error.hh"
#include "object.hh"
#include "prototype.hh"
#include "standard.hh"

EsObject *proto_obj_ = NULL;
EsObject *proto_fun_ = NULL;
EsObject *proto_arr_ = NULL;
EsObject *proto_date_ = NULL;
EsObject *proto_bool_ = NULL;
EsObject *proto_num_ = NULL;
EsObject *proto_str_ = NULL;
EsObject *proto_reg_exp_ = NULL;
EsObject *proto_err_ = NULL;
EsObject *proto_eval_err_ = NULL;
EsObject *proto_range_err_ = NULL;
EsObject *proto_ref_err_ = NULL;
EsObject *proto_syntax_err_ = NULL;
EsObject *proto_type_err_ = NULL;
EsObject *proto_uri_err_ = NULL;

void es_proto_create()
{
    assert(proto_obj_ == NULL);
    assert(proto_fun_ == NULL);
    assert(proto_arr_ == NULL);
    assert(proto_date_ == NULL);
    assert(proto_bool_ == NULL);
    assert(proto_num_ == NULL);
    assert(proto_str_ == NULL);
    assert(proto_reg_exp_ == NULL);
    assert(proto_err_ == NULL);
    assert(proto_eval_err_ == NULL);
    assert(proto_range_err_ == NULL);
    assert(proto_ref_err_ == NULL);
    assert(proto_syntax_err_ == NULL);
    assert(proto_type_err_ == NULL);
    assert(proto_uri_err_ == NULL);
    
    proto_obj_ = EsObject::create_raw();
    proto_fun_ = EsFunction::create_raw();
    proto_arr_ = EsArray::create_raw();
    proto_date_ = EsDate::create_raw();
    proto_bool_ = EsBooleanObject::create_raw();
    proto_num_ = EsNumberObject::create_raw();
    proto_str_ = EsStringObject::create_raw();
    proto_reg_exp_ = EsRegExp::create_raw();
    proto_err_ = EsError::create_raw();
    proto_eval_err_ = EsEvalError::create_raw();
    proto_range_err_ = EsRangeError::create_raw();
    proto_ref_err_ = EsReferenceError::create_raw();
    proto_syntax_err_ = EsSyntaxError::create_raw();
    proto_type_err_ = EsTypeError::create_raw();
    proto_uri_err_ = EsUriError::create_raw();
}

void es_proto_init()
{
    proto_obj_->make_proto();
    proto_fun_->make_proto();
    proto_arr_->make_proto();
    proto_date_->make_proto();
    proto_bool_->make_proto();
    proto_num_->make_proto();
    proto_str_->make_proto();
    proto_reg_exp_->make_proto();
    proto_err_->make_proto();
    proto_eval_err_->make_proto();
    proto_range_err_->make_proto();
    proto_ref_err_->make_proto();
    proto_syntax_err_->make_proto();
    proto_type_err_->make_proto();
    proto_uri_err_->make_proto();
}

EsObject *es_proto_obj()
{
    assert(proto_obj_);
    return proto_obj_;
}

EsObject *es_proto_fun()
{
    assert(proto_fun_);
    return proto_fun_;
}

EsObject *es_proto_arr()
{
    assert(proto_arr_);
    return proto_arr_;
}

EsObject *es_proto_date()
{
    assert(proto_date_);
    return proto_date_;
}

EsObject *es_proto_bool()
{
    assert(proto_bool_);
    return proto_bool_;
}

EsObject *es_proto_num()
{
    assert(proto_num_);
    return proto_num_;
}

EsObject *es_proto_str()
{
    assert(proto_str_);
    return proto_str_;
}

EsObject *es_proto_reg_exp()
{
    assert(proto_reg_exp_);
    return proto_reg_exp_;
}

EsObject *es_proto_err()
{
    assert(proto_err_);
    return proto_err_;
}

EsObject *es_proto_eval_err()
{
    assert(proto_eval_err_);
    return proto_eval_err_;
}

EsObject *es_proto_range_err()
{
    assert(proto_range_err_);
    return proto_range_err_;
}

EsObject *es_proto_ref_err()
{
    assert(proto_ref_err_);
    return proto_ref_err_;
}

EsObject *es_proto_syntax_err()
{
    assert(proto_syntax_err_);
    return proto_syntax_err_;
}

EsObject *es_proto_type_err()
{
    assert(proto_type_err_);
    return proto_type_err_;
}

EsObject *es_proto_uri_err()
{
    assert(proto_uri_err_);
    return proto_uri_err_;
}
