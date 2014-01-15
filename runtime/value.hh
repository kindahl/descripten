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
#include "types.hh"
#include "value_data.h"

class EsFunction;
class EsObject;
class EsString;

/**
 * @brief Holds a primitive value or a pointer to a string or an object.
 *
 * This value implementation uses NaN-boxing, favoring numbers. This means that
 * the value can always be interpreted as an IEEE 754-1985 number. All
 * non-number values are hidden within a quiet NaN value.
 *
 * IEEE 754-1985:
 *
 *      0 sign (1 bit)
 *  1..11 exponent (11 bits)
 * 12..53 fraction (52 bits)
 *
 * NaN: sign: zero or one.
 *      exponent: all ones.
 *      fraction: anything but zero bits.
 *
 * signaling_NaN(): 0111111111110100000000000000000000000000000000000000000000000000
 *     quiet_NaN(): 0111111111111000000000000000000000000000000000000000000000000000
 *
 *
 *
 * The first 13 bits signals that the number is a quiet NaN. Sign bit is zero,
 * exponent bits are all ones to use a NaN, and the first fraction bit is one
 * to use a quiet NaN.
 *
 * |   bits 0..15   |
 *  0111111111111
 *               000 number
 *               001 nothing
 *               010 undefined
 *               011 null
 *               100 boolean
 *               101 string
 *               110 object
 */
class EsValue : public EsValueData
{
public:
    /**
     * @brief Defines different value types this class can store.
     *
     * @note The values must be aligned with the type tags specified in the
     *       ES_VALUE_TAG_* macros.
     */
    enum Type
    {
        TYPE_NOTHING = 1,

        TYPE_UNDEFINED = 2,
        TYPE_NULL = 3,
        TYPE_BOOLEAN= 4,
        TYPE_NUMBER = 0,
        TYPE_STRING = 5,
        TYPE_OBJECT = 6
    };

private:
    /**
     * Creates a value of the specified type, this should only be used for null
     * and undefined values since only the type will be initialized.
     * @param [in] type Value type.
     */
    EsValue(Type type)
    {
        assert(type == TYPE_NOTHING || type == TYPE_NULL || type == TYPE_UNDEFINED);

        data.bits = ES_VALUE_TAG_NAN | (static_cast<uint64_t>(type) << 48);
    }

public:
    EsValue()
    {
        data.bits = ES_VALUE_TAG_NOTHING;
    }

    EsValue(const EsValueData &value)
    {
        data = value.data;
    }

    /**
     * The "nothing" value. This is not an actual ECMA-262 value, but is used
     * to represent elided array elements.
     */
    static const EsValue nothing;

    /**
     * The null value.
     */
    static const EsValue null;

    /**
     * The undefined value.
     */
    static const EsValue undefined;

    /**
     * Creates a value from a boolean.
     * @param [in] val Boolean value.
     * @return Wrapped boolean value.
     */
    inline static EsValue from_bool(bool val) { EsValue res; res.set_bool(val); return res; }

    /**
     * Creates a value from a number.
     * @param [in] val Number value.
     * @return Wrapped number value.
     */
    inline static EsValue from_num(double val) { EsValue res; res.set_num(val); return res; }

    /**
     * Creates a value from a 32-bit signed integer.
     * @param [in] val Integer value.
     * @return Wrapped integer value.
     */
    inline static EsValue from_i32(int32_t val) { EsValue res; res.set_num(static_cast<double>(val)); return res; }

    /**
     * Creates a value from a 32-bit unsigned integer.
     * @param [in] val Integer value.
     * @return Wrapped integer value.
     */
    inline static EsValue from_u32(uint32_t val) { EsValue res; res.set_num(static_cast<double>(val)); return res; }

    /**
     * Creates a value from a 64-bit signed integer.
     * @param [in] val Integer value.
     * @return Wrapped integer value.
     */
    inline static EsValue from_i64(int64_t val) { EsValue res; res.set_num(static_cast<double>(val)); return res; }

    /**
     * Creates a value from a 64-bit unsigned integer.
     * @param [in] val Integer value.
     * @return Wrapped integer value.
     */
    inline static EsValue from_u64(uint64_t val) { EsValue res; res.set_num(static_cast<double>(val)); return res; }

    /**
     * Creates a value from a string.
     * @param [in] val String value.
     * @return Wrapped string value.
     */
    inline static EsValue from_str(const EsString *val) { EsValue res; res.set_str(val); return res; }

    /**
     * Creates a value from an object.
     * @param [in] val Object value.
     * @return Wrapped object value.
     */
    inline static EsValue from_obj(EsObject *val) { EsValue res; res.set_obj(val); return res; }

    /**
     * @return Value type.
     */
    inline Type type() const
    {
        if (is_number())
            return TYPE_NUMBER;

        return static_cast<Type>((data.bits >> 48) & 0x07);
    }

    /**
     * Sets a boolean value.
     * @param [in] val Boolean value.
     */
    inline void set_bool(bool val)
    {
        data.bits = ES_VALUE_TAG_BOOL | static_cast<uint64_t>(val);
    }

    /**
     * Sets a numeric value.
     * @param [in] val Number value.
     */
    inline void set_num(double val)
    {
        data.num = val;
    }

    /**
     * Sets a numeric value.
     * @param [in] val Number value.
     */
    inline void set_i64(int64_t val)
    {
        data.num = static_cast<double>(val);
    }

#ifdef UNUSED
    inline void set_nan()
    {
        set_number(std::numeric_limits<double>::quiet_NaN());
    }
#endif

    /**
     * Sets a string value.
     * @param [in] str String value.
     */
    inline void set_str(const EsString *str)
    {
        assert(reinterpret_cast<uintptr_t>(str) < UINT64_C(0xffffffffffff));
        data.bits = ES_VALUE_TAG_STRING | reinterpret_cast<uint64_t>(str);
    }

    /**
     * Sets an object.
     * @param [in] obj Pointer to object.
     */
    inline void set_obj(EsObject *obj)
    {
        assert(reinterpret_cast<uintptr_t>(obj) < UINT64_C(0xffffffffffff));
        data.bits = ES_VALUE_TAG_OBJECT | reinterpret_cast<uint64_t>(obj);
    }

    /**
     * @return true if the value is "nothing", false otherwise.
     */
    inline bool is_nothing() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_NOTHING;
    }

    /**
     * @return true if value is undefined, false otherwise.
     */
    inline bool is_undefined() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_UNDEFINED;
    }

    /**
     * @return true if value is null, false otherwise.
     */
    inline bool is_null() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_NULL;
    }

    /**
     * @return true if value is a boolean, false otherwise.
     */
    inline bool is_boolean() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_BOOL;
    }

    /**
     * @return true if value is a number, false otherwise.
     */
    inline bool is_number() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_NUMBER ||      // NaN.
               (data.bits & ES_VALUE_MASK_NO_TAG) != ES_VALUE_TAG_NAN;    // Any other number.
    }

    /**
     * @return true if value is a string, false otherwise.
     */
    inline bool is_string() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_STRING;
    }

    /**
     * @return true if value is an object, false otherwise.
     */
    inline bool is_object() const
    {
        return (data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_OBJECT;
    }

    /**
     * @return true if value is a primitive value, false otherwise.
     */
    inline bool is_primitive() const { return !is_object(); }

    /**
     * @return Primitive boolean value.
     * @pre Value is a boolean.
     */
    inline bool as_boolean() const
    {
        assert(is_boolean());
        return static_cast<bool>(data.bits - ES_VALUE_TAG_BOOL);
    }

    /**
     * @return Primitive number value.
     * @pre Value is a number.
     */
    inline double as_number() const
    {
        assert(is_number());
        return data.num;
    }

    /**
     * @return Primitive string value. If the value is not a string value, an
     *         empty string will be returned.
     * @pre Value is a string.
     */
    inline EsString *as_string() const
    {
        assert(is_string());
        return reinterpret_cast<EsString *>(data.bits - ES_VALUE_TAG_STRING);
    }

    /**
     * @return Object value. If the value is not an object value, a NULL
     *         pointer will be returned.
     * @pre Value is an object.
     */
    inline EsObject *as_object() const
    {
        assert(is_object());
        return reinterpret_cast<EsObject *>(data.bits - ES_VALUE_TAG_OBJECT);
    }

    /**
     * Converts the current value into a primitive value.
     * @param [in] hint Preferred primitive value type.
     * @param [out] value Current value as a primitive value.
     * @return true on normal return, false if an exception was thrown.
     */
    bool to_primitive(EsTypeHint hint, EsValue &value) const;

    /**
     * Converts the current primitive value into a number value.
     * @return Current value converted to a number value.
     * @pre Value is not a pointer to an object.
     */
    double primitive_to_number() const;

    /**
     * Converts the current primitive value into an integer value.
     * @return Current value converted into an integer value.
     * @pre Value is not a pointer to an object.
     */
    int64_t primitive_to_integer() const;

    /**
     * Converts the current primitive value into a 32-bit signed integer value.
     * @return Current value converted into a 32-bit signed integer value.
     * @pre Value is not a pointer to an object.
     */
    int32_t primitive_to_int32() const;

    /**
     * Converts the current primitive value into a 32-bit unsigned integer
     * value.
     * @return Current value converted into a 32-bit unsigned integer value.
     * @pre Value is not a pointer to an object.
     */
    uint32_t primitive_to_uint32() const;

    /**
     * Converts the current primitive value into a string.
     * @return Current value converted into a string.
     * @pre Value is not a pointer to an object.
     */
    const EsString *primitive_to_string() const;

    /**
     * Converts the current value into a boolean value.
     * @return Current value converted into a boolean.
     */
    bool to_boolean() const;

    /**
     * Converts the current value into a number value.
     * @param [out] result Current value converted to a number value.
     * @return true on normal return, false if an exception was thrown.
     */
    bool to_number(double &result) const;

    /**
     * Converts the current value into an integer value.
     * @param [out] result Current value converted into an integer value.
     * @return true on normal return, false if an exception was thrown.
     */
    bool to_integer(int64_t &result) const;

    /**
     * Converts the current value into a 32-bit signed integer value.
     * @param [out] result Current value converted into a 32-bit signed integer value.
     * @return true on normal return, false if an exception was thrown.
     */
    bool to_int32(int32_t &result) const;

    /**
     * Converts the current value into a 32-bit unsigned integer value.
     * @param [out] result Current value converted into a 32-bit unsigned
     *                     integer value.
     * @return true on normal return, false if an exception was thrown.
     */
    bool to_uint32(uint32_t &result) const;

    /**
     * Converts the current value into a string.
     * @return Current value converted into a string on success, a NULL-pointer
     *         if an exception was thrown.
     */
    const EsString *to_stringT() const;

    /**
     * Converts the current value into an object.
     * @return Current value converted into an object on success, a NULL-pointer
     *         if an exception was thrown.
     * @throw TypeError if the value cannot be converted into an object.
     */
    EsObject *to_objectT() const;

    /**
     * Checks if the value can be converted into an object.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeError if the value cannot be converted into an object.
     */
    bool chk_obj_coercibleT() const;

    /**
     * Tests if the value is callable like a function.
     * @return true if the value is callable and false otherwise.
     */
    bool is_callable() const;

    /**
     * @return Function object. If the value is not a function object, a NULL
     *         pointer will be returned.
     * @pre Value is an object.
     */
    EsFunction *as_function() const;

    /**
     * Compares the value to a right hand side value for equality.
     * @param [in] rhs Right hand side value to compare to.
     * @return true if the values are considered equal and false otherwise.
     */
    bool operator==(const EsValue &rhs) const;

    /**
     * Compares the value to a right hand side value for inequality.
     * @param [in] rhs Right hand side value to compare to.
     * @return true if the values are not considered equal and false otherwise.
     */
    bool operator!=(const EsValue &rhs) const;
};
