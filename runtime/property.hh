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
#include "common/maybe.hh"
#include "value.hh"

#define ES_PROPERTY_APPLY               _ESTR("apply")
#define ES_PROPERTY_ARGUMENTS           _ESTR("arguments")
#define ES_PROPERTY_ABS                 _ESTR("abs")
#define ES_PROPERTY_ACOS                _ESTR("acos")
#define ES_PROPERTY_ASIN                _ESTR("asin")
#define ES_PROPERTY_ATAN                _ESTR("atan")
#define ES_PROPERTY_ATAN2               _ESTR("atan2")
#define ES_PROPERTY_BIND                _ESTR("bind")
#define ES_PROPERTY_CALL                _ESTR("call")
#define ES_PROPERTY_CALLEE              _ESTR("callee")
#define ES_PROPERTY_CALLER              _ESTR("caller")
#define ES_PROPERTY_CEIL                _ESTR("ceil")
#define ES_PROPERTY_CHARAT              _ESTR("charAt")
#define ES_PROPERTY_CHARCODEAT          _ESTR("charCodeAt")
#define ES_PROPERTY_CONCAT              _ESTR("concat")
#define ES_PROPERTY_CONFIGURABLE        _ESTR("configurable")
#define ES_PROPERTY_CONSTRUCTOR         _ESTR("constructor")
#define ES_PROPERTY_COS                 _ESTR("cos")
#define ES_PROPERTY_CREATE              _ESTR("create")
#define ES_PROPERTY_DECODEURI           _ESTR("decodeURI")
#define ES_PROPERTY_DECODEURICOMPONENT  _ESTR("decodeURIComponent")
#define ES_PROPERTY_DEFINEPROPERTIES    _ESTR("defineProperties")
#define ES_PROPERTY_DEFINEPROPERTY      _ESTR("defineProperty")
#define ES_PROPERTY_E                   _ESTR("E")
#define ES_PROPERTY_ENCODEURI           _ESTR("encodeURI")
#define ES_PROPERTY_ENCODEURICOMPONENT  _ESTR("encodeURIComponent")
#define ES_PROPERTY_ENUMERABLE          _ESTR("enumerable")
#define ES_PROPERTY_EVAL                _ESTR("eval")
#define ES_PROPERTY_EVERY               _ESTR("every")
#define ES_PROPERTY_EXEC                _ESTR("exec")
#define ES_PROPERTY_EXP                 _ESTR("exp")
#define ES_PROPERTY_FILTER              _ESTR("filter")
#define ES_PROPERTY_FLOOR               _ESTR("floor")
#define ES_PROPERTY_FOREACH             _ESTR("forEach")
#define ES_PROPERTY_FREEZE              _ESTR("freeze")
#define ES_PROPERTY_FROMCHARCODE        _ESTR("fromCharCode")
#define ES_PROPERTY_GETDATE             _ESTR("getDate")
#define ES_PROPERTY_GETDAY              _ESTR("getDay")
#define ES_PROPERTY_GETFULLYEAR         _ESTR("getFullYear")
#define ES_PROPERTY_GET                 _ESTR("get")
#define ES_PROPERTY_GETHOURS            _ESTR("getHours")
#define ES_PROPERTY_GETMILLISECONDS     _ESTR("getMilliseconds")
#define ES_PROPERTY_GETMINUTES          _ESTR("getMinutes")
#define ES_PROPERTY_GETMONTH            _ESTR("getMonth")
#define ES_PROPERTY_GETOWNPROPDESC      _ESTR("getOwnPropertyDescriptor")
#define ES_PROPERTY_GETOWNPROPNAMES     _ESTR("getOwnPropertyNames")
#define ES_PROPERTY_GETPROTOTYPEOF      _ESTR("getPrototypeOf")
#define ES_PROPERTY_GETSECONDS          _ESTR("getSeconds")
#define ES_PROPERTY_GETTIME             _ESTR("getTime")
#define ES_PROPERTY_GETTIMEZONEOFFSET   _ESTR("getTimezoneOffset")
#define ES_PROPERTY_GETUTCDATE          _ESTR("getUTCDate")
#define ES_PROPERTY_GETUTCDAY           _ESTR("getUTCDay")
#define ES_PROPERTY_GETUTCFULLYEAR      _ESTR("getUTCFullYear")
#define ES_PROPERTY_GETUTCHOURS         _ESTR("getUTCHours")
#define ES_PROPERTY_GETUTCMILLISECONDS  _ESTR("getUTCMilliseconds")
#define ES_PROPERTY_GETUTCMINUTES       _ESTR("getUTCMinutes")
#define ES_PROPERTY_GETUTCMONTH         _ESTR("getUTCMonth")
#define ES_PROPERTY_GETUTCSECONDS       _ESTR("getUTCSeconds")
#define ES_PROPERTY_GLOBAL              _ESTR("global")
#define ES_PROPERTY_HASOWNPROPERTY      _ESTR("hasOwnProperty")
#define ES_PROPERTY_IGNORECASE          _ESTR("ignoreCase")
#define ES_PROPERTY_INDEX               _ESTR("index")
#define ES_PROPERTY_INDEXOF             _ESTR("indexOf")
#define ES_PROPERTY_INFINITY            _ESTR("infinity")
#define ES_PROPERTY_INPUT               _ESTR("input")
#define ES_PROPERTY_ISARRAY             _ESTR("isArray")
#define ES_PROPERTY_ISEXTENSIBLE        _ESTR("isExtensible")
#define ES_PROPERTY_ISFINITE            _ESTR("isFinite")
#define ES_PROPERTY_ISFROZEN            _ESTR("isFrozen")
#define ES_PROPERTY_ISNAN               _ESTR("isNaN")
#define ES_PROPERTY_ISPROTOTYPEOF       _ESTR("isPrototypeOf")
#define ES_PROPERTY_ISSEALED            _ESTR("isSealed")
#define ES_PROPERTY_JOIN                _ESTR("join")
#define ES_PROPERTY_KEYS                _ESTR("keys")
#define ES_PROPERTY_LASTINDEX           _ESTR("lastIndex")
#define ES_PROPERTY_LASTINDEXOF         _ESTR("lastIndexOf")
#define ES_PROPERTY_LENGTH              _ESTR("length")
#define ES_PROPERTY_LN10                _ESTR("LN10")
#define ES_PROPERTY_LN2                 _ESTR("LN2")
#define ES_PROPERTY_LOCALECOMPARE       _ESTR("localeCompare")
#define ES_PROPERTY_LOG                 _ESTR("log")
#define ES_PROPERTY_LOG10E              _ESTR("LOG10E")
#define ES_PROPERTY_LOG2E               _ESTR("LOG2E")
#define ES_PROPERTY_MAP                 _ESTR("map")
#define ES_PROPERTY_MATCH               _ESTR("match")
#define ES_PROPERTY_MAX                 _ESTR("max")
#define ES_PROPERTY_MAXVALUE            _ESTR("MAX_VALUE")
#define ES_PROPERTY_MESSAGE             _ESTR("message")
#define ES_PROPERTY_MIN                 _ESTR("min")
#define ES_PROPERTY_MINVALUE            _ESTR("MIN_VALUE")
#define ES_PROPERTY_MULTILINE           _ESTR("multiline")
#define ES_PROPERTY_NAME                _ESTR("name")
#define ES_PROPERTY_NAN                 _ESTR("NaN")
#define ES_PROPERTY_NEGATIVEINFINITY    _ESTR("NEGATIVE_INFINITY")
#define ES_PROPERTY_NOW                 _ESTR("now")
#define ES_PROPERTY_PARSE               _ESTR("parse")
#define ES_PROPERTY_PARSEFLOAT          _ESTR("parseFloat")
#define ES_PROPERTY_PARSEINT            _ESTR("parseInt")
#define ES_PROPERTY_PI                  _ESTR("PI")
#define ES_PROPERTY_POP                 _ESTR("pop")
#define ES_PROPERTY_POSITIVEINFINITY    _ESTR("POSITIVE_INFINITY")
#define ES_PROPERTY_POW                 _ESTR("pow")
#define ES_PROPERTY_PREVENTEXTS         _ESTR("preventExtensions")
#define ES_PROPERTY_PROPERYISENUMERABLE _ESTR("propertyIsEnumerable")
#define ES_PROPERTY_PROTOTYPE           _ESTR("prototype")
#define ES_PROPERTY_PUSH                _ESTR("push")
#define ES_PROPERTY_RANDOM              _ESTR("random")
#define ES_PROPERTY_REDUCE              _ESTR("reduce")
#define ES_PROPERTY_REDUCERIGHT         _ESTR("reduceRight")
#define ES_PROPERTY_REPLACE             _ESTR("replace")
#define ES_PROPERTY_REVERSE             _ESTR("reverse")
#define ES_PROPERTY_ROUND               _ESTR("round")
#define ES_PROPERTY_SEAL                _ESTR("seal")
#define ES_PROPERTY_SEARCH              _ESTR("search")
#define ES_PROPERTY_SETDATE             _ESTR("setDate")
#define ES_PROPERTY_SETFULLYEAR         _ESTR("setFullYear")
#define ES_PROPERTY_SETHOURS            _ESTR("setHours")
#define ES_PROPERTY_SETMILLISECONDS     _ESTR("setMilliseconds")
#define ES_PROPERTY_SETMINUTES          _ESTR("setMinutes")
#define ES_PROPERTY_SETMONTH            _ESTR("setMonth")
#define ES_PROPERTY_SETSECONDS          _ESTR("setSeconds")
#define ES_PROPERTY_SET                 _ESTR("set")
#define ES_PROPERTY_SETTIME             _ESTR("setTime")
#define ES_PROPERTY_SETUTCDATE          _ESTR("setUTCDate")
#define ES_PROPERTY_SETUTCFULLYEAR      _ESTR("setUTCFullYear")
#define ES_PROPERTY_SETUTCHOURS         _ESTR("setUTCHours")
#define ES_PROPERTY_SETUTCMILLISECONDS  _ESTR("setUTCMilliseconds")
#define ES_PROPERTY_SETUTCMINUTES       _ESTR("setUTCMinutes")
#define ES_PROPERTY_SETUTCMONTH         _ESTR("setUTCMonth")
#define ES_PROPERTY_SETUTCSECONDS       _ESTR("setUTCSeconds")
#define ES_PROPERTY_SHIFT               _ESTR("shift")
#define ES_PROPERTY_SIN                 _ESTR("sin")
#define ES_PROPERTY_SLICE               _ESTR("slice")
#define ES_PROPERTY_SOME                _ESTR("some")
#define ES_PROPERTY_SORT                _ESTR("sort")
#define ES_PROPERTY_SOURCE              _ESTR("source")
#define ES_PROPERTY_SPLICE              _ESTR("splice")
#define ES_PROPERTY_SPLIT               _ESTR("split")
#define ES_PROPERTY_SQRT                _ESTR("sqrt")
#define ES_PROPERTY_SQRT2               _ESTR("SQRT2")
#define ES_PROPERTY_SQRT1_2             _ESTR("SQRT1_2")
#define ES_PROPERTY_STRINGIFY           _ESTR("stringify")
#define ES_PROPERTY_SUBSTR              _ESTR("substr")
#define ES_PROPERTY_SUBSTRING           _ESTR("substring")
#define ES_PROPERTY_TAN                 _ESTR("tan")
#define ES_PROPERTY_TEST                _ESTR("test")
#define ES_PROPERTY_TODATESTRING        _ESTR("toDateString")
#define ES_PROPERTY_TOEXPONENTIAL       _ESTR("toExponential")
#define ES_PROPERTY_TOFIXED             _ESTR("toFixed")
#define ES_PROPERTY_TOISOSTRING         _ESTR("toISOString")
#define ES_PROPERTY_TOJSON              _ESTR("toJSON")
#define ES_PROPERTY_TOLOCALEDATESTRING  _ESTR("toLocaleDateString")
#define ES_PROPERTY_TOLOCALELOWERCASE   _ESTR("toLocaleLowerCase")
#define ES_PROPERTY_TOLOCALESTRING      _ESTR("toLocaleString")
#define ES_PROPERTY_TOLOCALETIMESTRING  _ESTR("toLocaleTimeString")
#define ES_PROPERTY_TOLOCALEUPPERCASE   _ESTR("toLocaleUpperCase")
#define ES_PROPERTY_TOLOWERCASE         _ESTR("toLowerCase")
#define ES_PROPERTY_TOPRECISION         _ESTR("toPrecision")
#define ES_PROPERTY_TOSTRING            _ESTR("toString")
#define ES_PROPERTY_TOTIMESTRING        _ESTR("toTimeString")
#define ES_PROPERTY_TOUPPERCASE         _ESTR("toUpperCase")
#define ES_PROPERTY_TOUTCSTRING         _ESTR("toUTCString")
#define ES_PROPERTY_TRIM                _ESTR("trim")
#define ES_PROPERTY_UNDEFINED           _ESTR("undefined")
#define ES_PROPERTY_UNSHIFT             _ESTR("unshift")
#define ES_PROPERTY_UTC                 _ESTR("UTC")
#define ES_PROPERTY_VALUE               _ESTR("value")
#define ES_PROPERTY_VALUEOF             _ESTR("valueOf")
#define ES_PROPERTY_WRITABLE            _ESTR("writable")

class EsPropertyDescriptor;

/**
 * @brief Property class.
 */
class EsProperty final
{
private:
    // Both accessor and data property attributes.
    bool enumerable_;
    bool configurable_;

    // Data only property attributes.
    Maybe<bool> writable_;
    Maybe<EsValue> value_;

    // Accessor only property attributes.
    Maybe<EsValue> getter_;
    Maybe<EsValue> setter_;

public:
    /**
     * Creates a new data property.
     */
    inline EsProperty(bool enumerable, bool configurable, bool writable,
                      const Maybe<EsValue> &value)
        : enumerable_(enumerable)
        , configurable_(configurable)
        , writable_(writable)
        , value_(value) {}

    /**
     * Creates a new accessor property.
     */
    inline EsProperty(bool enumerable, bool configurable,
                      const Maybe<EsValue> &getter,
                      const Maybe<EsValue> &setter)
        : enumerable_(enumerable)
        , configurable_(configurable)
        , getter_(getter)
        , setter_(setter) {}

    /**
     * @return true if the property is an accessor property.
     */
    inline bool is_accessor() const
    {
        return getter_ || setter_;
    }

    /**
     * @return true if the property is a data property.
     */
    inline bool is_data() const
    {
        return value_ || writable_;
    }

    /**
     * @return true if the property is writable.
     */
    inline bool is_writable() const
    {
        return writable_ && *writable_;
    }

    /**
     * @return true if the property is enumerable.
     */
    inline bool is_enumerable() const
    {
        return enumerable_;
    }

    /**
     * @return true if the property is configurable.
     */
    inline bool is_configurable() const
    {
        return configurable_;
    }

    /**
     * Sets the [[Writable]] flag of the property.
     * @param [in] writable true to make the property writable.
     */
    inline void set_writable(bool writable)
    {
        writable_ = writable;
    }

    /**
     * Sets the [[Enumerable]] flag of the property.
     * @param [in] enumerable true to make the property enumerable.
     */
    inline void set_enumerable(bool enumerable)
    {
        enumerable_ = enumerable;
    }

    /**
     * Sets the [[Configurable]] flag of the property.
     * @param [in] configurable true to make the property configurable.
     */
    inline void set_configurable(bool configurable)
    {
        configurable_ = configurable;
    }

    /**
     * Sets the [[Value]] flag of the property.
     * @param [in] value New property value.
     */
    inline void set_value(const Maybe<EsValue> &value)
    {
        value_ = value;
    }

    /**
     * @return Value if the property has one and undefined otherwise.
     */
    inline EsValue value_or_undefined() const
    {
        return value_ ? *value_ : EsValue::undefined;
    }

    /**
     * @return Getter if the property has one and undefined otherwise.
     */
    inline EsValue getter_or_undefined() const
    {
        return getter_ ? *getter_ : EsValue::undefined;
    }

    /**
     * @return Getter if the property has one and undefined otherwise.
     */
    inline EsValue setter_or_undefined() const
    {
        return setter_ ? *setter_ : EsValue::undefined;
    }

    /**
     * Converts the property into an accessor property.
     * @pre Property is a data property.
     */
    inline void convert_to_accessor()
    {
        assert(is_data());

        writable_.clear();
        value_.clear();
        getter_.clear();
        setter_.clear();
    }

    /**
     * Converts the property into a data property.
     * @pre Property is an accessor property.
     */
    inline void convert_to_data()
    {
        assert(is_accessor());

        writable_ = false;
        value_.clear();
        getter_.clear();
        setter_.clear();
    }

    /**
     * Checks if this property contains all fields specified by the descriptor,
     * and that all the fields have the same value according to the
     * "same value algorithm" specified in 9.12.
     * @param [in] desc Right hand side property.
     */
    bool described_by(const EsPropertyDescriptor &desc) const;

    /**
     * Copies all present fields in the property descriptor into this property.
     * @param [in] desc Property descriptor to copy from.
     */
    void copy_from(const EsPropertyDescriptor &desc);
};

/**
 * @brief Property descriptor class.
 */
class EsPropertyDescriptor final
{
public:
    friend class EsProperty;

private:
    // Both accessor and data property attributes.
    Maybe<bool> enumerable_;
    Maybe<bool> configurable_;

    // Data only property attributes.
    Maybe<bool> writable_;
    Maybe<EsValue> value_;

    // Accessor only property attributes.
    Maybe<EsValue> getter_;
    Maybe<EsValue> setter_;

public:
    /**
     * Creates a new empty property descriptor.
     */
    EsPropertyDescriptor() {}

    /**
     * Creates a new data property descriptor.
     */
    inline EsPropertyDescriptor(const Maybe<bool> &enumerable,
                                const Maybe<bool> &configurable,
                                const Maybe<bool> &writable,
                                const Maybe<EsValue> &value)
        : enumerable_(enumerable)
        , configurable_(configurable)
        , writable_(writable)
        , value_(value) {}

    /**
     * Creates a new accessor property descriptor.
     */
    inline EsPropertyDescriptor(const Maybe<bool> &enumerable,
                                const Maybe<bool> &configurable,
                                const Maybe<EsValue> &getter,
                                const Maybe<EsValue> &setter)
        : enumerable_(enumerable)
        , configurable_(configurable)
        , getter_(getter)
        , setter_(setter) {}

    /**
     * @return true if the property descriptor is empty.
     */
    inline bool empty() const
    {
        return !value_ && !setter_ && !getter_ &&
            !enumerable_ && !configurable_ && !writable_;
    }

    /**
     * Instantiates a new accessor property from the descriptor.
     * @return New accessor property.
     */
    inline EsProperty create_accessor() const
    {
        return EsProperty(enumerable_ && *enumerable_,
                          configurable_ && *configurable_,
                          getter_, setter_);
    }

    /**
     * Instantiates a new data property from the descriptor.
     * @return New data property.
     */
    inline EsProperty create_data() const
    {
        return EsProperty(enumerable_ && *enumerable_,
                          configurable_ && *configurable_,
                          writable_ && *writable_, value_);
    }

    /**
     * @return true if the property descriptor describes an accessor property.
     */
    inline bool is_accessor() const
    {
        return getter_ || setter_;
    }

    /**
     * @return true if the property descriptor describes a data property.
     */
    inline bool is_data() const
    {
        return value_ || writable_;
    }

    /**
     * @return true if the descriptor is generic.
     */
    inline bool is_generic() const
    {
        return !is_accessor() && !is_data();
    }

    /**
     * @return true if the descriptor has the [[Writable]] flag set.
     */
    inline bool has_writable() const
    {
        return writable_;
    }

    /**
     * @return true if the descriptor has the [[Enumerable]] flag set.
     */
    inline bool has_enumerable() const
    {
        return enumerable_;
    }

    /**
     * @return true if the descriptor has the [[Configurable]] flag set.
     */
    inline bool has_configurable() const
    {
        return configurable_;
    }

    /**
     * @return true if the [[Writable]] flag is true.
     */
    inline bool is_writable() const
    {
        return writable_ && *writable_;
    }

    /**
     * @return true if the [[Enumerable]] flag is true.
     */
    inline bool is_enumerable() const
    {
        return enumerable_ && *enumerable_;
    }

    /**
     * @return true if the [[Configurable]] flag is true.
     */
    inline bool is_configurable() const
    {
        return configurable_ && *configurable_;
    }

    /**
     * Sets the [[Writable]] flag of the property descriptor.
     * @param [in] writable New value of the [[Writable]] flag.
     */
    inline void set_writable(bool writable)
    {
        writable_ = writable;
    }

    /**
     * Sets the [[Enumerable]] flag of the property descriptor.
     * @param [in] enumerable New value of the [[Enumerable]] flag.
     */
    inline void set_enumerable(bool enumerable)
    {
        enumerable_ = enumerable;
    }

    /**
     * Sets the [[Configurable]] flag of the property descriptor.
     * @param [in] writable New value of the [[Configurable]] flag.
     */
    inline void set_configurable(bool configurable)
    {
        configurable_ = configurable;
    }

    /**
     * @return [[Value]] flag.
     */
    inline const Maybe<EsValue> &value() const
    {
        return value_;
    }

    /**
     * @return [[Get]] flag.
     */
    inline const Maybe<EsValue> &getter() const
    {
        return getter_;
    }

    /**
     * @return [[Set]] flag.
     */
    inline const Maybe<EsValue> &setter() const
    {
        return setter_;
    }

    /**
     * Sets the [[Value]] flag of the property descriptor.
     * @param [in] value New value.
     */
    inline void set_value(const EsValue &value)
    {
        value_ = value;
    }
};
