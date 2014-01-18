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

#include "ir.hh"
#include "optimizer.hh"

namespace ir {

Optimizer::Optimizer()
{
}

void Optimizer::visit_module(Module *module)
{
    for (const Resource *res : module->resources())
        Resource::Visitor::visit(const_cast<Resource *>(res));

    FunctionVector::const_iterator it;
    for (it = module->functions().begin(); it != module->functions().end(); ++it)
        Node::Visitor::visit(*it);
}

void Optimizer::visit_fun(Function *fun)
{
    BlockList::Iterator it_block;
    for (it_block = fun->mutable_blocks().begin(); it_block != fun->mutable_blocks().end(); ++it_block)
    {
        // Eliminated blocks that are never visited.
        Block *block = it_block.raw_pointer();
        if (it_block != fun->mutable_blocks().begin())
        {
            if (block->referrers().empty())
            {
                fun->mutable_blocks().erase(it_block);
                continue;
            }
        }

        Node::Visitor::visit(block);
    }
}

void Optimizer::visit_block(Block *block)
{
    InstructionVector::const_iterator it;
    for (it = block->instructions().begin();
        it != block->instructions().end(); ++it)
    {
        // FIXME: Remove instructions after any terminating instruction that
        //        isnt' the last instruction of a block.
        /*Instruction *instr = *it;
        if (instr->is_terminating())
            assert(it == --block->instructions().end());*/
        Instruction::Visitor::visit(*it);
    }
}

void Optimizer::visit_instr_args_obj_init(ArgumentsObjectInitInstruction *instr)
{
}

void Optimizer::visit_instr_args_obj_link(ArgumentsObjectLinkInstruction *instr)
{
}

void Optimizer::visit_instr_arr(ArrayInstruction *instr)
{
}

void Optimizer::visit_instr_bin(BinaryInstruction *instr)
{
}

void Optimizer::visit_instr_bnd_extra_init(BindExtraInitInstruction *instr)
{
}

void Optimizer::visit_instr_bnd_extra_ptr(BindExtraPtrInstruction *instr)
{
}

void Optimizer::visit_instr_call(CallInstruction *instr)
{
}

void Optimizer::visit_instr_call_keyed(CallKeyedInstruction *instr)
{
}

void Optimizer::visit_instr_call_keyed_slow(CallKeyedSlowInstruction *instr)
{
}

void Optimizer::visit_instr_call_named(CallNamedInstruction *instr)
{
}

void Optimizer::visit_instr_val(ValueInstruction *instr)
{
}

void Optimizer::visit_instr_br(BranchInstruction *instr)
{
}

void Optimizer::visit_instr_jmp(JumpInstruction *instr)
{
}

void Optimizer::visit_instr_ret(ReturnInstruction *instr)
{
}

void Optimizer::visit_instr_store(StoreInstruction *instr)
{
}

void Optimizer::visit_instr_get_elm_ptr(GetElementPointerInstruction *instr)
{
}

void Optimizer::visit_instr_stk_alloc(StackAllocInstruction *instr)
{
}

void Optimizer::visit_instr_stk_free(StackFreeInstruction *instr)
{
}

void Optimizer::visit_instr_stk_push(StackPushInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_set_strict(ContextSetStrictInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_enter_catch(ContextEnterCatchInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_enter_with(ContextEnterWithInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_leave(ContextLeaveInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_get(ContextGetInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_put(ContextPutInstruction *instr)
{
}

void Optimizer::visit_instr_ctx_del(ContextDeleteInstruction *instr)
{
}

void Optimizer::visit_instr_ex_save_state(ExceptionSaveStateInstruction *instr)
{
}

void Optimizer::visit_instr_ex_load_state(ExceptionLoadStateInstruction *instr)
{
}

void Optimizer::visit_instr_ex_set(ExceptionSetInstruction *instr)
{
}

void Optimizer::visit_instr_ex_clear(ExceptionClearInstruction *instr)
{
}

void Optimizer::visit_instr_init_args(InitArgumentsInstruction *instr)
{
}

void Optimizer::visit_instr_decl(Declaration *instr)
{
}

void Optimizer::visit_instr_link(Link *instr)
{
}

void Optimizer::visit_instr_prp_def_data(PropertyDefineDataInstruction *instr)
{
}

void Optimizer::visit_instr_prp_def_accessor(PropertyDefineAccessorInstruction *instr)
{
}

void Optimizer::visit_instr_prp_it_new(PropertyIteratorNewInstruction *instr)
{
}

void Optimizer::visit_instr_prp_it_next(PropertyIteratorNextInstruction *instr)
{
}

void Optimizer::visit_instr_prp_get(PropertyGetInstruction *instr)
{
}

void Optimizer::visit_instr_prp_get_slow(PropertyGetSlowInstruction *instr)
{
}

void Optimizer::visit_instr_prp_put(PropertyPutInstruction *instr)
{
}

void Optimizer::visit_instr_prp_put_slow(PropertyPutSlowInstruction *instr)
{
}

void Optimizer::visit_instr_prp_del(PropertyDeleteInstruction *instr)
{
}

void Optimizer::visit_instr_prp_del_slow(PropertyDeleteSlowInstruction *instr)
{
}

void Optimizer::visit_instr_es_new_arr(EsNewArrayInstruction *instr)
{
}

void Optimizer::visit_instr_es_new_fun_decl(EsNewFunctionDeclarationInstruction *instr)
{
}

void Optimizer::visit_instr_es_new_fun_expr(EsNewFunctionExpressionInstruction *instr)
{
}

void Optimizer::visit_instr_es_new_obj(EsNewObjectInstruction *instr)
{
}

void Optimizer::visit_instr_es_new_rex(EsNewRegexInstruction *instr)
{
}

void Optimizer::visit_instr_es_bin(EsBinaryInstruction *instr)
{
}

void Optimizer::visit_instr_es_unary(EsUnaryInstruction *instr)
{
}

void Optimizer::visit_str_res(StringResource *res)
{
}

void Optimizer::optimize(Module *module)
{
    Node::Visitor::visit(module);
}

}
