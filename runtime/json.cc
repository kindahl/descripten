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

#include "common/lexical.hh"
#include "conversion.hh"
#include "error.hh"
#include "json.hh"
#include "property.hh"
#include "utility.hh"

bool json_is_white_space(uni_char c)
{
    // 15.12.1.1
    // WhiteSpace :: <TAB> <CR> <LF> <SP>

    static uni_char ws[] =
    {
        0x0009, /* TAB */
        0x000a, /* LF */
        0x000d, /* CR */
        0x0020, /* SP */
    };

    for (size_t i = 0; i < sizeof(ws)/sizeof(uni_char); i++)
    {
        if (c == ws[i])
            return true;
    }

    return false;
}

JsonParser::JsonParser(UnicodeStream &stream)
    : stream_(stream)
{
}

bool JsonParser::expect(const char *text)
{
    const char *ptr = text;
    while (*ptr)
    {
        uni_char c = stream_.next();

        if (c != static_cast<uni_char>(*ptr))
        {
            ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C', expected '%c' at position %d in '%s'.",
                                                           c, *ptr, ptr - text, text));
            return false;
        }

        ptr++;
    }

    return true;
}

void JsonParser::skip_white_space()
{
    uni_char c0 = stream_.next();
    while (c0 != static_cast<uni_char>(-1) && json_is_white_space(c0))
    {
        c0 = stream_.next();
    }

    stream_.push(c0);
}

int32_t JsonParser::read_hex_number(int num_digits)
{
    uni_char digits[4];
    assert(num_digits <= 4);    // Assert to detect when debugging.
    if (num_digits > 4)
        return -1;

    int32_t res = 0;
    for (int i = 0; i < num_digits; i++)
    {
        uni_char c = stream_.next();
        digits[i] = c;

        if (!es_is_hex_digit(c))
        {
            for (int j = i; j >= 0; j--)
                stream_.push(digits[j]);

            return -1;
        }

        res = res * 16 + es_as_hex_digit(c);
    }

    return res;
}

bool JsonParser::parse_object(EsValue &result)
{
    EsObject *obj = EsObject::create_inst();

#ifdef DEBUG
    uni_char c0 = stream_.next();
    assert(c0 == '{');
#else
    stream_.next();
#endif

    skip_white_space();

    uni_char c1 = stream_.next();
    while (c1 != static_cast<uni_char>(-1) && c1 != '}')
    {
        stream_.push(c1);
        if (c1 != '"')
        {
            ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C', expected '\"'.", c1));
            return false;
        }

        EsValue member_name;
        if (!parse_string(member_name))
            return false;

        skip_white_space();
        if (!expect(":"))
            return false;
        skip_white_space();

        EsValue member_value;
        if (!parse_value(member_value))
            return false;

        skip_white_space();

        if (!obj->define_own_propertyT(EsPropertyKey::from_str(member_name.as_string()),
                                       EsPropertyDescriptor(true, true, true,
                                                            member_value), true))
            return false;

        c1 = stream_.next();
        if (c1 == ',')
        {
            skip_white_space();
            c1 = stream_.next();
        }
        else if (c1 != '}')
        {
            ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C' in json object member.", c1));
            return false;
        }
    }

    skip_white_space();
    if (c1 != '}')
    {
        ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C', expected '}' in json object.", c1));
        return false;
    }

    result = EsValue::from_obj(obj);
    return true;
}

bool JsonParser::parse_array(EsValue &result)
{
#ifdef DEBUG
    uni_char c0 = stream_.next();
    assert(c0 == '[');
#else
    stream_.next();
#endif

    EsValueVector items;

    uni_char c1 = stream_.next();
    while (c1 != static_cast<uni_char>(-1) && c1 != ']')
    {
        stream_.push(c1);

        EsValue val;
        if (!parse_value(val))
            return false;

        items.push_back(val);
        skip_white_space();

        c1 = stream_.next();
        if (c1 == ',')
        {
            skip_white_space();
            c1 = stream_.next();
        }
        else if (c1 != ']')
        {
            ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C' in json array member.", c1));
            return false;
        }
    }

    skip_white_space();
    if (c1 != ']')
    {
        ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C', expected ']' in json array.", c1));
        return false;
    }

    result = EsValue::from_obj(EsArray::create_inst_from_lit(static_cast<int>(items.size()),
                                                             &items[0]));
    return true;
}

bool JsonParser::parse_string(EsValue &result)
{
    sb_.clear();

#ifdef DEBUG
    uni_char c0 = stream_.next();
    assert(c0 == '"');
#else
    stream_.next();
#endif

    uni_char c1 = stream_.next();
    while (c1 != static_cast<uni_char>(-1) &&
           c1 > static_cast<uni_char>(0x1f) && c1 != '"')
    {
        if (c1 == '\\')
        {
            uni_char c2 = stream_.next();
            switch (c2)
            {
                // Single escape character.
                case '"':
                case '/':
                case '\\':
                    sb_.append(c2);
                    break;
                case 'b':
                    sb_.append('\b');
                    break;
                case 'f':
                    sb_.append('\f');
                    break;
                case 'n':
                    sb_.append('\n');
                    break;
                case 'r':
                    sb_.append('\r');
                    break;
                case 't':
                    sb_.append('\t');
                    break;
                // Unicode escape sequence.
                case 'u':
                {
                    int32_t val = read_hex_number(4);
                    if (val == -1)
                    {
                        ES_THROW(EsSyntaxError, StringBuilder::sprintf("illegal character in unicode escape sequence."));
                        return false;
                    }
                    else
                    {
                        sb_.append(static_cast<uni_char>(val));
                    }
                    break;
                }
                default:
                    ES_THROW(EsSyntaxError, StringBuilder::sprintf("illegal character in escape sequence."));
                    return false;
            }
        }
        else
        {
            sb_.append(c1);
        }

        c1 = stream_.next();
    }

    if (c1 != '"')
    {
        ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C' in json string.", c1));
        return false;
    }

    result = EsValue::from_str(sb_.string());
    return true;
}

bool JsonParser::parse_number(EsValue &result)
{
    sb_.clear();

    uni_char c = stream_.next();
    if (c == '-')
    {
        sb_.append(c);
        c = stream_.next();
    }

    while (es_is_dec_digit(c))
    {
        sb_.append(c);
        c = stream_.next();
    }

    if (c == '.')
    {
        sb_.append(c);

        c = stream_.next();
        while (es_is_dec_digit(c))
        {
            sb_.append(c);
            c = stream_.next();
        }

        // Return the non-digit.
        stream_.push(c);
    }
    else
    {
        // Return the non-dot.
        stream_.push(c);
    }

    // Scan exponent.
    c = stream_.next();
    if (c == 'e' || c == 'E')
    {
        sb_.append(c);

        c = stream_.next();
        if (c == '+' || c == '-')
        {
            sb_.append(c);
            c = stream_.next();
        }

        if (!es_is_dec_digit(c))
        {
            ES_THROW(EsSyntaxError, StringBuilder::sprintf("illegal token '%C' in json number literal.", c));
            return false;
        }

        while (es_is_dec_digit(c))
        {
            sb_.append(c);
            c = stream_.next();
        }

        // Return the non-digit.
        stream_.push(c);
    }
    else
    {
        // Return non 'e' or 'E' letter.
        stream_.push(c);
    }

    result = EsValue::from_num(es_str_to_num(sb_.string()));
    return true;
}

bool JsonParser::parse_value(EsValue &result)
{
    skip_white_space();

    uni_char c0 = stream_.next();
    stream_.push(c0);

    switch (c0)
    {
        case 'n':
            if (!expect("null"))
                return false;
            result = EsValue::null;
            return true;
        case 't':
            if (!expect("true"))
                return false;
            result = EsValue::from_bool(true);
            return true;
        case 'f':
            if (!expect("false"))
                return false;
            result = EsValue::from_bool(false);
            return true;
        case '{':
            return parse_object(result);
        case '[':
            return parse_array(result);
        case '"':
            return parse_string(result);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return parse_number(result);
        //case -1:
        default:
            ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C' in json value.", c0));
            return false;
    }

    assert(false);
    result = EsValue::null;
    return true;
}

bool JsonParser::parse(EsValue &result)
{
    if (!parse_value(result))
        return false;

    uni_char c0 = stream_.next();
    if (c0 != static_cast<uni_char>(-1))
    {
        ES_THROW(EsSyntaxError, StringBuilder::sprintf("unexpected token '%C', expected end of input.", c0));
        return false;
    }

    return true;
}
