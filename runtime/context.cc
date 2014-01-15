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

#ifdef DEBUG
#include <iostream>
#include <iomanip>
#endif
#include <cassert>
#include <vector>
#include <gc_cpp.h>
#include "context.hh"
#include "conversion.hh"
#include "global.hh"
#include "utility.hh"

EsContext::EsContext(EsContext *outer, Type type,
                     bool strict,
                     EsLexicalEnvironment *lex_env,
                     EsLexicalEnvironment *var_env)
    : outer_(outer)
    , type_(type)
    , strict_(strict)
    , lex_env_(lex_env)
    , var_env_(var_env)
    , pending_exception_(EsValue::nothing)
{
}

EsContext::EsContext(EsContext &rhs)
{
    outer_ = &rhs;
    type_ = rhs.type_;

    strict_ = rhs.strict_;
    lex_env_  = rhs.lex_env_;
    var_env_ = rhs.var_env_;
    pending_exception_ = rhs.pending_exception_;
}

EsContext *EsContext::outer()
{
    return outer_;
}

bool EsContext::is_obj_context() const
{
    switch (type_)
    {
        case ES_EVAL:
            assert(outer_);
            return strict_ ? false : outer_->is_obj_context();
        case ES_GLOBAL:
        case ES_WITH:
            return true;
        default:
            return false;
    }
}

EsLexicalEnvironment *EsContext::lex_env()
{
    return lex_env_;
}

EsLexicalEnvironment *EsContext::var_env()
{
    return var_env_;
}

bool EsContext::is_strict() const
{
    return strict_;
}

void EsContext::set_strict(bool strict)
{
    strict_ = strict;
}

bool EsContext::has_pending_exception() const
{
    return !pending_exception_.is_nothing();
}

void EsContext::clear_pending_exception()
{
    pending_exception_ = EsValue::nothing;
}

void EsContext::set_pending_exception(const EsValue &val)
{
    pending_exception_ = val;
}

EsValue EsContext::get_pending_exception() const
{
    return pending_exception_;
}

EsGlobalContext::EsGlobalContext(bool strict)
{
    EsContextStack::instance().push_global(strict);
}

EsGlobalContext::~EsGlobalContext()
{
    EsContextStack::instance().pop();
}

EsGlobalContext::operator EsContext *()
{
    return EsContextStack::instance().top();
}

EsEvalContext::EsEvalContext(bool strict)
{
    EsContextStack::instance().push_eval(strict);
}

EsEvalContext::~EsEvalContext()
{
    EsContextStack::instance().pop();
}

EsEvalContext::operator EsContext *()
{
    return EsContextStack::instance().top();
}

EsFunctionContext::EsFunctionContext(bool strict,
                                     EsLexicalEnvironment *scope)
{
    EsContextStack::instance().push_fun(strict, scope);
}

EsFunctionContext::~EsFunctionContext()
{
    EsContextStack::instance().pop();
}

EsFunctionContext::operator EsContext *()
{
    return EsContextStack::instance().top();
}

EsContextStack::EsContextStack()
{
}

EsContextStack &EsContextStack::instance()
{
    static EsContextStack inst;
    return inst;
}

EsContext *EsContextStack::top()
{
    return stack_.empty() ? NULL : stack_.back();
}

void EsContextStack::push_global(bool strict)
{
    stack_.push_back(new (GC)EsContext(top(), EsContext::ES_GLOBAL, strict,
                                       es_global_env(), es_global_env()));
}

void EsContextStack::push_eval(bool strict)
{
    // 10.4.2
    if (strict)
    {
        // FIXME: This is not nice, we're referencing the top context which is
        //        a new context, although it happens to be a copy of the
        //        previously top context.
        EsLexicalEnvironment *strict_var_env = es_new_decl_env(top()->lex_env());

        stack_.push_back(new (GC)EsContext(top(), EsContext::ES_EVAL, strict,
                                           strict_var_env, strict_var_env));
    }
    else
    {
        stack_.push_back(new (GC)EsContext(*top()));
    }
}

void EsContextStack::push_fun(bool strict, EsLexicalEnvironment *scope)
{
    // 10.4.3
    EsLexicalEnvironment *local_env = es_new_decl_env(scope);

    stack_.push_back(new (GC)EsContext(top(), EsContext::ES_FUNCTION, strict,
                                       local_env, local_env));
}

void EsContextStack::push_catch(EsPropertyKey key, const EsValue &c)
{
    EsLexicalEnvironment *catch_env = es_new_decl_env(top()->lex_env());

    assert(catch_env->env_rec()->is_decl_env());
    EsDeclarativeEnvironmentRecord *env =
        static_cast<EsDeclarativeEnvironmentRecord *>(catch_env->env_rec());

    env->create_mutable_binding(key, false);
    env->set_mutable_binding(key, c);

    stack_.push_back(new (GC)EsContext(top(), EsContext::ES_CATCH, top()->is_strict(),
                                       catch_env, catch_env));
}

bool EsContextStack::push_withT(const EsValue &val)
{
    EsObject *obj = val.to_objectT();
    if (!obj)
        return false;

    // 12.10
    EsLexicalEnvironment *new_env = es_new_obj_env(obj, top()->lex_env(), true);

    stack_.push_back(new (GC)EsContext(top(), EsContext::ES_WITH, top()->is_strict(),
                                       new_env, top()->var_env()));

    return true;
}

void EsContextStack::pop()
{
    assert(stack_.size() > 1);

    EsContext *old = top();
    stack_.pop_back();
    EsContext *cur = top();

    if (old->has_pending_exception())
        cur->set_pending_exception(old->get_pending_exception());
}

void EsContextStack::unwind_to(uint32_t depth)
{
    EsContext *old = top();

    stack_.resize(depth);
    assert(stack_.size() == depth);

    EsContext *cur = top();

    if (old->has_pending_exception())
        cur->set_pending_exception(old->get_pending_exception());
}
