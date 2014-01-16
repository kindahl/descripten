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
#include <vector>
#include <gc_cpp.h>             // NOTE: 3rd party.
#include <gc/gc_allocator.h>    // NOTE: 3rd party.
#include "container.hh"
#include "string.hh"
#include "types.hh"

class EsFunction;
class EsObject;
class EsRegExp;

namespace algorithm
{
    /**
     * @brief Class representing a string match result.
     */
    struct MatchResult
    {
    public:
        int end_index_;     ///< Index of last matched substring.
        EsStringVector cap_;
    };

    /**
     * @brief Keeps track of the state for the JSON stringify routine.
     * @see json_str, json_quote, json_ja and json_jo.
     */
    struct JsonState
    {
        const EsString *indent;
        const EsString *gap;
        EsStringVector prop_list;
        EsFunction *replacer_fun;   ///< Replacer function, NULL means undefined.
        EsValueVector stack;

        JsonState()
            : indent(EsString::create())
            , gap(EsString::create())
            , replacer_fun(NULL) {}
    };

    /**
     * Implements the abstract relational comparison algorithm according to
     * 11.8.5.
     * @param [in] x X-component.
     * @param [in] y Y-component.
     * @param [in] left_first true if the X-component should be evaluated before
     *                        the Y-component and false if the other evaluation
     *                        order should be used.
     * @param [out] result true if x is less than y, false if not and nothing if
     *                     undefined (as in the case with NaN comparisons).
     * @return true on normal return, false if an exception was thrown.
     */
    bool abstr_rel_compT(const EsValue &x, const EsValue &y, bool left_first,
                         Maybe<bool> &result);
    
    /**
     * Implements the abstract equality comparison algorithm according to
     * 11.9.3.
     * @param [in] x X-component.
     * @param [in] y Y-component.
     * @param [out] result true if x equals y and false otherwise.
     * @return true on normal return, false if an exception was thrown.
     */
    bool abstr_eq_compT(const EsValue &x, const EsValue &y, bool &result);
    
    /**
     * Implements the strict equality comparison algorithm according to
     * 11.9.6.
     * @param [in] x X-component.
     * @param [in] y Y-component.
     * @return true if x equals y and false otherwise.
     */
    bool strict_eq_comp(const EsValue &x, const EsValue &y);
    
    /**
     * Implements the same value algorithm according to 9.12.
     * @param [in] x X-component.
     * @param [in] y Y-component.
     * @return true if the objects are considered to be the same and false
     *              otherwise.
     */
    bool same_value(const EsValue &x, const EsValue &y);

    /**
     * Implements the split match algorithm according to 15.5.4.14.
     * @param [in] s String to operate on.
     * @param [in] q String offset at which to start splitting.
     * @param [in] r Regular expression pattern for splitting.
     * @return On success the sub strings, on failure NULL.
     */
    MatchResult *split_match(const EsString *s, uint32_t q, EsRegExp *r);

    /**
     * Implements the split match algorithm according to 15.5.4.14.
     * @param [in] s String to operate on.
     * @param [in] q String offset at which to start splitting.
     * @param [in] r String separate to split by.
     * @return On success the sub strings, on failure NULL.
     */
    MatchResult *split_match(const EsString *s, uint32_t q, const EsString *r);

    /**
     * Implements the sort compare algorithm according to 15.4.4.11.
     * @param [in] obj Object to sort.
     * @param [in] j J-th element.
     * @param [in] k K-th element.
     * @param [in] comparefn Optional compare function.
     * @param [out] result < 0.0 if the j-th element should be placed before
     *                     the k-th element. > 0.0 if the k-th element should
     *                     be placed before the j-th element. 0.0 if the
     *                     elements are considered equal.
     * @return true on normal return, false if an exception was thrown.
     */
    bool sort_compareT(EsObject *obj, uint32_t j, uint32_t k,
                       EsFunction *comparefn, double &result);

    /**
     * Implements the JSON walk algorithm according to 15.12.2.
     * @param [in] name Name of property in holder object.
     * @param [in] holder Holder object.
     * @param [in] reviver Reviver function.
     * @param [out] result Result of calling reviver with walk data.
     * @return true on normal return, false if an exception was thrown.
     */
    bool json_walkT(const EsString *name, EsObject *holder, EsFunction *reviver,
                    EsValue &result);

    /**
     * Implements the JSON string conversion algorithm according to 15.12.3.
     * @param [in] key Key property name.
     * @param [in] holder Holder object.
     * @param [in,out] state Internal state, must be initialized by caller.
     * @param [in] replacer_func Replacer function.
     * @param [out] result JSON expression in string format.
     * @return true on normal return, false if an exception was thrown.
     */
    bool json_strT(const EsString *key, EsObject *holder, JsonState &state,
                   EsValue &result);

    /**
     * Implements the JSON string quote wrapping algorithm according to
     * 15.12.3.
     * @param [in] val String to wrap and escape.
     * @return val with escaped content wrapped in quotes.
     */
    const EsString *json_quote(const EsString *val);

    /**
     * Implements the JSON array serialization algorithm according to 15.12.3.
     * @param [in] val Array object to serialize.
     * @param [in,out] state Internal state, must be initialized by caller.
     * @param [out] result val serialized to a string.
     * @return true on normal return, false if an exception was thrown.
     */
    bool json_jaT(EsObject *val, JsonState &state, EsValue &result);

    /**
     * Implements the JSON object serialization algorithm according to 15.12.3.
     * @param [in] val Object to serialize.
     * @param [in,out] state Internal state, must be initialized by caller.
     * @param [out] result val serialized to a string.
     * @return true on normal return, false if an exception was thrown.
     */
    bool json_joT(EsObject *val, JsonState &state, EsValue &result);
}
