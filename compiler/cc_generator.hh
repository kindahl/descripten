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
#include <cassert>
#include <map>
#include "allocator.hh"
#include "generator.hh"
#include "rope.hh"

namespace ir {
    class Module;
    class Function;
    class Block;
    class ArgumentsObjectInitInstruction;
    class ArgumentsObjectLinkInstruction;
    class ArrayInstruction;
    class BinaryInstruction;
    class BindExtraInitInstruction;
    class BindExtraPtrInstruction;
    class CallInstruction;
    class CallKeyedInstruction;
    class CallKeyedSlowInstruction;
    class CallNamedInstruction;
    class ValueInstruction;
    class BranchInstruction;
    class JumpInstruction;
    class ReturnInstruction;
    class StoreInstruction;
    class GetElementPointerInstruction;
    class StackAllocInstruction;
    class StackFreeInstruction;
    class StackPushInstruction;
    class ContextSetStrictInstruction;
    class ContextEnterCatchInstruction;
    class ContextEnterWithInstruction;
    class ContextLeaveInstruction;
    class ContextGetInstruction;
    class ContextPutInstruction;
    class ContextDeleteInstruction;
    class ExceptionSaveStateInstruction;
    class ExceptionLoadStateInstruction;
    class ExceptionSetInstruction;
    class ExceptionClearInstruction;
    class InitArgumentsInstruction;
    class Declaration;
    class Link;
    class PropertyDefineDataInstruction;
    class PropertyDefineAccessorInstruction;
    class PropertyIteratorNewInstruction;
    class PropertyIteratorNextInstruction;
    class PropertyGetInstruction;
    class PropertyPutInstruction;
    class PropertyPutSlowInstruction;
    class PropertyDeleteInstruction;
    class PropertyDeleteSlowInstruction;
    class EsNewArrayInstruction;
    class EsNewFunctionDeclarationInstruction;
    class EsNewFunctionExpressionInstruction;
    class EsNewObjectInstruction;
    class EsNewRegexInstruction;
    class EsAssignmentInstruction;
    class EsBinaryInstruction;
    class EsUnaryInstruction;
    class ArrayElementConstant;
    class NullConstant;
    class BooleanConstant;
    class DoubleConstant;
    class StringifiedDoubleConstant;
    class StringConstant;
    class ValueConstant;

    class StringResource;
}

class CcGenerator : public Generator,
                    public ir::Instruction::Visitor,
                    public ir::Node::Visitor,
                    public ir::Resource::Visitor
{
private:
    Rope *decl_out_;    ///< Declarations, top of document.
    Rope *main_out_;    ///< Main output, follows the declarative region.

    ir::Block *cur_block_;  ///< Current block that's being processed.

private:
    /**
     * @return String stream for raw output.
     */
    std::stringstream &raw()
    {
        assert(main_out_);
        return main_out_->stream();
    }

    /**
     * @return String stream for indented output.
     */
    std::stringstream &out()
    {
        assert(main_out_);
        main_out_->stream() << "  ";
        return main_out_->stream();
    }

private:
    Allocator allocator_;

public:
    static std::string string(const String &str);
    static std::string boolean(bool val);
    static std::string number(double val);
    static std::string type(const ir::Type *type);
    static std::string allocate(const ir::Type *type, const std::string &name);
    static std::string uint32(uint32_t val);
    static std::string uint64(uint64_t val);
    std::string value(ir::Value *val);

private:
    virtual void visit_module(ir::Module *module) override;
    virtual void visit_fun(ir::Function *fun) override;
    virtual void visit_block(ir::Block *block) override;
    virtual void visit_instr_args_obj_init(ir::ArgumentsObjectInitInstruction *instr) override;
    virtual void visit_instr_args_obj_link(ir::ArgumentsObjectLinkInstruction *instr) override;
    virtual void visit_instr_arr(ir::ArrayInstruction *instr) override;
    virtual void visit_instr_bin(ir::BinaryInstruction *instr) override;
    virtual void visit_instr_bnd_extra_init(ir::BindExtraInitInstruction *instr) override;
    virtual void visit_instr_bnd_extra_ptr(ir::BindExtraPtrInstruction *instr) override;
    virtual void visit_instr_call(ir::CallInstruction *instr) override;
    virtual void visit_instr_call_keyed(ir::CallKeyedInstruction *instr) override;
    virtual void visit_instr_call_keyed_slow(ir::CallKeyedSlowInstruction *instr) override;
    virtual void visit_instr_call_named(ir::CallNamedInstruction *instr) override;
    virtual void visit_instr_val(ir::ValueInstruction *instr) override;
    virtual void visit_instr_br(ir::BranchInstruction *instr) override;
    virtual void visit_instr_jmp(ir::JumpInstruction *instr) override;
    virtual void visit_instr_ret(ir::ReturnInstruction *instr) override;
    virtual void visit_instr_store(ir::StoreInstruction *instr) override;
    virtual void visit_instr_get_elm_ptr(ir::GetElementPointerInstruction *instr) override;
    virtual void visit_instr_stk_alloc(ir::StackAllocInstruction *instr) override;
    virtual void visit_instr_stk_free(ir::StackFreeInstruction *instr) override;
    virtual void visit_instr_stk_push(ir::StackPushInstruction *instr) override;
    virtual void visit_instr_ctx_set_strict(ir::ContextSetStrictInstruction *instr) override;
    virtual void visit_instr_ctx_enter_catch(ir::ContextEnterCatchInstruction *instr) override;
    virtual void visit_instr_ctx_enter_with(ir::ContextEnterWithInstruction *instr) override;
    virtual void visit_instr_ctx_leave(ir::ContextLeaveInstruction *instr) override;
    virtual void visit_instr_ctx_get(ir::ContextGetInstruction *instr) override;
    virtual void visit_instr_ctx_put(ir::ContextPutInstruction *instr) override;
    virtual void visit_instr_ctx_del(ir::ContextDeleteInstruction *instr) override;
    virtual void visit_instr_ex_save_state(ir::ExceptionSaveStateInstruction *instr) override;
    virtual void visit_instr_ex_load_state(ir::ExceptionLoadStateInstruction *instr) override;
    virtual void visit_instr_ex_set(ir::ExceptionSetInstruction *instr) override;
    virtual void visit_instr_ex_clear(ir::ExceptionClearInstruction *instr) override;
    virtual void visit_instr_init_args(ir::InitArgumentsInstruction *instr) override;
    virtual void visit_instr_decl(ir::Declaration *instr) override;
    virtual void visit_instr_link(ir::Link *instr) override;
    virtual void visit_instr_prp_def_data(ir::PropertyDefineDataInstruction *instr) override;
    virtual void visit_instr_prp_def_accessor(ir::PropertyDefineAccessorInstruction *instr) override;
    virtual void visit_instr_prp_it_new(ir::PropertyIteratorNewInstruction *instr) override;
    virtual void visit_instr_prp_it_next(ir::PropertyIteratorNextInstruction *instr) override;
    virtual void visit_instr_prp_get(ir::PropertyGetInstruction *instr) override;
    virtual void visit_instr_prp_get_slow(ir::PropertyGetSlowInstruction *instr) override;
    virtual void visit_instr_prp_put(ir::PropertyPutInstruction *instr) override;
    virtual void visit_instr_prp_put_slow(ir::PropertyPutSlowInstruction *instr) override;
    virtual void visit_instr_prp_del(ir::PropertyDeleteInstruction *instr) override;
    virtual void visit_instr_prp_del_slow(ir::PropertyDeleteSlowInstruction *instr) override;
    virtual void visit_instr_es_new_arr(ir::EsNewArrayInstruction *instr) override;
    virtual void visit_instr_es_new_fun_decl(ir::EsNewFunctionDeclarationInstruction *instr) override;
    virtual void visit_instr_es_new_fun_expr(ir::EsNewFunctionExpressionInstruction *instr) override;
    virtual void visit_instr_es_new_obj(ir::EsNewObjectInstruction *instr) override;
    virtual void visit_instr_es_new_rex(ir::EsNewRegexInstruction *instr) override;
    virtual void visit_instr_es_bin(ir::EsBinaryInstruction *instr) override;
    virtual void visit_instr_es_unary(ir::EsUnaryInstruction *instr) override;

    virtual void visit_str_res(ir::StringResource *res) override;

public:
    CcGenerator();

    void generate(ir::Module *module, const std::string &file_path);
};
