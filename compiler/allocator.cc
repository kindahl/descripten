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

#include <algorithm>
#include <gc_cpp.h>
#include "ir/ir.hh"
#include "allocator.hh"

Allocator::RegisterPool::RegisterPool()
    : next_reg_number_(0)
{
}

void Allocator::RegisterPool::put(Allocator::Register *reg)
{
    if (reg->persistent())
        return;

    assert(reg);
    type_reg_map_[reg->type()].push_back(reg);
}

Allocator::Register *Allocator::RegisterPool::get(const ir::Value *value)
{
    TypeRegisterMap::iterator it = type_reg_map_.find(value->type());
    if (it == type_reg_map_.end() || it->second.empty())
    {
        Allocator::Register *reg =
            new (GC)Allocator::Register(value->type(), next_reg_number_++, value->persistent());
        registers_.push_back(reg);
        return reg;
    }

    Allocator::Register *reg = it->second.back();
    it->second.pop_back();

    assert(reg != NULL);
    return reg;
}

const Allocator::RegisterVector &Allocator::RegisterPool::registers() const
{
    return registers_;
}

Allocator::Allocator()
    : cur_fun_(NULL)
{
}

void Allocator::touch(ir::Value *val)
{
    assert(cur_fun_);
    assert(val->type()->identifier() != ir::Type::ID_VOID);

    IntervalMap::iterator it_existing = interval_map_.find(val);
    if (it_existing != interval_map_.end())
    {
        it_existing->second->grow_to(cur_fun_->cur_pos_);
        return;
    }

    Interval *interval = new (GC)Interval(val, cur_fun_->cur_pos_);
    interval_map_.insert(std::make_pair(val, interval));
    cur_fun_->interval_map_.insert(std::make_pair(val, interval));
}

void Allocator::visit_module(ir::Module *module)
{
    ir::FunctionVector::const_iterator it;
    for (it = module->functions().begin(); it != module->functions().end(); ++it)
        ir::Node::Visitor::visit(*it);
}

void Allocator::visit_fun(ir::Function *fun)
{
    cur_fun_ = new (GC)Function();
    fun_map_[fun] = cur_fun_;

    ir::BlockList::ConstIterator it;
    for (it = fun->blocks().begin(); it != fun->blocks().end(); ++it)
        ir::Node::Visitor::visit(const_cast<ir::Block *>(it.raw_pointer()));

    // Run a slightly modified version of the linear scan register allocation
    // algorithm.
    typedef std::multiset<Interval *,
                          Interval::IncreasingEndComparator,
                          gc_allocator<Interval *> > IntervalSet;
    IntervalSet active_set;

    typedef std::vector<Interval *,
                        gc_allocator<Interval *> > IntervalVector;
    IntervalVector live_intervals;

    IntervalMap::iterator it_iterval;
    for (it_iterval = cur_fun_->interval_map_.begin();
        it_iterval != cur_fun_->interval_map_.end(); ++it_iterval)
    {
        live_intervals.push_back(it_iterval->second);
    }

    std::sort(live_intervals.begin(), live_intervals.end(),
              Interval::IncreasingStartComparator());

    IntervalVector::iterator it_live;
    for (it_live = live_intervals.begin();
        it_live != live_intervals.end(); ++it_live)
    {
        Interval *live = *it_live;

        // Expire old intervals.
        IntervalSet::iterator it_active;
        for (it_active = active_set.begin(); it_active != active_set.end();)
        {
            Interval *active = *it_active;
            if (active->end() >= live->start())
                break;

            IntervalSet::iterator to_delete = it_active++;
            active_set.erase(to_delete);

            cur_fun_->register_pool_.put(active->reg());
        }

        // Allocate register.
        live->set_register(cur_fun_->register_pool_.get(live->value()));
        active_set.insert(live);
    }
}

void Allocator::visit_block(ir::Block *block)
{
    ir::InstructionVector::const_iterator it;
    for (it = block->instructions().begin();
        it != block->instructions().end(); ++it)
    {
        ir::Instruction::Visitor::visit(*it);
    }
}

void Allocator::visit_instr_args_obj_init(ir::ArgumentsObjectInitInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_args_obj_link(ir::ArgumentsObjectLinkInstruction *instr)
{
    touch(instr->arguments());
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_arr(ir::ArrayInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::ArrayInstruction::GET:
        {
            touch(instr);
            touch(instr->array());
            break;
        }
        case ir::ArrayInstruction::PUT:
        {
            touch(instr->array());
            touch(instr->value());
            break;
        }
        default:
            assert(false);
            break;
    }

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_bin(ir::BinaryInstruction *instr)
{
    touch(instr);
    touch(instr->left());
    touch(instr->right());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_bnd_extra_init(ir::BindExtraInitInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_bnd_extra_ptr(ir::BindExtraPtrInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_call(ir::CallInstruction *instr)
{
    touch(instr);
    touch(instr->function());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_call_keyed(ir::CallKeyedInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_call_keyed_slow(ir::CallKeyedSlowInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->key());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_call_named(ir::CallNamedInstruction *instr)
{
    touch(instr);
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_val(ir::ValueInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::ValueInstruction::TO_BOOLEAN:
            touch(instr);
            touch(instr->value());
            break;
        case ir::ValueInstruction::TO_DOUBLE:
        case ir::ValueInstruction::TO_STRING:
            touch(instr);
            touch(instr->value());
            touch(instr->result());
            break;

        case ir::ValueInstruction::FROM_BOOLEAN:
        case ir::ValueInstruction::FROM_DOUBLE:
        case ir::ValueInstruction::FROM_STRING:
        case ir::ValueInstruction::IS_NULL:
        case ir::ValueInstruction::IS_UNDEFINED:
        case ir::ValueInstruction::TEST_COERCIBILITY:
            touch(instr);
            touch(instr->value());
            break;

        default:
            assert(false);
            break;
    }

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_br(ir::BranchInstruction *instr)
{
    touch(instr->condition());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_jmp(ir::JumpInstruction *instr)
{
    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ret(ir::ReturnInstruction *instr)
{
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_mem_alloc(ir::MemoryAllocInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_mem_store(ir::MemoryStoreInstruction *instr)
{
    //touch(instr->destination());
    touch(instr->source());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_mem_elm_ptr(ir::MemoryElementPointerInstruction *instr)
{
    touch(instr);
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_stk_alloc(ir::StackAllocInstruction *instr)
{
    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_stk_free(ir::StackFreeInstruction *instr)
{
    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_stk_push(ir::StackPushInstruction *instr)
{
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_set_strict(ir::ContextSetStrictInstruction *instr)
{
    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_enter_catch(ir::ContextEnterCatchInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_enter_with(ir::ContextEnterWithInstruction *instr)
{
    touch(instr);
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_leave(ir::ContextLeaveInstruction *instr)
{
    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_get(ir::ContextGetInstruction *instr)
{
    touch(instr);
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_put(ir::ContextPutInstruction *instr)
{
    touch(instr);
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ctx_del(ir::ContextDeleteInstruction *instr)
{
    touch(instr);
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ex_save_state(ir::ExceptionSaveStateInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ex_load_state(ir::ExceptionLoadStateInstruction *instr)
{
    touch(instr->state());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ex_set(ir::ExceptionSetInstruction *instr)
{
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_ex_clear(ir::ExceptionClearInstruction *instr)
{
    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_init_args(ir::InitArgumentsInstruction *instr)
{
    touch(instr->destination());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_decl(ir::Declaration *instr)
{
    switch (instr->kind())
    {
        case ir::Declaration::FUNCTION:
            touch(instr);
            touch(instr->value());
            break;

        case ir::Declaration::VARIABLE:
            touch(instr);
            break;

        case ir::Declaration::PARAMETER:
            touch(instr);
            touch(instr->parameter_array());
            break;

        default:
            assert(false);
            break;
    }

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_link(ir::Link *instr)
{
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_def_data(ir::PropertyDefineDataInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->key());
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_def_accessor(ir::PropertyDefineAccessorInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->function());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_it_new(ir::PropertyIteratorNewInstruction *instr)
{
    touch(instr);
    touch(instr->object());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_it_next(ir::PropertyIteratorNextInstruction *instr)
{
    touch(instr);
    touch(instr->iterator());
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_get(ir::PropertyGetInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_get_slow(ir::PropertyGetSlowInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->key());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_put(ir::PropertyPutInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_put_slow(ir::PropertyPutSlowInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->key());
    touch(instr->value());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_del(ir::PropertyDeleteInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_prp_del_slow(ir::PropertyDeleteSlowInstruction *instr)
{
    touch(instr);
    touch(instr->object());
    touch(instr->key());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_new_arr(ir::EsNewArrayInstruction *instr)
{
    touch(instr);
    touch(instr->values());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_new_fun_decl(ir::EsNewFunctionDeclarationInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_new_fun_expr(ir::EsNewFunctionExpressionInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_new_obj(ir::EsNewObjectInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_new_rex(ir::EsNewRegexInstruction *instr)
{
    touch(instr);

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_bin(ir::EsBinaryInstruction *instr)
{
    touch(instr);
    touch(instr->left());
    touch(instr->right());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

void Allocator::visit_instr_es_unary(ir::EsUnaryInstruction *instr)
{
    touch(instr);
    touch(instr->value());
    touch(instr->result());

    assert(cur_fun_);
    cur_fun_->cur_pos_++;
}

int Allocator::lookup(ir::Value *val)
{
    IntervalMap::const_iterator it = interval_map_.find(val);
    assert(it != interval_map_.end());

    return it->second->reg()->number();
}

const Allocator::RegisterVector &Allocator::allocations(ir::Function *fun)
{
    assert(fun_map_.count(fun) > 0);

    return fun_map_[fun]->register_pool_.registers();
}

void Allocator::run(ir::Module *module)
{
    interval_map_.clear();
    fun_map_.clear();
    cur_fun_ = NULL;

    ir::Node::Visitor::visit(module);
}
