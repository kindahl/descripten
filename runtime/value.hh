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
#include "types.hh"
#include "value_base.hh"

class EsObject;

/**
 * @brief Holds a primitive value or a pointer to an object.
 */
class EsValue : public EsValueBase
{
private:
    EsValue(Type type)
        : EsValueBase(type) {}

public:
    EsValue() {}

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
    inline static EsValue from_str(const String &val) { EsValue res; res.set_str(val); return res; }

    /**
     * Creates a value from an object.
     * @param [in] val Object value.
     * @return Wrapped object value.
     */
    inline static EsValue from_obj(EsObject *val) { EsValue res; res.set_obj(val); return res; }

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
    String primitive_to_string() const;

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
     * @param [out] result Current value converted into a string.
     * @return true on normal return, false if an exception was thrown.
     */
    bool to_string(String &result) const;

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
