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
#include <cassert>
#include <functional>
#include <stdint.h>
#include "common/cast.hh"
#include "common/conversion.hh"
#include "common/string.hh"
#include "resources.hh"

/**
 * @brief Represents key, identifying a property.
 *
 * In ECMA-262 5, all properties are identified by strings. For optimization
 * purposes it can however be beneficial to treat integer properties (property
 * names that are integers) differently.
 *
 * This class represents a property key that can be either a string or an
 * integer.
 */
class EsPropertyKey
{
public:
    friend class EsPropertyKeySet;

    /**
     * Property key hash comparator.
     */
    struct Hash
    {
        inline size_t operator()(const EsPropertyKey &key) const
        {
            return std::hash<uint64_t>()(key.id_);
        }
    };

    /**
     * Property "less" comparator.
     */
    struct Less
    {
        inline size_t operator()(const EsPropertyKey &key1, const EsPropertyKey &key2) const
        {
            return key1.id_ < key2.id_;
        }
    };

private:
    /**
     * @brief Property key flags.
     */
    enum Flags
    {
        /** The property keys is an index (default). */
        IS_INDEX  = 0x0000000000000000,
        /** The property key is a string identifier. */
        IS_STRING = 0x8000000000000000,
    };

private:
    uint64_t id_;

    inline EsPropertyKey(uint64_t id)
        : id_(id) {}

public:
    inline EsPropertyKey()
        : id_(0) {}

    inline static EsPropertyKey from_raw(uint64_t id)
    {
        return EsPropertyKey(id);
    }

    static EsPropertyKey from_str(const String &str);
    static EsPropertyKey from_u32(uint32_t i);

    uint64_t as_raw() const
    {
        return id_;
    }

    /**
     * Converts the property key into a string. If the property key is a
     * string, its natural string value will be returned. If the property key
     * is an index, the index will be converted into a string.
     * @return Property key in string format.
     */
    String to_string() const;

    inline bool is_string() const
    {
        return (id_ & IS_STRING) != 0;
    }

    inline bool is_index() const
    {
        return (id_ & IS_STRING) == 0;
    }

    inline const String &as_string() const
    {
        assert(is_string());
        return strings().lookup(id_ & 0xffffffff);
    }

    inline uint32_t as_index() const
    {
        assert(is_index());
        assert(id_ <= 0xffffffff);
        return static_cast<uint32_t>(id_);
    }

    inline bool operator==(const EsPropertyKey &rhs) const
    {
        return id_ == rhs.id_;
    }

    inline bool operator!=(const EsPropertyKey &rhs) const
    {
        return id_ != rhs.id_;
    }
};

/**
 * @brief Set of property keys.
 */
class EsPropertyKeySet
{
public:
    EsPropertyKey apply;
    EsPropertyKey arguments;
    EsPropertyKey abs;
    EsPropertyKey acos;
    EsPropertyKey asin;
    EsPropertyKey atan;
    EsPropertyKey atan2;
    EsPropertyKey bind;
    EsPropertyKey call;
    EsPropertyKey callee;
    EsPropertyKey caller;
    EsPropertyKey ceil;
    EsPropertyKey char_at;
    EsPropertyKey char_code_at;
    EsPropertyKey concat;
    EsPropertyKey configurable;
    EsPropertyKey constructor;
    EsPropertyKey cos;
    EsPropertyKey create;
    EsPropertyKey decode_uri;
    EsPropertyKey decode_uri_component;
    EsPropertyKey define_properties;
    EsPropertyKey define_property;
    EsPropertyKey e;
    EsPropertyKey encode_uri;
    EsPropertyKey encode_uri_component;
    EsPropertyKey enumerable;
    EsPropertyKey eval;
    EsPropertyKey every;
    EsPropertyKey exec;
    EsPropertyKey exp;
    EsPropertyKey filter;
    EsPropertyKey floor;
    EsPropertyKey for_each;
    EsPropertyKey freeze;
    EsPropertyKey from_char_code;
    EsPropertyKey get_date;
    EsPropertyKey get_day;
    EsPropertyKey get_full_year;
    EsPropertyKey get;
    EsPropertyKey get_hours;
    EsPropertyKey get_milliseconds;
    EsPropertyKey get_minutes;
    EsPropertyKey get_month;
    EsPropertyKey get_own_property_descriptor;
    EsPropertyKey get_own_property_names;
    EsPropertyKey get_prototype_of;
    EsPropertyKey get_seconds;
    EsPropertyKey get_time;
    EsPropertyKey get_timezone_offset;
    EsPropertyKey get_utc_date;
    EsPropertyKey get_utc_day;
    EsPropertyKey get_utc_full_year;
    EsPropertyKey get_utc_hours;
    EsPropertyKey get_utc_milliseconds;
    EsPropertyKey get_utc_minutes;
    EsPropertyKey get_utc_month;
    EsPropertyKey get_utc_seconds;
    EsPropertyKey global;
    EsPropertyKey has_own_property;
    EsPropertyKey ignore_case;
    EsPropertyKey index;
    EsPropertyKey index_of;
    EsPropertyKey infinity;
    EsPropertyKey input;
    EsPropertyKey is_array;
    EsPropertyKey is_extensible;
    EsPropertyKey is_finite;
    EsPropertyKey is_frozen;
    EsPropertyKey is_nan;
    EsPropertyKey is_prototype_of;
    EsPropertyKey is_sealed;
    EsPropertyKey join;
    EsPropertyKey keys;
    EsPropertyKey last_index;
    EsPropertyKey last_index_of;
    EsPropertyKey length;
    EsPropertyKey ln10;
    EsPropertyKey ln2;
    EsPropertyKey locale_compare;
    EsPropertyKey log;
    EsPropertyKey log10e;
    EsPropertyKey log2e;
    EsPropertyKey map;
    EsPropertyKey match;
    EsPropertyKey max;
    EsPropertyKey max_value;
    EsPropertyKey message;
    EsPropertyKey min;
    EsPropertyKey min_value;
    EsPropertyKey multiline;
    EsPropertyKey name;
    EsPropertyKey nan;
    EsPropertyKey negative_infinity;
    EsPropertyKey now;
    EsPropertyKey parse;
    EsPropertyKey parse_float;
    EsPropertyKey parse_int;
    EsPropertyKey pi;
    EsPropertyKey pop;
    EsPropertyKey positive_infinity;
    EsPropertyKey pow;
    EsPropertyKey prevent_extensions;
    EsPropertyKey property_is_enumerable;
    EsPropertyKey prototype;
    EsPropertyKey push;
    EsPropertyKey random;
    EsPropertyKey reduce;
    EsPropertyKey reduce_right;
    EsPropertyKey replace;
    EsPropertyKey reverse;
    EsPropertyKey round;
    EsPropertyKey seal;
    EsPropertyKey search;
    EsPropertyKey set_date;
    EsPropertyKey set_full_year;
    EsPropertyKey set_hours;
    EsPropertyKey set_milliseconds;
    EsPropertyKey set_minutes;
    EsPropertyKey set_month;
    EsPropertyKey set_seconds;
    EsPropertyKey set;
    EsPropertyKey set_time;
    EsPropertyKey set_utc_date;
    EsPropertyKey set_utc_full_year;
    EsPropertyKey set_utc_hours;
    EsPropertyKey set_utc_milliseconds;
    EsPropertyKey set_utc_minutes;
    EsPropertyKey set_utc_month;
    EsPropertyKey set_utc_seconds;
    EsPropertyKey shift;
    EsPropertyKey sin;
    EsPropertyKey slice;
    EsPropertyKey some;
    EsPropertyKey sort;
    EsPropertyKey source;
    EsPropertyKey splice;
    EsPropertyKey split;
    EsPropertyKey sqrt;
    EsPropertyKey sqrt1_2;
    EsPropertyKey sqrt2;
    EsPropertyKey stringify;
    EsPropertyKey substr;
    EsPropertyKey substring;
    EsPropertyKey tan;
    EsPropertyKey test;
    EsPropertyKey to_date_string;
    EsPropertyKey to_exponential;
    EsPropertyKey to_fixed;
    EsPropertyKey to_iso_string;
    EsPropertyKey to_json;
    EsPropertyKey to_locale_date_string;
    EsPropertyKey to_locale_lower_case;
    EsPropertyKey to_locale_string;
    EsPropertyKey to_locale_time_string;
    EsPropertyKey to_locale_upper_case;
    EsPropertyKey to_lower_case;
    EsPropertyKey to_precision;
    EsPropertyKey to_string;
    EsPropertyKey to_time_string;
    EsPropertyKey to_upper_case;
    EsPropertyKey to_utc_string;
    EsPropertyKey trim;
    EsPropertyKey undefined;
    EsPropertyKey unshift;
    EsPropertyKey utc;
    EsPropertyKey value;
    EsPropertyKey value_of;
    EsPropertyKey writable;

    void initialize();
};

extern EsPropertyKeySet property_keys;
