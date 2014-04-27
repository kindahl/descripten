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
#include <map>

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
    class PropertyGetSlowInstruction;
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
}

class Allocator : public ir::Instruction::Visitor, public ir::Node::Visitor
{
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

public:
    class Register
    {
    private:
        const ir::Type *type_;
        int number_;
        bool persistent_;

    public:
        Register(const ir::Type *type, int number, bool persistent)
            : type_(type)
            , number_(number)
            , persistent_(persistent) {}

        const ir::Type *type() const
        {
            assert(type_);
            return type_;
        }

        int number() const
        {
            return number_;
        }

        bool persistent() const
        {
            return persistent_;
        }
    };

    typedef std::vector<Register *,
                        gc_allocator<Register *> > RegisterVector;

    class Interval
    {
    public:
        struct IncreasingStartComparator
        {
            bool operator()(const Interval *i1, const Interval *i2) const
            {
                return i1->start_ < i2->start_;
            }
        };

        struct IncreasingEndComparator
        {
            bool operator()(const Interval *i1, const Interval *i2) const
            {
                return i1->end_ < i2->end_;
            }
        };

    private:
        const ir::Value *value_;
        int start_;
        int end_;

        Register *reg_;

    public:
        Interval(const ir::Value *value, int start)
            : value_(value)
            , start_(start)
            , end_(start)
            , reg_(NULL) {}

        const ir::Value *value() const
        {
            return value_;
        }

        int start() const
        {
            return start_;
        }

        int end() const
        {
            return end_;
        }

        Register *reg() const
        {
            assert(reg_);
            return reg_;
        }

        void set_register(Register *reg)
        {
            assert(reg_ == NULL);
            reg_ = reg;
            assert(reg_ != NULL);
        }

        void grow_to(int end)
        {
            assert(end >= start_);
            end_ = end;
        }
    };

    class RegisterPool
    {
    private:
        /** Maps types to a vector of registers, true if the register is used, false
         * if it's free. */
        typedef std::map<const ir::Type *, RegisterVector,
                         ir::Type::less,
                         gc_allocator<std::pair<const ir::Type *,
                                                RegisterVector> > > TypeRegisterMap;
        TypeRegisterMap type_reg_map_;

        /** All registers allocated through the register pool. */
        RegisterVector registers_;

        int next_reg_number_;

    public:
        RegisterPool();

        void put(Register *reg);
        Register *get(const ir::Value *value);

        const RegisterVector &registers() const;
    };

    // Maps values to type specific live intervals.
    typedef std::map<ir::Value *, Interval *,
                     std::less<ir::Value *>,
                     gc_allocator<std::pair<ir::Value *, Interval *> > > IntervalMap;

private:
    IntervalMap interval_map_;

private:
    struct Function
    {
        int cur_pos_;
        RegisterPool register_pool_;    ///< Function register pool.
        IntervalMap interval_map_;      ///< Allocations for this function.

        Function()
            : cur_pos_(0) {}
    };

    typedef std::map<ir::Function *, Function *,
                     std::less<ir::Function *>,
                     gc_allocator<std::pair<ir::Function *, Function *> > > FunctionMap;
    FunctionMap fun_map_;

    Function *cur_fun_;

private:
    void touch(ir::Value *val);

public:
    Allocator();

    /**
     * @return Number of slot to store value in.
     */
    int lookup(ir::Value *val);

    /**
     * Returns a map of all allocations occurring in a given function.
     * @param [in] fun Function to retrieve allocations for.
     * @return Map of allocations.
     */
    const RegisterVector &allocations(ir::Function *fun);

    /**
     * Run the allocator on the given module.
     * @param [in] module Module to run allocator on.
     */
    void run(ir::Module *module);
};
