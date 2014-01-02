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

#define ES_PROPERTY_APPLY               _USTR("apply")
#define ES_PROPERTY_ARGUMENTS           _USTR("arguments")
#define ES_PROPERTY_ABS                 _USTR("abs")
#define ES_PROPERTY_ACOS                _USTR("acos")
#define ES_PROPERTY_ASIN                _USTR("asin")
#define ES_PROPERTY_ATAN                _USTR("atan")
#define ES_PROPERTY_ATAN2               _USTR("atan2")
#define ES_PROPERTY_BIND                _USTR("bind")
#define ES_PROPERTY_CALL                _USTR("call")
#define ES_PROPERTY_CALLEE              _USTR("callee")
#define ES_PROPERTY_CALLER              _USTR("caller")
#define ES_PROPERTY_CEIL                _USTR("ceil")
#define ES_PROPERTY_CHARAT              _USTR("charAt")
#define ES_PROPERTY_CHARCODEAT          _USTR("charCodeAt")
#define ES_PROPERTY_CONCAT              _USTR("concat")
#define ES_PROPERTY_CONFIGURABLE        _USTR("configurable")
#define ES_PROPERTY_CONSTRUCTOR         _USTR("constructor")
#define ES_PROPERTY_COS                 _USTR("cos")
#define ES_PROPERTY_CREATE              _USTR("create")
#define ES_PROPERTY_DECODEURI           _USTR("decodeURI")
#define ES_PROPERTY_DECODEURICOMPONENT  _USTR("decodeURIComponent")
#define ES_PROPERTY_DEFINEPROPERTIES    _USTR("defineProperties")
#define ES_PROPERTY_DEFINEPROPERTY      _USTR("defineProperty")
#define ES_PROPERTY_E                   _USTR("E")
#define ES_PROPERTY_ENCODEURI           _USTR("encodeURI")
#define ES_PROPERTY_ENCODEURICOMPONENT  _USTR("encodeURIComponent")
#define ES_PROPERTY_ENUMERABLE          _USTR("enumerable")
#define ES_PROPERTY_EVAL                _USTR("eval")
#define ES_PROPERTY_EVERY               _USTR("every")
#define ES_PROPERTY_EXEC                _USTR("exec")
#define ES_PROPERTY_EXP                 _USTR("exp")
#define ES_PROPERTY_FILTER              _USTR("filter")
#define ES_PROPERTY_FLOOR               _USTR("floor")
#define ES_PROPERTY_FOREACH             _USTR("forEach")
#define ES_PROPERTY_FREEZE              _USTR("freeze")
#define ES_PROPERTY_FROMCHARCODE        _USTR("fromCharCode")
#define ES_PROPERTY_GETDATE             _USTR("getDate")
#define ES_PROPERTY_GETDAY              _USTR("getDay")
#define ES_PROPERTY_GETFULLYEAR         _USTR("getFullYear")
#define ES_PROPERTY_GET                 _USTR("get")
#define ES_PROPERTY_GETHOURS            _USTR("getHours")
#define ES_PROPERTY_GETMILLISECONDS     _USTR("getMilliseconds")
#define ES_PROPERTY_GETMINUTES          _USTR("getMinutes")
#define ES_PROPERTY_GETMONTH            _USTR("getMonth")
#define ES_PROPERTY_GETOWNPROPDESC      _USTR("getOwnPropertyDescriptor")
#define ES_PROPERTY_GETOWNPROPNAMES     _USTR("getOwnPropertyNames")
#define ES_PROPERTY_GETPROTOTYPEOF      _USTR("getPrototypeOf")
#define ES_PROPERTY_GETSECONDS          _USTR("getSeconds")
#define ES_PROPERTY_GETTIME             _USTR("getTime")
#define ES_PROPERTY_GETTIMEZONEOFFSET   _USTR("getTimezoneOffset")
#define ES_PROPERTY_GETUTCDATE          _USTR("getUTCDate")
#define ES_PROPERTY_GETUTCDAY           _USTR("getUTCDay")
#define ES_PROPERTY_GETUTCFULLYEAR      _USTR("getUTCFullYear")
#define ES_PROPERTY_GETUTCHOURS         _USTR("getUTCHours")
#define ES_PROPERTY_GETUTCMILLISECONDS  _USTR("getUTCMilliseconds")
#define ES_PROPERTY_GETUTCMINUTES       _USTR("getUTCMinutes")
#define ES_PROPERTY_GETUTCMONTH         _USTR("getUTCMonth")
#define ES_PROPERTY_GETUTCSECONDS       _USTR("getUTCSeconds")
#define ES_PROPERTY_GLOBAL              _USTR("global")
#define ES_PROPERTY_HASOWNPROPERTY      _USTR("hasOwnProperty")
#define ES_PROPERTY_IGNORECASE          _USTR("ignoreCase")
#define ES_PROPERTY_INDEX               _USTR("index")
#define ES_PROPERTY_INDEXOF             _USTR("indexOf")
#define ES_PROPERTY_INFINITY            _USTR("infinity")
#define ES_PROPERTY_INPUT               _USTR("input")
#define ES_PROPERTY_ISARRAY             _USTR("isArray")
#define ES_PROPERTY_ISEXTENSIBLE        _USTR("isExtensible")
#define ES_PROPERTY_ISFINITE            _USTR("isFinite")
#define ES_PROPERTY_ISFROZEN            _USTR("isFrozen")
#define ES_PROPERTY_ISNAN               _USTR("isNaN")
#define ES_PROPERTY_ISPROTOTYPEOF       _USTR("isPrototypeOf")
#define ES_PROPERTY_ISSEALED            _USTR("isSealed")
#define ES_PROPERTY_JOIN                _USTR("join")
#define ES_PROPERTY_KEYS                _USTR("keys")
#define ES_PROPERTY_LASTINDEX           _USTR("lastIndex")
#define ES_PROPERTY_LASTINDEXOF         _USTR("lastIndexOf")
#define ES_PROPERTY_LENGTH              _USTR("length")
#define ES_PROPERTY_LN10                _USTR("LN10")
#define ES_PROPERTY_LN2                 _USTR("LN2")
#define ES_PROPERTY_LOCALECOMPARE       _USTR("localeCompare")
#define ES_PROPERTY_LOG                 _USTR("log")
#define ES_PROPERTY_LOG10E              _USTR("LOG10E")
#define ES_PROPERTY_LOG2E               _USTR("LOG2E")
#define ES_PROPERTY_MAP                 _USTR("map")
#define ES_PROPERTY_MATCH               _USTR("match")
#define ES_PROPERTY_MAX                 _USTR("max")
#define ES_PROPERTY_MAXVALUE            _USTR("MAX_VALUE")
#define ES_PROPERTY_MESSAGE             _USTR("message")
#define ES_PROPERTY_MIN                 _USTR("min")
#define ES_PROPERTY_MINVALUE            _USTR("MIN_VALUE")
#define ES_PROPERTY_MULTILINE           _USTR("multiline")
#define ES_PROPERTY_NAME                _USTR("name")
#define ES_PROPERTY_NAN                 _USTR("NaN")
#define ES_PROPERTY_NEGATIVEINFINITY    _USTR("NEGATIVE_INFINITY")
#define ES_PROPERTY_NOW                 _USTR("now")
#define ES_PROPERTY_PARSE               _USTR("parse")
#define ES_PROPERTY_PARSEFLOAT          _USTR("parseFloat")
#define ES_PROPERTY_PARSEINT            _USTR("parseInt")
#define ES_PROPERTY_PI                  _USTR("PI")
#define ES_PROPERTY_POP                 _USTR("pop")
#define ES_PROPERTY_POSITIVEINFINITY    _USTR("POSITIVE_INFINITY")
#define ES_PROPERTY_POW                 _USTR("pow")
#define ES_PROPERTY_PREVENTEXTS         _USTR("preventExtensions")
#define ES_PROPERTY_PROPERYISENUMERABLE _USTR("propertyIsEnumerable")
#define ES_PROPERTY_PROTOTYPE           _USTR("prototype")
#define ES_PROPERTY_PUSH                _USTR("push")
#define ES_PROPERTY_RANDOM              _USTR("random")
#define ES_PROPERTY_REDUCE              _USTR("reduce")
#define ES_PROPERTY_REDUCERIGHT         _USTR("reduceRight")
#define ES_PROPERTY_REPLACE             _USTR("replace")
#define ES_PROPERTY_REVERSE             _USTR("reverse")
#define ES_PROPERTY_ROUND               _USTR("round")
#define ES_PROPERTY_SEAL                _USTR("seal")
#define ES_PROPERTY_SEARCH              _USTR("search")
#define ES_PROPERTY_SETDATE             _USTR("setDate")
#define ES_PROPERTY_SETFULLYEAR         _USTR("setFullYear")
#define ES_PROPERTY_SETHOURS            _USTR("setHours")
#define ES_PROPERTY_SETMILLISECONDS     _USTR("setMilliseconds")
#define ES_PROPERTY_SETMINUTES          _USTR("setMinutes")
#define ES_PROPERTY_SETMONTH            _USTR("setMonth")
#define ES_PROPERTY_SETSECONDS          _USTR("setSeconds")
#define ES_PROPERTY_SET                 _USTR("set")
#define ES_PROPERTY_SETTIME             _USTR("setTime")
#define ES_PROPERTY_SETUTCDATE          _USTR("setUTCDate")
#define ES_PROPERTY_SETUTCFULLYEAR      _USTR("setUTCFullYear")
#define ES_PROPERTY_SETUTCHOURS         _USTR("setUTCHours")
#define ES_PROPERTY_SETUTCMILLISECONDS  _USTR("setUTCMilliseconds")
#define ES_PROPERTY_SETUTCMINUTES       _USTR("setUTCMinutes")
#define ES_PROPERTY_SETUTCMONTH         _USTR("setUTCMonth")
#define ES_PROPERTY_SETUTCSECONDS       _USTR("setUTCSeconds")
#define ES_PROPERTY_SHIFT               _USTR("shift")
#define ES_PROPERTY_SIN                 _USTR("sin")
#define ES_PROPERTY_SLICE               _USTR("slice")
#define ES_PROPERTY_SOME                _USTR("some")
#define ES_PROPERTY_SORT                _USTR("sort")
#define ES_PROPERTY_SOURCE              _USTR("source")
#define ES_PROPERTY_SPLICE              _USTR("splice")
#define ES_PROPERTY_SPLIT               _USTR("split")
#define ES_PROPERTY_SQRT                _USTR("sqrt")
#define ES_PROPERTY_SQRT2               _USTR("SQRT2")
#define ES_PROPERTY_SQRT1_2             _USTR("SQRT1_2")
#define ES_PROPERTY_STRINGIFY           _USTR("stringify")
#define ES_PROPERTY_SUBSTR              _USTR("substr")
#define ES_PROPERTY_SUBSTRING           _USTR("substring")
#define ES_PROPERTY_TAN                 _USTR("tan")
#define ES_PROPERTY_TEST                _USTR("test")
#define ES_PROPERTY_TODATESTRING        _USTR("toDateString")
#define ES_PROPERTY_TOEXPONENTIAL       _USTR("toExponential")
#define ES_PROPERTY_TOFIXED             _USTR("toFixed")
#define ES_PROPERTY_TOISOSTRING         _USTR("toISOString")
#define ES_PROPERTY_TOJSON              _USTR("toJSON")
#define ES_PROPERTY_TOLOCALEDATESTRING  _USTR("toLocaleDateString")
#define ES_PROPERTY_TOLOCALELOWERCASE   _USTR("toLocaleLowerCase")
#define ES_PROPERTY_TOLOCALESTRING      _USTR("toLocaleString")
#define ES_PROPERTY_TOLOCALETIMESTRING  _USTR("toLocaleTimeString")
#define ES_PROPERTY_TOLOCALEUPPERCASE   _USTR("toLocaleUpperCase")
#define ES_PROPERTY_TOLOWERCASE         _USTR("toLowerCase")
#define ES_PROPERTY_TOPRECISION         _USTR("toPrecision")
#define ES_PROPERTY_TOSTRING            _USTR("toString")
#define ES_PROPERTY_TOTIMESTRING        _USTR("toTimeString")
#define ES_PROPERTY_TOUPPERCASE         _USTR("toUpperCase")
#define ES_PROPERTY_TOUTCSTRING         _USTR("toUTCString")
#define ES_PROPERTY_TRIM                _USTR("trim")
#define ES_PROPERTY_UNDEFINED           _USTR("undefined")
#define ES_PROPERTY_UNSHIFT             _USTR("unshift")
#define ES_PROPERTY_UTC                 _USTR("UTC")
#define ES_PROPERTY_VALUE               _USTR("value")
#define ES_PROPERTY_VALUEOF             _USTR("valueOf")
#define ES_PROPERTY_WRITABLE            _USTR("writable")

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
