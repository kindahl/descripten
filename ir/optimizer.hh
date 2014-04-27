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

namespace ir {

class Optimizer : public Instruction::Visitor,
                  public Node::Visitor,
                  public Resource::Visitor
{
private:
    virtual void visit_module(Module *module) override;
    virtual void visit_fun(Function *fun) override;
    virtual void visit_block(Block *block) override;
    virtual void visit_instr_args_obj_init(ArgumentsObjectInitInstruction *instr) override;
    virtual void visit_instr_args_obj_link(ArgumentsObjectLinkInstruction *instr) override;
    virtual void visit_instr_arr(ArrayInstruction *instr) override;
    virtual void visit_instr_bin(BinaryInstruction *instr) override;
    virtual void visit_instr_bnd_extra_init(BindExtraInitInstruction *instr) override;
    virtual void visit_instr_bnd_extra_ptr(BindExtraPtrInstruction *instr) override;
    virtual void visit_instr_call(CallInstruction *instr) override;
    virtual void visit_instr_call_keyed(CallKeyedInstruction *instr) override;
    virtual void visit_instr_call_keyed_slow(CallKeyedSlowInstruction *instr) override;
    virtual void visit_instr_call_named(CallNamedInstruction *instr) override;
    virtual void visit_instr_val(ValueInstruction *instr) override;
    virtual void visit_instr_br(BranchInstruction *instr) override;
    virtual void visit_instr_jmp(JumpInstruction *instr) override;
    virtual void visit_instr_ret(ReturnInstruction *instr) override;
    virtual void visit_instr_store(StoreInstruction *instr) override;
    virtual void visit_instr_get_elm_ptr(GetElementPointerInstruction *instr) override;
    virtual void visit_instr_stk_alloc(StackAllocInstruction *instr) override;
    virtual void visit_instr_stk_free(StackFreeInstruction *instr) override;
    virtual void visit_instr_stk_push(StackPushInstruction *instr) override;
    virtual void visit_instr_ctx_set_strict(ContextSetStrictInstruction *instr) override;
    virtual void visit_instr_ctx_enter_catch(ContextEnterCatchInstruction *instr) override;
    virtual void visit_instr_ctx_enter_with(ContextEnterWithInstruction *instr) override;
    virtual void visit_instr_ctx_leave(ContextLeaveInstruction *instr) override;
    virtual void visit_instr_ctx_get(ContextGetInstruction *instr) override;
    virtual void visit_instr_ctx_put(ContextPutInstruction *instr) override;
    virtual void visit_instr_ctx_del(ContextDeleteInstruction *instr) override;
    virtual void visit_instr_ex_save_state(ExceptionSaveStateInstruction *instr) override;
    virtual void visit_instr_ex_load_state(ExceptionLoadStateInstruction *instr) override;
    virtual void visit_instr_ex_set(ExceptionSetInstruction *instr) override;
    virtual void visit_instr_ex_clear(ExceptionClearInstruction *instr) override;
    virtual void visit_instr_init_args(InitArgumentsInstruction *instr) override;
    virtual void visit_instr_decl(Declaration *instr) override;
    virtual void visit_instr_link(Link *instr) override;
    virtual void visit_instr_prp_def_data(PropertyDefineDataInstruction *instr) override;
    virtual void visit_instr_prp_def_accessor(PropertyDefineAccessorInstruction *instr) override;
    virtual void visit_instr_prp_it_new(PropertyIteratorNewInstruction *instr) override;
    virtual void visit_instr_prp_it_next(PropertyIteratorNextInstruction *instr) override;
    virtual void visit_instr_prp_get(PropertyGetInstruction *instr) override;
    virtual void visit_instr_prp_get_slow(PropertyGetSlowInstruction *instr) override;
    virtual void visit_instr_prp_put(PropertyPutInstruction *instr) override;
    virtual void visit_instr_prp_put_slow(PropertyPutSlowInstruction *instr) override;
    virtual void visit_instr_prp_del(PropertyDeleteInstruction *instr) override;
    virtual void visit_instr_prp_del_slow(PropertyDeleteSlowInstruction *instr) override;
    virtual void visit_instr_es_new_arr(EsNewArrayInstruction *instr) override;
    virtual void visit_instr_es_new_fun_decl(EsNewFunctionDeclarationInstruction *instr) override;
    virtual void visit_instr_es_new_fun_expr(EsNewFunctionExpressionInstruction *instr) override;
    virtual void visit_instr_es_new_obj(EsNewObjectInstruction *instr) override;
    virtual void visit_instr_es_new_rex(EsNewRegexInstruction *instr) override;
    virtual void visit_instr_es_bin(EsBinaryInstruction *instr) override;
    virtual void visit_instr_es_unary(EsUnaryInstruction *instr) override;

    virtual void visit_str_res(StringResource *res) override;

public:
    Optimizer();

    void optimize(Module *module);
};

}
