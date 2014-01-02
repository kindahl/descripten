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
#include "common/cast.hh"
#include "common/string.hh"
#include "types.hh"

class EsFunction;
class EsObject;

class EsValueBase
{
public:
    /**
     * @brief Defines different value types this class can store.
     */
    enum Type
    {
        TYPE_NOTHING,

        TYPE_UNDEFINED,
        TYPE_NULL,
        TYPE_BOOLEAN,
        TYPE_NUMBER,
        TYPE_STRING,
        TYPE_OBJECT
    };

private:
    Type type_;         ///< Value type.

    union
    {
        bool bool_;     ///< Boolean value for TYPE_BOOLEAN.
        double num_;    ///< Number value for TYPE_NUMBER.
        EsObject *obj_; ///< Object pointer for TYPE_OBJECT.
        struct
        {
            const uni_char *data_;  ///< Pointer to Unicode points, see String::data_.
            uint32_t len_;          ///< Number of Unicode points, see String::len_;
        } str_;         ///< String value for TYPE_STRING.
    } data_;

protected:
    /**
     * Creates a value of the specified type, this should only be used for null
     * and undefined values since only the type will be initialized.
     * @param [in] type Value type.
     */
    EsValueBase(Type type)
        : type_(type)
    {
        assert(type == TYPE_NOTHING || type == TYPE_NULL || type == TYPE_UNDEFINED);
    }

public:
    /**
     * Creates a "nothing" value.
     */
    inline EsValueBase() : type_(TYPE_NOTHING) {}

    /**
     * @return Value type.
     */
    inline Type type() const { return type_; }

    /**
     * Sets a boolean value.
     * @param [in] val Boolean value.
     */
    inline void set_bool(bool val)
    {
        type_ = TYPE_BOOLEAN;
        data_.bool_ = val;
    }

    /**
     * Sets a numeric value.
     * @param [in] val Number value.
     */
    inline void set_num(double val)
    {
        type_ = TYPE_NUMBER;
        data_.num_ = val;
    }

    /**
     * Sets a numeric value.
     * @param [in] val Number value.
     */
    inline void set_i64(int64_t val)
    {
        type_ = TYPE_NUMBER;
        data_.num_ = static_cast<double>(val);
    }

#ifdef UNUSED
    inline void set_nan()
    {
        set_number(std::numeric_limits<double>::quiet_NaN());
    }
#endif

    /**
     * Sets a string value.
     * @param [in] ptr Pointer to NULL-terminated UTF-8 string.
     */
    inline void set_str(const char *ptr)
    {
        type_ = TYPE_STRING;

        String str(ptr);
        data_.str_.data_ = str.data();
        data_.str_.len_ = str.length();
    }

    /**
     * Sets a string value.
     * @param [in] raw Pointer to a potentially non-NULL-terminated UTF-8
     *                 string.
     * @param [in] len Number of bytes to read from raw.
     */
    inline void set_str(const char *raw, size_t len)
    {
        type_ = TYPE_STRING;

        String str(raw, len);
        data_.str_.data_ = str.data();
        data_.str_.len_ = str.length();
    }

    /**
     * Sets a string value.
     * @param [in] str String value.
     */
    inline void set_str(const String &str)
    {
        type_ = TYPE_STRING;
        data_.str_.data_ = str.data();
        data_.str_.len_ = str.length();
    }

    /**
     * Sets an object.
     * @param [in] obj Pointer to object.
     */
    inline void set_obj(EsObject *obj)
    {
        type_ = TYPE_OBJECT;
        data_.obj_ = obj;
    }

    /**
     * @return true if the value is "nothing", false otherwise.
     */
    inline bool is_nothing() const { return type_ == TYPE_NOTHING; }

    /**
     * @return true if value is undefined, false otherwise.
     */
    inline bool is_undefined() const { return type_ == TYPE_UNDEFINED; }

    /**
     * @return true if value is null, false otherwise.
     */
    inline bool is_null() const { return type_ == TYPE_NULL; }

    /**
     * @return true if value is a boolean, false otherwise.
     */
    inline bool is_boolean() const { return type_ == TYPE_BOOLEAN; }

    /**
     * @return true if value is a number, false otherwise.
     */
    inline bool is_number() const { return type_ == TYPE_NUMBER; }

    /**
     * @return true if value is a string, false otherwise.
     */
    inline bool is_string() const { return type_ == TYPE_STRING; }

    /**
     * @return true if value is an object, false otherwise.
     */
    inline bool is_object() const { return type_ == TYPE_OBJECT; }

    /**
     * @return true if value is a primitive value, false otherwise.
     */
    inline bool is_primitive() const { return type_ != TYPE_OBJECT; }

    /**
     * @return Primitive boolean value.
     * @pre Value is a boolean.
     */
    inline bool as_boolean() const
    {
        assert(type_ == TYPE_BOOLEAN);
        return data_.bool_;
    }

    /**
     * @return Primitive number value.
     * @pre Value is a number.
     */
    inline double as_number() const
    {
        assert(type_ == TYPE_NUMBER);
        return data_.num_;
    }

    /**
     * @return Primitive string value. If the value is not a string value, an
     *         empty string will be returned.
     * @pre Value is a string.
     */
    inline String as_string() const
    {
        assert(type_ == TYPE_STRING);
        return type_ == TYPE_STRING ? String::wrap(data_.str_.data_, data_.str_.len_) : String();
    }

    /**
     * @return Object value. If the value is not an object value, a NULL
     *         pointer will be returned.
     * @pre Value is an object.
     */
    inline EsObject *as_object() const
    {
        assert(type_ == TYPE_OBJECT);
        return type_ == TYPE_OBJECT ? data_.obj_ : NULL;
    }
};
