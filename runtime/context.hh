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
#include <stddef.h>
#include "environment.hh"
#include "object.hh"

class EsContext
{
public:
    friend class EsContextStack;    // FIXME:

public:
    /**
     * @brief Context types.
     */
    enum Type
    {
        ES_GLOBAL,
        ES_EVAL,
        ES_FUNCTION,
        ES_CATCH,
        ES_WITH
    };

private:
    EsContext *outer_;                  ///< Outer context.
    Type type_;                         ///< Context type.
    bool strict_;

    EsLexicalEnvironment *lex_env_;     ///< [[LexicalEnvironment]]
    EsLexicalEnvironment *var_env_;     ///< [[VariableEnvironment]]
    EsValue this_binding_;              ///< [[ThisBinding]]

    /** Remember the original this value. Built-in functions are allowed to
     * access the original this value which has not been subjected to the
     * sanitation process in ECMA-262 10.4.3. */
    EsValue this_val_;
    
    /** If set to something other than 'nothing' this member flags that an
     * exception has been thrown and not catched. */
    EsValue pending_exception_;

public:
    EsContext(EsContext *outer, Type type,
              bool strict,
              EsLexicalEnvironment *lex_env,
              EsLexicalEnvironment *var_env,
              const EsValue &this_binding);
    EsContext(EsContext *outer, Type type,
              bool strict,
              EsLexicalEnvironment *lex_env,
              EsLexicalEnvironment *var_env,
              const EsValue &this_binding,
              const EsValue &this_val);
    EsContext(EsContext &rhs);
    
    /**
     * @return Pointer to the outer scope if there is any. If there is no outer
     *         scope a NULL-pointer is returned.
     */
    EsContext *outer();

    /**
     * @return true if the context is bound to an object, false if not.
     */
    bool is_obj_context() const;

    /**
     * @return Lexical environment used to resolved identifier references made
     *         by code within this execution context.
     */
    EsLexicalEnvironment *lex_env();
    
    /**
     * @return Lexical environment whose environment record holds bindings
     *         created by variable statements and function declarations within
     *         this execution context.
     */
    EsLexicalEnvironment *var_env();
    
    /**
     * @return Value associated with the 'this' keyword within ECMAScript code
     *         associated with this execution context.
     */
    EsValue this_binding();

    /**
     * @return Original this value supplied when creating the context. This
     *         value has not been subjected to the process described in
     *         ECMA-262 10.4.3.
     */
    EsValue this_value();
     
    /**
     * @return true if the context is in strict mode and false otherwise.
     */
    bool is_strict() const;

    /**
     * Enables or disables strict mode.
     * @param [in] strict true to enable and false to disable strict mode.
     */
    void set_strict(bool strict);

    /**
     * @return true if there is a pending exception.
     */
    bool has_pending_exception() const;

    /**
     * Clears any pending exception.
     */
    void clear_pending_exception();

    /**
     * Set a pending exception.
     * @param [in] val Thrown value, the value 'nothing' will clear the
     *                 pending exception.
     */
    void set_pending_exception(const EsValue &val);

    /**
     * @return Current pending exception. The value will be 'nothing' if there
     *         is no pending exception.
     */
    EsValue get_pending_exception() const;
};

class EsGlobalContext
{
public:
    EsGlobalContext(bool strict);
    ~EsGlobalContext();

    operator EsContext *();
};

class EsEvalContext
{
public:
    EsEvalContext(bool strict);
    ~EsEvalContext();

    operator EsContext *();
};

class EsFunctionContext
{
public:
    EsFunctionContext(bool strict,
                      EsLexicalEnvironment *scope,
                      const EsValue &this_val);
    ~EsFunctionContext();

    operator EsContext *();
};

class EsContextStack
{
private:
    typedef std::vector<EsContext *, gc_allocator<EsContext *> > EsContextVector;
    EsContextVector stack_;

    EsContextStack();
    EsContextStack(const EsContextStack &rhs);
    EsContextStack &operator=(const EsContextStack &rhs);

public:
    static EsContextStack &instance();

    EsContext *top();

    void push_global(bool strict);
    void push_eval(bool strict);
    void push_fun(bool strict, EsLexicalEnvironment *scope,
                  const EsValue &this_val);
    void push_catch(EsPropertyKey key,
                    const EsValue &c);
    bool push_withT(const EsValue &val);

    void pop();
    void unwind_to(uint32_t depth);
};
