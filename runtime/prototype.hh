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

class EsObject;

/**
 * Creates all built-in prototype objects. This function should only be
 * called once at startup.
 */
void es_proto_create();

/**
 * Initializes all built-in prototype objects. This function should only be
 * called once at startup.
 */
void es_proto_init();

/**
 * @pre es_proto_init() has been called.
 * @return Default object prototype object.
 */
EsObject *es_proto_obj();

/**
 * @pre es_proto_init() has been called.
 * @return Default function prototype object.
 */
EsObject *es_proto_fun();

/**
 * @pre es_proto_init() has been called.
 * @return Default array prototype object.
 */
EsObject *es_proto_arr();

/**
 * @pre es_proto_init() has been called.
 * @return Default date prototype object.
 */
EsObject *es_proto_date();

/**
 * @pre es_proto_init() has been called.
 * @return Default boolean prototype object.
 */
EsObject *es_proto_bool();

/**
 * @pre es_proto_init() has been called.
 * @return Default number prototype object.
 */
EsObject *es_proto_num();

/**
 * @pre es_proto_init() has been called.
 * @return Default string prototype object.
 */
EsObject *es_proto_str();

/**
 * @pre es_proto_init() has been called.
 * @return Default regular expression prototype object.
 */
EsObject *es_proto_reg_exp();

/**
 * @pre es_proto_init() has been called.
 * @return Default error prototype object.
 */
EsObject *es_proto_err();

/**
 * @pre es_proto_init() has been called.
 * @return Default native evaluation error prototype object.
 */
EsObject *es_proto_eval_err();

/**
 * @pre es_proto_init() has been called.
 * @return Default native range error prototype object.
 */
EsObject *es_proto_range_err();

/**
 * @pre es_proto_init() has been called.
 * @return Default native reference error prototype object.
 */
EsObject *es_proto_ref_err();

/**
 * @pre es_proto_init() has been called.
 * @return Default native syntax error prototype object.
 */
EsObject *es_proto_syntax_err();

/**
 * @pre es_proto_init() has been called.
 * @return Default native type error prototype object.
 */
EsObject *es_proto_type_err();

/**
 * @pre es_proto_init() has been called.
 * @return Default native URI error prototype object.
 */
EsObject *es_proto_uri_err();
