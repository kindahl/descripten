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

#define ES_VALUE_MASK               UINT64_C(0xffff000000000000)
#define ES_VALUE_MASK_NO_TAG        UINT64_C(0xfff8000000000000)

#define ES_VALUE_TAG_NAN            UINT64_C(0x7ff8000000000000)

#define ES_VALUE_TAG_NUMBER         UINT64_C(0x7ff8000000000000)
#define ES_VALUE_TAG_NOTHING        UINT64_C(0x7ff9000000000000)
#define ES_VALUE_TAG_UNDEFINED      UINT64_C(0x7ffa000000000000)
#define ES_VALUE_TAG_NULL           UINT64_C(0x7ffb000000000000)
#define ES_VALUE_TAG_BOOL           UINT64_C(0x7ffc000000000000)
#define ES_VALUE_TAG_STRING         UINT64_C(0x7ffd000000000000)
#define ES_VALUE_TAG_OBJECT         UINT64_C(0x7ffe000000000000)

/**
 * @brief Holds a primitive value or a pointer to an object.
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
class EsValueBoxedBase
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
    union
    {
        uint64_t bits_;
        double num_;
    } data_;

    uint32_t str_len_;  // FIXME: TYPE_STRING shouldn't represent a String object, but rather be a pointer to an EsString.

protected:
    /**
     * Creates a value of the specified type, this should only be used for null
     * and undefined values since only the type will be initialized.
     * @param [in] type Value type.
     */
    EsValueBoxedBase(Type type)
    {
        assert(type == TYPE_NOTHING || type == TYPE_NULL || type == TYPE_UNDEFINED);

        data_.bits_ = ES_VALUE_TAG_NAN | (static_cast<uint64_t>(type) << 48);
    }

public:
    /**
     * Creates a "nothing" value.
     */
    inline EsValueBoxedBase()
    {
        data_.bits_ = ES_VALUE_TAG_NOTHING;
    }

    /**
     * @return Value type.
     */
    inline Type type() const 
    {
        if (is_number())
            return TYPE_NUMBER;

        return static_cast<Type>((data_.bits_ >> 48) & 0x07);
    }

    /**
     * Sets a boolean value.
     * @param [in] val Boolean value.
     */
    inline void set_bool(bool val)
    {
        data_.bits_ = ES_VALUE_TAG_BOOL | static_cast<uint64_t>(val);
    }

    /**
     * Sets a numeric value.
     * @param [in] val Number value.
     */
    inline void set_num(double val)
    {
        data_.num_ = val;
    }
    
    /**
     * Sets a numeric value.
     * @param [in] val Number value.
     */
    inline void set_i64(int64_t val)
    {
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
        String str(ptr);
        assert(reinterpret_cast<uintptr_t>(str.data()) < UINT64_C(0xffffffffffff));

        data_.bits_ = ES_VALUE_TAG_STRING | reinterpret_cast<uint64_t>(str.data());
        str_len_ = str.length();    // FIXME:
    }
    
    /**
     * Sets a string value.
     * @param [in] raw Pointer to a potentially non-NULL-terminated UTF-8
     *                 string.
     * @param [in] len Number of bytes to read from raw.
     */
    inline void set_str(const char *raw, size_t len)
    {
        String str(raw, len);
        assert(reinterpret_cast<uintptr_t>(str.data()) < UINT64_C(0xffffffffffff));

        data_.bits_ = ES_VALUE_TAG_STRING | reinterpret_cast<uint64_t>(str.data());
        str_len_ = str.length();    // FIXME:
    }
    
    /**
     * Sets a string value.
     * @param [in] str String value.
     */
    inline void set_str(const String &str)
    {
        assert(reinterpret_cast<uintptr_t>(str.data()) < UINT64_C(0xffffffffffff));

        data_.bits_ = ES_VALUE_TAG_STRING | reinterpret_cast<uint64_t>(str.data());
        str_len_ = str.length();    // FIXME:
    }

    /**
     * Sets an object.
     * @param [in] obj Pointer to object.
     */
    inline void set_obj(EsObject *obj)
    {
        assert(reinterpret_cast<uintptr_t>(obj) < UINT64_C(0xffffffffffff));
        data_.bits_ = ES_VALUE_TAG_OBJECT | reinterpret_cast<uint64_t>(obj);
    }

    /**
     * @return true if the value is "nothing", false otherwise.
     */
    inline bool is_nothing() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_NOTHING;
    }

    /**
     * @return true if value is undefined, false otherwise.
     */
    inline bool is_undefined() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_UNDEFINED;
    }

    /**
     * @return true if value is null, false otherwise.
     */
    inline bool is_null() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_NULL;
    }

    /**
     * @return true if value is a boolean, false otherwise.
     */
    inline bool is_boolean() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_BOOL;
    }

    /**
     * @return true if value is a number, false otherwise.
     */
    inline bool is_number() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_NUMBER ||      // NaN.
               (data_.bits_ & ES_VALUE_MASK_NO_TAG) != ES_VALUE_TAG_NAN;    // Any other number.
    }

    /**
     * @return true if value is a string, false otherwise.
     */
    inline bool is_string() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_STRING;
    }

    /**
     * @return true if value is an object, false otherwise.
     */
    inline bool is_object() const
    {
        return (data_.bits_ & ES_VALUE_MASK) == ES_VALUE_TAG_OBJECT;
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
        return static_cast<bool>(data_.bits_ - ES_VALUE_TAG_BOOL);
    }

    /**
     * @return Primitive number value.
     * @pre Value is a number.
     */
    inline double as_number() const
    {
        assert(is_number());
        return data_.num_;
    }

    /**
     * @return Primitive string value. If the value is not a string value, an
     *         empty string will be returned.
     * @pre Value is a string.
     */
    inline String as_string() const
    {
        assert(is_string());

        const uni_char *data = reinterpret_cast<const uni_char *>(
            data_.bits_ - ES_VALUE_TAG_STRING);
        
        return String::wrap(data, str_len_);
    }

    /**
     * @return Object value. If the value is not an object value, a NULL
     *         pointer will be returned.
     * @pre Value is an object.
     */
    inline EsObject *as_object() const
    {
        assert(is_object());
        return reinterpret_cast<EsObject *>(data_.bits_ - ES_VALUE_TAG_OBJECT);
    }
};
