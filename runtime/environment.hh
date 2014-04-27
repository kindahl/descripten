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
#include "object.hh"
#include "value.hh"

/**
 * @brief Interface representing an environment record.
 */
class EsEnvironmentRecord
{
public:
    virtual ~EsEnvironmentRecord() {}

    virtual bool is_decl_env() = 0;
    virtual bool is_obj_env() = 0;

    /**
     * Determines if an environment record has a binding for a bound identifier
     * name.
     * @param [in] n Bound name.
     * @return true if a binding exists for the identifier and false otherwise.
     */
    virtual bool has_binding(const EsPropertyKey &n) = 0;

    /**
     * Returns the value to use as the this value on calls to function objects
     * that are obtained as binding values from this environment record.
     * @return Implicit this value.
     */
    virtual EsValue implicit_this_value() const = 0;
};

/**
 * @brief Class representing a declarative environment record.
 */
class EsDeclarativeEnvironmentRecord : public EsEnvironmentRecord
{
private:
    struct Value
    {
        EsValue *val_;
        bool mutable_;
        bool removable_;

        Value(EsValue *val, bool is_mutable, bool removable)
            : val_(val), mutable_(is_mutable), removable_(removable) {}
    };

private:
    typedef std::map<EsPropertyKey, Value,
                     EsPropertyKey::Less,
                     gc_allocator<std::pair<EsPropertyKey, Value> > > VariableMap;  // FIXME: unordered_map?

    EsValue *storage_;      ///< Memory for storing values.
    VariableMap variables_;

public:
    EsDeclarativeEnvironmentRecord();
    virtual ~EsDeclarativeEnvironmentRecord() {}

    void set_storage(EsValue *storage)
    {
        storage_ = storage;
    }

    EsValue *storage()
    {
        return storage_;
    }

    virtual bool is_decl_env() override
    {
        return true;
    }

    virtual bool is_obj_env() override
    {
        return false;
    }

    /**
     * @copydoc EsEnvironmentRecord::has_binding
     */
    virtual bool has_binding(const EsPropertyKey &n) override;

    /**
     * @copydoc EsEnvironmentRecord::implicit_this_value
     */
    virtual EsValue implicit_this_value() const override;

    /**
     * Creates a new mutable binding in an environment record, linked to a pre-
     * allocated value in memory. If a binding already exist, the value of the
     * existing binding will be written to the location of the new link.
     * @param [in] n Bound name.
     * @param [in] d If true binding is may be subsequently deleted.
     * @param [in] v Pointer to value storage.
     * @param [in] inherit true if the value of any existing binding should be
     *                     copied to the link.
     */
    void link_mutable_binding(const EsPropertyKey &n, bool d, EsValue *v, bool inherit);

    /**
     * Creates a new mutable binding in an environment record, linked to a pre-
     * allocated value in memory. If a binding already exist, the value of the
     * existing binding will be written to the location of the new link.
     * @param [in] n Bound name.
     * @param [in] d If true binding is may be subsequently deleted.
     * @param [in] v Pointer to value storage.
     */
    void link_immutable_binding(const EsPropertyKey &n, EsValue *v);

    /**
     * Creates a new mutable binding in an environment record.
     * @param [in] n Bound name.
     * @param [in] d If true binding is may be subsequently deleted.
     */
    void create_mutable_binding(const EsPropertyKey &n, bool d);

    /**
     * Sets the value of an already existing mutable binding in an environment
     * record. If a matching bounding doesn't exist, this function will do
     * nothing.
     * @param [in] n Bound name.
     * @param [in] v Binding value, may be any ECMAScript language type.
     */
    void set_mutable_binding(const EsPropertyKey &n, const EsValue &v);

    /**
     * Sets the value of an already existing mutable binding in an environment
     * record.
     * @param [in] n Bound name.
     * @param [in] v Binding value, may be any ECMAScript language type.
     * @param [in] s If true and the binding cannot be set a TypeError exception
     *               will be thrown. It is used to identify strict mode
     *               references.
     * @return true on normal return, false if an exception was thrown.
     * @throw TypeError if s is true and the binding cannot be set.
     */
    bool set_mutable_bindingT(const EsPropertyKey &n, const EsValue &v, bool s);

    /**
     * Returns the value of an already existing binding from an environment
     * record.
     * @param [in] n Bound name.
     * @param [in] s Used to identify strict mode references. If true and the
     *               binding does not exist or is uninitialized a ReferenceError
     *               exception will be thrown.
     * @param [out] v Value of binding matching bound name.
     * @return true on normal return, false if an exception was thrown.
     * @throw ReferenceError If s is true and binding doesn't exist.
     */
    bool get_binding_valueT(const EsPropertyKey &n, bool s, EsValue &v);
    
    /**
     * Deletes a binding from an environment record.
     * @param [in] n Bound name.
     * @param [out] deleted true if the binding could be removed or if it
     *                      didn't exist and false otherwise.
     * @return true on normal return, false if an exception was thrown.
     */
    bool delete_bindingT(const EsPropertyKey &n, bool &deleted);

    /**
     * Creates and initializes a new immutable binding in an environment
     * record. This combines the CreateImmutableBinding and
     * InitializeImmutableBinding functions defined in ECMA-262.
     * @param [in] n Bound name.
     */
    void create_immutable_binding(const EsPropertyKey &n, const EsValue &v);
};

/**
 * @brief Class representing an object environment record.
 */
class EsObjectEnvironmentRecord : public EsEnvironmentRecord
{
private:
    bool provide_this_;
    EsObject *binding_object_;

public:
    EsObjectEnvironmentRecord(EsObject *binding_object, bool provide_this);
    virtual ~EsObjectEnvironmentRecord() {}

    virtual bool is_decl_env() override
    {
        return false;
    }

    virtual bool is_obj_env() override
    {
        return true;
    }

    EsObject *binding_object()
    {
        return binding_object_;
    }

    /**
     * @copydoc EsEnvironmentRecord::has_binding
     */
    virtual bool has_binding(const EsPropertyKey &n) override;

    /**
     * @copydoc EsEnvironmentRecord::implicit_this_value
     */
    virtual EsValue implicit_this_value() const override;
};

/**
 * @brief Class representing a lexical environment.
 */
class EsLexicalEnvironment
{
private:
    EsLexicalEnvironment *outer_;
    EsEnvironmentRecord *env_rec_;
    
public:
    EsLexicalEnvironment(EsLexicalEnvironment *outer,
                         EsEnvironmentRecord *env_rec);
    
    EsLexicalEnvironment *outer();
    EsEnvironmentRecord *env_rec();
};

EsValue es_get_this_value(EsLexicalEnvironment *lex, const EsPropertyKey &key);

EsLexicalEnvironment *es_new_decl_env(EsLexicalEnvironment *e);
EsLexicalEnvironment *es_new_obj_env(EsObject *o, EsLexicalEnvironment *e,
                                     bool provide_this);
