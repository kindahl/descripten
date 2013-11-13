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

TemporaryValueAllocator::TemporaryValueAllocator(Compiler *compiler)
    : parent_(NULL)
    , compiler_(compiler)
    , scope_(compiler->current_fun_scope(true))
{
    if (!compiler_->temporaries_stack_.empty())
        parent_ = compiler_->temporaries_stack_.back();
    compiler_->temporaries_stack_.push_back(this);
}

TemporaryValueAllocator::~TemporaryValueAllocator()
{
    for (Value *val : temporaries_)
        scope_->put_temporary(val);

    if (!compiler_->temporaries_stack_.empty())
        compiler_->temporaries_stack_.pop_back();
}

TemporaryValueAllocator *TemporaryValueAllocator::parent()
{
    return parent_;
}

Value *TemporaryValueAllocator::get()
{
    Value *val = scope_->get_temporary();
    temporaries_.insert(val);
    return val;
}

void TemporaryValueAllocator::release(Value *value)
{
    assert(temporaries_.count(value) == 1);

    scope_->put_temporary(value);
    temporaries_.erase(value);
}

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

Scope *Compiler::current_fun_scope(bool accept_with)
{
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        Scope *scope = *it;
        switch (scope->type())
        {
            case Scope::TYPE_FUNCTION:
                return scope;
            case Scope::TYPE_WITH:
                if (!accept_with)
                    return NULL;
                break;
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

ValueHandle Compiler::expand_ref_get_inplace(ValueHandle &ref,
                                             ValueHandle &dst,
                                             Function *fun, Block *expt_block)
{
    assert(ref);

    if (!dst)
        return ValueHandle();

    if (ref == dst)
        return ref;

    if (!ref->type()->is_reference())
    {
        fun->last_block()->push_mem_store(dst, ref);
        return dst;
    }

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        _ = expand_prp_get(dst, fun, prp_load);
            fun->last_block()->push_trm_br(_, done_block, expt_block);

        fun->push_block(done_block);
        return dst;
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        _ = fun->last_block()->push_ctx_get(ctx_load->key(), dst,
                                            get_ctx_cid(ctx_load->key()));
            fun->last_block()->push_trm_br(_, done_block, expt_block);

        fun->push_block(done_block);
        return dst;
    }

    assert(false);
    return ValueHandle();
}

ValueHandle Compiler::expand_ref_get_inplace_lazy(ValueHandle &ref,
                                                  Function *fun,
                                                  Block *expt_block,
                                                  ValueAllocator &allocator)
{
    assert(ref);

    if (!ref->type()->is_reference())
        return ref;

    ValueHandle dst = ValueHandle::lazy(&allocator);

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        _ = expand_prp_get(dst, fun, prp_load);
            fun->last_block()->push_trm_br(_, done_block, expt_block);

        fun->push_block(done_block);
        return dst;
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        _ = fun->last_block()->push_ctx_get(ctx_load->key(), dst,
                                            get_ctx_cid(ctx_load->key()));
            fun->last_block()->push_trm_br(_, done_block, expt_block);

        fun->push_block(done_block);
        return dst;
    }

    assert(false);
    return ValueHandle();
}

ValueHandle Compiler::expand_ref_get(ValueHandle &ref,
                                     ValueHandle &dst,
                                     Function *fun,
                                     Block *done_block, Block *expt_block)
{
    if (!ref || !dst)
    {
        fun->last_block()->push_trm_jmp(done_block);
        return ValueHandle();
    }

    if (ref == dst)
    {
        fun->last_block()->push_trm_jmp(done_block);
        return ref;
    }

    if (!ref->type()->is_reference())
    {
        fun->last_block()->push_mem_store(dst, ref);
        fun->last_block()->push_trm_jmp(done_block);
        return dst;
    }

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;

        _ = expand_prp_get(dst, fun, prp_load);
            fun->last_block()->push_trm_br(_, done_block, expt_block);
        return dst;
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;

        _ = fun->last_block()->push_ctx_get(ctx_load->key(), dst,
                                            get_ctx_cid(ctx_load->key()));
            fun->last_block()->push_trm_br(_, done_block, expt_block);
        return dst;
    }

    assert(false);
    return dst;
}

ValueHandle Compiler::expand_ref_get_lazy(ValueHandle &ref, Function *fun,
                                          Block *done_block, Block *expt_block,
                                          ValueAllocator &allocator)
{
    if (!ref)
    {
        fun->last_block()->push_trm_jmp(done_block);
        return ValueHandle();
    }

    if (!ref->type()->is_reference())
    {
        fun->last_block()->push_trm_jmp(done_block);
        return ref;
    }

    ValueHandle dst = ValueHandle::lazy(&allocator);

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;

        _ = expand_prp_get(dst, fun, prp_load);
            fun->last_block()->push_trm_br(_, done_block, expt_block);
        return dst;
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;

        _ = fun->last_block()->push_ctx_get(ctx_load->key(), dst,
                                            get_ctx_cid(ctx_load->key()));
            fun->last_block()->push_trm_br(_, done_block, expt_block);
        return dst;
    }

    assert(false);
    return dst;
}

void Compiler::expand_ref_put_inplace(ValueHandle &ref,
                                      ValueHandle &val,
                                      Function *fun, Block *expt_block)
{
    assert(val);

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        _ = expand_prp_put(val, fun, prp_load);
            fun->last_block()->push_trm_br(_, done_block, expt_block);

        fun->push_block(done_block);
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;
        Block *done_block = new (GC)Block(NameGenerator::instance().next());

        _ = fun->last_block()->push_ctx_put(ctx_load->key(), val,
                                            get_ctx_cid(ctx_load->key()));
            fun->last_block()->push_trm_br(_, done_block, expt_block);

        fun->push_block(done_block);
    }
    else
    {
        fun->last_block()->push_mem_store(ref, val);
    }
}

void Compiler::expand_ref_put(ValueHandle &ref, ValueHandle &val,
                              Function *fun,
                              Block *done_block, Block *expt_block)
{
    assert(val);

    if (!ref)
        fun->last_block()->push_trm_jmp(done_block);

    if (MetaPropertyLoadInstruction *prp_load =
        dynamic_cast<MetaPropertyLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;

        _ = expand_prp_put(val, fun, prp_load);
            fun->last_block()->push_trm_br(_, done_block, expt_block);
    }
    else if (MetaContextLoadInstruction *ctx_load =
        dynamic_cast<MetaContextLoadInstruction *>(ref.get()))
    {
        Value *_ = NULL;

        _ = fun->last_block()->push_ctx_put(ctx_load->key(), val,
                                            get_ctx_cid(ctx_load->key()));
            fun->last_block()->push_trm_br(_, done_block, expt_block);
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

    // Parse declarations.
    AnalyzedFunction *analyzed_fun =
        analyzer_.lookup(const_cast<parser::FunctionLiteral *>(lit));   // FIXME: const_cast
    assert(analyzed_fun);

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(Scope::TYPE_FUNCTION));
    ScopedVectorValue<TemplateBlock> expt_action(
        exception_actions_, new (GC)ReturnFalseTemplateBlock());

    TemporaryValueAllocator temporaries(this);

    Block *body_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    // Locals, extra and temporary registers.
    ValueHandle e, t;

    ValueHandle fp = new (GC)FramePointer();
    ValueHandle vp = new (GC)ValuePointer();

    size_t start_extras = 0;    // Start of first non-parameter extra in extras array.

    if (!lit->needs_args_obj())
    {
        // Allocate locals.
        fun->last_block()->push_stk_alloc(
            scope->call_frame_value_count().get_proxy());

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
        for (const AnalyzedVariable *var : analyzed_fun->variables())
        {
            if (!var->is_parameter())
                continue;

            if (!var->is_allocated())
                continue;

            switch (var->storage())
            {
                case AnalyzedVariable::STORAGE_LOCAL:
                {
                    scope->add_local(var->name(), new (GC)ArrayElementConstant(fp, var->parameter_index()));

                    if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                    {
                        t = fun->last_block()->push_mem_elm_ptr(fp, var->parameter_index());
                        t = fun->last_block()->push_link_var(get_prp_key(var->name()), lit->is_strict_mode(), t);
                    }
                    break;
                }

                case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
                {
                    Value *v = new (GC)ArrayElementConstant(e, start_extras++);
                    scope->add_local(var->name(), v);

                    t = fun->last_block()->push_mem_store(v, new (GC)ArrayElementConstant(fp, var->parameter_index()));
                    break;
                }

                case AnalyzedVariable::STORAGE_CONTEXT:
                {
                    Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

                    t = fun->last_block()->push_decl_prm(get_prp_key(var->name()),
                                                         lit->is_strict_mode(),
                                                         var->parameter_index(), fp);
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
        // Allocate locals.
        fun->last_block()->push_stk_alloc(
            scope->call_frame_value_count().get_proxy());

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

        a = fun->last_block()->push_args_obj_init();

        // Allocate parameters.
        for (const AnalyzedVariable *var : analyzed_fun->variables())
        {
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

    size_t vp_index = 0;
    size_t ep_index = start_extras;

    // Allocate callee access.
    for (const AnalyzedVariable *var : analyzed_fun->variables())
    {
        if (!var->is_callee())
            continue;

        if (!var->is_allocated())
            continue;

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(vp, -3));    // FIXME: Use constant offset.
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                Value *v = new (GC)ArrayElementConstant(e, ep_index++);
                t = fun->last_block()->push_mem_store(v, new (GC)ArrayElementConstant(vp, -3));

                scope->add_local(var->name(), v);
                break;
            }
        }
    }

    // Allocate function declarations.
    for (const AnalyzedVariable *var : analyzed_fun->variables())
    {
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
                scope->add_local(var->name(), new (GC)ArrayElementConstant(vp, vp_index++));
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(e, ep_index++));
                break;
            }
        }
    }

    // Allocate variable declarations.
    for (const AnalyzedVariable *var : analyzed_fun->variables())
    {
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
                scope->add_local(var->name(), new (GC)ArrayElementConstant(vp, vp_index++));
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                scope->add_local(var->name(), new (GC)ArrayElementConstant(e, ep_index++));
                break;
            }
        }
    }

    vp_index = 0;
    ep_index = start_extras;

    // Increment storage indexes for callee allocation.
    // FIXME: What's this?
    for (const AnalyzedVariable *var : analyzed_fun->variables())
    {
        if (!var->is_callee())
            continue;

        if (!var->is_allocated())
            continue;

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
                ep_index++;
                break;
        }
    }

    // Parse function declarations.
    for (const AnalyzedVariable *var : analyzed_fun->variables())
    {
        if (!var->is_declaration())
            continue;

        if (!var->is_allocated())
            continue;

        const parser::Declaration *decl = var->declaration();
        if (!decl->is_function())
            continue;

        ValueHandle f = parse(decl->as_function(), fun, &temporaries);

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                t = fun->last_block()->push_arr_put(vp_index, vp, f);
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(vp, vp_index);
                    t = fun->last_block()->push_link_fun(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                vp_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                t = fun->last_block()->push_arr_put(ep_index, e, f);
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(e, ep_index);
                    t = fun->last_block()->push_link_fun(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                ep_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_CONTEXT:
            {
                Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

                t = fun->last_block()->push_decl_fun(get_prp_key(var->name()),
                                                     lit->is_strict_mode(), f);
                t = fun->last_block()->push_trm_br(t, blk0_block, expt_block);

                fun->push_block(blk0_block);
                break;
            }

            default:
                assert(false);
                break;
        }

        f.release();
    }

    // Parse variable declarations.
    for (const AnalyzedVariable *var : analyzed_fun->variables())
    {
        if (!var->is_declaration())
            continue;

        if (!var->is_allocated())
            continue;

        const parser::Declaration *decl = var->declaration();
        if (!decl->is_variable())
            continue;

        parse(decl->as_variable(), fun, NULL);

        switch (var->storage())
        {
            case AnalyzedVariable::STORAGE_LOCAL:
            {
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(vp, vp_index);
                    t = fun->last_block()->push_link_var(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                vp_index++;
                break;
            }

            case AnalyzedVariable::STORAGE_LOCAL_EXTRA:
            {
                if (analyzed_fun->tainted_by_eval() || var->name() == _USTR("arguments"))
                {
                    t = fun->last_block()->push_mem_elm_ptr(e, ep_index);
                    t = fun->last_block()->push_link_var(get_prp_key(var->name()), lit->is_strict_mode(), t);
                }

                ep_index++;
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

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    fun->push_block(body_block);

    // Parse statements.
    parser::StatementVector::const_iterator it_stmt;
    for (it_stmt = lit->body().begin(); it_stmt != lit->body().end(); ++it_stmt)
        parse(*it_stmt, fun, NULL);

    // Make sure the function return something.
    if (fun->last_block()->empty() || !fun->last_block()->last_instr()->is_terminating())
        fun->last_block()->push_trm_ret(new (GC)BooleanConstant(true));

    // Fulfil promises.
    assert(analyzed_fun->num_locals() <= scope->num_locals());
    scope->commit();

    return fun;
}

ValueHandle Compiler::parse_binary_expr(parser::BinaryExpression *expr,
                                        Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R, X = ValueHandle::lazy(rva ? rva : &temporaries);

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    if (expr->operation() == parser::BinaryExpression::LOG_AND)
    {
        R = parse(expr->left(), fun, &temporaries);
        expand_ref_get_inplace(R, X, fun, expt_block);

        // Short-circuit evaluation.
        Block *next_block = new (GC)Block(NameGenerator::instance().next());

        _ = fun->last_block()->push_val_to_bool(X);
            fun->last_block()->push_trm_br(_, next_block, done_block);

        fun->push_block(next_block);
        {
            R = parse(expr->right(), fun, &temporaries);
            expand_ref_get(R, X, fun, done_block, expt_block);
        }
    }
    else if (expr->operation() == parser::BinaryExpression::LOG_OR)
    {
        R = parse(expr->left(), fun, &temporaries);
        expand_ref_get_inplace(R, X, fun, expt_block);

        // Short-circuit evaluation.
        Block *next_block = new (GC)Block(NameGenerator::instance().next());

        _ = fun->last_block()->push_val_to_bool(X);
            fun->last_block()->push_trm_br(_, done_block, next_block);

        fun->push_block(next_block);
        {
            R = parse(expr->right(), fun, &temporaries);
            expand_ref_get(R, X, fun, done_block, expt_block);
        }
    }
    else
    {
        ValueHandle l, r;

        R = parse(expr->left(), fun, &temporaries);
        l = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

        R = parse(expr->right(), fun, &temporaries);
        r = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

        switch (expr->operation())
        {
            case parser::BinaryExpression::COMMA:
            {
                // Don't do anything, the comma expression only requires us to
                // call GetValue which we already have done.
                fun->last_block()->push_mem_store(X, r);
                fun->last_block()->push_trm_jmp(done_block);
                break;
            }

            // Arithmetic.
            case parser::BinaryExpression::MUL:
            {
                _ = fun->last_block()->push_es_bin_mul(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::DIV:
            {
                _ = fun->last_block()->push_es_bin_div(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::MOD:
            {
                _ = fun->last_block()->push_es_bin_mod(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::ADD:
            {
                _ = fun->last_block()->push_es_bin_add(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::SUB:
            {
                _ = fun->last_block()->push_es_bin_sub(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::LS:  // <<
            {
                _ = fun->last_block()->push_es_bin_ls(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::RSS: // >>
            {
                _ = fun->last_block()->push_es_bin_rss(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::RUS: // >>>
            {
                _ = fun->last_block()->push_es_bin_rus(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }

            // Relational.
            case parser::BinaryExpression::LT:
            {
                _ = fun->last_block()->push_es_bin_lt(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::GT:
            {
                _ = fun->last_block()->push_es_bin_gt(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::LTE:
            {
                _ = fun->last_block()->push_es_bin_lte(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::GTE:
            {
                _ = fun->last_block()->push_es_bin_gte(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::IN:
            {
                _ = fun->last_block()->push_es_bin_in(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::INSTANCEOF:
            {
                _ = fun->last_block()->push_es_bin_instanceof(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }

            // Equality.
            case parser::BinaryExpression::EQ:
            {
                _ = fun->last_block()->push_es_bin_eq(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::NEQ:
            {
                _ = fun->last_block()->push_es_bin_neq(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::STRICT_EQ:
            {
                _ = fun->last_block()->push_es_bin_strict_eq(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::STRICT_NEQ:
            {
                _ = fun->last_block()->push_es_bin_strict_neq(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }

            // Bitwise.
            case parser::BinaryExpression::BIT_AND:
            {
                _ = fun->last_block()->push_es_bin_bit_and(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::BIT_XOR:
            {
                _ = fun->last_block()->push_es_bin_bit_xor(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
                break;
            }
            case parser::BinaryExpression::BIT_OR:
            {
                _ = fun->last_block()->push_es_bin_bit_or(l, r, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
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

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_unary_expr(parser::UnaryExpression *expr,
                                       Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R;

    ValueHandle X;

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
                X = ValueHandle::lazy(rva ? rva : &temporaries);

                ValueHandle o;

                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *expt_block = new (GC)Block(NameGenerator::instance().next());

                R = parse(prop->obj(), fun, &temporaries);
                o = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

                _ = fun->last_block()->push_prp_del(o, get_prp_key(immediate_key_str), X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);

                if (!expt_block->referrers().empty())
                {
                    exception_action()->inflate(expt_block, fun);
                    fun->push_block(expt_block);
                }

                if (!done_block->referrers().empty())
                {
                    fun->push_block(done_block);
                }
            }
            else
            {
                X = ValueHandle::lazy(rva ? rva : &temporaries);

                ValueHandle k, o;

                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *expt_block = new (GC)Block(NameGenerator::instance().next());

                R = parse(prop->key(), fun, &temporaries);
                k = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

                R = parse(prop->obj(), fun, &temporaries);
                o = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

                _ = fun->last_block()->push_prp_del_slow(o, k, X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);

                if (!expt_block->referrers().empty())
                {
                    exception_action()->inflate(expt_block, fun);
                    fun->push_block(expt_block);
                }

                if (!done_block->referrers().empty())
                {
                    fun->push_block(done_block);
                }
            }
        }
        else if (parser::IdentifierLiteral * ident =
            dynamic_cast<parser::IdentifierLiteral *>(expr->expression()))
        {
            Scope *scope = current_fun_scope(false);
            if (scope && scope->has_local(ident->value()))
            {
                // Having a local implies:
                // 1. The local represents a binding in a declarative
                //    environment record.
                // 2. The local does not belong to an eval context.
                // This means that trying to delete the entity should fail.
                X = new (GC)ValueConstant(ValueConstant::VALUE_FALSE);
            }
            else
            {
                X = ValueHandle::lazy(rva ? rva : &temporaries);

                Block *done_block = new (GC)Block(NameGenerator::instance().next());
                Block *expt_block = new (GC)Block(NameGenerator::instance().next());

                _ = fun->last_block()->push_ctx_del(get_prp_key(ident->value()), X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);

                if (!expt_block->referrers().empty())
                {
                    exception_action()->inflate(expt_block, fun);
                    fun->push_block(expt_block);
                }

                if (!done_block->referrers().empty())
                {
                    fun->push_block(done_block);
                }
            }
        }
        else
        {
            X = new (GC)ValueConstant(ValueConstant::VALUE_TRUE);
        }

        return rva ? X : ValueHandle();
    }

    R = parse(expr->expression(), fun, &temporaries);

    switch (expr->operation())
    {
        case parser::UnaryExpression::VOID:
        {
            X = new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED);

            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            expand_ref_get_lazy(R, fun, done_block, expt_block, temporaries);

            // FIXME: These can be expanded in expand_ref_get*
            if (!expt_block->referrers().empty())
            {
                exception_action()->inflate(expt_block, fun);
                fun->push_block(expt_block);
            }

            if (!done_block->referrers().empty())
            {
                fun->push_block(done_block);
            }
            break;
        }
        case parser::UnaryExpression::PLUS:
        case parser::UnaryExpression::PRE_INC:
        case parser::UnaryExpression::PRE_DEC:
        case parser::UnaryExpression::POST_INC:
        case parser::UnaryExpression::POST_DEC:
        {
            ValueHandle d, e;

            Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            e = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            d = fun->last_block()->push_mem_alloc(Type::_double());
            _ = fun->last_block()->push_val_to_double(e, d);
            _ = fun->last_block()->push_trm_br(_, blk0_block, expt_block);

            fun->push_block(blk0_block);
            switch (expr->operation())
            {
                case parser::UnaryExpression::PLUS:
                    if (rva)
                    {
                        X = ValueHandle::lazy(rva ? rva : &temporaries);
                        fun->last_block()->push_val_from_double(d, X);
                    }

                    fun->last_block()->push_trm_jmp(done_block);

                    // Needed for early return:
                    if (!expt_block->referrers().empty())
                    {
                        exception_action()->inflate(expt_block, fun);
                        fun->push_block(expt_block);
                    }

                    if (!done_block->referrers().empty())
                    {
                        fun->push_block(done_block);
                    }
                    return rva ? X : ValueHandle(); // Early return.
                case parser::UnaryExpression::PRE_INC:
                    X = ValueHandle::lazy(rva ? rva : &temporaries);

                    _ = fun->last_block()->push_bin_add(d, new (GC)DoubleConstant(1.0));
                        fun->last_block()->push_val_from_double(_, X);
                    _ = X;
                    break;
                case parser::UnaryExpression::PRE_DEC:
                    X = ValueHandle::lazy(rva ? rva : &temporaries);

                    _ = fun->last_block()->push_bin_sub(d, new (GC)DoubleConstant(1.0));
                        fun->last_block()->push_val_from_double(_, X);
                    _ = X;
                    break;
                case parser::UnaryExpression::POST_INC:
                {
                    if (rva)
                    {
                        X = ValueHandle::lazy(rva ? rva : &temporaries);
                        fun->last_block()->push_val_from_double(d, X);
                    }

                    ValueHandle t = ValueHandle::lazy(&temporaries);

                    _ = fun->last_block()->push_bin_add(d, new (GC)DoubleConstant(1.0));
                        fun->last_block()->push_val_from_double(_, t);
                    _ = t;
                    break;
                }
                case parser::UnaryExpression::POST_DEC:
                {
                    ValueHandle t = ValueHandle::lazy(&temporaries);

                    if (rva)
                    {
                        X = ValueHandle::lazy(rva ? rva : &temporaries);
                        fun->last_block()->push_val_from_double(d, X);
                    }

                    _ = fun->last_block()->push_bin_sub(d, new (GC)DoubleConstant(1.0));
                        fun->last_block()->push_val_from_double(_, t);
                    _ = t;
                    break;
                }
            }

            expand_ref_put(R, _, fun, done_block, expt_block);

            if (!expt_block->referrers().empty())
            {
                exception_action()->inflate(expt_block, fun);
                fun->push_block(expt_block);
            }

            if (!done_block->referrers().empty())
            {
                fun->push_block(done_block);
            }
            break;
        }
        case parser::UnaryExpression::TYPEOF:
        {
            ValueHandle e;

            X = ValueHandle::lazy(rva ? rva : &temporaries);

            Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
            Block *fail_block = new (GC)Block(NameGenerator::instance().next());

            e = expand_ref_get_lazy(R, fun, blk0_block, fail_block, temporaries);

            fun->push_block(fail_block);
            fun->last_block()->push_ex_clear();
            fun->last_block()->push_mem_store(e,
                new (GC)ValueConstant(ValueConstant::VALUE_UNDEFINED));
            fun->last_block()->push_trm_jmp(blk0_block);

            fun->push_block(blk0_block);

            _ = fun->last_block()->push_es_unary_typeof(e, X);

            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            fun->last_block()->push_trm_br(_, done_block, expt_block);

            if (!expt_block->referrers().empty())
            {
                exception_action()->inflate(expt_block, fun);
                fun->push_block(expt_block);
            }

            if (!done_block->referrers().empty())
            {
                fun->push_block(done_block);
            }
            break;
        }
        case parser::UnaryExpression::MINUS:
        case parser::UnaryExpression::BIT_NOT:
        case parser::UnaryExpression::LOG_NOT:
        {
            ValueHandle e;

            X = ValueHandle::lazy(rva ? rva : &temporaries);

            Block *done_block = new (GC)Block(NameGenerator::instance().next());
            Block *expt_block = new (GC)Block(NameGenerator::instance().next());

            switch (expr->operation())
            {
                case parser::UnaryExpression::MINUS:
                {
                    e = expand_ref_get_inplace_lazy(R, fun, expt_block,
                                                    temporaries);
                    _ = fun->last_block()->push_es_unary_neg(e, X);
                    break;
                }
                case parser::UnaryExpression::BIT_NOT:
                    e = expand_ref_get_inplace_lazy(R, fun, expt_block,
                                                    temporaries);
                    _ = fun->last_block()->push_es_unary_bit_not(e, X);
                    break;
                case parser::UnaryExpression::LOG_NOT:
                    e = expand_ref_get_inplace_lazy(R, fun, expt_block,
                                                    temporaries);
                    _ = fun->last_block()->push_es_unary_log_not(e, X);
                    break;
            }

            fun->last_block()->push_trm_br(_, done_block, expt_block);

            if (!expt_block->referrers().empty())
            {
                exception_action()->inflate(expt_block, fun);
                fun->push_block(expt_block);
            }

            if (!done_block->referrers().empty())
            {
                fun->push_block(done_block);
            }
            break;
        }

        default:
            assert(false);
            break;
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_assign_expr(parser::AssignmentExpression *expr,
                                        Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    if (expr->operation() == parser::AssignmentExpression::ASSIGN)
    {
        ValueHandle L, R, X, l;

        if (rva)
        {
            L = parse(expr->lhs(), fun, &temporaries);
            if (L->type()->is_reference())
            {
                R = parse(expr->rhs(), fun, rva);
                l = expand_ref_get_inplace_lazy(R, fun, expt_block, *rva);
                    expand_ref_put_inplace(L, l, fun, expt_block);
            }
            else
            {
                FixedValueAllocator fva(L);
                R = parse(expr->rhs(), fun, &fva);
                l = expand_ref_get_inplace(R, L, fun, expt_block);
            }

            fun->last_block()->push_trm_jmp(done_block);

            if (rva)
                X = l;
        }
        else
        {
            SingleValueAllocator sva(temporaries);

            L = parse(expr->lhs(), fun, &temporaries);
            if (L->type()->is_reference())
            {
                R = parse(expr->rhs(), fun, &sva);
                l = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
                    expand_ref_put_inplace(L, l, fun, expt_block);
            }
            else
            {
                FixedValueAllocator fva(L);
                R = parse(expr->rhs(), fun, &fva);
                l = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
            }

            fun->last_block()->push_trm_jmp(done_block);
        }

        if (!expt_block->referrers().empty())
        {
            exception_action()->inflate(expt_block, fun);
            fun->push_block(expt_block);
        }

        if (!done_block->referrers().empty())
        {
            fun->push_block(done_block);
        }

        return rva ? X : ValueHandle();
    }

    ValueHandle _, L, R, X, l, r;

    L = parse(expr->lhs(), fun, &temporaries);
    R = parse(expr->rhs(), fun, &temporaries);

    Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

    l = expand_ref_get_inplace_lazy(L, fun, expt_block, temporaries);
    r = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

    switch (expr->operation())
    {
        case parser::AssignmentExpression::ASSIGN_ADD:
            _ = fun->last_block()->push_es_bin_add(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_SUB:
            _ = fun->last_block()->push_es_bin_sub(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_MUL:
            _ = fun->last_block()->push_es_bin_mul(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_MOD:
            _ = fun->last_block()->push_es_bin_mod(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_LS:
            _ = fun->last_block()->push_es_bin_ls(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_RSS:
            _ = fun->last_block()->push_es_bin_rss(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_RUS:
            _ = fun->last_block()->push_es_bin_rus(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_BIT_AND:
            _ = fun->last_block()->push_es_bin_bit_and(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_BIT_OR:
            _ = fun->last_block()->push_es_bin_bit_or(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_BIT_XOR:
            _ = fun->last_block()->push_es_bin_bit_xor(l, r, l);
            break;
        case parser::AssignmentExpression::ASSIGN_DIV:
            _ = fun->last_block()->push_es_bin_div(l, r, l);
            break;

        default:
            assert(false);
            break;
    }

    fun->last_block()->push_trm_br(_, blk0_block, expt_block);
    fun->push_block(blk0_block);

    expand_ref_put(L, l, fun, done_block, expt_block);

    if (rva)
        X = l;

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_cond_expr(parser::ConditionalExpression *expr,
                                      Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R, X;

    Block *true_block = new (GC)Block(NameGenerator::instance().next());
    Block *false_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    R = parse(expr->condition(), fun, &temporaries);
    _ = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
    _ = fun->last_block()->push_val_to_bool(_);
        fun->last_block()->push_trm_br(_, true_block, false_block);

    X = ValueHandle::lazy(rva ? rva : &temporaries);

    // True block.
    fun->push_block(true_block);
    {
        R = parse(expr->left(), fun, rva);
        expand_ref_get(R, X, fun, done_block, expt_block);
    }

    // False block.
    fun->push_block(false_block);
    {
        R = parse(expr->right(), fun, rva);
        expand_ref_get(R, X, fun, done_block, expt_block);
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_prop_expr(parser::PropertyExpression *expr,
                                      Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle R;

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
        ValueHandle o;

        Block *done_block = new (GC)Block(NameGenerator::instance().next());
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        assert(rva);

        R = parse(expr->obj(), fun, rva);
        // FIXME: We must use temporaries.parent() because rva might re-use the
        //        same value for multiple calls to get.
        //o = expand_ref_get_lazy(R, fun, done_block, expt_block, *rva);
        o = expand_ref_get_lazy(R, fun, done_block, expt_block,
                                *temporaries.parent());

        if (!expt_block->referrers().empty())
        {
            exception_action()->inflate(expt_block, fun);
            fun->push_block(expt_block);
        }

        if (!done_block->referrers().empty())
        {
            fun->push_block(done_block);
        }

        return fun->last_block()->push_meta_prp_load(
                o, new (GC)StringConstant(immediate_key_str));
    }
    else
    {
        ValueHandle k, o;

        Block *done_block = new (GC)Block(NameGenerator::instance().next());
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        assert(rva);

        R = parse(expr->key(), fun, rva);
        // FIXME: We must use temporaries.parent() because rva might re-use the
        //        same value for multiple calls to get.
        //k = expand_ref_get_inplace_lazy(R, fun, expt_block, *rva);
        k = expand_ref_get_inplace_lazy(R, fun, expt_block,
                                        *temporaries.parent());

        R = parse(expr->obj(), fun, rva);
        // FIXME: We must use temporaries.parent() because rva might re-use the
        //        same value for multiple calls to get.
        //o = expand_ref_get_inplace_lazy(R, fun, expt_block, *rva);
        o = expand_ref_get_inplace_lazy(R, fun, expt_block,
                                        *temporaries.parent());

        fun->last_block()->push_trm_jmp(done_block);

        if (!expt_block->referrers().empty())
        {
            exception_action()->inflate(expt_block, fun);
            fun->push_block(expt_block);
        }

        if (!done_block->referrers().empty())
        {
            fun->push_block(done_block);
        }

        return fun->last_block()->push_meta_prp_load(o, k);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_call_expr(parser::CallExpression *expr,
                                      Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R, X;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    parser::ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        ValueHandle v;

        R = parse(*it, fun, &temporaries);
        v = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

        // FIXME: What to do if the second or later argument throws? The stack
        //        must be restored somehow.
        //
        //        We only have to act if the exception is catched in this
        //        function or the stack will be cleaned up when the frame is
        //        popped.
        fun->last_block()->push_stk_push(v);

        v.release();
    }

    X = ValueHandle::lazy(rva ? rva : &temporaries);

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
            ValueHandle o;

            R = parse(prop->obj(), fun, &temporaries);
            o = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            _ = fun->last_block()->push_call_keyed(
                    o, get_prp_key(immediate_key_str),
                    static_cast<int>(expr->arguments().size()), X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
        }
        else
        {
            ValueHandle k, o;

            R = parse(prop->key(), fun, &temporaries);
            k = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            R = parse(prop->obj(), fun, &temporaries);
            o = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            fun->last_block()->push_call_keyed_slow(
                    o, k, static_cast<int>(expr->arguments().size()), X);
                    fun->last_block()->push_trm_br(_, done_block, expt_block);
        }
    }
    else if (parser::IdentifierLiteral * ident =
        dynamic_cast<parser::IdentifierLiteral *>(expr->expression()))
    {
        _ = get_local(ident->value(), fun);
        if (_)
        {
            _ = fun->last_block()->push_call(
                    _, static_cast<int>(expr->arguments().size()), X);
        }
        else
        {
            _ = fun->last_block()->push_call_named(
                    get_prp_key(ident->value()),
                    static_cast<int>(expr->arguments().size()), X);
        }
        fun->last_block()->push_trm_br(_, done_block, expt_block);
    }
    else
    {
        ValueHandle f;

        f = parse(expr->expression(), fun, &temporaries);

        assert(!f->type()->is_reference());

        _ = fun->last_block()->push_call(
                f, static_cast<int>(expr->arguments().size()), X);
        _ = fun->last_block()->push_trm_br(_, done_block, expt_block);
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_call_new_expr(parser::CallNewExpression *expr,
                                          Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R, X, f;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    X = ValueHandle::lazy(rva ? rva : &temporaries);

    R = parse(expr->expression(), fun, &temporaries);
    f = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

    parser::ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        ValueHandle v;

        R = parse(*it, fun, &temporaries);
        v = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

        fun->last_block()->push_stk_push(v);

        v.release();
    }

    _ = fun->last_block()->push_call_new(
            f, static_cast<int>(expr->arguments().size()), X);
            fun->last_block()->push_trm_br(_, done_block, expt_block);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_regular_expr(parser::RegularExpression *expr,
                                         Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle X = ValueHandle::lazy(rva ? rva : &temporaries);

    fun->last_block()->push_es_new_rex(expr->pattern(), expr->flags(), X);

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_fun_expr(parser::FunctionExpression *expr,
                                     Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle X = ValueHandle::lazy(rva ? rva : &temporaries);

    const parser::FunctionLiteral *lit = expr->function();

    Function *new_fun = parse_fun(lit, false);
    fun->last_block()->push_es_new_fun_expr(
        new_fun, static_cast<int>(lit->parameters().size()),
        lit->is_strict_mode(), X);

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_this_lit(parser::ThisLiteral *lit,
                                     Function *fun, ValueAllocator *rva)
{
    return rva
            ? new (GC)ArrayElementConstant(new (GC)ValuePointer(), -2)
            : ValueHandle();
}

ValueHandle Compiler::parse_ident_lit(parser::IdentifierLiteral *lit,
                                      Function *fun, ValueAllocator *rva)
{
    Value* X = NULL;

    X = get_local(lit->value(), fun);
    if (X)
        return rva ? ValueHandle(X) : ValueHandle();

    return fun->last_block()->push_meta_ctx_load(get_prp_key(lit->value()));
}

ValueHandle Compiler::parse_null_lit(parser::NullLiteral *lit,
                                     Function *fun, ValueAllocator *rva)
{
    return rva
            ? new (GC)ValueConstant(ValueConstant::VALUE_NULL)
            : ValueHandle();
}

ValueHandle Compiler::parse_bool_lit(parser::BoolLiteral *lit,
                                     Function *fun, ValueAllocator *rva)
{
    ValueHandle X = ValueHandle::lazy(rva);

    if (X)
    {
        fun->last_block()->push_val_from_bool(
                new (GC)BooleanConstant(lit->value()),
                X);
    }

    return rva ? X :ValueHandle();
}

ValueHandle Compiler::parse_num_lit(parser::NumberLiteral *lit,
                                    Function *fun, ValueAllocator *rva)
{
    ValueHandle X = ValueHandle::lazy(rva);

    if (X)
    {
        fun->last_block()->push_val_from_double(
                new (GC)StringifiedDoubleConstant(lit->as_string()),
                X);
    }

    return rva ? X :ValueHandle();
}

ValueHandle Compiler::parse_str_lit(parser::StringLiteral *lit,
                                    Function *fun, ValueAllocator *rva)
{
    ValueHandle X = ValueHandle::lazy(rva);

    if (X)
    {
        fun->last_block()->push_val_from_str(
                new (GC)StringConstant(lit->value()),
                X);
    }

    return rva ? X :ValueHandle();
}

ValueHandle Compiler::parse_fun_lit(parser::FunctionLiteral *lit,
                                    Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle X = ValueHandle::lazy(rva ? rva : &temporaries);

    Function *new_fun = parse_fun(lit, false);
    if (lit->type() == parser::FunctionLiteral::TYPE_DECLARATION)
    {
        fun->last_block()->push_es_new_fun(
                new_fun,
                static_cast<int>(lit->parameters().size()),
                lit->is_strict_mode(),
                X);
    }
    else
    {
        fun->last_block()->push_es_new_fun_expr(
                new_fun,
                static_cast<int>(lit->parameters().size()),
                lit->is_strict_mode(),
                X);
    }

    return rva ? X :ValueHandle();
}

ValueHandle Compiler::parse_var_lit(parser::VariableLiteral *lit,
                                    Function *fun, ValueAllocator *rva)
{
    // Do nothing, dealt with when parsing functions.
    return ValueHandle();
}

ValueHandle Compiler::parse_array_lit(parser::ArrayLiteral *lit,
                                      Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle a, R, X;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    // FIXME: Should use stack.
    a = fun->last_block()->push_mem_alloc(
        new (GC)ArrayType(Type::value(), lit->values().size()));

    int i = 0;
    parser::ExpressionVector::const_iterator it;
    for (it = lit->values().begin(); it != lit->values().end(); ++it, i++)
    {
        ValueHandle v = new (GC)ArrayElementConstant(a, i);
        FixedValueAllocator fva(v);

        R = parse(*it, fun, &fva);
        expand_ref_get_inplace(R, v, fun, expt_block);
    }

    X = ValueHandle::lazy(rva ? rva : &temporaries);

    fun->last_block()->push_es_new_arr(lit->values().size(), a, X);
    fun->last_block()->push_trm_jmp(done_block);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X :ValueHandle();
}

ValueHandle Compiler::parse_obj_lit(parser::ObjectLiteral *lit,
                                    Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R, X;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    X = ValueHandle::lazy(rva ? rva : &temporaries);

    fun->last_block()->push_es_new_obj(X);

    SingleValueAllocator sva_k(temporaries);
    SingleValueAllocator sva_v(temporaries);

    parser::ObjectLiteral::PropertyVector::const_iterator it;
    for (it = lit->properties().begin(); it != lit->properties().end(); ++it)
    {
        const parser::ObjectLiteral::Property *prop = *it;

        if (prop->type() == parser::ObjectLiteral::Property::DATA)
        {
            ValueHandle k, v;

            Block *done_block = new (GC)Block(NameGenerator::instance().next());

            R = parse(prop->key(), fun, &sva_k);
            k = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            R = parse(prop->val(), fun, &sva_v);
            v = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            _ = fun->last_block()->push_prp_def_data(X, k, v);
                fun->last_block()->push_trm_br(_, done_block, expt_block);

            fun->push_block(done_block);

            k.release();
            v.release();
        }
        else
        {
            ValueHandle v;

            Block *done_block = new (GC)Block(NameGenerator::instance().next());

            R = parse(prop->val(), fun, &sva_v);
            v = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            _ = fun->last_block()->push_prp_def_accessor(
                    X, get_prp_key(prop->accessor_name()),
                    v, prop->type() == parser::ObjectLiteral::Property::SETTER);
                fun->last_block()->push_trm_br(_, done_block, expt_block);

            fun->push_block(done_block);

            v.release();
        }
    }

    fun->last_block()->push_trm_jmp(done_block);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X :ValueHandle();
}

ValueHandle Compiler::parse_nothing_lit(parser::NothingLiteral *lit,
                                        Function *fun, ValueAllocator *rva)
{
    return rva
            ? new (GC)ValueConstant(ValueConstant::VALUE_NOTHING)
            : ValueHandle();
}

ValueHandle Compiler::parse_empty_stmt(parser::EmptyStatement *stmt,
                                       Function *fun, ValueAllocator *rva)
{
    return ValueHandle();
}

ValueHandle Compiler::parse_expr_stmt(parser::ExpressionStatement *stmt,
                                      Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle R, X;

    R = parse(stmt->expression(), fun, rva ? rva : &temporaries);

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    X = expand_ref_get_lazy(R, fun, done_block, expt_block, temporaries);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return rva ? X : ValueHandle();
}

ValueHandle Compiler::parse_block_stmt(parser::BlockStatement *stmt,
                                       Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

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
        parse(*it_stmt, fun, NULL);

    // FIXME: We might be able to optimize away this jump if the done block isn't breaked from.
    if (fun->last_block()->empty() || !fun->last_block()->last_instr()->is_terminating())
        fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(done_block);
    return ValueHandle();
}

ValueHandle Compiler::parse_if_stmt(parser::IfStatement *stmt,
                                    Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R;

    Block *true_block = new (GC)Block(NameGenerator::instance().next());
    Block *false_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ValueHandle c;
    R = parse(stmt->condition(), fun, &temporaries);
    c = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
    _ = fun->last_block()->push_val_to_bool(c);
        fun->last_block()->push_trm_br(_, true_block, stmt->has_else() ? false_block : done_block);
    c.release();

    // if block.
    fun->push_block(true_block);
    {
        parse(stmt->if_statement(), fun, NULL);
        fun->last_block()->push_trm_jmp(done_block);
    }

    // else block.
    if (stmt->has_else())
    {
        fun->push_block(false_block);
        {
            parse(stmt->else_statement(), fun, NULL);
            fun->last_block()->push_trm_jmp(done_block);
        }
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_do_while_stmt(parser::DoWhileStatement *stmt,
                                          Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R;

    Block *next_block = new (GC)Block(NameGenerator::instance().next());
    Block *cond_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    fun->last_block()->push_trm_jmp(next_block);

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
        parse(stmt->body(), fun, NULL);
        fun->last_block()->push_trm_jmp(cond_block);
    }

    // Condition block.
    fun->push_block(cond_block);
    {
        if (stmt->has_condition())
        {
            ValueHandle c;
            R = parse(stmt->condition(), fun, &temporaries);
            c = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
            _ = fun->last_block()->push_val_to_bool(c);
                fun->last_block()->push_trm_br(_, next_block, done_block);
            c.release();
        }
        else
        {
            fun->last_block()->push_trm_jmp(next_block);
        }
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_while_stmt(parser::WhileStatement *stmt,
                                       Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R;

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

    fun->last_block()->push_trm_jmp(cond_block);

    // Condition block.
    fun->push_block(cond_block);
    {
        ValueHandle c;
        R = parse(stmt->condition(), fun, &temporaries);
        c = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
        _ = fun->last_block()->push_val_to_bool(c);
            fun->last_block()->push_trm_br(_, next_block, done_block);
        c.release();
    }

    fun->push_block(next_block);
    {
        parse(stmt->body(), fun, NULL);
        fun->last_block()->push_trm_jmp(cond_block);
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_for_in_stmt(parser::ForInStatement *stmt,
                                        Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle i, _, R;

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

    ValueHandle e;
    R = parse(stmt->enumerable(), fun, &temporaries);
    e = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

    _ = fun->last_block()->push_bin_or(
        fun->last_block()->push_val_is_null(e),
        fun->last_block()->push_val_is_undefined(e));
    _ = fun->last_block()->push_trm_br(_, done_block, init_block);  // FIXME: Not needed.

    fun->push_block(init_block);
    {
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        i = fun->last_block()->push_prp_it_new(e);
        _ = fun->last_block()->push_bin_eq(i,
            new (GC)NullConstant(new (GC)OpaqueType("EsPropertyIterator")));
        _ = fun->last_block()->push_trm_br(_, expt_block, cond_block);

        if (!expt_block->referrers().empty())
        {
            exception_action()->inflate(expt_block, fun);
            fun->push_block(expt_block);
        }
    }

    e.release();

    ValueHandle p = ValueHandle::lazy(&temporaries);

    fun->push_block(cond_block);
    {
        _ = fun->last_block()->push_prp_it_next(i, p);
        _ = fun->last_block()->push_trm_br(_, body_block, done_block);
    }

    fun->push_block(body_block);
    {
        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());

        R = parse(stmt->declaration(), fun, &temporaries);
        expand_ref_put(R, p, fun, blk0_block, expt_block);

        fun->push_block(blk0_block);
        parse(stmt->body(), fun, NULL);
        fun->last_block()->push_trm_jmp(cond_block);
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_for_stmt(parser::ForStatement *stmt,
                                     Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R;

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
        parse(stmt->initializer(), fun, NULL);

    fun->last_block()->push_trm_jmp(cond_block);

    fun->push_block(cond_block);

    if (stmt->has_condition())
    {
        ValueHandle c;
        R = parse(stmt->condition(), fun, &temporaries);
        c = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);
        _ = fun->last_block()->push_val_to_bool(c);
            fun->last_block()->push_trm_br(_, body_block, done_block);
        c.release();
    }
    else
    {
        fun->last_block()->push_trm_jmp(body_block);
    }

    fun->push_block(body_block);
    {
        parse(stmt->body(), fun, NULL);
        fun->last_block()->push_trm_jmp(next_block);
    }

    fun->push_block(next_block);
    {
        if (stmt->has_next())
            parse(stmt->next(), fun, NULL);

        // Jump to top.
        fun->last_block()->push_trm_jmp(cond_block);
    }

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_cont_stmt(parser::ContinueStatement *stmt,
                                      Function *fun, ValueAllocator *rva)
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

    return ValueHandle();
}

ValueHandle Compiler::parse_break_stmt(parser::BreakStatement *stmt,
                                       Function *fun, ValueAllocator *rva)
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

    return ValueHandle();
}

ValueHandle Compiler::parse_ret_stmt(parser::ReturnStatement *stmt,
                                     Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle R;

    if (stmt->has_expression())
    {
        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
        Block *expt_block = new (GC)Block(NameGenerator::instance().next());

        ValueHandle r = new (GC)ArrayElementConstant(new (GC)ValuePointer(), -1);   // FIXME: Constant offset.

        FixedValueAllocator fva(r);
        R = parse(stmt->expression(), fun, &fva);
        expand_ref_get(R, r, fun, blk0_block, expt_block);

        if (!blk0_block->referrers().empty())
            fun->push_block(blk0_block);

        unroll_for_return(fun);
        fun->last_block()->push_trm_ret(new (GC)BooleanConstant(true));

        if (!expt_block->referrers().empty())
        {
            exception_action()->inflate(expt_block, fun);
            fun->push_block(expt_block);
        }
    }
    else
    {
        unroll_for_return(fun);
        fun->last_block()->push_trm_ret(new (GC)BooleanConstant(true));
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_with_stmt(parser::WithStatement *stmt,
                                      Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle _, R;

    Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt0_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt1_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(scopes_, new (GC)Scope(Scope::TYPE_WITH));
    scope->set_epilogue(new (GC)LeaveContextTemplateBlock());

    // 12.10.
    ValueHandle v;
    R = parse(stmt->expression(), fun, &temporaries);
    v = expand_ref_get_inplace_lazy(R, fun, expt0_block, temporaries);
    _ = fun->last_block()->push_ctx_enter_with(v);
        fun->last_block()->push_trm_br(_, blk0_block, expt0_block);
    v.release();

    fun->push_block(expt0_block);
    exception_action()->inflate(expt0_block, fun);

    fun->push_block(blk0_block);

    MultiTemplateBlock *multi_block = new (GC)MultiTemplateBlock();
    multi_block->push_back(new (GC)LeaveContextTemplateBlock());
    multi_block->push_back(exception_action());
    ScopedVectorValue<TemplateBlock> expt_action(
        exception_actions_, multi_block);

    parse(stmt->body(), fun, rva);
    fun->last_block()->push_ctx_leave();
    fun->last_block()->push_trm_jmp(done_block);

    fun->push_block(expt1_block);
    exception_action()->inflate(expt1_block, fun);

    fun->push_block(done_block);
    return ValueHandle();
}

ValueHandle Compiler::parse_switch_stmt(parser::SwitchStatement *stmt,
                                        Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle b, _, R;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    ScopedVectorValue<Scope> scope(
        scopes_, new (GC)Scope(Scope::TYPE_SWITCH, done_block));

    // Map labels.
    const parser::LabelList &labels = stmt->labels();

    StringVector::const_iterator it_label;
    for (it_label = labels.begin(); it_label != labels.end(); ++it_label)
        scope->push_label((*it_label).utf8());

    ValueHandle e;

    R = parse(stmt->expression(), fun, &temporaries);
    e = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

    b = fun->last_block()->push_mem_alloc(Type::boolean());    // true if case name is found.
        fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(false));   // FIXME: Can this be removed?

    parser::SwitchStatement::CaseClauseVector::const_iterator it;
    for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
    {
        const parser::SwitchStatement::CaseClause *clause = *it;

        if (!clause->is_default())
        {
            Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
            Block *blk1_block = new (GC)Block(NameGenerator::instance().next());
            Block *skip_block = new (GC)Block(NameGenerator::instance().next());

            fun->last_block()->push_trm_br(b, skip_block, blk0_block);

            fun->push_block(blk0_block);

            ValueHandle v;
            R = parse(clause->label(), fun, &temporaries);
            v = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

            ValueHandle c = ValueHandle::lazy(&temporaries);
            _ = fun->last_block()->push_es_bin_strict_eq(v, e, c);
                fun->last_block()->push_trm_br(_, blk1_block, expt_block);

            v.release();

            fun->push_block(blk1_block);
            _ = fun->last_block()->push_val_to_bool(c);
            c.release();
            fun->last_block()->push_mem_store(b, _);
            fun->last_block()->push_trm_jmp(skip_block);    // FIXME:

            fun->push_block(skip_block);
        }

        Block *blk0_block = new (GC)Block(NameGenerator::instance().next());
        Block *skip_block = new (GC)Block(NameGenerator::instance().next());

        fun->last_block()->push_trm_br(b, blk0_block, skip_block);

        fun->push_block(blk0_block);
        {
            parser::StatementVector::const_iterator it_stmt;
            for (it_stmt = clause->body().begin();
                it_stmt != clause->body().end(); ++it_stmt)
            {
                parse(*it_stmt, fun, NULL);
            }
        }

        fun->last_block()->push_trm_jmp(skip_block);

        fun->push_block(skip_block);
    }

    Block *tdef_block = new (GC)Block(NameGenerator::instance().next());

    fun->last_block()->push_trm_br(b, done_block, tdef_block);

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
                    parse(*it_stmt, fun, NULL);
                }
            }
        }
    }

    fun->last_block()->push_trm_jmp(done_block);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_throw_stmt(parser::ThrowStatement *stmt,
                                       Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle R, t;

    Block *done_block = new (GC)Block(NameGenerator::instance().next());
    Block *expt_block = new (GC)Block(NameGenerator::instance().next());

    // Inflate the exception action into the last block.
    R = parse(stmt->expression(), fun, &temporaries);
    t = expand_ref_get_inplace_lazy(R, fun, expt_block, temporaries);

    fun->last_block()->push_ex_set(t);
    fun->last_block()->push_trm_jmp(expt_block);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_try_stmt(parser::TryStatement *stmt,
                                     Function *fun, ValueAllocator *rva)
{
    TemporaryValueAllocator temporaries(this);

    ValueHandle b;

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
    fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(true));
    {
        ScopedVectorValue<TemplateBlock> action(
            exception_actions_, new (GC)JumpTemplateBlock(fail_block));

        parse(stmt->try_block(), fun, rva);
        fun->last_block()->push_mem_store(b, new (GC)BooleanConstant(false));

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

        parse(stmt->catch_block(), fun, rva);
        fun->last_block()->push_ctx_leave();

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
        ValueHandle t = ValueHandle::lazy(&temporaries);

        fun->last_block()->push_ex_save_state(t);
        parse(stmt->finally_block(), fun, rva);
        fun->last_block()->push_ex_load_state(t);

        t.release();
    }

    // On failure, execute the previous exception action.
    fun->last_block()->push_trm_br(b, expt_block, done_block);

    if (!expt_block->referrers().empty())
    {
        exception_action()->inflate(expt_block, fun);
        fun->push_block(expt_block);
    }

    if (!done_block->referrers().empty())
    {
        fun->push_block(done_block);
    }

    return ValueHandle();
}

ValueHandle Compiler::parse_dbg_stmt(parser::DebuggerStatement *stmt,
                                     Function *fun, ValueAllocator *rva)
{
    return ValueHandle();
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
