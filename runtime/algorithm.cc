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

#include <cassert>
#include <cmath>
#include <limits>
#include "common/cast.hh"
#include "common/stringbuilder.hh"
#include "algorithm.hh"
#include "conversion.hh"
#include "error.hh"
#include "frame.hh"
#include "object.hh"
#include "property.hh"
#include "property_key.hh"
#include "utility.hh"
#include "value.hh"
#include <stdio.h>

namespace algorithm
{
    bool abstr_rel_comp(const EsValue &x, const EsValue &y, bool left_first,
                        Maybe<bool> &result)
    {
        EsValue px, py;
        if (left_first)
        {
            if (!x.to_primitive(ES_HINT_NUMBER, px))
                return false;
            if (!y.to_primitive(ES_HINT_NUMBER, py))
                return false;
        }
        else
        {
            if (!y.to_primitive(ES_HINT_NUMBER, py))
                return false;
            if (!x.to_primitive(ES_HINT_NUMBER, px))
                return false;
        }

        if (px.is_string() && py.is_string())
        {
            result = px.as_string() < py.as_string();
            return true;
        }
        else
        {
            double nx = px.primitive_to_number();
            double ny = py.primitive_to_number();

            if (std::isnan(nx) || std::isnan(ny))
            {
                result.clear();
                return true;
            }
            if (nx == ny)
            {
                result = false;
                return true;
            }
            if (nx ==  std::numeric_limits<double>::infinity() ||
                ny == -std::numeric_limits<double>::infinity())
            {
                result = false;
                return true;
            }
            if (nx == -std::numeric_limits<double>::infinity() ||
                ny ==  std::numeric_limits<double>::infinity())
            {
                result = true;
                return true;
            }

            result = nx < ny;
            return true;
        }
    }
    
    bool abstr_eq_comp(const EsValue &x, const EsValue &y, bool &result)
    {
        if (x.type() == y.type())
        {
            if (x.is_undefined() || x.is_null())
            {
                result = true;
                return true;
            }
            if (x.is_number())
            {
                double x_val = x.primitive_to_number();
                double y_val = y.primitive_to_number();
                
                if (std::isnan(x_val) || std::isnan(y_val))
                    result = false;
                else
                    result = x_val == y_val;
                return true;
            }
            if (x.is_string())
            {
                String x_val = x.primitive_to_string();
                String y_val = y.primitive_to_string();
                result = x_val == y_val;
                return true;
            }
            if (x.is_boolean())
            {
                bool x_val = x.to_boolean();
                bool y_val = y.to_boolean();
                result = x_val == y_val;
                return true;
            }

            assert(x.is_object() && y.is_object());
            result = x.as_object() == y.as_object();
            return true;
        }
        if ((x.is_null() && y.is_undefined()) ||
            (y.is_null() && x.is_undefined()))
        {
            result = true;
            return true;
        }
        if (x.is_number() && y.is_string())
            return abstr_eq_comp(x, EsValue::from_num(y.primitive_to_number()), result);
        if (x.is_string() && y.is_number())
            return abstr_eq_comp(EsValue::from_num(x.primitive_to_number()), y, result);
        if (x.is_boolean())
            return abstr_eq_comp(EsValue::from_num(x.primitive_to_number()), y, result);
        if (y.is_boolean())
            return abstr_eq_comp(x, EsValue::from_num(y.primitive_to_number()), result);
        if ((x.is_string() || x.is_number()) && y.is_object())
        {
            EsValue v;
            if (!y.to_primitive(ES_HINT_NONE, v))
                return false;
            return abstr_eq_comp(x, v, result);
        }
        if ((y.is_string() || y.is_number()) && x.is_object())
        {
            EsValue v;
            if (!x.to_primitive(ES_HINT_NONE, v))
                return false;
            return abstr_eq_comp(v, y, result);
        }
        result = false;
        return true;
    }
    
    bool strict_eq_comp(const EsValue &x, const EsValue &y)
    {
        // 11.9.6. The Strict Equality Comparison Algorithm
        if (x.type() != y.type())
            return false;
        if (x.is_undefined() || x.is_null())
            return true;
        if (x.is_number())
        {
            double x_val = x.primitive_to_number();
            double y_val = y.primitive_to_number();

            if (std::isnan(x_val) || std::isnan(y_val))
                return false;
            if (x_val == y_val)
                return true;
            
            return false;
        }
        if (x.is_string())
        {
            String x_val = x.primitive_to_string();
            String y_val = y.primitive_to_string();
            return x_val == y_val;
        }
        if (x.is_boolean())
        {
            bool x_val = x.to_boolean();
            bool y_val = y.to_boolean();
            return x_val == y_val;
        }
        
        assert(x.is_object() && y.is_object());
        return x.as_object() == y.as_object();
    }
    
    bool same_value(const EsValue &x, const EsValue &y)
    {
        // 9.12. The Strict Equality Comparison Algorithm
        if (x.type() != y.type())
            return false;
        if (x.is_undefined() || x.is_null())
            return true;
        if (x.is_number())
        {
            double x_val = x.primitive_to_number();
            double y_val = y.primitive_to_number();
            
            if (std::isnan(x_val) && std::isnan(y_val))
                return true;
            if ((std::signbit(x_val) && !std::signbit(y_val)) ||
                (std::signbit(y_val) && !std::signbit(x_val)))
                return false;
            if (x_val == y_val)
                return true;
            
            return false;
        }
        if (x.is_string())
        {
            String x_val = x.primitive_to_string();
            String y_val = y.primitive_to_string();
            return x_val == y_val;
        }
        if (x.is_boolean())
        {
            bool x_val = x.to_boolean();
            bool y_val = y.to_boolean();
            return x_val == y_val;
        }
        
        assert(x.is_object() && y.is_object());
        return x.as_object() == y.as_object();
    }

    MatchResult *split_match(const String &s, uint32_t q, EsRegExp *r)
    {
        EsRegExp::MatchResult *state = r->match(s, q);
        if (!state)
            return NULL;

        MatchResult *res = new (GC)MatchResult();
        res->end_index_ = state->end_index();

        EsRegExp::MatchResult::const_iterator it;
        for (it = state->begin(); it != state->end(); ++it)
            res->cap_.push_back((*it).string());

        return res;
    }

    MatchResult *split_match(const String &s, uint32_t q, const String &r)
    {
        if (q + r.length() > s.length())
            return NULL;

        for (size_t i = 0; i < r.length(); i++)
        {
            if (s[q + i] != r[i])
                return NULL;
        }

        MatchResult *res = new (GC)MatchResult();
        res->end_index_ = static_cast<int>(q + r.length());
        return res;
    }

    bool sort_compare(EsObject *obj, uint32_t j, uint32_t k,
                      EsFunction *comparefn, double &result)
    {
        bool has_j = obj->has_property(EsPropertyKey::from_u32(j));
        bool has_k = obj->has_property(EsPropertyKey::from_u32(k));

        if (!has_j && !has_k)
        {
            result = 0.0;
            return true;
        }

        if (!has_j)
        {
            result = 1.0;
            return true;
        }

        if (!has_k)
        {
            result = -1.0;
            return true;
        }

        EsValue x;
        if (!obj->getT(EsPropertyKey::from_u32(j), x))
            return false;

        EsValue y;
        if (!obj->getT(EsPropertyKey::from_u32(k), y))
            return false;

        if (x.is_undefined() && y.is_undefined())
        {
            result = 0.0;
            return true;
        }

        if (x.is_undefined())
        {
            result = 1.0;
            return true;
        }

        if (y.is_undefined())
        {
            result = -1.0;
            return true;
        }

        if (comparefn)
        {
            EsCallFrame frame = EsCallFrame::push_function(
                2, comparefn, EsValue::undefined);
            frame.fp()[0] = x;
            frame.fp()[1] = y;

            if (!comparefn->callT(frame))
                return false;
            if (!frame.result().to_number(result))
                return false;

            return true;
        }

        String x_str;
        if (!x.to_string(x_str))
            return false;
        String y_str;
        if (!y.to_string(y_str))
            return false;

        if (x_str < y_str)
            result = -1.0;
        else if (y_str < x_str)
            result = 1.0;
        else
            result = 0.0;

        return true;
    }

    bool json_walk(const String &name, EsObject *holder, EsFunction *reviver, EsValue &result)
    {
        EsValue val;
        if (!holder->getT(EsPropertyKey::from_str(name), val))
            return false;

        if (val.is_object())
        {
            EsObject *val_obj = val.as_object();
            if (val_obj->class_name() == _USTR("Array"))
            {
                EsValue len;
                if (!val_obj->getT(property_keys.length, len))
                    return false;

                for (uint32_t i = 0; i < len.primitive_to_uint32(); i++)
                {
                    EsValue new_elem;
                    if (!json_walk(String(lexical_cast<const char *>(i)), val_obj, reviver, new_elem))
                        return false;

                    if (new_elem.is_undefined())
                    {
                        if (!val_obj->removeT(EsPropertyKey::from_u32(i), false))
                            return false;
                    }
                    else
                    {
                        if (!ES_DEF_PROPERTY(val_obj, EsPropertyKey::from_u32(i), new_elem, true, true, true))
                            return false;
                    }
                }
            }
            else
            {
                for (EsObject::Iterator it = val_obj->begin();
                    it != val_obj->end(); ++it)
                {
                    EsPropertyKey key = *it;

                    EsPropertyReference prop = val_obj->get_property(key);
                    if (!prop->is_enumerable())
                        continue;

                    EsValue new_elem;
                    if (!json_walk(key.to_string(), val_obj, reviver, new_elem))
                        return false;

                    if (new_elem.is_undefined())
                    {
                        if (!val_obj->removeT(key, false))
                            return false;
                    }
                    else
                    {
                        if (!ES_DEF_PROPERTY(val_obj, key, new_elem, true, true, true))
                            return false;
                    }
                }
            }
        }

        EsCallFrame frame = EsCallFrame::push_function(
            2, reviver, EsValue::from_obj(holder));
        frame.fp()[0].set_str(name);
        frame.fp()[1] = val;
        if (!reviver->callT(frame))
            return false;

        result = frame.result();
        return true;
    }

    bool json_str(const String &key, EsObject *holder, JsonState &state,
                  EsValue &result)
    {
        EsValue val;
        if (!holder->getT(EsPropertyKey::from_str(key), val))
            return false;

        if (val.is_object())
        {
            EsObject *val_obj = val.as_object();

            EsValue to_json;
            if (!val_obj->getT(property_keys.to_json, to_json))
                return false;

            if (to_json.is_callable())
            {
                EsCallFrame frame = EsCallFrame::push_function(
                    1, to_json.as_function(), val);
                frame.fp()[0].set_str(key);

                if (!to_json.as_function()->callT(frame))
                    return false;

                val = frame.result();
            }
        }

        if (state.replacer_fun != NULL)    // NOTE: Using NULL as undefined.
        {
            EsCallFrame frame = EsCallFrame::push_function(
                2, state.replacer_fun, EsValue::from_obj(holder));
            frame.fp()[0].set_str(key);
            frame.fp()[1] = val;

            if (!state.replacer_fun->callT(frame))
                return false;

            val = frame.result();
        }

        if (val.is_object())
        {
            EsObject *val_obj = val.as_object();

            // Note: this code has been optimized to return early compared to
            //       what the specification states.
            if (val_obj->class_name() == _USTR("Number"))
            {
                double num = 0.0;
                if (!val.to_number(num))
                    return false;

                result = EsValue::from_str(std::isfinite(num) ? es_num_to_str(num) : _USTR("null"));
                return true;
            }
            if (val_obj->class_name() == _USTR("String"))
            {
                String str;
                if (!val.to_string(str))
                    return false;

                result = EsValue::from_str(json_quote(str));
                return true;
            }
            if (EsBooleanObject *bool_obj = dynamic_cast<EsBooleanObject *>(val_obj))
            {
                result = EsValue::from_str(bool_obj->primitive_value() ? _USTR("true") : _USTR("false"));
                return true;
            }
        }

        if (val.is_null())
        {
            result = EsValue::from_str(_USTR("null"));
            return true;
        }

        if (val.is_boolean())
        {
            result = EsValue::from_str(val.as_boolean() ? _USTR("true") : _USTR("false"));
            return true;
        }
        else if (val.is_string())
        {
            result = EsValue::from_str(json_quote(val.as_string()));
            return true;
        }
        else if (val.is_number())
        {
            result = EsValue::from_str(std::isfinite(val.as_number()) ?
                es_num_to_str(val.as_number()) : _USTR("null"));
            return true;
        }

        if (val.is_object())
        {
            EsObject *val_obj = val.as_object();

            if (!val.is_callable())
            {
                if (val_obj->class_name() == _USTR("Array"))
                    return json_ja(val_obj, state, result);
                else
                    return json_jo(val_obj, state, result);
            }
        }

        result = EsValue::undefined;
        return true;
    }

    String json_quote(const String &val)
    {
        StringBuilder product;
        product.append('"');

        for (size_t i = 0; i < val.length(); i++)
        {
            uni_char c = val[i];
            switch (c)
            {
                case '"':
                case '\\':
                    product.append('\\');
                    product.append(c);
                    break;

                case '\b':
                    product.append("\\b");
                    break;

                case '\f':
                    product.append("\\f");
                    break;

                case '\n':
                    product.append("\\n");
                    break;

                case '\r':
                    product.append("\\r");
                    break;

                case '\t':
                    product.append("\\t");
                    break;

                default:
                    if (c < ' ')
                    {
                        product.append('\\');
                        product.append('u');
                        product.append(StringBuilder::sprintf("%.4x", static_cast<int>(c)));
                    }
                    else
                    {
                        product.append(c);
                    }
                    break;
            }
        }

        product.append('"');
        return product.string();
    }

    bool json_ja(EsObject *val, JsonState &state, EsValue &result)
    {
        // FIXME: Why doesn't the following work?
        /*if (std::find(stack.begin(), stack.end(), val) != stack.end())
        {
            ES_THROW(EsTypeError, "FIXME: cannot serialize json object, the structure is cyclical.");
            return false;
        }*/
        {
            EsValueVector::iterator it;
            for (it = state.stack.begin(); it != state.stack.end(); ++it)
            {
                EsValue &cur_val = *it;

                if (cur_val.is_object() && cur_val.as_object() == val)
                {
                    ES_THROW(EsTypeError, _USTR("FIXME: cannot serialize json object, the structure is cyclical."));
                    return false;
                }
            }
        }

        state.stack.push_back(EsValue::from_obj(val));

        String stepback = state.indent;
        state.indent = state.indent + state.gap;

        StringVector partial;

        EsValue len;
        if (!val->getT(property_keys.length, len))
            return false;

        for (uint32_t i = 0; i < len.primitive_to_uint32(); i++)
        {
            EsValue str_p;
            if (!json_str(String(lexical_cast<const char *>(i)), val, state, str_p))
                return false;

            if (str_p.is_undefined())
                partial.push_back(_USTR("null"));
            else
            {
                String str;
                if (!str_p.to_string(str))
                    return false;

                partial.push_back(str); // CUSTOM: to_string.
            }
        }

        StringBuilder final;
        if (partial.empty())
        {
            final.append("[]");
        }
        else
        {
            String separator = _USTR(",\n");
            if (state.gap.empty())
            {
                separator = _USTR(",");
            }
            else
            {
                separator = _USTR(",\n");
                separator = separator + state.indent;
            }

            final.append('[');

            StringVector::const_iterator it;
            for (it = partial.begin(); it != partial.end(); ++it)
            {
                if (it != partial.begin())
                    final.append(separator);

                final.append(*it);
            }

            final.append(']');
        }

        state.stack.pop_back();
        state.indent = stepback;
        result = EsValue::from_str(final.string());
        return true;
    }

    bool json_jo(EsObject *val, JsonState &state, EsValue &result)
    {
        // FIXME: Why doesn't the following work?
        /*if (std::find(stack.begin(), stack.end(), val) != stack.end())
        {
            ES_THROW(EsTypeError, "FIXME: cannot serialize json object, the structure is cyclical.");
            return false;
        }*/
        {
            EsValueVector::iterator it;
            for (it = state.stack.begin(); it != state.stack.end(); ++it)
            {
                EsValue &cur_val = *it;

                if (cur_val.is_object() && cur_val.as_object() == val)
                {
                    ES_THROW(EsTypeError, _USTR("FIXME: cannot serialize json object, the structure is cyclical."));
                    return false;
                }
            }
        }

        state.stack.push_back(EsValue::from_obj(val));

        String stepback = state.indent;
        state.indent = state.indent + state.gap;

        StringVector k;
        if (!state.prop_list.empty())   // NOTE: Emptiness used to test undefinedness.
        {
            k = state.prop_list;
        }
        else
        {
            // This must be of the same ordering as Object.keys.
            for (EsObject::Iterator it = val->begin(); it != val->end(); ++it)
            {
                EsPropertyKey key = *it;

                EsPropertyReference prop = val->get_property(key);
                if (!prop->is_enumerable())
                    continue;

                k.push_back(key.to_string());
            }
        }

        StringBuilder member;

        StringVector partial;
        for (StringVector::const_iterator it = k.begin(); it != k.end(); ++it)
        {
            const String &p = *it;

            EsValue str_p;
            if (!json_str(p, val, state, str_p))
                return false;

            if (!str_p.is_undefined())
            {
                member.clear();
                member.append(json_quote(p));
                member.append(':');
                if (!state.gap.empty())
                    member.append(' ');

                String str;
                if (!str_p.to_string(str))
                    return false;

                member.append(str); // CUSTOM: to_string.

                partial.push_back(member.string());
            }
        }

        StringBuilder final;
        if (partial.empty())
        {
            final.append("{}");
        }
        else
        {
            String separator = _USTR(",\n");
            if (state.gap.empty())
            {
                separator = _USTR(",");
            }
            else
            {
                separator = _USTR(",\n");
                separator = separator + state.indent;
            }

            final.append('{');

            for (StringVector::const_iterator it = partial.begin(); it != partial.end(); ++it)
            {
                if (it != partial.begin())
                    final.append(separator);

                final.append(*it);
            }

            final.append('}');
        }

        state.stack.pop_back();
        state.indent = stepback;
        result = EsValue::from_str(final.string());
        return true;
    }
}
