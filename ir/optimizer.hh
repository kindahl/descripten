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
#include "common/core.hh"

namespace ir {

class Optimizer : public Instruction::Visitor,
                  public Node::Visitor,
                  public Resource::Visitor
{
private:
    virtual void visit_module(Module *module) OVERRIDE;
    virtual void visit_fun(Function *fun) OVERRIDE;
    virtual void visit_block(Block *block) OVERRIDE;
    virtual void visit_instr_args_obj_init(ArgumentsObjectInitInstruction *instr) OVERRIDE;
    virtual void visit_instr_args_obj_link(ArgumentsObjectLinkInstruction *instr) OVERRIDE;
    virtual void visit_instr_arr(ArrayInstruction *instr) OVERRIDE;
    virtual void visit_instr_bin(BinaryInstruction *instr) OVERRIDE;
    virtual void visit_instr_bnd_extra_init(BindExtraInitInstruction *instr) OVERRIDE;
    virtual void visit_instr_bnd_extra_ptr(BindExtraPtrInstruction *instr) OVERRIDE;
    virtual void visit_instr_call(CallInstruction *instr) OVERRIDE;
    virtual void visit_instr_call_keyed(CallKeyedInstruction *instr) OVERRIDE;
    virtual void visit_instr_call_keyed_slow(CallKeyedSlowInstruction *instr) OVERRIDE;
    virtual void visit_instr_call_named(CallNamedInstruction *instr) OVERRIDE;
    virtual void visit_instr_val(ValueInstruction *instr) OVERRIDE;
    virtual void visit_instr_br(BranchInstruction *instr) OVERRIDE;
    virtual void visit_instr_jmp(JumpInstruction *instr) OVERRIDE;
    virtual void visit_instr_ret(ReturnInstruction *instr) OVERRIDE;
    virtual void visit_instr_store(StoreInstruction *instr) OVERRIDE;
    virtual void visit_instr_get_elm_ptr(GetElementPointerInstruction *instr) OVERRIDE;
    virtual void visit_instr_stk_alloc(StackAllocInstruction *instr) OVERRIDE;
    virtual void visit_instr_stk_free(StackFreeInstruction *instr) OVERRIDE;
    virtual void visit_instr_stk_push(StackPushInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_set_strict(ContextSetStrictInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_enter_catch(ContextEnterCatchInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_enter_with(ContextEnterWithInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_leave(ContextLeaveInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_get(ContextGetInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_put(ContextPutInstruction *instr) OVERRIDE;
    virtual void visit_instr_ctx_del(ContextDeleteInstruction *instr) OVERRIDE;
    virtual void visit_instr_ex_save_state(ExceptionSaveStateInstruction *instr) OVERRIDE;
    virtual void visit_instr_ex_load_state(ExceptionLoadStateInstruction *instr) OVERRIDE;
    virtual void visit_instr_ex_set(ExceptionSetInstruction *instr) OVERRIDE;
    virtual void visit_instr_ex_clear(ExceptionClearInstruction *instr) OVERRIDE;
    virtual void visit_instr_init_args(InitArgumentsInstruction *instr) OVERRIDE;
    virtual void visit_instr_decl(Declaration *instr) OVERRIDE;
    virtual void visit_instr_link(Link *instr) OVERRIDE;
    virtual void visit_instr_prp_def_data(PropertyDefineDataInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_def_accessor(PropertyDefineAccessorInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_it_new(PropertyIteratorNewInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_it_next(PropertyIteratorNextInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_get(PropertyGetInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_get_slow(PropertyGetSlowInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_put(PropertyPutInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_put_slow(PropertyPutSlowInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_del(PropertyDeleteInstruction *instr) OVERRIDE;
    virtual void visit_instr_prp_del_slow(PropertyDeleteSlowInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_new_arr(EsNewArrayInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_new_fun_decl(EsNewFunctionDeclarationInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_new_fun_expr(EsNewFunctionExpressionInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_new_obj(EsNewObjectInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_new_rex(EsNewRegexInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_bin(EsBinaryInstruction *instr) OVERRIDE;
    virtual void visit_instr_es_unary(EsUnaryInstruction *instr) OVERRIDE;

    virtual void visit_str_res(StringResource *res) OVERRIDE;

public:
    Optimizer();

    void optimize(Module *module);
};

}
