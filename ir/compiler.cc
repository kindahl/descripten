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

#include <gc_cpp.h>
#include "common/cast.hh"
#include "common/conversion.hh"
#include "common/exception.hh"
#include "compiler.hh"
#include "name_generator.hh"
#include "template.hh"
#include "utility.hh"

namespace ir {

Compiler::Compiler()
    : is_in_epilogue_(false)
    , module_(NULL)
{
}

Compiler::~Compiler()
{
}

uint32_t Compiler::get_str_id(const String &str)
{
    // The runtime will generate string ids starting at zero going up. In order
    // to avoid collisions (compiler generating an id that will also be
    // selected by the runtime for another string) we start high and go low.
    // Theoretically there could be collisions, but that would mean that we
    // are handling more than 2^32-1 strings; at which point things start
    // breaking down for other reasons.
    static uint32_t next_id = std::numeric_limits<uint32_t>::max();

    StringIdMap::iterator it = strings_.find(str);
    if (it != strings_.end())
        return it->second;

    strings_.insert(std::make_pair(str, next_id));
    return next_id--;
}

Scope *Compiler::current_fun_scope()
{
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        Scope *scope = *it;
        switch (scope->type())
        {
            case Scope::TYPE_FUNCTION:
                return scope;
            case Scope::TYPE_WITH:
                return NULL;
        }
    }

    return NULL;
}

// FIXME: Rename to get_binding?
Value *Compiler::get_local(const String &name, Function *fun)
{
    assert(!scopes_.empty());

    int hops = 0;

    Scope *cur_fun_scope = NULL;
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        Scope *scope = *it;
        if (scope->type() == Scope::TYPE_FUNCTION)
        {
            cur_fun_scope = scope;
            break;
        }
    }

    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        Scope *scope = *it;

        // If we encounter a with scope, we cannot proceed as the lookup
        // depends on the runtime properties of the bound with object.
        if (scope->type() == Scope::TYPE_WITH)
            return NULL;

        if (scope->has_local(name))
        {
            if (hops == 0)
                return scope->get_local(name);

            // Argument object must be accessed through context lookup if not
            // present in current scope. The arguments object isn't known
            // outside the runtime. However, a local variable may override
            // the arguments object which is why we allow local access in the
            // current scope. We cannot presume that it's safe to reference an
            // arguments local in a parent scope since the current scope might
            // create an actual arguments object, not visible at compile-time.
            if (name == _USTR("arguments"))
                return NULL;

            // We need to do hops so we cannot use the indexed lookup directly.
            size_t index = safe_cast<ArrayElementConstant *>(
                scope->get_local(name))->index();

            //return fun->last_block()->push_bnd_extra_get(hops, index);
            assert(cur_fun_scope);
            return new (GC)ArrayElementConstant(cur_fun_scope->get_scope_stack(hops),
                                                index);
        }

        if (scope->type() == Scope::TYPE_FUNCTION)
            hops++;
    }

    return NULL;
}

Scope *Compiler::unroll_for_continue(Function *fun, const std::string &label)
{
    // Operate on a copy of the scope vector since the epilogues themselves
    // might alter the global state vector.
    ScopeVector scopes = scopes_;

    Block *unrl_block = NULL;

    ScopeVector::reverse_iterator it;
    for (it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        Scope *scope = *it;

        if (label.empty())
        {
            if (scope->type() == Scope::TYPE_ITERATION)
                return scope;
        }
        else
        {
            if (scope->type() == Scope::TYPE_ITERATION && scope->has_label(label))
                return scope;
        }

        if (!is_in_epilogue_ && scope->has_epilogue())
        {
            ScopedValue<bool> is_unrolling(is_in_epilogue_, true);

            if (unrl_block == NULL)
            {
                unrl_block = new (GC)Block(NameGenerator::instance().next());
                fun->last_block()->push_trm_jmp(unrl_block);

                fun->push_block(unrl_block);
            }

            scope->epilogue()->inflate(unrl_block, fun);
        }
    }

    return NULL;
}

Scope *Compiler::unroll_for_break(Function *fun, const std::string &label)
{
    // Operate on a copy of the scope vector since the epilogues themselves
    // might alter the global state vector.
    ScopeVector scopes = scopes_;

    Block *unrl_block = NULL;

    ScopeVector::reverse_iterator it;
    for (it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        Scope *scope = *it;

        if (label.empty())
        {
            if (scope->type() == Scope::TYPE_ITERATION ||
                scope->type() == Scope::TYPE_SWITCH)
                return scope;
        }
        else
        {
            if (scope->has_label(label))
                return scope;
        }

        if (!is_in_epilogue_ && scope->has_epilogue())
        {
            ScopedValue<bool> is_unrolling(is_in_epilogue_, true);

            if (unrl_block == NULL)
            {
                unrl_block = new (GC)Block(NameGenerator::instance().next());
                fun->last_block()->push_trm_jmp(unrl_block);

                fun->push_block(unrl_block);
            }

            scope->epilogue()->inflate(unrl_block, fun);
        }
    }

    return NULL;
}

Scope *Compiler::unroll_for_return(Function *fun)
{
    // Operate on a copy of the scope vector since the epilogues themselves
    // might alter the global state vector.
    ScopeVector scopes = scopes_;

    Block *unrl_block = NULL;

    ScopeVector::reverse_iterator it;
    for (it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        Scope *scope = *it;

        if (!is_in_epilogue_ && scope->has_epilogue())
        {
            ScopedValue<bool> is_unrolling(is_in_epilogue_, true);

            if (unrl_block == NULL)
            {
                unrl_block = new (GC)Block(NameGenerator::instance().next());
                fun->last_block()->push_trm_jmp(unrl_block);

                fun->push_block(unrl_block);
            }

            scope->epilogue()->inflate(unrl_block, fun);
        }

        if (scope->type() == Scope::TYPE_FUNCTION)
            return scope;
    }

    return NULL;
}

uint16_t Compiler::get_ctx_cid(uint64_t key)
{
    ScopeVector::reverse_iterator it;
    for (it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        Scope *scope = *it;

        if (scope->type() == Scope::TYPE_FUNCTION ||
            scope->type() == Scope::TYPE_WITH)
        {
            return scope->get_ctx_cache_id(key);
        }
    }

    assert(false);
    return 0;
}

uint64_t Compiler::get_prp_key(uint32_t id)
{
    return static_cast<uint64_t>(id);
}

uint64_t Compiler::get_prp_key(const String &str)
{
    assert(!str.empty());

    enum Flags
    {
        /** The property keys is an index (default). */
        IS_INDEX  = 0x0000000000000000,
        /** The property key is a string identifier. */
        IS_STRING = 0x8000000000000000,
    };

    uint32_t index = 0;
    if (es_str_to_index(str, index))
        return get_prp_key(index);

    return static_cast<uint64_t>(get_str_id(str)) | IS_STRING;
}

Value *Compiler::expand_prp_get(Value *dst, Function *fun,
                                MetaPropertyLoadInstruction *prp_load)
{
    if (StringConstant *str_const = dynamic_cast<StringConstant *>(prp_load->key()))
    {
        return fun->last_block()->push_prp_get(prp_load->object(),
                                               get_prp_key(str_const->value()), dst);
    }

    return fun->last_block()->push_prp_get_slow(prp_load->object(), prp_load->key(), dst);
}

Value *Compiler::expand_prp_put(Value *val, Function *fun,
                                MetaPropertyLoadInstruction *prp_load)
{
    if (StringConstant *str_const = dynamic_cast<StringConstant *>(prp_load->key()))
    {
        return fun->last_block()->push_prp_put(prp_load->object(),
                                               get_prp_key(str_const->value()), val);
    }

    return fun->last_block()->push_prp_put_slow(prp_load->object(), prp_load->key(), val);
}

Value *Compiler::expand_ref_get(Value *ref, Function *fun, Block *expt_block)
{
    if (!ref->type()->is_reference())
        return ref;

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref))
    {
        Value *r = NULL, *t = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        r = fun->last_block()->push_mem_alloc(Type::value());
        t = expand_prp_get(r, fun, prp_load);
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);

        fun->push_block(done_block);
        return r;
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref))
    {
        Value *r = NULL, *t = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        r = fun->last_block()->push_mem_alloc(Type::value());
        t = fun->last_block()->push_ctx_get(ctx_load->key(), r,
                                            get_ctx_cid(ctx_load->key()));
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);

        fun->push_block(done_block);
        return r;
    }

    assert(false);
    return NULL;
}

Value *Compiler::expand_ref_get(Value *ref, Value *dst, Function *fun,
                                Block *done_block, Block *expt_block)
{
    if (!ref->type()->is_reference())
    {
        fun->last_block()->push_mem_store(dst, ref);
        fun->last_block()->push_trm_jmp(done_block);
        return dst;
    }

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref))
    {
        Value *t = NULL;

        t = expand_prp_get(dst, fun, prp_load);
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);
        return dst;
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref))
    {
        Value *t = NULL;

        t = fun->last_block()->push_ctx_get(ctx_load->key(), dst,
                                            get_ctx_cid(ctx_load->key()));
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);
        return dst;
    }

    assert(false);
    return dst;
}

void Compiler::expand_ref_put(Value *ref, Value *val, Function *fun,
                              Block *expt_block)
{
    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref))
    {
        Value *t = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        t = expand_prp_put(val, fun, prp_load);
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);

        fun->push_block(done_block);
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref))
    {
        Value *t = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        t = fun->last_block()->push_ctx_put(ctx_load->key(), val,
                                            get_ctx_cid(ctx_load->key()));
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);

        fun->push_block(done_block);
    }
    else
    {
        fun->last_block()->push_mem_store(ref, val);
    }
}

void Compiler::expand_ref_put(Value *ref, Value *val, Function *fun,
                              Block *done_block, Block *expt_block)
{
    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref))
    {
        Value *t = NULL;

        t = expand_prp_put(val, fun, prp_load);
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref))
    {
        Value *t = NULL;

        t = fun->last_block()->push_ctx_put(ctx_load->key(), val,
                                            get_ctx_cid(ctx_load->key()));
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);
    }
    else
    {
        fun->last_block()->push_mem_store(ref, val);
        fun->last_block()->push_trm_jmp(done_block);
    }
}

void Compiler::reset()
{
    module_ = NULL;
}

Function *Compiler::parse_fun(const parser::FunctionLiteral *lit,
                              bool is_global)
{
    const std::string &fun_name = is_global
        ? RUNTIME_GLOBAL_FUNCTION_NAME
        : NameGenerator::instance().next(lit->name().utf8());

    Function *fun = new (GC)Function(fun_name, is_global);
    fun->set_meta(new (GC)Meta(lit->name(),
                               lit->location().begin(),
                               lit->location().end()));

    module_->push_function(fun);

    if (is_global)
        fun->last_block()->push_ctx_set_strict(lit->is_strict_mode());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(Scope::TYPE_FUNCTION));
    ScopedVectorValue<TemplateBlock> expt_action(
        exception_actions_, new (GC)ReturnFalseTemplateBlock());

    // Parse declarations.
    AnalyzedFunction *analyzed_fun =
        analyzer_.lookup(const_cast<parser::FunctionLiteral *>(lit));   // FIXME: const_cast
    assert(analyzed_fun);

    Block *body_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    // Locals, extra and temporary registers.
    Value *l = NULL, *e = NULL, *t = NULL;

    size_t start_locals = 0;    // Start of first non-parameter local in locals array.
    size_t start_extras = 0;    // Start of first non-parameter extra in extras array.

    if (!lit->needs_args_obj())
    {
        // Allocate locals stack.
        size_t num_params = lit->parameters().size();
        size_t num_locals = analyzed_fun->num_locals();
        if (num_locals > 0 || num_params > 0)
        {
            l = fun->last_block()->push_mem_alloc(
                new (GC)ArrayType(Type::value(), num_params + num_locals)); // FIXME: Doesn't this allocate too much?
            l->make_persistent();
        }

        // Copy parameters into locals array.
        if (num_params > 0)
            t = fun->last_block()->push_init_args(l, static_cast<int>(num_params));

        start_locals = num_params;

        // Initialize locals extra stack.
        size_t num_extra = analyzed_fun->num_extra();
        if (num_extra > 0)
        {
            e = fun->last_block()->push_bnd_extra_init(num_extra);
            e->make_persistent();
        }

        // Link references scope stacks.
        for (int hops : analyzed_fun->referenced_scopes())
        {
            t = fun->last_block()->push_bnd_extra_ptr(hops);
            t->make_persistent();
            scope->add_scope_stack(hops, t);
        }

        // Allocate parameters.
        start_extras = 0;

        AnalyzedVariableSet::const_iterator it_var;
        for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
        {
            const AnalyzedVariable *var = *it_var;
            if (!var->is_parameter())
                continue;

            if (!var->is_allocated())
                continue;

            switch (var->storage())
            {
                case AnalyzedVariable::STORAGE_LOCAL:
                {
                    scope->add_local(var->name(), new (GC)ArrayElementConstant(l, var->parameter_index()));

                    if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                    {
                        t = fun->last_block()->push_mem_elm_ptr(l, var->parameter_index());
                        t = fun->last_block()->push_link_var(get_prp_key(var->name()), lit->is_strict_mode(), t);
                    }
                    break;
                }

                case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
                {
                    Value *v = new (GC)ArrayElementConstant(e, start_extras++);
                    scope->add_local(var->name(), v);

                    t = fun->last_block()->push_mem_store(v, new (GC)ArrayElementConstant(l, var->parameter_index()));
                    break;
                }

                case AnalyzedVariable::STORAGE_CONTEXT:
                {
                    Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

                    t = fun->last_block()->push_decl_prm(get_prp_key(var->name()), lit->is_strict_mode(),
                                                         var->parameter_index(), l);
                    t = fun->last_block()->push_trm_br(t, blk0_block, expt_block);

                    fun->push_block(blk0_block);
                    break;
                }

                default:
                    assert(false);
                    break;
            }
        }
    }
    else
    {
        // Allocate locals stack.
        size_t num_locals = analyzed_fun->num_locals();
        if (num_locals > 0)
        {
            l = fun->last_block()->push_mem_alloc(
                new (GC)ArrayType(Type::value(), num_locals));
            l->make_persistent();
        }

        // Initialize locals extra stack.
        size_t num_extra = analyzed_fun->num_extra();
        if (num_extra > 0)
        {
            e = fun->last_block()->push_bnd_extra_init(num_extra);
            e->make_persistent();
        }

        // Copy parameters into extra array.
        size_t num_params = lit->parameters().size();
        if (num_params > 0)
        {
            // Note, parameters are stored in extra.
            t = fun->last_block()->push_init_args(e, static_cast<int>(num_params));
        }

        start_extras = num_params;

        // Link references scope stacks.
        for (int hops : analyzed_fun->referenced_scopes())
        {
            t = fun->last_block()->push_bnd_extra_ptr(hops);
            t->make_persistent();
            scope->add_scope_stack(hops, t);
        }

        // Initialize the arguments object.
        Value *a = NULL;

        a = fun->last_block()->push_args_obj_init(0);

        // Allocate parameters.
        AnalyzedVariableSet::const_iterator it_var;
        for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
        {
            const AnalyzedVariable *var = *it_var;
            if (!var->is_parameter())
                continue;

            if (!var->is_allocated())
                continue;

            t = fun->last_block()->push_mem_elm_ptr(e, var->parameter_index());
            t = fun->last_block()->push_args_obj_link(a, var->parameter_index(), t);

            switch (var->storage())
            {
                case AnalyzedVariable::STORAGE_LOCAL:
                case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
                {
                    scope->add_local(var->name(), new (GC)ArrayElementConstant(e, var->parameter_index()));

                    if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                    {
                        t = fun->last_block()->push_mem_elm_ptr(e, var->parameter_index());
                        t = fun->last_block()->push_link_prm(get_prp_key(var->name()), lit->is_strict_mode(), t);
                    }
                    break;
                }

                case AnalyzedVariable::STORAGE_CONTEXT:
                {
                    scope->add_local(var->name(), new (GC)ArrayElementConstant(e, var->parameter_index()));

                    t = fun->last_block()->push_mem_elm_ptr(e, var->parameter_index());
                    t = fun->last_block()->push_link_prm(get_prp_key(var->name()), lit->is_strict_mode(), t);
                    break;
                }

                default:
                    assert(false);
                    break;
            }
        }
    }

    size_t locals_index = start_locals;
    size_t extras_index = start_extras;

    // Allocate callee access.
    AnalyzedVariableSet::const_iterator it_var;
    for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
    {
        const AnalyzedVariable *var = *it_var;
        if (!var->is_callee())
            continue;

        if (!var->is_allocated())
            continue;

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                Value *v = new (GC)ArrayElementConstant(l, locals_index++);
                t = fun->last_block()->push_mem_store(v, new (GC)CalleeConstant());

                scope->add_local(var->name(), v);
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                Value *v = new (GC)ArrayElementConstant(e, extras_index++);
                t = fun->last_block()->push_mem_store(v, new (GC)CalleeConstant());

                scope->add_local(var->name(), v);
                break;
            }
        }
    }

    // Allocate function declarations.
    for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
    {
        const AnalyzedVariable *var = *it_var;
        if (!var->is_declaration())
            continue;

        if (!var->is_allocated())
            continue;

        const parser::Declaration *decl = var->declaration();
        if (!decl->is_function())
            continue;

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(l, locals_index++));
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(e, extras_index++));
                break;
            }
        }
    }

    // Allocate variable declarations.
    for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
    {
        const AnalyzedVariable *var = *it_var;
        if (!var->is_declaration())
            continue;

        if (!var->is_allocated())
            continue;

        const parser::Declaration *decl = var->declaration();
        if (!decl->is_variable())
            continue;

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(l, locals_index++));
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(e, extras_index++));
                break;
            }
        }
    }

    locals_index = start_locals;
    extras_index = start_extras;

    // Increment storage indexes for callee allocation.
    for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
    {
        const AnalyzedVariable *var = *it_var;
        if (!var->is_callee())
            continue;

        if (!var->is_allocated())
            continue;

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
                locals_index++;
                break;

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
                extras_index++;
                break;
        }
    }

    // Parse function declarations.
    for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
    {
        const AnalyzedVariable *var = *it_var;
        if (!var->is_declaration())
            continue;

        if (!var->is_allocated())
            continue;

        const parser::Declaration *decl = var->declaration();
        if (!decl->is_function())
            continue;

        Value *f = parse(decl->as_function(), fun);

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                t = fun->last_block()->push_arr_put(locals_index, l, f);
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(l, locals_index);
                    t = fun->last_block()->push_link_fun(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                locals_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                t = fun->last_block()->push_arr_put(extras_index, e, f);
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(e, extras_index);
                    t = fun->last_block()->push_link_fun(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                extras_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_CONTEXT:
            {
                Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

                t = fun->last_block()->push_decl_fun(get_prp_key(var->name()), lit->is_strict_mode(), f);
                t = fun->last_block()->push_trm_br(t, blk0_block, expt_block);

                fun->push_block(blk0_block);
                break;
            }

            default:
                assert(false);
                break;
        }
    }

    // Parse variable declarations.
    for (it_var = analyzed_fun->variables().begin(); it_var != analyzed_fun->variables().end(); ++it_var)
    {
        const AnalyzedVariable *var = *it_var;
        if (!var->is_declaration())
            continue;

        if (!var->is_allocated())
            continue;

        const parser::Declaration *decl = var->declaration();
        if (!decl->is_variable())
            continue;

        parse(decl->as_variable(), fun);

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                t = fun->last_block()->push_arr_put(locals_index, l,
                                                    new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED));
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(l, locals_index);
                    t = fun->last_block()->push_link_var(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                locals_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                t = fun->last_block()->push_arr_put(extras_index, e,
                                                    new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED));
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(e, extras_index);
                    t = fun->last_block()->push_link_var(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                extras_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_CONTEXT:
            {
                Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

                t = fun->last_block()->push_decl_var(get_prp_key(var->name()), lit->is_strict_mode());
                t = fun->last_block()->push_trm_br(t, blk0_block, expt_block);

                fun->push_block(blk0_block);
                break;
            }

            default:
                assert(false);
                break;
        }
    }

    fun->last_block()->push_trm_jmp(body_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(body_block);

    // Parse statements.
    parser::StatementVector::const_iterator it_stmt;
    for (it_stmt = lit->body().begin(); it_stmt != lit->body().end(); ++it_stmt)
        parse(*it_stmt, fun);

    // Make sure the function return something.
    if (fun->last_block()->empty() || !fun->last_block()->last_instr()->is_terminating())
    {
        fun->last_block()->push_mem_store(new (GC)ReturnConstant(),
                                          new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED));
        fun->last_block()->push_trm_ret(new (GC)BooleanConstant(true));
    }

    return fun;
}

Value *Compiler::parse_binary_expr(parser::BinaryExpression *expr,
                                   Function *fun)
{
    Value *r = NULL, *t = NULL, *lhs = NULL;  // Return, temporary and LHS values.

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    t = parse(expr->left(), fun);
    lhs = expand_ref_get(t, fun, expt_block);
    r = fun->last_block()->push_mem_alloc(Type::value());

    if (expr->operation() == parser::BinaryExpression::LOG_AND)
    {
        // Short-circuit evaluation.
        Block *true_block = new (GC)Block(NameGenerator::instance().next());
        Block *false_block = new (GC)Block(NameGenerator::instance().next());

        Value *b = NULL;

        b = fun->last_block()->push_val_to_bool(lhs);
        t = fun->last_block()->push_trm_br(b, true_block, false_block);

        // True block.
        fun->push_block(true_block);
        {
            t = parse(expr->right(), fun);
            t = expand_ref_get(t, r, fun, done_block, expt_block);
        }

        // False block.
        fun->push_block(false_block);
        {
            t = fun->last_block()->push_mem_store(r, lhs);
            t = fun->last_block()->push_trm_jmp(done_block);
        }
    }
    else if (expr->operation() == parser::BinaryExpression::LOG_OR)
    {
        // Short-circuit evaluation.
        Block *true_block = new (GC)Block(NameGenerator::instance().next());
        Block *false_block = new (GC)Block(NameGenerator::instance().next());

        t = fun->last_block()->push_val_to_bool(lhs);
        t = fun->last_block()->push_trm_br(t, true_block, false_block);

        // True block.
        fun->push_block(true_block);
        {
            t = fun->last_block()->push_mem_store(r, lhs);
            t = fun->last_block()->push_trm_jmp(done_block);
        }

        // False block.
        fun->push_block(false_block);
        {
            t = parse(expr->right(), fun);
            t = expand_ref_get(t, r, fun, done_block, expt_block);
        }
    }
    else
    {
        Value *rhs = NULL;

        t = parse(expr->right(), fun);
        rhs = expand_ref_get(t, fun, expt_block);

        switch (expr->operation())
        {
            case parser::BinaryExpression::COMMA:
            {
                // Don't do anything, the comma expression only requires us to
                // call GetValue which we already have done.
                t = fun->last_block()->push_mem_store(r, rhs);
                t = fun->last_block()->push_trm_jmp(done_block);
                break;
            }

            // Arithmetic.
            case parser::BinaryExpression::MUL:
            {
                t = fun->last_block()->push_es_bin_mul(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::DIV:
            {
                t = fun->last_block()->push_es_bin_div(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::MOD:
            {
                t = fun->last_block()->push_es_bin_mod(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::ADD:
            {
                t = fun->last_block()->push_es_bin_add(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::SUB:
            {
                t = fun->last_block()->push_es_bin_sub(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::LS:  // <<
            {
                t = fun->last_block()->push_es_bin_ls(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::RSS: // >>
            {
                t = fun->last_block()->push_es_bin_rss(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::RUS: // >>>
            {
                t = fun->last_block()->push_es_bin_rus(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }

            // Relational.
            case parser::BinaryExpression::LT:
            {
                t = fun->last_block()->push_es_bin_lt(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::GT:
            {
                t = fun->last_block()->push_es_bin_gt(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::LTE:
            {
                t = fun->last_block()->push_es_bin_lte(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::GTE:
            {
                t = fun->last_block()->push_es_bin_gte(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::IN:
            {
                t = fun->last_block()->push_es_bin_in(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::INSTANCEOF:
            {
                t = fun->last_block()->push_es_bin_instanceof(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }

            // Equality.
            case parser::BinaryExpression::EQ:
            {
                t = fun->last_block()->push_es_bin_eq(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::NEQ:
            {
                t = fun->last_block()->push_es_bin_neq(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::STRICT_EQ:
            {
                t = fun->last_block()->push_es_bin_strict_eq(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::STRICT_NEQ:
            {
                t = fun->last_block()->push_es_bin_strict_neq(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }

            // Bitwise.
            case parser::BinaryExpression::BIT_AND:
            {
                t = fun->last_block()->push_es_bin_bit_and(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::BIT_XOR:
            {
                t = fun->last_block()->push_es_bin_bit_xor(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::BIT_OR:
            {
                t = fun->last_block()->push_es_bin_bit_or(lhs, rhs, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);
                break;
            }

            // Logical.
            case parser::BinaryExpression::LOG_AND:
            case parser::BinaryExpression::LOG_OR:
                break;

            default:
                assert(false);
                break;
        }
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_unary_expr(parser::UnaryExpression *expr,
                                  Function *fun)
{
    Value *r = NULL, *t = NULL, *e = NULL;  // Return, temporary and expression values.

    if (expr->operation() == parser::UnaryExpression::DELETE)
    {
        if (parser::PropertyExpression *prop =
            dynamic_cast<parser::PropertyExpression *>(expr->expression()))
        {
            // Test if we have an immediate property key that we can use. If that's the
            // case we don't have to go the to_string() path and can intern the string
            // in the data section for performance.
            String immediate_key_str;
            if (parser::NumberLiteral *lit =
                dynamic_cast<parser::NumberLiteral *>(prop->key()))
            {
                immediate_key_str = lit->as_string();
            }

            if (parser::StringLiteral *lit =
                dynamic_cast<parser::StringLiteral *>(prop->key()))
            {
                immediate_key_str = lit->value();
            }

            if (!immediate_key_str.empty())
            {
                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *expt_block = new (GC)Block(NameGenerator::instance().next());

                Value *obj = expand_ref_get(parse(prop->obj(), fun), fun, expt_block);

                r = fun->last_block()->push_mem_alloc(Type::value());
                t = fun->last_block()->push_prp_del(obj, get_prp_key(immediate_key_str), r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);

                fun->push_block(expt_block);
                exception_action()->inflate(expt_block, fun);

                fun->push_block(done_block);
            }
            else
            {
                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *expt_block = new (GC)Block(NameGenerator::instance().next());

                Value *key = expand_ref_get(parse(prop->key(), fun), fun, expt_block);
                Value *obj = expand_ref_get(parse(prop->obj(), fun), fun, expt_block);

                r = fun->last_block()->push_mem_alloc(Type::value());
                t = fun->last_block()->push_prp_del_slow(obj, key, r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);

                fun->push_block(expt_block);
                exception_action()->inflate(expt_block, fun);

                fun->push_block(done_block);
            }
        }
        else if (parser::IdentifierLiteral * ident =
            dynamic_cast<parser::IdentifierLiteral *>(expr->expression()))
        {
            Scope *scope = current_fun_scope();
            if (scope && scope->has_local(ident->value()))
            {
                // Having a local implies:
                // 1. The local represents a binding in a declarative
                //    environment record.
                // 2. The local does not belong to an eval context.
                // This means that trying to delete the entity should fail.
                r = new (GC)ValueConstant(ValueConstant::VALUE_FALSE);
            }
            else
            {
                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *expt_block = new (GC)Block(NameGenerator::instance().next());

                r = fun->last_block()->push_mem_alloc(Type::value());
                t = fun->last_block()->push_ctx_del(get_prp_key(ident->value()), r);
                t = fun->last_block()->push_trm_br(t, done_block, expt_block);

                fun->push_block(expt_block);
                exception_action()->inflate(expt_block, fun);

                fun->push_block(done_block);
            }
        }
        else
        {
            r = new (GC)ValueConstant(ValueConstant::VALUE_TRUE);
        }

        assert(r);
        return r;
    }

    e = parse(expr->expression(), fun);

    switch (expr->operation())
    {
        case parser::UnaryExpression::VOID:
        {
            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            r = new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED);
            t = fun->last_block()->push_mem_alloc(Type::value());
            t = expand_ref_get(e, t, fun, done_block, expt_block);

            fun->push_block(expt_block);
            exception_action()->inflate(expt_block, fun);

            fun->push_block(done_block);
            break;
        }
        case parser::UnaryExpression::PLUS:
        case parser::UnaryExpression::PRE_INC:
        case parser::UnaryExpression::PRE_DEC:
        case parser::UnaryExpression::POST_INC:
        case parser::UnaryExpression::POST_DEC:
        {
            Value *d = NULL, *v = NULL;

            Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            v = expand_ref_get(e, fun, expt_block);

            d = fun->last_block()->push_mem_alloc(Type::_double());
            t = fun->last_block()->push_val_to_double(v, d);
            t = fun->last_block()->push_trm_br(t, blk0_block, expt_block);

            fun->push_block(blk0_block);
            switch (expr->operation())
            {
                case parser::UnaryExpression::PLUS:
                    r = fun->last_block()->push_val_from_double(d);
                    t = fun->last_block()->push_trm_jmp(done_block);

                    // Needed for early return:
                    fun->push_block(expt_block);
                    exception_action()->inflate(expt_block, fun);

                    fun->push_block(done_block);
                    return r;   // Early return.
                case parser::UnaryExpression::PRE_INC:
                    t = fun->last_block()->push_bin_add(d, new (GC)DoubleConstant(1.0));
                    t = fun->last_block()->push_val_from_double(t);
                    r = t;
                    break;
                case parser::UnaryExpression::PRE_DEC:
                    t = fun->last_block()->push_bin_sub(d, new (GC)DoubleConstant(1.0));
                    t = fun->last_block()->push_val_from_double(t);
                    r = t;
                    break;
                case parser::UnaryExpression::POST_INC:
                    r = fun->last_block()->push_val_from_double(d);
                    t = fun->last_block()->push_bin_add(d, new (GC)DoubleConstant(1.0));
                    t = fun->last_block()->push_val_from_double(t);
                    break;
                case parser::UnaryExpression::POST_DEC:
                    r = fun->last_block()->push_val_from_double(d);
                    t = fun->last_block()->push_bin_sub(d, new (GC)DoubleConstant(1.0));
                    t = fun->last_block()->push_val_from_double(t);
                    break;
            }

            expand_ref_put(e, t, fun, done_block, expt_block);

            fun->push_block(expt_block);
            exception_action()->inflate(expt_block, fun);

            fun->push_block(done_block);
            break;
        }
        case parser::UnaryExpression::TYPEOF:
        {
            r = fun->last_block()->push_mem_alloc(Type::value());

            Value *v = NULL;
            if (e->type()->is_reference())
            {
                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *fail_block = new (GC)Block(NameGenerator::instance().next());

                v = fun->last_block()->push_mem_alloc(Type::value());
                t = expand_ref_get(e, v, fun, done_block, fail_block);

                fun->push_block(fail_block);
                t = fun->last_block()->push_ex_clear();
                t = fun->last_block()->push_mem_store(v,
                    new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED));
                t = fun->last_block()->push_trm_jmp(done_block);

                fun->push_block(done_block);
            }
            else
            {
                v = e;
            }

            assert(v);
            t = fun->last_block()->push_es_unary_typeof(v, r);

            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            fun->last_block()->push_trm_br(t, done_block, expt_block);

            fun->push_block(expt_block);
            exception_action()->inflate(expt_block, fun);

            fun->push_block(done_block);
            break;
        }
        case parser::UnaryExpression::MINUS:
        case parser::UnaryExpression::BIT_NOT:
        case parser::UnaryExpression::LOG_NOT:
        {
            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            r = fun->last_block()->push_mem_alloc(Type::value());

            switch (expr->operation())
            {
                case parser::UnaryExpression::MINUS:
                    t = fun->last_block()->push_es_unary_neg(
                        expand_ref_get(e, fun, expt_block), r);
                    break;
                case parser::UnaryExpression::BIT_NOT:
                    t = fun->last_block()->push_es_unary_bit_not(
                        expand_ref_get(e, fun, expt_block), r);
                    break;
                case parser::UnaryExpression::LOG_NOT:
                    t = fun->last_block()->push_es_unary_log_not(
                        expand_ref_get(e, fun, expt_block), r);
                    break;
            }

            assert(t != NULL);

            fun->last_block()->push_trm_br(t, done_block, expt_block);

            fun->push_block(expt_block);
            exception_action()->inflate(expt_block, fun);

            fun->push_block(done_block);
            break;
        }

        default:
            assert(false);
            break;
    }

    assert(r);
    return r;
}

Value *Compiler::parse_assign_expr(parser::AssignmentExpression *expr,
                                   Function *fun)
{
    Value *l = parse(expr->lhs(), fun);
    Value *r = parse(expr->rhs(), fun);
    Value *v = fun->last_block()->push_mem_alloc(Type::value());
    Value *t = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    if (expr->operation() == parser::AssignmentExpression::ASSIGN)
    {
        v = expand_ref_get(r, fun, expt_block);
    }
    else
    {
        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

        switch (expr->operation())
        {
            case parser::AssignmentExpression::ASSIGN_ADD:
                t = fun->last_block()->push_es_bin_add(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_SUB:
                t = fun->last_block()->push_es_bin_sub(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_MUL:
                t = fun->last_block()->push_es_bin_mul(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_MOD:
                t = fun->last_block()->push_es_bin_mod(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_LS:
                t = fun->last_block()->push_es_bin_ls(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_RSS:
                t = fun->last_block()->push_es_bin_rss(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_RUS:
                t = fun->last_block()->push_es_bin_rus(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_BIT_AND:
                t = fun->last_block()->push_es_bin_bit_and(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_BIT_OR:
                t = fun->last_block()->push_es_bin_bit_or(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_BIT_XOR:
                t = fun->last_block()->push_es_bin_bit_xor(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;
            case parser::AssignmentExpression::ASSIGN_DIV:
                t = fun->last_block()->push_es_bin_div(
                    expand_ref_get(l, fun, expt_block),
                    expand_ref_get(r, fun, expt_block), v);
                break;

            default:
                assert(false);
                break;
        }

        t = fun->last_block()->push_trm_br(t, blk0_block, expt_block);
        fun->push_block(blk0_block);
    }

    expand_ref_put(l, v, fun, done_block, expt_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return v;
}

Value *Compiler::parse_cond_expr(parser::ConditionalExpression *expr,
                                 Function *fun)
{
    // Result, temporary and boolean values.
    Value *r = NULL, *t = NULL, *b = NULL;

    Block *true_block = new (GC)Block(NameGenerator::instance().next());
    Block *false_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    r = fun->last_block()->push_mem_alloc(Type::value());
    t = parse(expr->condition(), fun);
    t = expand_ref_get(t, fun, expt_block);
    b = fun->last_block()->push_val_to_bool(t);
    t = fun->last_block()->push_trm_br(b, true_block, false_block);

    // True block.
    fun->push_block(true_block);
    {
        t = parse(expr->left(), fun);
        t = expand_ref_get(t, r, fun, done_block, expt_block);
    }

    // False block.
    fun->push_block(false_block);
    {
        t = parse(expr->right(), fun);
        t = expand_ref_get(t, r, fun, done_block, expt_block);
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_prop_expr(parser::PropertyExpression *expr,
                                 Function *fun)
{
    // Result and temporary values.
    Value *r = NULL, *t = NULL;

    // Test if we have an immediate property key that we can use. If that's the
    // case we don't have to go the to_string() path and can intern the string
    // in the data section for performance.
    String immediate_key_str;
    if (parser::NumberLiteral *lit =
        dynamic_cast<parser::NumberLiteral *>(expr->key()))
    {
        immediate_key_str = lit->as_string();
    }

    if (parser::StringLiteral *lit =
        dynamic_cast<parser::StringLiteral *>(expr->key()))
    {
        immediate_key_str = lit->value();
    }

    if (!immediate_key_str.empty())
    {
        Block *done_block = new (GC)Block(NameGenerator::instance().next());
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        t = fun->last_block()->push_mem_alloc(Type::value());
        Value *obj = expand_ref_get(parse(expr->obj(), fun), t, fun, done_block, expt_block);

        fun->push_block(expt_block);
        exception_action()->inflate(expt_block, fun);

        fun->push_block(done_block);
        r = fun->last_block()->push_meta_prp_load(obj, new (GC)StringConstant(immediate_key_str));
    }
    else
    {
        Block *done_block = new (GC)Block(NameGenerator::instance().next());
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        Value *key = expand_ref_get(parse(expr->key(), fun), fun, expt_block);
        Value *obj = expand_ref_get(parse(expr->obj(), fun), fun, expt_block);
        t = fun->last_block()->push_trm_jmp(done_block);

        fun->push_block(expt_block);
        exception_action()->inflate(expt_block, fun);

        fun->push_block(done_block);
        r = fun->last_block()->push_meta_prp_load(obj, key);
    }

    return r;
}

Value *Compiler::parse_call_expr(parser::CallExpression *expr,
                                 Function *fun)
{
    // Result, temporary, function and arguments values.
    Value *r = NULL, *t = NULL, *a = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    a = fun->last_block()->push_mem_alloc(
        new (GC)ArrayType(Type::value(), expr->arguments().size()));

    size_t i = 0;
    parser::ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        t = parse(*it, fun);
        t = expand_ref_get(t, fun, expt_block);
        t = fun->last_block()->push_arr_put(i++, a, t);
    }

    if (parser::PropertyExpression *prop =
        dynamic_cast<parser::PropertyExpression *>(expr->expression()))
    {
        // Test if we have an immediate property key that we can use. If that's the
        // case we don't have to go the to_string() path and can intern the string
        // in the data section for performance.
        String immediate_key_str;
        if (parser::NumberLiteral *lit =
            dynamic_cast<parser::NumberLiteral *>(prop->key()))
        {
            immediate_key_str = lit->as_string();
        }

        if (parser::StringLiteral *lit =
            dynamic_cast<parser::StringLiteral *>(prop->key()))
        {
            immediate_key_str = lit->value();
        }

        if (!immediate_key_str.empty())
        {
            Value *obj = expand_ref_get(parse(prop->obj(), fun), fun, expt_block);

            r = fun->last_block()->push_mem_alloc(Type::value());
            t = fun->last_block()->push_call_keyed(obj, get_prp_key(immediate_key_str),
                                                   static_cast<int>(expr->arguments().size()), a, r);
            t = fun->last_block()->push_trm_br(t, done_block, expt_block);
        }
        else
        {
            Value *key = expand_ref_get(parse(prop->key(), fun), fun, expt_block);
            Value *obj = expand_ref_get(parse(prop->obj(), fun), fun, expt_block);

            r = fun->last_block()->push_mem_alloc(Type::value());
            t = fun->last_block()->push_call_keyed_slow(obj, key,
                                                        static_cast<int>(expr->arguments().size()), a, r);
            t = fun->last_block()->push_trm_br(t, done_block, expt_block);
        }
    }
    else if (parser::IdentifierLiteral * ident =
        dynamic_cast<parser::IdentifierLiteral *>(expr->expression()))
    {
        r = fun->last_block()->push_mem_alloc(Type::value());

        t = get_local(ident->value(), fun);
        if (t)
        {
            t = fun->last_block()->push_call(t, static_cast<int>(expr->arguments().size()), a, r);
        }
        else
        {
            t = fun->last_block()->push_call_named(get_prp_key(ident->value()),
                                                   static_cast<int>(expr->arguments().size()), a, r);
        }
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);
    }
    else
    {
        Value *f = parse(expr->expression(), fun);
        assert(!f->type()->is_reference());

        r = fun->last_block()->push_mem_alloc(Type::value());
        t = fun->last_block()->push_call(f, static_cast<int>(expr->arguments().size()), a, r);
        t = fun->last_block()->push_trm_br(t, done_block, expt_block);
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_call_new_expr(parser::CallNewExpression *expr,
                                     Function *fun)
{
    // Result, temporary, function and arguments values.
    Value *r = NULL, *t = NULL, *f = NULL, *a = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    f = expand_ref_get(parse(expr->expression(), fun), fun, expt_block);
    a = fun->last_block()->push_mem_alloc(
        new (GC)ArrayType(Type::value(), expr->arguments().size()));

    size_t i = 0;
    parser::ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        t = parse(*it, fun);
        t = expand_ref_get(t, fun, expt_block);
        t = fun->last_block()->push_arr_put(i++, a, t);
    }

    r = fun->last_block()->push_mem_alloc(Type::value());
    t = fun->last_block()->push_call_new(f, static_cast<int>(expr->arguments().size()), a, r);
    t = fun->last_block()->push_trm_br(t, done_block, expt_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_regular_expr(parser::RegularExpression *expr,
                                    Function *fun)
{
    Value *r = NULL;
    r = fun->last_block()->push_es_new_rex(expr->pattern(), expr->flags());
    return r;
}

Value *Compiler::parse_fun_expr(parser::FunctionExpression *expr,
                                Function *fun)
{
    Value *r = NULL;

    const parser::FunctionLiteral *lit = expr->function();

    Function *new_fun = parse_fun(lit, false);
    r = fun->last_block()->push_es_new_fun_expr(new_fun, static_cast<int>(lit->parameters().size()),
                                                lit->is_strict_mode());
    return r;
}

Value *Compiler::parse_this_lit(parser::ThisLiteral *lit,
                                Function *fun)
{
    Value *r = NULL;
    r = fun->last_block()->push_ctx_this();
    return r;
}

Value *Compiler::parse_ident_lit(parser::IdentifierLiteral *lit,
                                 Function *fun)
{
    Value *r = NULL;

    r = get_local(lit->value(), fun);
    if (!r)
        r = fun->last_block()->push_meta_ctx_load(get_prp_key(lit->value()));

    return r;
}

Value *Compiler::parse_null_lit(parser::NullLiteral *lit,
                                Function *fun)
{
    Value *r = NULL;
    r = new (GC)ValueConstant(ValueConstant::VALUE_NULL);
    return r;
}

Value *Compiler::parse_bool_lit(parser::BoolLiteral *lit,
                                Function *fun)
{
    Value *r = NULL, *n = NULL;

    n = new (GC)BooleanConstant(lit->value());
    r = fun->last_block()->push_val_from_bool(n);

    return r;
}

Value *Compiler::parse_num_lit(parser::NumberLiteral *lit,
                               Function *fun)
{
    Value *r = NULL, *n = NULL;

    n = new (GC)StringifiedDoubleConstant(lit->as_string());
    r = fun->last_block()->push_val_from_double(n);

    return r;
}

Value *Compiler::parse_str_lit(parser::StringLiteral *lit,
                               Function *fun)
{
    Value *r = NULL, *n = NULL;

    n = new (GC)StringConstant(lit->value());
    r = fun->last_block()->push_val_from_str(n);

    return r;
}

Value *Compiler::parse_fun_lit(parser::FunctionLiteral *lit,
                               Function *fun)
{
    Value *r = NULL;

    Function *new_fun = parse_fun(lit, false);
    if (lit->type() == parser::FunctionLiteral::TYPE_DECLARATION)
    {
        r = fun->last_block()->push_es_new_fun(new_fun, static_cast<int>(lit->parameters().size()),
                                               lit->is_strict_mode());
    }
    else
    {
        r = fun->last_block()->push_es_new_fun_expr(new_fun,
                                                    static_cast<int>(lit->parameters().size()),
                                                    lit->is_strict_mode());
    }

    assert(r);
    return r;
}

Value *Compiler::parse_var_lit(parser::VariableLiteral *lit,
                               Function *fun)
{
    // Do nothing, dealt with when parsing functions.
    return NULL;
}

Value *Compiler::parse_array_lit(parser::ArrayLiteral *lit,
                                 Function *fun)
{
    Value *r = NULL, *t = NULL, *a = NULL, *v = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    a = fun->last_block()->push_mem_alloc(
        new (GC)ArrayType(Type::value(), lit->values().size()));

    int i = 0;
    parser::ExpressionVector::const_iterator it;
    for (it = lit->values().begin(); it != lit->values().end(); ++it, i++)
    {
        t = parse(*it, fun);
        v = expand_ref_get(t, fun, expt_block);
        t = fun->last_block()->push_arr_put(i, a, v);
    }

    r = fun->last_block()->push_es_new_arr(lit->values().size(), a);
    t = fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_obj_lit(parser::ObjectLiteral *lit,
                               Function *fun)
{
    Value *r = NULL, *t = NULL, *k = NULL, *v = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    r = fun->last_block()->push_es_new_obj();
    k = fun->last_block()->push_mem_alloc(Type::value());
    v = fun->last_block()->push_mem_alloc(Type::value());

    parser::ObjectLiteral::PropertyVector::const_iterator it;
    for (it = lit->properties().begin(); it != lit->properties().end(); ++it)
    {
        const parser::ObjectLiteral::Property *prop = *it;

        if (prop->type() == parser::ObjectLiteral::Property::DATA)
        {
            Block *done_block = new (GC)Block(NameGenerator::instance().next());

            t = parse(prop->key(), fun);
            k = expand_ref_get(t, fun, expt_block);

            t = parse(prop->val(), fun);
            v = expand_ref_get(t, fun, expt_block);

            t = fun->last_block()->push_prp_def_data(r, k, v);
            t = fun->last_block()->push_trm_br(t, done_block, expt_block);

            fun->push_block(done_block);
        }
        else
        {
            Block *done_block = new (GC)Block(NameGenerator::instance().next());

            t = parse(prop->val(), fun);
            v = expand_ref_get(t, fun, expt_block);

            t = fun->last_block()->push_prp_def_accessor(
                r, get_prp_key(prop->accessor_name()),
                v, prop->type() == parser::ObjectLiteral::Property::SETTER);
            t = fun->last_block()->push_trm_br(t, done_block, expt_block);

            fun->push_block(done_block);
        }
    }

    fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_nothing_lit(parser::NothingLiteral *lit,
                                   Function *fun)
{
    Value *r = NULL;
    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_empty_stmt(parser::EmptyStatement *stmt,
                                  Function *fun)
{
    Value *r = NULL;
    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_expr_stmt(parser::ExpressionStatement *stmt,
                                 Function *fun)
{
    Value *t = NULL, *v = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    t = parse(stmt->expression(), fun);
    v = fun->last_block()->push_mem_alloc(Type::value());
    t = expand_ref_get(t, v, fun, done_block, expt_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);
    return v;
}

Value *Compiler::parse_block_stmt(parser::BlockStatement *stmt,
                                  Function *fun)
{
    Value *r = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(Scope::TYPE_DEFAULT, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    parser::StatementVector::const_iterator it_stmt;
    for (it_stmt = stmt->body().begin(); it_stmt != stmt->body().end(); ++it_stmt)
        parse(*it_stmt, fun);

    // FIXME: We might be able to optimize away this jump if the done block isn't breaked from.
    if (fun->last_block()->empty() || !fun->last_block()->last_instr()->is_terminating())
        fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_if_stmt(parser::IfStatement *stmt,
                               Function *fun)
{
    Value *r = NULL, *t = NULL;

    Block *true_block = new (GC)Block(NameGenerator::instance().next());
    Block *false_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    t = parse(stmt->condition(), fun);
    t = expand_ref_get(t, fun, expt_block);
    t = fun->last_block()->push_val_to_bool(t);
    t = fun->last_block()->push_trm_br(t, true_block, stmt->has_else() ? false_block : done_block);

    // if block.
    fun->push_block(true_block);
    {
        t = parse(stmt->if_statement(), fun);
        t = fun->last_block()->push_trm_jmp(done_block);
    }

    // else block.
    if (stmt->has_else())
    {
        fun->push_block(false_block);
        {
            t = parse(stmt->else_statement(), fun);
            t = fun->last_block()->push_trm_jmp(done_block);
        }
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_do_while_stmt(parser::DoWhileStatement *stmt,
                                     Function *fun)
{
    Value *r = NULL, *t = NULL;

    Block *next_block = new (GC)Block(NameGenerator::instance().next());
    Block *cond_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    t = fun->last_block()->push_trm_jmp(next_block);

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(cond_block, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    // Next block.
    fun->push_block(next_block);
    {
        t = parse(stmt->body(), fun);
        t = fun->last_block()->push_trm_jmp(cond_block);
    }

    // Condition block.
    fun->push_block(cond_block);
    {
        if (stmt->has_condition())
        {
            t = parse(stmt->condition(), fun);
            t = expand_ref_get(t, fun, expt_block);
            t = fun->last_block()->push_val_to_bool(t);
            t = fun->last_block()->push_trm_br(t, next_block, done_block);
        }
        else
        {
            t = fun->last_block()->push_trm_jmp(next_block);
        }
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_while_stmt(parser::WhileStatement *stmt,
                                  Function *fun)
{
    Value *r = NULL, *t = NULL;

    Block *cond_block = new (GC)Block(NameGenerator::instance().next());
    Block *next_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(cond_block, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    t = fun->last_block()->push_trm_jmp(cond_block);

    // Condition block.
    fun->push_block(cond_block);
    {
        t = parse(stmt->condition(), fun);
        t = expand_ref_get(t, fun, expt_block);
        t = fun->last_block()->push_val_to_bool(t);
        t = fun->last_block()->push_trm_br(t, next_block, done_block);
    }

    fun->push_block(next_block);
    {
        t = parse(stmt->body(), fun);
        t = fun->last_block()->push_trm_jmp(cond_block);
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_for_in_stmt(parser::ForInStatement *stmt,
                                   Function *fun)
{
    // Result, temporary, enumerable, iterator and property values.
    Value *r = NULL, *t = NULL, *e = NULL, *i = NULL, *p = NULL;

    Block *init_block = new (GC)Block(NameGenerator::instance().next());
    Block *cond_block = new (GC)Block(NameGenerator::instance().next());
    Block *body_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(cond_block, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    t = parse(stmt->enumerable(), fun);
    e = expand_ref_get(t, fun, expt_block);
    t = fun->last_block()->push_bin_or(
        fun->last_block()->push_val_is_null(e),
        fun->last_block()->push_val_is_undefined(e));
    t = fun->last_block()->push_trm_br(t, done_block, init_block);

    fun->push_block(init_block);
    {
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        i = fun->last_block()->push_prp_it_new(e);
        t = fun->last_block()->push_bin_eq(i,
            new (GC)NullConstant(new (GC)OpaqueType("EsPropertyIterator")));
        t = fun->last_block()->push_trm_br(t, expt_block, cond_block);

        fun->push_block(expt_block);
        exception_action()->inflate(expt_block, fun);
    }

    fun->push_block(cond_block);
    {
        p = fun->last_block()->push_mem_alloc(Type::value());
        t = fun->last_block()->push_prp_it_next(i, p);
        t = fun->last_block()->push_trm_br(t, body_block, done_block);
    }

    fun->push_block(body_block);
    {
        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

        t = parse(stmt->declaration(), fun);
        expand_ref_put(t, p, fun, blk0_block, expt_block);

        fun->push_block(blk0_block);
        t = parse(stmt->body(), fun);
        t = fun->last_block()->push_trm_jmp(cond_block);
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_for_stmt(parser::ForStatement *stmt,
                                Function *fun)
{
    Value *r = NULL, *t = NULL;

    Block *cond_block = new (GC)Block(NameGenerator::instance().next());
    Block *next_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *body_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(next_block, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    if (stmt->has_initializer())
        t = parse(stmt->initializer(), fun);

    t = fun->last_block()->push_trm_jmp(cond_block);

    fun->push_block(cond_block);

    if (stmt->has_condition())
    {
        t = parse(stmt->condition(), fun);
        t = expand_ref_get(t, fun, expt_block);
        t = fun->last_block()->push_val_to_bool(t);
        t = fun->last_block()->push_trm_br(t, body_block, done_block);
    }
    else
    {
        t = fun->last_block()->push_trm_jmp(body_block);
    }

    fun->push_block(body_block);
    {
        t = parse(stmt->body(), fun);
        t = fun->last_block()->push_trm_jmp(next_block);
    }

    fun->push_block(next_block);
    {
        if (stmt->has_next())
            parse(stmt->next(), fun);

        // Jump to top.
        fun->last_block()->push_trm_jmp(cond_block);
    }

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_cont_stmt(parser::ContinueStatement *stmt,
                                 Function *fun)
{
    if (stmt->has_target())
    {
        // Any label from the list is fine.
        std::string target = stmt->target()->labels().first().utf8();
        Scope *scope = unroll_for_continue(fun, target);
        if (!scope)
        {
            THROW(InternalException,
                  "internal error: referencing unknown label.");
        }

        fun->last_block()->push_trm_jmp(scope->continue_target());
    }
    else
    {
        Scope *scope = unroll_for_continue(fun);
        if (!scope)
        {
            THROW(Exception,
                  "error: non-labeled continue statements are only allowed in loops.");
        }

        fun->last_block()->push_trm_jmp(scope->continue_target());
    }

    return NULL;
}

Value *Compiler::parse_break_stmt(parser::BreakStatement *stmt,
                                  Function *fun)
{
    if (stmt->has_target())
    {
        // Any label from the list is fine.
        std::string target = stmt->target()->labels().first().utf8();
        Scope *scope = unroll_for_break(fun, target);
        if (!scope)
        {
            THROW(InternalException,
                  "internal error: referencing unknown label.");
        }

        fun->last_block()->push_trm_jmp(scope->break_target());
    }
    else
    {
        Scope *scope = unroll_for_break(fun);
        if (!scope)
        {
            THROW(Exception,
                  "error: non-labeled break statements are only allowed in loops and switch statements.");
        }

        fun->last_block()->push_trm_jmp(scope->break_target());
    }

    return NULL;
}

Value *Compiler::parse_ret_stmt(parser::ReturnStatement *stmt,
                                Function *fun)
{
    Value *r = NULL, *t = NULL;

    r = new (GC)ReturnConstant();

    if (stmt->has_expression())
    {
        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        t = parse(stmt->expression(), fun);
        t = expand_ref_get(t, r, fun, blk0_block, expt_block);

        fun->push_block(blk0_block);
        unroll_for_return(fun);
        t = fun->last_block()->push_trm_ret(new (GC)BooleanConstant(true));

        fun->push_block(expt_block);
        exception_action()->inflate(expt_block, fun);
    }
    else
    {
        t = fun->last_block()->push_mem_store(r,
            new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED));

        unroll_for_return(fun);
        t = fun->last_block()->push_trm_ret(new (GC)BooleanConstant(true));
    }

    assert(r);
    return r;
}

Value *Compiler::parse_with_stmt(parser::WithStatement *stmt,
                                 Function *fun)
{
    Value *r = NULL, *t = NULL, *v = NULL;

    Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt0_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt1_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(scopes_, new (GC)Scope(Scope::TYPE_WITH));
    scope->set_epilogue(new (GC)LeaveContextTemplateBlock());

    // 12.10.
    t = parse(stmt->expression(), fun);
    v = expand_ref_get(t, fun, expt0_block);

    t = fun->last_block()->push_ctx_enter_with(v);
    t = fun->last_block()->push_trm_br(t, blk0_block, expt0_block);

    fun->push_block(expt0_block);
    exception_action()->inflate(expt0_block, fun);

    fun->push_block(blk0_block);

    MultiTemplateBlock *multi_block = new (GC)MultiTemplateBlock();
    multi_block->push_back(new (GC)LeaveContextTemplateBlock());
    multi_block->push_back(exception_action());
    ScopedVectorValue<TemplateBlock> expt_action(
        exception_actions_, multi_block);

    r = parse(stmt->body(), fun);
    t = fun->last_block()->push_ctx_leave();
    t = fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(expt1_block);
    exception_action()->inflate(expt1_block, fun);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_switch_stmt(parser::SwitchStatement *stmt,
                                   Function *fun)
{
    Value *r = NULL, *t = NULL, *e = NULL, *b = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(Scope::TYPE_SWITCH, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    t = parse(stmt->expression(), fun);
    e = expand_ref_get(t, fun, expt_block);

    b = fun->last_block()->push_mem_alloc(Type::boolean());    // true if case name is found.
    t = fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(false));  // FIXME: Can this be removed?

    parser::SwitchStatement::CaseClauseVector::const_iterator it;
    for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
    {
        const parser::SwitchStatement::CaseClause *clause = *it;

        if (!clause->is_default())
        {
            Value *c = NULL, *v = NULL;

            Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
            Block *blk1_block = new (GC)Block(NameGenerator::instance().next());
            Block *skip_block = new (GC)Block(NameGenerator::instance().next());

            t = fun->last_block()->push_trm_br(b, skip_block, blk0_block);

            fun->push_block(blk0_block);
            t = parse(clause->label(), fun);
            v = expand_ref_get(t, fun, expt_block);
            c = fun->last_block()->push_mem_alloc(Type::value());
            t = fun->last_block()->push_es_bin_strict_eq(v, e, c);
            t = fun->last_block()->push_trm_br(t, blk1_block, expt_block);

            fun->push_block(blk1_block);
            t = fun->last_block()->push_val_to_bool(c);
            t = fun->last_block()->push_mem_store(b, t);
            t = fun->last_block()->push_trm_jmp(skip_block);   // FIXME:

            fun->push_block(skip_block);
        }

        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
        Block *skip_block = new (GC)Block(NameGenerator::instance().next());

        t = fun->last_block()->push_trm_br(b, blk0_block, skip_block);

        fun->push_block(blk0_block);
        {
            parser::StatementVector::const_iterator it_stmt;
            for (it_stmt = clause->body().begin();
                it_stmt != clause->body().end(); ++it_stmt)
            {
                t = parse(*it_stmt, fun);
            }
        }

        t = fun->last_block()->push_trm_jmp(skip_block);

        fun->push_block(skip_block);
    }

    Block *tdef_block = new (GC)Block(NameGenerator::instance().next());

    t = fun->last_block()->push_trm_br(b, done_block, tdef_block);

    // Try finding and executing the default block.
    fun->push_block(tdef_block);
    {
        for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
        {
            const parser::SwitchStatement::CaseClause *clause = *it;

            if (clause->is_default())
            {
                parser::StatementVector::const_iterator it_stmt;
                for (it_stmt = clause->body().begin();
                    it_stmt != clause->body().end(); ++it_stmt)
                {
                    t = parse(*it_stmt, fun);
                }
            }
        }
    }

    t = fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_throw_stmt(parser::ThrowStatement *stmt,
                                  Function *fun)
{
    Value *r = NULL, *t = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    // Inflate the exception action into the last block.
    t = parse(stmt->expression(), fun);
    t = fun->last_block()->push_ex_set(expand_ref_get(t, fun, expt_block));
    t = fun->last_block()->push_trm_jmp(expt_block);

    fun->push_block(expt_block);
    exception_action()->inflate(expt_block, fun);

    fun->push_block(done_block);

    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Value *Compiler::parse_try_stmt(parser::TryStatement *stmt,
                                Function *fun)
{
    Value *r = NULL, *t = NULL, *b = NULL;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *fail_block = new (GC)Block(NameGenerator::instance().next());
    Block *skip_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(Scope::TYPE_DEFAULT, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    TemplateBlock *prv_exception_action = exception_action();

    if (stmt->has_finally_block())
    {
        scope->set_epilogue(new (GC)FinallyTemplateBlock(
            this, stmt->finally_block(), prv_exception_action));
    }

    b = fun->last_block()->push_mem_alloc(Type::boolean());    // Fail check.
    t = fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(true));
    {
        ScopedVectorValue<TemplateBlock> action(
            exception_actions_, new (GC)JumpTemplateBlock(fail_block));

        r = parse(stmt->try_block(), fun);
        t = fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(false));

        // If we have a finally block but no catch block there is no need to
        // jump, but we can fall directly through to the finally block.
        if (stmt->has_catch_block())
            fun->last_block()->push_trm_jmp(skip_block);
        else
            fun->last_block()->push_trm_jmp(fail_block);
    }

    fun->push_block(fail_block);

    if (stmt->has_catch_block())
    {
        fun->last_block()->push_ctx_enter_catch(get_prp_key(stmt->catch_identifier()));

        ScopedVectorValue<Scope> scope(
            scopes_, new (GC)Scope(Scope::TYPE_DEFAULT));
        scope->set_epilogue(new (GC)LeaveContextTemplateBlock());

        MultiTemplateBlock *multi_block = new (GC)MultiTemplateBlock();
        multi_block->push_back(new (GC)LeaveContextTemplateBlock());
        multi_block->push_back(new (GC)JumpTemplateBlock(skip_block));
        ScopedVectorValue<TemplateBlock> expt_action(
            exception_actions_, multi_block);

        r = parse(stmt->catch_block(), fun);
        t = fun->last_block()->push_ctx_leave();

        fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(false));
        fun->last_block()->push_trm_jmp(skip_block);
    }
    else
    {
        fun->last_block()->push_trm_jmp(skip_block);
    }

    fun->push_block(skip_block);

    if (stmt->has_finally_block())
    {
        t = fun->last_block()->push_ex_save_state();
        r = parse(stmt->finally_block(), fun);
        t = fun->last_block()->push_ex_load_state(t);
    }

    // On failure, execute the previous exception action.
    fun->last_block()->push_trm_br(b, expt_block, done_block);

    fun->push_block(expt_block);
    prv_exception_action->inflate(expt_block, fun);

    assert(r);

    fun->push_block(done_block);
    return r;
}

Value *Compiler::parse_dbg_stmt(parser::DebuggerStatement *stmt,
                                Function *fun)
{
    Value *r = NULL;
    r = new (GC)ValueConstant(ValueConstant::VALUE_NOTHING);
    return r;
}

Module *Compiler::compile(const parser::FunctionLiteral *root)
{
    assert(root);
    reset();

    analyzer_.analyze(const_cast<parser::FunctionLiteral *>(root));

    module_ = new (GC)Module();

    parse_fun(root, true);

#ifdef DEBUG
    // Assert that all blocks end with a terminating instruction.
    FunctionVector::const_iterator it_fun;
    for (it_fun = module_->functions().begin();
        it_fun != module_->functions().end(); ++it_fun)
    {
        const Function *fun = *it_fun;

        BlockList::ConstIterator it_blk;
        for (it_blk = fun->blocks().begin();
            it_blk != fun->blocks().end(); ++it_blk)
        {
            const Block &block = *it_blk;
            if (!block.instructions().empty())
                assert(block.last_instr()->is_terminating());
        }
    }
#endif

    // Register string resources.
    for (const std::pair<String, uint32_t> &item : strings_)
    {
        module_->push_resource(new (GC)StringResource(item.first, item.second));
    }

    return module_;
}

}
