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

#include "conversion.hh"
#include "property.hh"
#include "property_key.hh"

EsPropertyKeySet property_keys;

EsPropertyKey EsPropertyKey::from_str(const EsString *str)
{
    uint32_t index = 0;
    if (es_str_to_index(str->str(), index))
        return from_u32(index);

    return EsPropertyKey(static_cast<uint64_t>(IS_STRING) | strings().intern(str));
}

EsPropertyKey EsPropertyKey::from_u32(uint32_t i)
{
    return EsPropertyKey(i);
}

const EsString *EsPropertyKey::to_string() const
{
    if (is_string())
        return as_string();

    return EsString::create_from_utf8(
            lexical_cast<const char *>(as_index()));
}

void EsPropertyKeySet::initialize()
{
    apply = EsPropertyKey::from_str(ES_PROPERTY_APPLY);
    arguments = EsPropertyKey::from_str(ES_PROPERTY_ARGUMENTS);
    abs = EsPropertyKey::from_str(ES_PROPERTY_ABS);
    acos = EsPropertyKey::from_str(ES_PROPERTY_ACOS);
    asin = EsPropertyKey::from_str(ES_PROPERTY_ASIN);
    atan = EsPropertyKey::from_str(ES_PROPERTY_ATAN);
    atan2 = EsPropertyKey::from_str(ES_PROPERTY_ATAN2);
    bind = EsPropertyKey::from_str(ES_PROPERTY_BIND);
    call = EsPropertyKey::from_str(ES_PROPERTY_CALL);
    callee = EsPropertyKey::from_str(ES_PROPERTY_CALLEE);
    caller = EsPropertyKey::from_str(ES_PROPERTY_CALLER);
    ceil = EsPropertyKey::from_str(ES_PROPERTY_CEIL);
    char_at = EsPropertyKey::from_str(ES_PROPERTY_CHARAT);
    char_code_at = EsPropertyKey::from_str(ES_PROPERTY_CHARCODEAT);
    concat = EsPropertyKey::from_str(ES_PROPERTY_CONCAT);
    configurable = EsPropertyKey::from_str(ES_PROPERTY_CONFIGURABLE);
    constructor = EsPropertyKey::from_str(ES_PROPERTY_CONSTRUCTOR);
    cos = EsPropertyKey::from_str(ES_PROPERTY_COS);
    create = EsPropertyKey::from_str(ES_PROPERTY_CREATE);
    decode_uri = EsPropertyKey::from_str(ES_PROPERTY_DECODEURI);
    decode_uri_component = EsPropertyKey::from_str(ES_PROPERTY_DECODEURICOMPONENT);
    define_properties = EsPropertyKey::from_str(ES_PROPERTY_DEFINEPROPERTIES);
    define_property = EsPropertyKey::from_str(ES_PROPERTY_DEFINEPROPERTY);
    e = EsPropertyKey::from_str(ES_PROPERTY_E);
    encode_uri = EsPropertyKey::from_str(ES_PROPERTY_ENCODEURI);
    encode_uri_component = EsPropertyKey::from_str(ES_PROPERTY_ENCODEURICOMPONENT);
    enumerable = EsPropertyKey::from_str(ES_PROPERTY_ENUMERABLE);
    eval = EsPropertyKey::from_str(ES_PROPERTY_EVAL);
    every = EsPropertyKey::from_str(ES_PROPERTY_EVERY);
    exec = EsPropertyKey::from_str(ES_PROPERTY_EXEC);
    exp = EsPropertyKey::from_str(ES_PROPERTY_EXP);
    filter = EsPropertyKey::from_str(ES_PROPERTY_FILTER);
    floor = EsPropertyKey::from_str(ES_PROPERTY_FLOOR);
    for_each = EsPropertyKey::from_str(ES_PROPERTY_FOREACH);
    freeze = EsPropertyKey::from_str(ES_PROPERTY_FREEZE);
    from_char_code = EsPropertyKey::from_str(ES_PROPERTY_FROMCHARCODE);
    get_date = EsPropertyKey::from_str(ES_PROPERTY_GETDATE);
    get_day = EsPropertyKey::from_str(ES_PROPERTY_GETDAY);
    get_full_year = EsPropertyKey::from_str(ES_PROPERTY_GETFULLYEAR);
    get = EsPropertyKey::from_str(ES_PROPERTY_GET);
    get_hours = EsPropertyKey::from_str(ES_PROPERTY_GETHOURS);
    get_milliseconds = EsPropertyKey::from_str(ES_PROPERTY_GETMILLISECONDS);
    get_minutes = EsPropertyKey::from_str(ES_PROPERTY_GETMINUTES);
    get_month = EsPropertyKey::from_str(ES_PROPERTY_GETMONTH);
    get_own_property_descriptor = EsPropertyKey::from_str(ES_PROPERTY_GETOWNPROPDESC);
    get_own_property_names = EsPropertyKey::from_str(ES_PROPERTY_GETOWNPROPNAMES);
    get_prototype_of = EsPropertyKey::from_str(ES_PROPERTY_GETPROTOTYPEOF);
    get_seconds = EsPropertyKey::from_str(ES_PROPERTY_GETSECONDS);
    get_time = EsPropertyKey::from_str(ES_PROPERTY_GETTIME);
    get_timezone_offset = EsPropertyKey::from_str(ES_PROPERTY_GETTIMEZONEOFFSET);
    get_utc_date = EsPropertyKey::from_str(ES_PROPERTY_GETUTCDATE);
    get_utc_day = EsPropertyKey::from_str(ES_PROPERTY_GETUTCDAY);
    get_utc_full_year = EsPropertyKey::from_str(ES_PROPERTY_GETUTCFULLYEAR);
    get_utc_hours = EsPropertyKey::from_str(ES_PROPERTY_GETUTCHOURS);
    get_utc_milliseconds = EsPropertyKey::from_str(ES_PROPERTY_GETUTCMILLISECONDS);
    get_utc_minutes = EsPropertyKey::from_str(ES_PROPERTY_GETUTCMINUTES);
    get_utc_month = EsPropertyKey::from_str(ES_PROPERTY_GETUTCMONTH);
    get_utc_seconds = EsPropertyKey::from_str(ES_PROPERTY_GETUTCSECONDS);
    global = EsPropertyKey::from_str(ES_PROPERTY_GLOBAL);
    has_own_property = EsPropertyKey::from_str(ES_PROPERTY_HASOWNPROPERTY);
    ignore_case = EsPropertyKey::from_str(ES_PROPERTY_IGNORECASE);
    index = EsPropertyKey::from_str(ES_PROPERTY_INDEX);
    index_of = EsPropertyKey::from_str(ES_PROPERTY_INDEXOF);
    infinity = EsPropertyKey::from_str(ES_PROPERTY_INFINITY);
    input = EsPropertyKey::from_str(ES_PROPERTY_INPUT);
    is_array = EsPropertyKey::from_str(ES_PROPERTY_ISARRAY);
    is_extensible = EsPropertyKey::from_str(ES_PROPERTY_ISEXTENSIBLE);
    is_finite = EsPropertyKey::from_str(ES_PROPERTY_ISFINITE);
    is_frozen = EsPropertyKey::from_str(ES_PROPERTY_ISFROZEN);
    is_nan = EsPropertyKey::from_str(ES_PROPERTY_ISNAN);
    is_prototype_of = EsPropertyKey::from_str(ES_PROPERTY_ISPROTOTYPEOF);
    is_sealed = EsPropertyKey::from_str(ES_PROPERTY_ISSEALED);
    join = EsPropertyKey::from_str(ES_PROPERTY_JOIN);
    keys = EsPropertyKey::from_str(ES_PROPERTY_KEYS);
    last_index = EsPropertyKey::from_str(ES_PROPERTY_LASTINDEX);
    last_index_of = EsPropertyKey::from_str(ES_PROPERTY_LASTINDEXOF);
    length = EsPropertyKey::from_str(ES_PROPERTY_LENGTH);
    ln10 = EsPropertyKey::from_str(ES_PROPERTY_LN10);
    ln2 = EsPropertyKey::from_str(ES_PROPERTY_LN2);
    locale_compare = EsPropertyKey::from_str(ES_PROPERTY_LOCALECOMPARE);
    log = EsPropertyKey::from_str(ES_PROPERTY_LOG);
    log10e = EsPropertyKey::from_str(ES_PROPERTY_LOG10E);
    log2e = EsPropertyKey::from_str(ES_PROPERTY_LOG2E);
    map = EsPropertyKey::from_str(ES_PROPERTY_MAP);
    match = EsPropertyKey::from_str(ES_PROPERTY_MATCH);
    max = EsPropertyKey::from_str(ES_PROPERTY_MAX);
    max_value = EsPropertyKey::from_str(ES_PROPERTY_MAXVALUE);
    message = EsPropertyKey::from_str(ES_PROPERTY_MESSAGE);
    min = EsPropertyKey::from_str(ES_PROPERTY_MIN);
    min_value = EsPropertyKey::from_str(ES_PROPERTY_MINVALUE);
    multiline = EsPropertyKey::from_str(ES_PROPERTY_MULTILINE);
    name = EsPropertyKey::from_str(ES_PROPERTY_NAME);
    nan = EsPropertyKey::from_str(ES_PROPERTY_NAN);
    negative_infinity = EsPropertyKey::from_str(ES_PROPERTY_NEGATIVEINFINITY);
    now = EsPropertyKey::from_str(ES_PROPERTY_NOW);
    parse = EsPropertyKey::from_str(ES_PROPERTY_PARSE);
    parse_float = EsPropertyKey::from_str(ES_PROPERTY_PARSEFLOAT);
    parse_int = EsPropertyKey::from_str(ES_PROPERTY_PARSEINT);
    pi = EsPropertyKey::from_str(ES_PROPERTY_PI);
    pop = EsPropertyKey::from_str(ES_PROPERTY_POP);
    positive_infinity = EsPropertyKey::from_str(ES_PROPERTY_POSITIVEINFINITY);
    pow = EsPropertyKey::from_str(ES_PROPERTY_POW);
    prevent_extensions = EsPropertyKey::from_str(ES_PROPERTY_PREVENTEXTS);
    property_is_enumerable = EsPropertyKey::from_str(ES_PROPERTY_PROPERYISENUMERABLE);
    prototype = EsPropertyKey::from_str(ES_PROPERTY_PROTOTYPE);
    push = EsPropertyKey::from_str(ES_PROPERTY_PUSH);
    random = EsPropertyKey::from_str(ES_PROPERTY_RANDOM);
    reduce = EsPropertyKey::from_str(ES_PROPERTY_REDUCE);
    reduce_right = EsPropertyKey::from_str(ES_PROPERTY_REDUCERIGHT);
    replace = EsPropertyKey::from_str(ES_PROPERTY_REPLACE);
    reverse = EsPropertyKey::from_str(ES_PROPERTY_REVERSE);
    round = EsPropertyKey::from_str(ES_PROPERTY_ROUND);
    seal = EsPropertyKey::from_str(ES_PROPERTY_SEAL);
    search = EsPropertyKey::from_str(ES_PROPERTY_SEARCH);
    set_date = EsPropertyKey::from_str(ES_PROPERTY_SETDATE);
    set_full_year = EsPropertyKey::from_str(ES_PROPERTY_SETFULLYEAR);
    set_hours = EsPropertyKey::from_str(ES_PROPERTY_SETHOURS);
    set_milliseconds = EsPropertyKey::from_str(ES_PROPERTY_SETMILLISECONDS);
    set_minutes = EsPropertyKey::from_str(ES_PROPERTY_SETMINUTES);
    set_month = EsPropertyKey::from_str(ES_PROPERTY_SETMONTH);
    set_seconds = EsPropertyKey::from_str(ES_PROPERTY_SETSECONDS);
    set = EsPropertyKey::from_str(ES_PROPERTY_SET);
    set_time = EsPropertyKey::from_str(ES_PROPERTY_SETTIME);
    set_utc_date = EsPropertyKey::from_str(ES_PROPERTY_SETUTCDATE);
    set_utc_full_year = EsPropertyKey::from_str(ES_PROPERTY_SETUTCFULLYEAR);
    set_utc_hours = EsPropertyKey::from_str(ES_PROPERTY_SETUTCHOURS);
    set_utc_milliseconds = EsPropertyKey::from_str(ES_PROPERTY_SETUTCMILLISECONDS);
    set_utc_minutes = EsPropertyKey::from_str(ES_PROPERTY_SETUTCMINUTES);
    set_utc_month = EsPropertyKey::from_str(ES_PROPERTY_SETUTCMONTH);
    set_utc_seconds = EsPropertyKey::from_str(ES_PROPERTY_SETUTCSECONDS);
    shift = EsPropertyKey::from_str(ES_PROPERTY_SHIFT);
    sin = EsPropertyKey::from_str(ES_PROPERTY_SIN);
    slice = EsPropertyKey::from_str(ES_PROPERTY_SLICE);
    some = EsPropertyKey::from_str(ES_PROPERTY_SOME);
    sort = EsPropertyKey::from_str(ES_PROPERTY_SORT);
    source = EsPropertyKey::from_str(ES_PROPERTY_SOURCE);
    splice = EsPropertyKey::from_str(ES_PROPERTY_SPLICE);
    split = EsPropertyKey::from_str(ES_PROPERTY_SPLIT);
    sqrt = EsPropertyKey::from_str(ES_PROPERTY_SQRT);
    sqrt1_2 = EsPropertyKey::from_str(ES_PROPERTY_SQRT1_2);
    sqrt2 = EsPropertyKey::from_str(ES_PROPERTY_SQRT2);
    stringify = EsPropertyKey::from_str(ES_PROPERTY_STRINGIFY);
    substr = EsPropertyKey::from_str(ES_PROPERTY_SUBSTR);
    substring = EsPropertyKey::from_str(ES_PROPERTY_SUBSTRING);
    tan = EsPropertyKey::from_str(ES_PROPERTY_TAN);
    test = EsPropertyKey::from_str(ES_PROPERTY_TEST);
    to_date_string = EsPropertyKey::from_str(ES_PROPERTY_TODATESTRING);
    to_exponential = EsPropertyKey::from_str(ES_PROPERTY_TOEXPONENTIAL);
    to_fixed = EsPropertyKey::from_str(ES_PROPERTY_TOFIXED);
    to_iso_string = EsPropertyKey::from_str(ES_PROPERTY_TOISOSTRING);
    to_json = EsPropertyKey::from_str(ES_PROPERTY_TOJSON);
    to_locale_date_string = EsPropertyKey::from_str(ES_PROPERTY_TOLOCALEDATESTRING);
    to_locale_lower_case = EsPropertyKey::from_str(ES_PROPERTY_TOLOCALELOWERCASE);
    to_locale_string = EsPropertyKey::from_str(ES_PROPERTY_TOLOCALESTRING);
    to_locale_time_string = EsPropertyKey::from_str(ES_PROPERTY_TOLOCALETIMESTRING);
    to_locale_upper_case = EsPropertyKey::from_str(ES_PROPERTY_TOLOCALEUPPERCASE);
    to_lower_case = EsPropertyKey::from_str(ES_PROPERTY_TOLOWERCASE);
    to_precision = EsPropertyKey::from_str(ES_PROPERTY_TOPRECISION);
    to_string = EsPropertyKey::from_str(ES_PROPERTY_TOSTRING);
    to_time_string = EsPropertyKey::from_str(ES_PROPERTY_TOTIMESTRING);
    to_upper_case = EsPropertyKey::from_str(ES_PROPERTY_TOUPPERCASE);
    to_utc_string = EsPropertyKey::from_str(ES_PROPERTY_TOUTCSTRING);
    trim = EsPropertyKey::from_str(ES_PROPERTY_TRIM);
    undefined = EsPropertyKey::from_str(ES_PROPERTY_UNDEFINED);
    unshift = EsPropertyKey::from_str(ES_PROPERTY_UNSHIFT);
    utc = EsPropertyKey::from_str(ES_PROPERTY_UTC);
    value = EsPropertyKey::from_str(ES_PROPERTY_VALUE);
    value_of = EsPropertyKey::from_str(ES_PROPERTY_VALUEOF);
    writable = EsPropertyKey::from_str(ES_PROPERTY_WRITABLE);
}
