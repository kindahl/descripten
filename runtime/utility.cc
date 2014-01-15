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

#include <iostream>
#include <iomanip>
#include <cassert>
#include <gc_cpp.h> // 3rd party.
#include <string.h>
#include "common/exception.hh"
#include "conversion.hh"
#include "environment.hh"
#include "object.hh"
#include "property.hh"
#include "prototype.hh"
#include "utility.hh"

bool es_as_boolean(const EsValue &val, bool &boolean)
{
    if (val.is_object())
    {
        if (EsBooleanObject *obj = dynamic_cast<EsBooleanObject *>(val.as_object()))
        {
            boolean = obj->primitive_value();
            return true;
        }
    }
    else if (val.is_boolean())
    {
        boolean = val.as_boolean();
        return true;
    }

    return false;
}

bool es_as_number(const EsValue &val, double &number)
{
    if (val.is_object())
    {
        if (EsNumberObject *obj = dynamic_cast<EsNumberObject *>(val.as_object()))
        {
            number = obj->primitive_value();
            return true;
        }
    }
    else if (val.is_number())
    {
        number = val.as_number();
        return true;
    }

    return false;
}

bool es_as_string(const EsValue &val, String &str)
{
    if (val.is_object())
    {
        if (EsStringObject *obj = dynamic_cast<EsStringObject *>(val.as_object()))
        {
            str = obj->primitive_value();
            return true;
        }
    }
    else  if (val.is_string())
    {
        str = val.as_string();
        return true;
    }

    return false;
}

bool es_as_object(const EsValue &val, EsObject *&object, const char *class_name)
{
    if (val.is_object())
    {
        EsObject *obj = val.as_object();
        if (class_name == NULL || !strcmp(class_name, obj->class_name().utf8().c_str()))
        {
            object = obj;
            return true;
        }
    }

    return false;
}

EsDate *es_as_date(const EsValue &val)
{
    if (val.is_object())
    {
        if (EsDate* date = dynamic_cast<EsDate *>(val.as_object()))
            return date;
    }

    return NULL;
}

EsRegExp *es_as_reg_exp(const EsValue &val)
{
    if (val.is_object())
    {
        if (EsRegExp* rx = dynamic_cast<EsRegExp *>(val.as_object()))
            return rx;
    }

    return NULL;
}
