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
#include <cassert>
#include <map>
#include <vector>
#include <gc/gc_allocator.h>    // NOTE: 3rd party.
#define PCRE_STATIC
#include <pcre.h>               // NOTE: 3rd party.
#include "common/core.hh"
#include "common/string.hh"
#include "container.hh"
#include "map.hh"
#include "property_array.hh"
#include "property_key.hh"
#include "types.hh"

/**
 * Maximum index any item can have in an ECMA-262 Array object.
 */
#define ES_ARRAY_INDEX_MAX      0xffffffff

#ifndef ES_DEF_PROPERTY
#define ES_DEF_PROPERTY(obj, p, v, enumerable, configurable, writable) \
obj->EsObject::define_own_propertyT((p), EsPropertyDescriptor((enumerable), (configurable), (writable), (v)), false)
#endif

class EsCallFrame;
class EsContext;
class EsEnvironmentRecord;
class EsLexicalEnvironment;
class EsObject;
class EsPropertyDescriptor;
class EsFunction;

namespace parser
{
    class FunctionLiteral;
}
using parser::FunctionLiteral;

/**
 * The object hierarchy is as follows:
 * 
 * EsObject
 *   EsArguments
 *   EsArray
 *   EsBooleanObject
 *   EsNumberObject
 *   EsStringObject
 *   EsDate
 *   EsFunction
 *     EsBuiltinFunction
 *     EsEvalFunction
 *     EsFunctionBind
 *     EsArrayConstructor
 *     EsBooleanConstructor
 *     EsDateConstructor
 *     EsFunctionConstructor
 *     EsNumberConstructor
 *     EsStringConstructor
 *     EsArgumentGetter
 *     EsArgumentSetter
 *   EsRegExp
 */

/**
 * @brief Object type.
 */
class EsObject
{
public:
    friend class EsFunction;

public:
    /**
     * @brief Object property iterator.
     */
    class Iterator
    {
    public:
        friend class EsObject;

    private:
        EsObject *obj_;
        std::vector<EsPropertyKey> prop_keys_;
        std::vector<EsPropertyKey>::size_type pos_;

        /**
         * Constructs a new iterator from a property map iterator.
         * @param [in] obj Object to whom the property keys belong.
         * @param [in] prop_names List of property keys to iterate.
         * @param [in] begin true to create a "begin" iterator, false to create
         *                   an "end" iterator.
         */
        Iterator(EsObject *obj, const std::vector<EsPropertyKey> &prop_keys, bool begin)
            : obj_(obj)
            , prop_keys_(prop_keys)
            , pos_(begin ? 0 : prop_keys_.size())
        {
        }

    public:
        /**
         * Constructs an iterator pointing to nothing.
         */
        Iterator()
            : obj_(NULL)
            , pos_(static_cast<std::vector<EsPropertyKey>::size_type>(-1)) {}

        /**
         * @return Name of property at iterator position.
         * @pre Iterator points at a valid element.
         */
        EsPropertyKey operator*() const
        {
            assert(pos_ < prop_keys_.size());
            return prop_keys_[pos_];
        }

        /**
         * Increments iterator (pre-increment).
         * @return Current iterator.
         */
        Iterator &operator++()
        {
            if (pos_ < prop_keys_.size())
                pos_++;

            return *this;
        }

        /**
         * Increments iterator (post-increment).
         * @return Current iterator.
         */
        Iterator operator++(int)
        {
            Iterator res = *this;
            ++*this;
            return res;
        }

        /**
         * @return true if both iterators refer to the same property, false if
         *         not.
         */
        bool operator==(const Iterator &rhs) const
        {
            return pos_ == rhs.pos_;
        }

        /**
         * @return true if both iterators does not refer to the same property,
         *         false if not.
         */
        bool operator!=(const Iterator &rhs) const
        {
            return pos_ != rhs.pos_;
        }
    };

private:
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.

protected:
    EsObject *prototype_;   ///< [[Prototype]]
    String class_;          ///< [[Class]]
    bool extensible_;       ///< [[Extensible]]
    
    EsMap map_;
    EsPropertyArray indexed_properties_;

    EsObject();

public:
    virtual ~EsObject();
    
    /**
     * Turns the object into an object instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();

    /**
     * Turns the object into an object prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto();

    static EsObject *create_raw();
    static EsObject *create_inst();
    static EsObject *create_inst_with_class(const String &class_name);
    static EsObject *create_inst_with_prototype(EsObject *prototype);

    /**
     * @return Default object constructor.
     */
    static EsFunction *default_constr();
    
    /**
     * @return Object map.
     */
    const EsMap &map() const
    {
        return map_;
    }

    /**
     * @return Object map.
     */
    EsMap &map()
    {
        return map_;
    }

    /**
     * Returns the value to use as the this value on calls to function objects
     * that are obtained as binding values from this environment record.
     * @return Implicit this value.
     */
    virtual EsValue implicit_this_value()
    {
        return EsValue::from_obj(this);
    }

    /**
     * @return List of own property keys.
     */
    std::vector<EsPropertyKey> own_properties() const;

    /**
     * @return List of property keys, including inherited property keys.
     */
    std::vector<EsPropertyKey> properties() const;

    /**
     * @return Iterator to first property owned by the object.
     */
    Iterator begin();

    /**
     * @return Iterator to last property owned by the object.
     */
    Iterator end();

    /**
     * Creates an iterator for iterating the properties owned by this object
     * and all properties inherited recursively through the prototype chain.
     * @return Iterator to first property owned by the object.
     */
    Iterator begin_recursive();

    /**
     * Creates an iterator for iterating the properties owned by this object
     * and all properties inherited recursively through the prototype chain.
     * @return Iterator to last property owned by the root prototype.
     */
    Iterator end_recursive();

    /**
     * @return Prototype object if object has a prototype, NULL otherwise.
     * @see ECMA-262: [[Prototype]].
     */
    EsObject *prototype() { return prototype_; }

    /**
     * @return Object class name.
     * @see ECMA-262: [[Class]].
     */
    const String &class_name() const { return class_; }

    /**
     * @return true if the object is extensible and false otherwise.
     * @see ECMA-262: [[Extensible]].
     */
    bool is_extensible() { return extensible_; }

    /**
     * Sets the value of the internal [[Extensible]] property.
     * @see ECMA-262: [[Extensible]].
     */
    void set_extensible(bool extensible) { extensible_ = extensible; }
    
    /**
     * Retrieves a property hosted in this object matching the specified
     * property identifier.
     * @param [in] p Property name.
     * @return Property if there exist a property matching the name p, nothing
     *         otherwise.
     */
    virtual EsPropertyReference get_own_property(EsPropertyKey p);
    
    /**
     * Retrieves a property hosted in this object or anywhere in its prototype
     * chain.
     * @param [in] p Property name.
     * @return Property if there exist a property matching the name p, nothing
     *         otherwise.
     */
    EsPropertyReference get_property(EsPropertyKey p);
    
    /**
     * Implements internal method [[Get]] (P) as in 8.12.3.
     * Retrieves the result of evaluating the property matching the specified
     * prototype name.
     * @param [in] p Property name.
     * @param [out] v Evaluated value of property with name p.
     * @return true on normal return, false if an exception was thrown.
     */
    inline bool getT(EsPropertyKey p, EsValue &v)
    {
        EsPropertyReference prop;
        if (!getT(p, prop))
            return false;

        return get_resolveT(prop, v);
    }

    /**
     * Partial implementation of [[Get]] (P) as in 8.12.3. In contrast to the
     * how [[Get]] should be implemented this method doesn't resolve the
     * property reference but leaves that to the user. The property reference
     * can be resolved using get_resolveT().
     * @param [in] p Property name.
     * @param [out] prop Reference to property.
     * @return true on normal return, false if an exception was thrown.
     */
    virtual bool getT(EsPropertyKey p, EsPropertyReference &prop);

    /**
     * Resolves a property reference into a value. This involves calling the
     * getter on accessor properties or simply fetching the property value for
     * data properties.
     * @param [in] desc Reference to property.
     * @param [out] v Value obtained from property descriptor.
     * @return true on normal return, false if an exception was thrown.
     */
    bool get_resolveT(const EsPropertyReference &prop, EsValue &v);

    /**
     * Checks if it's possible to put a property with the specified property
     * identifier.
     * @param [in] p Property name.
     * @param [out] prop Reference to property to update, if any.
     * @return true if it's possible to put property p and false otherwise.
     */
    bool can_put(EsPropertyKey p, EsPropertyReference &prop);
    
    /**
     * Checks if it's possible to update an existing own property.
     * @param [in] current Reference to property to update.
     * @return true if it's possible to put property p and false otherwise.
     * @pre current is an own property of this object.
     */
    bool can_put_own(const EsPropertyReference &current);

    /**
     * Updates or creates a property with a new value.
     * @param [in] p Property name.
     * @param [in] v Property value.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function will fail silently.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeError on failure to put property.
     */
    bool putT(EsPropertyKey p, const EsValue &v, bool throws);

    /**
     * Updates an existing own property with a new value.
     * @param [in] p Property name.
     * @param [in] current Reference to property to update, may be invalid.
     * @param [in] v Property value.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function will fail silently.
     * @return true on normal return, false if an exception was thrown.
     * @pre current is an own property of this object.
     */
    bool put_ownT(EsPropertyKey p, EsPropertyReference &current,
                  const EsValue &v, bool throws);

    /**
     * Tests if a property exist.
     * @param [in] p Property name.
     * @return true if the property exist and false otherwise.
     */
    bool has_property(EsPropertyKey p);

    /**
     * Deletes a property from the object.
     * @param [in] p Property name.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function fail silently.
     * @param [out] removed true if the property was successfully removed or if
     *              no matching property exists and false otherwise.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeError if property couldn't be removed.
     */
    virtual bool removeT(EsPropertyKey p, bool throws, bool &removed);

    /**
     * Returns the default value for an object depending on its type.
     * @param [in] hint Preferred type if object supports multiple default
     *                  values.
     * @param [out] result Default type value.
     * @return true on normal return, false if an exception was thrown.
     */
    virtual bool default_valueT(EsTypeHint hint, EsValue &result);
    
    /**
     * Defines a new property for this object.
     * @param [in] p Property name.
     * @param [in] desc Property object.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function fail silently.
     * @param [out] defined true if the property was successfully defined,
     *                      false if it was not.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeException on failure.
     */
    virtual bool define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                      bool throws, bool &defined);

    /**
     * Utility function for defining a new property for this object.
     * @param [in] p Property name.
     * @param [in] desc Property object.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function fail silently.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeException on failure.
     */
    inline bool define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                     bool throws)
    {
        bool dummy;
        return define_own_propertyT(p, desc, throws, dummy);
    }

    /**
     * Updates an existing property.
     * @param [in] p Property name.
     * @param [in] current Reference to property to update.
     * @param [in] v New property value.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function fail silently.
     * @return true on normal return, false if an exception was thrown.
     */
    virtual bool update_own_propertyT(EsPropertyKey p, EsPropertyReference &current,
                                      const EsValue &v, bool throws);

    /**
     * Defines a new property for this object. This is an internal function
     * used when creating new objects. It should not be confused with the
     * define_own_propertyL which implements [[DefineOwnProperty]].
     * @param [in] p Property name.
     * @param [in] desc Property object.
     * @pre No property named p exist for the object.
     */
    void define_new_own_property(EsPropertyKey p, const EsPropertyDescriptor &desc);

    /**
     * Utility function for deleting a property from the object.
     * @param [in] p Property name.
     * @param [in] throws If true an exception will be thrown on failure, if
     *                    false the function fail silently.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeError if property couldn't be removed.
     */
    inline bool removeT(EsPropertyKey p, bool throws)
    {
        bool dummy;
        return removeT(p, throws, dummy);
    }
};

/**
 * @brief Array object class.
 */
class EsArguments : public EsObject
{
private:
    EsObject *param_map_;       ///< [[ParameterMap]]
    
    EsArguments();
    
    static EsFunction *make_arg_getter(EsValue *val);
    static EsFunction *make_arg_setter(EsValue *val);

public:    
    virtual ~EsArguments();

    static EsArguments *create_inst(EsFunction *callee,
                                    int argc, const EsValue argv[]);
    static EsArguments *create_inst(EsFunction *callee,
                                    int argc, EsValue argv[],
                                    int prmc, String prmv[]);

    void link_parameter(uint32_t i, EsValue *val);

    /**
     * @copydoc EsObject::get_own_property
     */
    virtual EsPropertyReference get_own_property(EsPropertyKey p) OVERRIDE;
    
    /**
     * @copydoc EsObject::getL
     */
    virtual bool getT(EsPropertyKey p, EsPropertyReference &prop) OVERRIDE;

    /**
     * @copydoc EsObject::removeL
     */
    virtual bool removeT(EsPropertyKey p, bool throws, bool &removed) OVERRIDE;
    
    /**
     * @copydoc EsObject::define_own_propertyL
     */
    virtual bool define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                      bool throws, bool &defined) OVERRIDE;

    /**
     * @copydoc EsObject::update_own_propertyL
     */
    virtual bool update_own_propertyT(EsPropertyKey p, EsPropertyReference &current,
                                      const EsValue &v, bool throws) OVERRIDE;
};

/**
 * @brief Array object class.
 */
class EsArray : public EsObject
{
private:
    EsArray();
    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.
    
public:
    virtual ~EsArray();
    
    /**
     * Turns the object into an array instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();
    
    /**
     * Turns the object into an array prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;
    
    static EsArray *create_raw();
    static EsArray *create_inst(uint32_t len = 0);
    static EsArray *create_inst_from_lit(int count, EsValue items[]);

    /**
     * @return Default array constructor.
     */
    static EsFunction *default_constr();

    /**
     * @copydoc EsObject::define_own_propertyL
     */
    virtual bool define_own_propertyT(EsPropertyKey p, const EsPropertyDescriptor &desc,
                                      bool throws, bool &defined) OVERRIDE;

    /**
     * @copydoc EsObject::update_own_propertyL
     */
    virtual bool update_own_propertyT(EsPropertyKey p, EsPropertyReference &current,
                                      const EsValue &v, bool throws) OVERRIDE;
};

/**
 * @brief Boolean object class.
 */
class EsBooleanObject : public EsObject
{
private:
    bool primitive_value_;      ///< [[PrimitiveValue]]
    
    EsBooleanObject();
    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.
    
public:
    virtual ~EsBooleanObject();
    
    /**
     * Turns the object into an array instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();

    /**
     * Turns the object into a boolean prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;
    
    static EsBooleanObject *create_raw();
    static EsBooleanObject *create_inst(bool primitive_value);
    
    /**
     * @return Value of the internal [[PrimitiveValue]] property.
     * @see ECMA-262: [[PrimitiveValue]].
     */
    bool primitive_value();
    
    /**
     * @return Default boolean constructor.
     */
    static EsFunction *default_constr();
};

/**
 * @brief Date object class.
 */
class EsDate : public EsObject
{
private:
    double primitive_value_;

    EsDate();
    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.
    
public:
    virtual ~EsDate();
    
    /**
     * Turns the object into a date instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();

    /**
     * Turns the object into a date prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;

    static EsDate *create_raw();
    static bool create_inst(EsValue &result);
    static bool create_inst(const EsValue &value, EsValue &result);
    static bool create_inst(const EsValue &year, const EsValue &month,
                            const EsValue *date, const EsValue *hours,
                            const EsValue *min, const EsValue *sec, const EsValue *ms,
                            EsValue &result);
    
    /**
     * @return Value of the internal [[PrimitiveValue]] property.
     * @see ECMA-262: [[PrimitiveValue]].
     */
    double primitive_value() const;

    /**
     * @return Value of the internal [[PrimitiveValue]] property converted to
     *         Unix time format.
     * @see ECMA-262: [[PrimitiveValue]].
     */
    time_t date_value() const;
    
    /**
     * @return Default date constructor.
     */
    static EsFunction *default_constr();

    /**
     * @copydoc EsObject::default_valueL
     */
    virtual bool default_valueT(EsTypeHint hint, EsValue &result) OVERRIDE;
};

/**
 * @brief Number object class.
 */
class EsNumberObject : public EsObject
{
private:
    double primitive_value_;    ///< [[PrimitiveValue]]
    
    EsNumberObject();
    EsNumberObject(double primitive_value);
    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.
    
public:    
    virtual ~EsNumberObject();
    
    /**
     * Turns the object into a number instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();

    /**
     * Turns the object into a number prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;

    static EsNumberObject *create_raw();
    static EsNumberObject *create_inst(double primitive_value);
    
    /**
     * @return Value of the internal [[PrimitiveValue]] property.
     * @see ECMA-262: [[PrimitiveValue]].
     */
    double primitive_value();

    /**
     * @return Default number constructor.
     */
    static EsFunction *default_constr();
};

/**
 * @brief String object class.
 */
class EsStringObject : public EsObject
{
private:
    String primitive_value_;    ///< [[PrimitiveValue]]
    
    EsStringObject();
    EsStringObject(const String &primitive_value);
    
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.

public:
    virtual ~EsStringObject();
    
    /**
     * Turns the object into a string instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();

    /**
     * Turns the object into a string prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;
    
    static EsStringObject *create_raw();
    static EsStringObject *create_inst(String primitive_value);
    
    /**
     * @return Value of the internal [[PrimitiveValue]] property.
     * @see ECMA-262: [[PrimitiveValue]].
     */
    const String &primitive_value() const;

    /**
     * @return Default string constructor.
     */
    static EsFunction *default_constr();
    
    /**
     * @copydoc EsObject::get_own_property
     */
    virtual EsPropertyReference get_own_property(EsPropertyKey p) OVERRIDE;
};

/**
 * @brief Callable function object.
 */
class EsFunction : public EsObject
{
public:
    friend class EsArray;
    friend class EsBooleanObject;
    friend class EsDate;
    friend class EsNumberObject;
    friend class EsStringObject;
    
public:
    typedef bool (*NativeFunction)(EsContext *ctx, int argc,
                                   EsValue *fp, EsValue *vp);

public:
    /**
     * @brief Call flags.
     */
    enum CallFlags
    {
        CALL_DIRECT_EVAL = 0x01,    ///< The call is a direct eval call.
    };

private:
    //EsFunction *throw_type_err_;  ///< [[ThrowTypeError]] Use es_throw_type_err().

    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.

protected:
    bool strict_;
    uint32_t len_;                  ///< Number of parameters.
    NativeFunction fun_;            ///< [[Code]] & [[FormatParameters]] mutually exclusive to code_.
    FunctionLiteral *code_;         ///< [[Code]] mutually exclusive to fun_.
    EsLexicalEnvironment *scope_;   ///< [[Scope]]
    
    /** true if calling this function requires creating the arguments
     * object. */
    bool needs_args_obj_;

    /** true if calling this function requires a this binding rather than a
     * this value. The this binding is used for accessing 'this' in lexically
     * created functions; that is functions created by the compiled program,
     * as well as any code evaluated by eval. */
    bool needs_this_binding_;

protected:
    EsFunction(EsLexicalEnvironment *scope, NativeFunction func,
               bool strict, uint32_t len, bool needs_this_binding);
    EsFunction(EsLexicalEnvironment *scope, FunctionLiteral *code,
               bool needs_this_binding);
    
public:
    virtual ~EsFunction();
    
    /**
     * Turns the object into a function instance.
     * @param [in] len Number of function parameters.
     * @paran [in] has_prototype true if the object should have a prototype property.
     * @pre Object has been created using create_raw().
     */
    void make_inst(bool has_prototype);

    /**
     * Turns the object into a function prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;

    static EsFunction *create_raw(bool strict = false);
    static EsFunction *create_inst(EsLexicalEnvironment *scope,
                                   NativeFunction func, bool strict,
                                   uint32_t len);
    static EsFunction *create_inst(EsLexicalEnvironment *scope,
                                   FunctionLiteral *code);

    uint32_t length() const { return len_; }
    
    /**
     * @return Default string constructor.
     */
    static EsFunction *default_constr();
    
    /**
     * @return true if the function is in strict mode and false if it's not.
     */
    bool is_strict() const { return strict_; }

    /**
     * @return Pointer to native function, if this is an interpreted function
     *         NULL is returned.
     */
    NativeFunction function() const { return fun_; }

    /**
     * @return Pointer to function code, if this is a native function NULL is
     *         returned.
     */
    FunctionLiteral *code() const { return code_; }
    
    /**
     * @return Function scope.
     */
    EsLexicalEnvironment *scope() const { return scope_; }

    /**
     * @return true if calling the function requires converting the this value
     *         into a formal ThisBinding.
     */
    bool needs_this_binding() const
    {
        return needs_this_binding_;
    }
    
    /**
     * Calls the function associated with this object.
     * @param [in,out] frame Call frame.
     * @param [in] flags Call flags.
     * @return true on normal return, false if an exception was thrown.
     */
    virtual bool callT(EsCallFrame &frame, int flags = 0);

    /**
     * @copydoc EsObject::getL
     */
    virtual bool getT(EsPropertyKey p, EsPropertyReference &prop) OVERRIDE;

    /**
     * Executes the function constructor.
     * @param [in,out] frame Call frame.
     *@return true on normal return, false if an exception was thrown.
     */
    virtual bool constructT(EsCallFrame &frame);
    
    /**
     * Tests if the function object inherits from the specified value.
     * @param [in] v Value to use for testing.
     * @param [out] result true if the function object inherits from v and
     *                     false otherwise.
     * @return true on normal return, false if an exception was thrown.
     */
    virtual bool has_instanceT(const EsValue &v, bool &result);
};

/**
 * @brief Built-in function object.
 */
class EsBuiltinFunction : public EsFunction
{
protected:
    EsBuiltinFunction(EsLexicalEnvironment *scope, NativeFunction func,
                      bool strict, uint32_t len, bool needs_this_binding);

public:
    virtual ~EsBuiltinFunction();

    static EsBuiltinFunction *create_inst(EsLexicalEnvironment *scope,
                                          NativeFunction func, int len,
                                          bool strict = false);

    /**
     * @copydoc EsFunction::callL
     */
    virtual bool callT(EsCallFrame &frame, int flags = 0) OVERRIDE;

    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief eval function object.
 */
class EsEvalFunction : public EsBuiltinFunction
{
protected:
    EsEvalFunction();

public:
    virtual ~EsEvalFunction();

    static EsEvalFunction *create_inst();

    /**
     * @copydoc EsFunction::callL
     */
    virtual bool callT(EsCallFrame &frame, int flags = 0) OVERRIDE;
};

/**
 * @brief Regular expression object.
 */
class EsRegExp : public EsObject
{
public:
    /**
     * @brief Single match in match result.
     */
    class MatchState
    {
    private:
        int off_;       ///< Offset of match in subject string.
        int len_;       ///< Length of match in subject string.
        String str_;    ///< Matched string.

    public:
        MatchState()
            : off_(-1), len_(-1) {}
        MatchState(int off, int len, String str)
            : off_(off), len_(len), str_(str) {}

        bool empty() const { return off_ == -1 && len_ == -1 && str_.empty(); }

        int offset() const { return off_; }
        int length() const { return len_; }
        const String &string() const { return str_; }
    };

    // FIXME: Merge with algorithm::MatchResult?
    class MatchResult
    {
    public:
        typedef std::vector<MatchState, gc_allocator<MatchState> > MatchStateVector;
        typedef MatchStateVector::const_iterator const_iterator;

    private:
        int end_index_;             ///< Index of last matched substring.
        MatchStateVector matches_;  ///< Matched substrings.

    public:
        MatchResult(const char *subject, int *out_ptr, int out_len);

        int end_index() const { return end_index_; }

        /**
         * Returns the list of matches strings. The first string identifies
         * portion of the subject string matched by the entire pattern. The
         * next string identifies the first capturing subpattern, and so on.
         * @return List of matched strings.
         */
        const MatchStateVector &matches() const { return matches_; }

        size_t length() const { return matches_.size(); }

        const_iterator begin() const { return matches_.begin(); }
        const_iterator end() const { return matches_.end(); }
    };

private:
    static EsFunction *default_constr_;     // Points to the default constructor, initialized lazily.

    String pattern_;
    bool global_;
    bool ignore_case_;
    bool multiline_;

    pcre *re_;          ///< Compiled expression, NULL if not compiled.
    int *re_out_ptr_;   ///< Output vector for pcre_exec function, must be a multiple of three in size.
    int re_out_len_;    ///< Length of re_out_ptr_.
    int re_capt_cnt_;   ///< Number of subexpressions to capture.

    /**
     * Compiles the regular expression.
     * @return true on normal return, false if an exception was thrown.
     */
    bool compile();

protected:
    EsRegExp(const String &pattern, bool global, bool ignore_case,
             bool multiline);
    
public:
    virtual ~EsRegExp();
    
    /**
     * Turns the object into a string instance.
     * @pre Object has been created using create_raw().
     */
    void make_inst();

    /**
     * Turns the object into a function prototype.
     * @pre Object has been created using create_raw().
     */
    virtual void make_proto() OVERRIDE;

    static EsRegExp *create_raw();
    static EsRegExp *create_inst(const String &pattern, bool global,
                                 bool ignore_case, bool multiline);
    static EsRegExp *create_inst(const String &pattern, const String &flags);

    /**
     * @return Regular expression pattern.
     */
    const String &pattern() const { return pattern_; }

    /**
     * @return Regular expression flags.
     */
    String flags() const;

    /**
     * Executes the regular expression given the specified subject.
     * @param [in] subject Subject string to run expression on.
     * @param [in] offset Offset in subject to start matching on.
     * @return Pointer to match result object on success and NULL on failure.
     */
    MatchResult *match(const String &subject, int offset);
    
    /**
     * @return Default string constructor.
     */
    static EsFunction *default_constr();
};

/**
 * @brief Function binding object.
 */
class EsFunctionBind : public EsFunction
{
private:
    EsFunction *target_fun_;        ///< [[TargetFunction]]
    EsValue bound_this_;            ///< [[BoundThis]]
    EsValueVector bound_args_;      ///< [[BoundArgs]]
    
private:
    EsFunctionBind(const EsValue &bound_this, EsFunction *target_fun,
                   const EsValueVector &args);

public:
    virtual ~EsFunctionBind();

    static EsFunctionBind *create_inst(EsFunction *target, const EsValue &bound_this,
                                       const EsValueVector &args);
    
    /**
     * @copydoc EsFunction::callL
     */
    virtual bool callT(EsCallFrame &frame, int flags = 0) OVERRIDE;
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
    
    /**
     * @copydoc EsFunction::has_instanceL
     */
    virtual bool has_instanceT(const EsValue &v, bool &result) OVERRIDE;
};

/**
 * @brief Array constructor class.
 */
class EsArrayConstructor : public EsFunction
{
private:
    EsArrayConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Boolean constructor class.
 */
class EsBooleanConstructor : public EsFunction
{
private:
    EsBooleanConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Date constructor class.
 */
class EsDateConstructor : public EsFunction
{
private:
    EsDateConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Number constructor class.
 */
class EsNumberConstructor : public EsFunction
{
private:
    EsNumberConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Function constructor class.
 */
class EsFunctionConstructor : public EsFunction
{
private:
    EsFunctionConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Object constructor class.
 */
class EsObjectConstructor : public EsFunction
{
private:
    EsObjectConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief String constructor class.
 */
class EsStringConstructor : public EsFunction
{
private:
    EsStringConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Regular expression constructor class.
 */
class EsRegExpConstructor : public EsFunction
{
private:
    EsRegExpConstructor(EsLexicalEnvironment *scope, NativeFunction func, bool strict);
    
public:
    static EsFunction *create_inst();
    
    /**
     * @copydoc EsFunction::constructL
     */
    virtual bool constructT(EsCallFrame &frame) OVERRIDE;
};

/**
 * @brief Getter function for function arguments.
 */
class EsArgumentGetter : public EsFunction
{
private:
    EsValue *val_;

    EsArgumentGetter(EsValue *val);

public:
    static EsArgumentGetter *create_inst(EsValue *val);

    /**
     * @copydoc EsFunction::callL
     */
    virtual bool callT(EsCallFrame &frame, int flags = 0) OVERRIDE;
};

/**
 * @brief Setter function for function arguments.
 */
class EsArgumentSetter : public EsFunction
{
private:
    EsValue *val_;

    EsArgumentSetter(EsValue *val);

public:
    static EsArgumentSetter *create_inst(EsValue *val);

    /**
     * @copydoc EsFunction::callL
     */
    virtual bool callT(EsCallFrame &frame, int flags = 0) OVERRIDE;
};
