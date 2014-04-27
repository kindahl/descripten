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
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EsObject;
struct EsString;

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

/*
 * Define different value types.
 *
 * The values must be aligned with the type tags specified in the
 * ES_VALUE_TAG_* macros.
 */
#define ES_VALUE_TYPE_NOTHING       1
#define ES_VALUE_TYPE_UNDEFINED     2
#define ES_VALUE_TYPE_NULL          3
#define ES_VALUE_TYPE_BOOLEAN       4
#define ES_VALUE_TYPE_NUMBER        0
#define ES_VALUE_TYPE_STRING        5
#define ES_VALUE_TYPE_OBJECT        6

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
typedef struct
{
    union
    {
        uint64_t bits;
        double num;
    } data;
} EsValueData;

/**
 * Creates a value of the specified type, this should only be used for null
 * and undefined values since only the type will be initialized.
 * @param [in] type Value type.
 */
inline EsValueData es_value_create(uint32_t type)
{
    EsValueData value;

    assert(type == ES_VALUE_TYPE_NOTHING ||
           type == ES_VALUE_TYPE_NULL ||
           type == ES_VALUE_TYPE_UNDEFINED);

    value.data.bits = ES_VALUE_TAG_NAN | ((uint64_t)type << 48);
    return value;
}

inline EsValueData es_value_nothing()
{
    EsValueData value;
    value.data.bits = ES_VALUE_TAG_NOTHING;
    return value;
}

inline EsValueData es_value_null()
{
    EsValueData value;
    value.data.bits = ES_VALUE_TAG_NULL;
    return value;
}

inline EsValueData es_value_undefined()
{
    EsValueData value;
    value.data.bits = ES_VALUE_TAG_UNDEFINED;
    return value;
}

inline EsValueData es_value_true()
{
    EsValueData value;
    value.data.bits = ES_VALUE_TAG_BOOL | 0x01;
    return value;
}

inline EsValueData es_value_false()
{
    EsValueData value;
    value.data.bits = ES_VALUE_TAG_BOOL;
    return value;
}

/**
 * Creates a boolean value.
 * @param [in] val Boolean value.
 */
inline EsValueData es_value_from_boolean(int val)
{
    EsValueData value;
    value.data.bits = ES_VALUE_TAG_BOOL | (uint64_t)val;
    return value;
}

/**
 * Creates a number value.
 * @param [in] val Number value.
 */
inline EsValueData es_value_from_number(double val)
{
    EsValueData value;
    value.data.num = val;
    return value;
}

/**
 * Creates a number value.
 * @param [in] val Number value.
 */
inline EsValueData es_value_from_i64(int64_t val)
{
    EsValueData value;
    value.data.num = (double)val;
    return value;
}

/**
 * Creates a string value.
 * @param [in] str String value.
 */
inline EsValueData es_value_from_string(const struct EsString *str)
{
    EsValueData value;

    assert((uintptr_t)str < UINT64_C(0xffffffffffff));

    value.data.bits = ES_VALUE_TAG_STRING | (uintptr_t)str;
    return value;
}

/**
 * Creates an object value.
 * @param [in] obj Pointer to object.
 */
inline EsValueData es_value_from_object(struct EsObject *obj)
{
    EsValueData value;

    assert((uintptr_t)obj < UINT64_C(0xffffffffffff));

    value.data.bits = ES_VALUE_TAG_OBJECT | (uintptr_t)obj;
    return value;
}

/**
 * @return 1 if the value is "nothing", 0 otherwise.
 */
inline int es_value_is_nothing(const EsValueData value)
{
    return (value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_NOTHING ? 1 : 0;
}

/**
 * @return 1 if value is undefined, 0 otherwise.
 */
inline int es_value_is_undefined(const EsValueData value)
{
    return (value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_UNDEFINED ? 1 : 0;
}

/**
 * @return 1 if value is null, 0 otherwise.
 */
inline int es_value_is_null(const EsValueData value)
{
    return (value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_NULL ? 1 : 0;
}

/**
 * @return 1 if value is a boolean, 0 otherwise.
 */
inline int es_value_is_boolean(const EsValueData value)
{
    return (value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_BOOL ? 1 : 0;
}

/**
 * @return 1 if value is a number, 0 otherwise.
 */
inline int es_value_is_number(const EsValueData value)
{
    return ((value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_NUMBER ||             /* NaN. */
            (value.data.bits & ES_VALUE_MASK_NO_TAG) != ES_VALUE_TAG_NAN) ? 1 : 0;  /* Any other number. */
}

/**
 * @return 1 if value is a string, 0 otherwise.
 */
inline int es_value_is_string(const EsValueData value)
{
    return (value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_STRING ? 1 : 0;
}

/**
 * @return 1 if value is an object, 0 otherwise.
 */
inline int es_value_is_object(const EsValueData value)
{
    return (value.data.bits & ES_VALUE_MASK) == ES_VALUE_TAG_OBJECT ? 1 : 0;
}

/**
 * @return Primitive boolean value.
 * @pre Value is a boolean.
 */
inline int es_value_as_boolean(const EsValueData value)
{
    assert(es_value_is_boolean(value));
    return (int)(value.data.bits - ES_VALUE_TAG_BOOL);
}

/**
 * @return Primitive number value.
 * @pre Value is a number.
 */
inline double es_value_as_number(const EsValueData value)
{
    assert(es_value_is_number(value));
    return value.data.num;
}

/**
 * @return Primitive string value. If the value is not a string value, an
 *         empty string will be returned.
 * @pre Value is a string.
 */
inline struct EsString *es_value_as_string(const EsValueData value)
{
    assert(es_value_is_string(value));
    return (struct EsString *)(value.data.bits - ES_VALUE_TAG_STRING);
}

/**
 * @return Object value. If the value is not an object value, a NULL
 *         pointer will be returned.
 * @pre Value is an object.
 */
inline struct EsObject *es_value_as_object(const EsValueData value)
{
    assert(es_value_is_object(value));
    return (struct EsObject *)(value.data.bits - ES_VALUE_TAG_OBJECT);
}

/**
 * @return Value type.
 */
inline uint32_t es_value_type(const EsValueData value)
{
    if (es_value_is_number(value))
        return ES_VALUE_TYPE_NUMBER;

    return (uint32_t)((value.data.bits >> 48) & 0x07);
}

#ifdef __cplusplus
}
#endif
