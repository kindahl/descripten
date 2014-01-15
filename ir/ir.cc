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

#include <cassert>
#include <gc_cpp.h>
#include "ir.hh"

namespace ir {

const Type *Type::_void()
{
    static Type *type = new (GC)Type(ID_VOID);
    return type;
}

const Type *Type::boolean()
{
    static Type *type = new (GC)Type(ID_BOOLEAN);
    return type;
}

const Type *Type::_double()
{
    static Type *type = new (GC)Type(ID_DOUBLE);
    return type;
}

const Type *Type::string()
{
    static Type *type = new (GC)Type(ID_STRING);
    return type;
}

const Type *Type::value()
{
    static Type *type = new (GC)Type(ID_VALUE);
    return type;
}

const Type *Type::reference()
{
    static Type *type = new (GC)ReferenceType(String());
    return type;
}

const FunctionVector &Module::functions() const
{
    return functions_;
}

void Module::push_function(Function *fun)
{
    functions_.push_back(fun);
}

const ResourceVector &Module::resources() const
{
    return resources_;
}

void Module::push_resource(Resource *res)
{
    resources_.push_back(res);
}

Function::Function(const std::string &name, bool is_global)
    : is_global_(is_global)
    , name_(name)
{
    // Create initial block.
    blocks_.push_back(new (GC)Block());
}

bool Function::is_global() const
{
    return is_global_;
}

const std::string &Function::name() const
{
    return name_;
}

BlockList &Function::mutable_blocks()
{
    return blocks_;
}

const BlockList &Function::blocks() const
{
    return blocks_;
}

void Function::push_block(Block *block)
{
#ifdef DEBUG
    // We do not allow empty blocks, but we cannot check that the new @a block
    // is not empty becase the compiler works by adding empty blocks and then
    // populating them. Though we cannot check @a block, we can make sure that
    // we're not leaving any empty blocks by checking the preivously last
    // block.
    if (!blocks_.empty())
        assert(!last_block()->empty());
#endif
    blocks_.push_back(block);
}

Block::Block()
{
}

Block::Block(const std::string &label)
    : label_(label)
{
}

void Block::add_referrer(Instruction *instr)
{
    referrers_.insert(instr);
}

void Block::remove_referrer(Instruction *instr)
{
    referrers_.erase(instr);
}

const InstructionSet &Block::referrers() const
{
    return referrers_;
}

Block *Function::last_block() const
{
    assert(!blocks_.empty());
    return &blocks_.back();
}

const std::string &Block::label() const
{
    return label_;
}

const InstructionVector &Block::instructions() const
{
    return instrs_;
}

Instruction *Block::last_instr() const
{
    assert(!instrs_.empty());
    return instrs_.back();
}

void Block::push_instr(Instruction *instr)
{
#ifdef DEBUG
    // We should not continue to add instructions after a terminating
    // instruction has been added.
    /*if (!instrs_.empty())
        assert(!last_instr()->is_terminating());*/
#endif

    instrs_.push_back(instr);
}

Value *Block::push_args_obj_init()
{
    Instruction *instr = new (GC)ArgumentsObjectInitInstruction();
    push_instr(instr);
    return instr;
}

Value *Block::push_args_obj_link(Value *args, int index, Value *val)
{
    Instruction *instr =
        new (GC)ArgumentsObjectLinkInstruction(args, index, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_arr_get(size_t index, Value *arr)
{
    Instruction *instr =
        new (GC)ArrayInstruction(ArrayInstruction::GET, index, arr);
    push_instr(instr);
    return instr;
}

Value *Block::push_arr_put(size_t index, Value *arr, Value *val)
{
    Instruction *instr =
        new (GC)ArrayInstruction(ArrayInstruction::PUT, index, arr, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_bin_add(Value *op1, Value *op2)
{
    Instruction *instr =
        new (GC)BinaryInstruction(BinaryInstruction::ADD, op1, op2);
    push_instr(instr);
    return instr;
}

Value *Block::push_bin_sub(Value *op1, Value *op2)
{
    Instruction *instr =
        new (GC)BinaryInstruction(BinaryInstruction::SUB, op1, op2);
    push_instr(instr);
    return instr;
}

Value *Block::push_bin_or(Value *op1, Value *op2)
{
    Instruction *instr =
        new (GC)BinaryInstruction(BinaryInstruction::OR, op1, op2);
    push_instr(instr);
    return instr;
}

Value *Block::push_bin_eq(Value *op1, Value *op2)
{
    Instruction *instr =
        new (GC)BinaryInstruction(BinaryInstruction::EQ, op1, op2);
    push_instr(instr);
    return instr;
}

Value *Block::push_bnd_extra_init(int num_extra)
{
    Instruction *instr =
        new (GC)BindExtraInitInstruction(num_extra);
    push_instr(instr);
    return instr;
}

Value *Block::push_bnd_extra_ptr(int hops)
{
    Instruction *instr =
        new (GC)BindExtraPtrInstruction(hops);
    push_instr(instr);
    return instr;
}

Value *Block::push_call(Value *fun, int argc, Value *res)
{
    Instruction *instr =
        new (GC)CallInstruction(CallInstruction::NORMAL, fun, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_keyed(Value *obj, uint64_t key, int argc, Value *res)
{
    Instruction *instr =
        new (GC)CallKeyedInstruction(obj, key, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_keyed_slow(Value *obj, Value *key, int argc,
                                   Value *res)
{
    Instruction *instr =
        new (GC)CallKeyedSlowInstruction(obj, key, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_named(uint64_t key, int argc, Value *res)
{
    Instruction *instr =
        new (GC)CallNamedInstruction(key, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_new(Value *fun, int argc, Value *res)
{
    Instruction *instr =
        new (GC)CallInstruction(CallInstruction::NEW, fun, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_mem_alloc(const Type *type)
{
    Instruction *instr = new (GC)MemoryAllocInstruction(type);
    push_instr(instr);
    return instr;
}

Value *Block::push_mem_store(Value *dst, Value *src)
{
    Instruction *instr = new (GC)MemoryStoreInstruction(dst, src);
    push_instr(instr);
    return instr;
}

Value *Block::push_mem_elm_ptr(Value *val, size_t index)
{
    // We will take the address of value. This means that we no longer can
    // perform life time analysis of it. It should be made persistent.
    val->make_persistent();

    Instruction *instr = new (GC)MemoryElementPointerInstruction(val, index);
    push_instr(instr);
    return instr;
}

Value *Block::push_stk_alloc(size_t count)
{
    Instruction *instr = new (GC)StackAllocInstruction(count);
    push_instr(instr);
    return instr;
}

Value *Block::push_stk_free(size_t count)
{
    Instruction *instr = new (GC)StackFreeInstruction(count);
    push_instr(instr);
    return instr;
}

Value *Block::push_stk_push(Value *val)
{
    Instruction *instr = new (GC)StackPushInstruction(val);
    push_instr(instr);
    return instr;
}

#ifdef UNUSED
Value *Block::push_phi_2(Block *block1, Value *value1, Block *block2, Value *value2)
{
    PhiInstruction::ArgumentVector args(2);
    args.push_back(new (GC)PhiInstruction::Argument(block1, value1));
    args.push_back(new (GC)PhiInstruction::Argument(block2, value2));

    Instruction *instr = new (GC)PhiInstruction(args);
    push_instr(instr);

    block1->add_referrer(instr);
    block2->add_referrer(instr);
    return instr;
}
#endif

Value *Block::push_prp_def_data(Value *obj, Value *key, Value *val)
{
    Instruction *instr =
        new (GC)PropertyDefineDataInstruction(obj, key, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_def_accessor(Value *obj, uint64_t key, Value *fun, bool is_setter)
{
    Instruction *instr =
        new (GC)PropertyDefineAccessorInstruction(obj, key, fun, is_setter);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_it_new(Value *obj)
{
    Instruction *instr = new (GC)PropertyIteratorNewInstruction(obj);
    instr->make_persistent();

    push_instr(instr);
    return instr;
}

Value *Block::push_prp_it_next(Value *it, Value *val)
{
    Instruction *instr = new (GC)PropertyIteratorNextInstruction(it, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_get(Value *obj, uint64_t key, Value *res)
{
    Instruction *instr = new (GC)PropertyGetInstruction(obj, key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_get_slow(Value *obj, Value *key, Value *res)
{
    Instruction *instr = new (GC)PropertyGetSlowInstruction(obj, key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_put(Value *obj, uint64_t key, Value *val)
{
    Instruction *instr = new (GC)PropertyPutInstruction(obj, key, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_put_slow(Value *obj, Value *key, Value *val)
{
    Instruction *instr = new (GC)PropertyPutSlowInstruction(obj, key, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_del(Value *obj, uint64_t key, Value *res)
{
    Instruction *instr =
        new (GC)PropertyDeleteInstruction(obj, key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_del_slow(Value *obj, Value *key, Value *res)
{
    Instruction *instr =
        new (GC)PropertyDeleteSlowInstruction(obj, key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_trm_br(Value *cond, Block *true_block, Block *false_block)
{
    Instruction *instr =
        new (GC)BranchInstruction(this, cond, true_block, false_block);
    push_instr(instr);

    true_block->add_referrer(instr);
    false_block->add_referrer(instr);
    return instr;
}

Value *Block::push_trm_jmp(Block *block)
{
    Instruction *instr = new (GC)JumpInstruction(this, block);
    push_instr(instr);

    block->add_referrer(instr);
    return instr;
}

Value *Block::push_trm_ret(Value *val)
{
    Instruction *instr = new (GC)ReturnInstruction(this, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_to_bool(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::TO_BOOLEAN, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_to_double(Value *val, Value *res)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::TO_DOUBLE, val, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_to_str(Value *val, Value *res)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::TO_STRING, val, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_from_bool(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::FROM_BOOLEAN, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_from_double(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::FROM_DOUBLE, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_from_str(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::FROM_STRING, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_is_null(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::IS_NULL, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_is_undefined(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::IS_UNDEFINED, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_tst_coerc(Value *val)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::TEST_COERCIBILITY, val);
    push_instr(instr);
    return instr;
}

Value *Block::push_meta_ctx_load(uint64_t key)
{
    Instruction *instr = new (GC)MetaContextLoadInstruction(key);
    push_instr(instr);
    return instr;
}

Value *Block::push_meta_prp_load(Value *obj, Value *key)
{
    Instruction *instr = new (GC)MetaPropertyLoadInstruction(obj, key);
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_set_strict(bool strict)
{
    Instruction *instr =
        new (GC)ContextSetStrictInstruction(strict);
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_enter_catch(uint64_t key)
{
    Instruction *instr =
        new (GC)ContextEnterCatchInstruction(key);
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_enter_with(Value *val)
{
    Instruction *instr =
        new (GC)ContextEnterWithInstruction(val);
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_leave()
{
    Instruction *instr = new (GC)ContextLeaveInstruction();
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_get(uint64_t key, Value *res, uint16_t cid)
{
    Instruction *instr = new (GC)ContextGetInstruction(key, res, cid);
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_put(uint64_t key, Value *val, uint16_t cid)
{
    Instruction *instr = new (GC)ContextPutInstruction(key, val, cid);
    push_instr(instr);
    return instr;
}

Value *Block::push_ctx_del(uint64_t key, Value *res)
{
    Instruction *instr =
        new (GC)ContextDeleteInstruction(key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_ex_save_state()
{
    Instruction *instr = new (GC)ExceptionSaveStateInstruction();
    push_instr(instr);
    return instr;
}

Value *Block::push_ex_load_state(Value *state)
{
    Instruction *instr = new (GC)ExceptionLoadStateInstruction(state);
    push_instr(instr);
    return instr;
}

Value *Block::push_ex_set(Value *val)
{
    Instruction *instr = new (GC)ExceptionSetInstruction(val);
    push_instr(instr);
    return instr;
}

Value *Block::push_ex_clear()
{
    Instruction *instr = new (GC)ExceptionClearInstruction();
    push_instr(instr);
    return instr;
}

Value *Block::push_init_args(Value *dst, int prmc)
{
    Instruction *instr = new (GC)InitArgumentsInstruction(dst, prmc);
    push_instr(instr);
    return instr;
}

Value *Block::push_decl_var(uint64_t key, bool is_strict)
{
    Instruction *instr = new (GC)Declaration(key, is_strict);
    push_instr(instr);
    return instr;
}

Value *Block::push_decl_fun(uint64_t key, bool is_strict, Value *fun)
{
    Instruction *instr = new (GC)Declaration(key, is_strict, fun);
    push_instr(instr);
    return instr;
}

Value *Block::push_decl_prm(uint64_t key, bool is_strict,
                            int prm_index, Value *prm_array)
{
    Instruction *instr = new (GC)Declaration(key, is_strict, prm_index, prm_array);
    push_instr(instr);
    return instr;
}

Value *Block::push_link_var(uint64_t key, bool is_strict, Value *var)
{
    Instruction *instr = new (GC)Link(Link::VARIABLE, key, is_strict, var);
    push_instr(instr);
    return instr;
}

Value *Block::push_link_fun(uint64_t key, bool is_strict, Value *fun)
{
    Instruction *instr = new (GC)Link(Link::FUNCTION, key, is_strict, fun);
    push_instr(instr);
    return instr;
}

Value *Block::push_link_prm(uint64_t key, bool is_strict, Value *prm)
{
    Instruction *instr = new (GC)Link(Link::PARAMETER, key, is_strict, prm);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_arr(size_t length, Value *vals)
{
    Instruction *instr =
        new (GC)EsNewArrayInstruction(length, vals);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_fun(Function *fun, int param_count, bool is_strict)
{
    Instruction *instr =
        new (GC)EsNewFunctionDeclarationInstruction(fun, param_count,
                                                    is_strict);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_fun_expr(Function *fun, int param_count,
                                   bool is_strict)
{
    Instruction *instr =
        new (GC)EsNewFunctionExpressionInstruction(fun, param_count,
                                                   is_strict);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_obj()
{
    Instruction *instr = new (GC)EsNewObjectInstruction();
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_rex(const String &pattern,
                              const String &flags)
{
    Instruction *instr = new (GC)EsNewRegexInstruction(pattern, flags);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_mul(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::MUL, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_div(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::DIV, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_mod(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::MOD, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_add(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::ADD, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_sub(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::SUB, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_ls(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::LS, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_rss(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::RSS, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_rus(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::RUS, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_lt(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::LT, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_gt(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::GT, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_lte(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::LTE, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_gte(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::GTE, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_in(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::IN, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_instanceof(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::INSTANCEOF, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_eq(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::EQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_neq(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::NEQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_strict_eq(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::STRICT_EQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_strict_neq(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::STRICT_NEQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_bit_and(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::BIT_AND, op1, op2, res);
    push_instr(instr);
    return instr;
}


Value *Block::push_es_bin_bit_xor(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::BIT_XOR, op1, op2, res);
    push_instr(instr);
    return instr;
}


Value *Block::push_es_bin_bit_or(Value *op1, Value *op2, Value *res)
{
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::BIT_OR, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_typeof(Value *op1, Value *res)
{
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::TYPEOF, op1, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_neg(Value *op1, Value *res)
{
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::NEG, op1, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_bit_not(Value *op1, Value *res)
{
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::BIT_NOT, op1, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_log_not(Value *op1, Value *res)
{
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::LOG_NOT, op1, res);
    push_instr(instr);
    return instr;
}

Value::Value()
    : persistent_(false)
{
}

Instruction::Instruction()
{
}

Instruction::~Instruction()
{
}

ArgumentsObjectInitInstruction::ArgumentsObjectInitInstruction()
{
}

const Type *ArgumentsObjectInitInstruction::type() const
{
    return Type::value();
}

ArgumentsObjectLinkInstruction::ArgumentsObjectLinkInstruction(
    Value *args, int index, Value *val)
    : args_(args)
    , index_(index)
    , val_(val)
{
}

Value *ArgumentsObjectLinkInstruction::arguments() const
{
    return args_;
}

int ArgumentsObjectLinkInstruction::index() const
{
    return index_;
}

Value *ArgumentsObjectLinkInstruction::value() const
{
    return val_;
}

const Type *ArgumentsObjectLinkInstruction::type() const
{
    return Type::_void();
}

ArrayInstruction::ArrayInstruction(Operation op, size_t index, Value *arr,
                                   Value *val)
    : op_(op)
    , index_(index)
    , arr_(arr)
    , val_(val)
{
    assert(op == PUT);
}

ArrayInstruction::ArrayInstruction(Operation op, size_t index, Value *arr)
    : op_(op)
    , index_(index)
    , arr_(arr)
    , val_(NULL)
{
    assert(op == GET);
}

ArrayInstruction::Operation ArrayInstruction::operation() const
{
    return op_;
}

size_t ArrayInstruction::index() const
{
    return index_;
}

Value *ArrayInstruction::array() const
{
    return arr_;
}

Value *ArrayInstruction::value() const
{
    return val_;
}

const Type *ArrayInstruction::type() const
{
    assert(arr_->type()->is_array());
    return op_ == PUT
        ? Type::_void()
        : static_cast<const ArrayType *>(arr_->type())->type();
}

BinaryInstruction::BinaryInstruction(Operation op, Value *lval, Value *rval)
    : op_(op)
    , lval_(lval)
    , rval_(rval)
{
}

BinaryInstruction::Operation BinaryInstruction::operation() const
{
    return op_;
}

Value *BinaryInstruction::left() const
{
    return lval_;
}

Value *BinaryInstruction::right() const
{
    return rval_;
}

const Type *BinaryInstruction::type() const
{
    return op_ == EQ ? Type::boolean() : lval_->type();
}

BindExtraInitInstruction::BindExtraInitInstruction(int num_extra)
    : num_extra_(num_extra)
{
}

int BindExtraInitInstruction::num_extra() const
{
    return num_extra_;
}

const Type *BindExtraInitInstruction::type() const
{
    return new (GC)PointerType(Type::value());
}

BindExtraPtrInstruction::BindExtraPtrInstruction(int hops)
    : hops_(hops)
{
}

int BindExtraPtrInstruction::hops() const
{
    return hops_;
}

const Type *BindExtraPtrInstruction::type() const
{
    return new (GC)PointerType(Type::value());
}

CallInstruction::CallInstruction(Operation op, Value *fun, int argc,
                                 Value *res)
    : op_(op)
    , fun_(fun)
    , argc_(argc)
    , res_(res)
{
}

CallInstruction::Operation CallInstruction::operation() const
{
    return op_;
}

Value *CallInstruction::function() const
{
    return fun_;
}

int CallInstruction::argc() const
{
    return argc_;
}

Value *CallInstruction::result() const
{
    return res_;
}

const Type *CallInstruction::type() const
{
    return Type::boolean();
}

CallKeyedInstruction::CallKeyedInstruction(Value *obj, uint64_t key, int argc,
                                           Value *res)
    : obj_(obj)
    , key_(key)
    , argc_(argc)
    , res_(res)
{
}

Value *CallKeyedInstruction::object() const
{
    return obj_;
}

uint64_t CallKeyedInstruction::key() const
{
    return key_;
}

int CallKeyedInstruction::argc() const
{
    return argc_;
}

Value *CallKeyedInstruction::result() const
{
    return res_;
}

const Type *CallKeyedInstruction::type() const
{
    return Type::boolean();
}

CallKeyedSlowInstruction::CallKeyedSlowInstruction(Value *obj, Value *key,
                                                   int argc, Value *res)
    : obj_(obj)
    , key_(key)
    , argc_(argc)
    , res_(res)
{
}

Value *CallKeyedSlowInstruction::object() const
{
    return obj_;
}

Value *CallKeyedSlowInstruction::key() const
{
    return key_;
}

int CallKeyedSlowInstruction::argc() const
{
    return argc_;
}

Value *CallKeyedSlowInstruction::result() const
{
    return res_;
}

const Type *CallKeyedSlowInstruction::type() const
{
    return Type::boolean();
}

CallNamedInstruction::CallNamedInstruction(uint64_t key, int argc,
                                           Value *res)
    : key_(key)
    , argc_(argc)
    , res_(res)
{
}

uint64_t CallNamedInstruction::key() const
{
    return key_;
}

int CallNamedInstruction::argc() const
{
    return argc_;
}

Value *CallNamedInstruction::result() const
{
    return res_;
}

const Type *CallNamedInstruction::type() const
{
    return Type::boolean();
}

#ifdef UNUSED
PhiInstruction::PhiInstruction(const ArgumentVector &args)
    : args_(args)
{
    // FIXME: Verify that all items have the same type.
    // FIXME: Verify that we have at least one argument.
}

Type PhiInstruction::type() const
{
    assert(!args_.empty());
    return args_.front()->value()->type();
}
#endif

ValueInstruction::ValueInstruction(Operation op, Value *val, Value *res)
    : op_(op)
    , val_(val)
    , res_(res)
{
    assert(val->type()->is_value());
    assert(op == TO_DOUBLE || op == TO_STRING);

#ifdef DEBUG
    if (op == TO_DOUBLE) { assert(res->type()->is_double()); }
    if (op == TO_STRING) { assert(res->type()->is_string()); }
#endif
}

ValueInstruction::ValueInstruction(Operation op, Value *val)
    : op_(op)
    , val_(val)
    , res_(NULL)
{
#ifdef DEBUG
    if (op == TO_BOOLEAN) { assert(val->type()->is_value()); }
    if (op == FROM_BOOLEAN) { assert(val->type()->is_boolean()); }
    if (op == FROM_DOUBLE) { assert(val->type()->is_double()); }
    if (op == FROM_STRING) { assert(val->type()->is_string()); }
    if (op == IS_NULL) { assert(val->type()->is_value()); }
    if (op == IS_UNDEFINED) { assert(val->type()->is_value()); }
    if (op == TEST_COERCIBILITY) { assert(val->type()->is_value()); }
#endif
}

ValueInstruction::Operation ValueInstruction::operation() const
{
    return op_;
}

Value *ValueInstruction::value() const
{
    return val_;
}

Value *ValueInstruction::result() const
{
    assert(res_);
    return res_;
}

const Type *ValueInstruction::type() const
{
    switch (op_)
    {
        case TO_BOOLEAN:
        case TO_DOUBLE:
        case TO_STRING:
            return Type::boolean();
            return Type::boolean();

        case FROM_BOOLEAN:
        case FROM_DOUBLE:
        case FROM_STRING:
            return Type::value();

        case IS_NULL:
        case IS_UNDEFINED:
        case TEST_COERCIBILITY:
            return Type::boolean();
    }

    assert(false);
    return Type::_void();
}

BranchInstruction::BranchInstruction(Block *host, Value *cond,
                                     Block *true_block, Block *false_block)
    : TerminateInstruction(host)
    , cond_(cond)
    , true_block_(true_block)
    , false_block_(false_block)
{
    assert(cond_->type()->is_boolean());
}

Value *BranchInstruction::condition() const
{
    return cond_;
}

Block *BranchInstruction::true_block() const
{
    return true_block_;
}

Block *BranchInstruction::false_block() const
{
    return false_block_;
}

const Type *BranchInstruction::type() const
{
    return Type::_void();
}

JumpInstruction::JumpInstruction(Block *host, Block *block)
    : TerminateInstruction(host)
    , block_(block)
{
}

Block *JumpInstruction::block() const
{
    return block_;
}

const Type *JumpInstruction::type() const
{
    return Type::_void();
}

ReturnInstruction::ReturnInstruction(Block *host, Value *val)
    : TerminateInstruction(host)
    , val_(val)
{
}

Value *ReturnInstruction::value() const
{
    return val_;
}

const Type *ReturnInstruction::type() const
{
    return Type::_void();
}

MemoryAllocInstruction::MemoryAllocInstruction(const Type *type)
    : type_(/*new (GC)PointerType(type)*/type)
{
}

const Type *MemoryAllocInstruction::type() const
{
    return type_;
}

MemoryStoreInstruction::MemoryStoreInstruction(Value *dst, Value *src)
    : dst_(dst)
    , src_(src)
{
    //assert(dst->type()->is_pointer());
    //assert(static_cast<const PointerType *>(dst->type())->type() == src->type());
}

Value *MemoryStoreInstruction::destination() const
{
    return dst_;
}

Value *MemoryStoreInstruction::source() const
{
    return src_;
}

const Type *MemoryStoreInstruction::type() const
{
    return Type::_void();
}

MemoryElementPointerInstruction::MemoryElementPointerInstruction(Value *val, size_t index)
    : val_(val)
    , index_(index)
{
    assert(val_->type()->is_array() ||
           val_->type()->is_pointer());
}

Value *MemoryElementPointerInstruction::value() const
{
    return val_;
}

size_t MemoryElementPointerInstruction::index() const
{
    return index_;
}

const Type *MemoryElementPointerInstruction::type() const
{
    assert(val_->type()->is_array() ||
           val_->type()->is_pointer());
    return new (GC)PointerType(static_cast<const PointerType *>(val_->type())->type());
}

StackAllocInstruction::StackAllocInstruction(size_t count)
    : count_(count)
{
}

size_t StackAllocInstruction::count() const
{
    return count_;
}

const Type *StackAllocInstruction::type() const
{
    return Type::_void();
}

StackFreeInstruction::StackFreeInstruction(size_t count)
    : count_(count)
{
}

size_t StackFreeInstruction::count() const
{
    return count_;
}

const Type *StackFreeInstruction::type() const
{
    return Type::_void();
}

StackPushInstruction::StackPushInstruction(Value *val)
    : val_(val)
{
    assert(val_->type()->is_value());
}

Value *StackPushInstruction::value() const
{
    return val_;
}

const Type *StackPushInstruction::type() const
{
    return Type::_void();
}

MetaContextLoadInstruction::MetaContextLoadInstruction(uint64_t key)
    : key_(key)
{
}

uint64_t MetaContextLoadInstruction::key() const
{
    return key_;
}

const Type *MetaContextLoadInstruction::type() const
{
    return new (GC)ReferenceType(String(""));   // FIXME:
}

MetaPropertyLoadInstruction::MetaPropertyLoadInstruction(Value *obj, Value *key)
    : obj_(obj)
    , key_(key)
{
}

Value *MetaPropertyLoadInstruction::object() const
{
    return obj_;
}

Value *MetaPropertyLoadInstruction::key() const
{
    return key_;
}

const Type *MetaPropertyLoadInstruction::type() const
{
    return Type::reference();
}

ContextSetStrictInstruction::ContextSetStrictInstruction(bool strict)
    : strict_(strict)
{
}

bool ContextSetStrictInstruction::strict() const
{
    return strict_;
}

const Type *ContextSetStrictInstruction::type() const
{
    return Type::_void();
}

ContextEnterCatchInstruction::ContextEnterCatchInstruction(uint64_t key)
    : key_(key)
{
}

uint64_t ContextEnterCatchInstruction::key() const
{
    return key_;
}

const Type *ContextEnterCatchInstruction::type() const
{
    return Type::boolean();
}

ContextEnterWithInstruction::ContextEnterWithInstruction(Value *val)
    : val_(val)
{
}

Value *ContextEnterWithInstruction::value() const
{
    return val_;
}

const Type *ContextEnterWithInstruction::type() const
{
    return Type::boolean();
}

const Type *ContextLeaveInstruction::type() const
{
    return Type::_void();
}

ContextGetInstruction::ContextGetInstruction(uint64_t key, Value *res,
                                             uint16_t cid)
    : key_(key)
    , res_(res)
    , cid_(cid)
{
}

uint64_t ContextGetInstruction::key() const
{
    return key_;
}

Value *ContextGetInstruction::result() const
{
    return res_;
}

uint16_t ContextGetInstruction::cache_id() const
{
    return cid_;
}

const Type *ContextGetInstruction::type() const
{
    return Type::boolean();
}

ContextPutInstruction::ContextPutInstruction(uint64_t key, Value *val,
                                             uint16_t cid)
    : key_(key)
    , val_(val)
    , cid_(cid)
{
}

uint64_t ContextPutInstruction::key() const
{
    return key_;
}

Value *ContextPutInstruction::value() const
{
    return val_;
}

uint16_t ContextPutInstruction::cache_id() const
{
    return cid_;
}

const Type *ContextPutInstruction::type() const
{
    return Type::boolean();
}

ContextDeleteInstruction::ContextDeleteInstruction(uint64_t key, Value *res)
    : key_(key)
    , res_(res)
{
    assert(res->type()->is_value());
}

uint64_t ContextDeleteInstruction::key() const
{
    return key_;
}

Value *ContextDeleteInstruction::result() const
{
    return res_;
}

const Type *ContextDeleteInstruction::type() const
{
    return Type::boolean();
}

const Type *ExceptionSaveStateInstruction::type() const
{
    return Type::value();
}

ExceptionLoadStateInstruction::ExceptionLoadStateInstruction(Value *state)
    : state_(state)
{
    assert(state->type()->is_value());
}

Value *ExceptionLoadStateInstruction::state() const
{
    return state_;
}

const Type *ExceptionLoadStateInstruction::type() const
{
    return Type::_void();
}

ExceptionSetInstruction::ExceptionSetInstruction(Value *val)
    : val_(val)
{
    assert(val->type()->is_value() ||
           val->type()->is_reference());
}

Value *ExceptionSetInstruction::value() const
{
    return val_;
}

const Type *ExceptionSetInstruction::type() const
{
    return Type::_void();
}

ExceptionClearInstruction::ExceptionClearInstruction()
{
}

const Type *ExceptionClearInstruction::type() const
{
    return Type::_void();
}

InitArgumentsInstruction::InitArgumentsInstruction(Value *dst, int prmc)
    : dst_(dst)
    , prmc_(prmc)
{
}

Value *InitArgumentsInstruction::destination() const
{
    return dst_;
}

int InitArgumentsInstruction::parameter_count() const
{
    return prmc_;
}

const Type *InitArgumentsInstruction::type() const
{
    return Type::_void();
}

Declaration::Declaration(uint64_t key, bool is_strict)
    : kind_(VARIABLE)
    , key_(key)
    , is_strict_(is_strict)
    , val_(NULL)
    , prm_index_(-1)
    , prm_array_(NULL)
{
}

Declaration::Declaration(uint64_t key, bool is_strict, Value *val)
    : kind_(FUNCTION)
    , key_(key)
    , is_strict_(is_strict)
    , val_(val)
    , prm_index_(-1)
    , prm_array_(NULL)
{
}

Declaration::Declaration(uint64_t key, bool is_strict,
                         int prm_index, Value *prm_array)
    : kind_(PARAMETER)
    , key_(key)
    , is_strict_(is_strict)
    , val_(NULL)
    , prm_index_(prm_index)
    , prm_array_(prm_array)
{
}

Declaration::Kind Declaration::kind() const
{
    return kind_;
}

uint64_t Declaration::key() const
{
    return key_;
}

bool Declaration::is_strict() const
{
    return is_strict_;
}

Value *Declaration::value() const
{
    assert(kind_ == FUNCTION);
    return val_;
}

int Declaration::parameter_index() const
{
    assert(kind_ == PARAMETER);
    return prm_index_;
}

Value *Declaration::parameter_array() const
{
    assert(kind_ == PARAMETER);
    return prm_array_;
}

const Type *Declaration::type() const
{
    return Type::boolean();
}

Link::Link(Kind kind, uint64_t key, bool is_strict, Value *val)
    : kind_(kind)
    , key_(key)
    , is_strict_(is_strict)
    , val_(val)
{
    assert(val->type()->is_pointer());
    assert(static_cast<const PointerType *>(val->type())->type()->is_value());
}

Link::Kind Link::kind() const
{
    return kind_;
}

uint64_t Link::key() const
{
    return key_;
}

bool Link::is_strict() const
{
    return is_strict_;
}

Value *Link::value() const
{
    return val_;
}

const Type *Link::type() const
{
    return Type::_void();
}

PropertyDefineDataInstruction::PropertyDefineDataInstruction(Value *obj, Value *key, Value *val)
    : obj_(obj)
    , key_(key)
    , val_(val)
{
}

Value *PropertyDefineDataInstruction::object() const
{
    return obj_;
}

Value *PropertyDefineDataInstruction::key() const
{
    return key_;
}

Value *PropertyDefineDataInstruction::value() const
{
    return val_;
}

const Type *PropertyDefineDataInstruction::type() const
{
    return Type::boolean();
}

PropertyDefineAccessorInstruction::PropertyDefineAccessorInstruction(Value *obj, uint64_t key,
                                                                     Value *fun, bool is_setter)
    : obj_(obj)
    , key_(key)
    , fun_(fun)
    , is_setter_(is_setter)
{
}

Value *PropertyDefineAccessorInstruction::object() const
{
    return obj_;
}

uint64_t PropertyDefineAccessorInstruction::key() const
{
    return key_;
}

Value *PropertyDefineAccessorInstruction::function() const
{
    return fun_;
}

bool PropertyDefineAccessorInstruction::is_setter() const
{
    return is_setter_;
}

const Type *PropertyDefineAccessorInstruction::type() const
{
    return Type::boolean();
}

PropertyIteratorNewInstruction::PropertyIteratorNewInstruction(Value *obj)
    : obj_(obj)
{
}

Value *PropertyIteratorNewInstruction::object() const
{
    return obj_;
}

const Type *PropertyIteratorNewInstruction::type() const
{
    return new (GC)OpaqueType("EsPropertyIterator");
}

PropertyIteratorNextInstruction::PropertyIteratorNextInstruction(Value *it,
                                                                 Value *val)
    : it_(it)
    , val_(val)
{
}

Value *PropertyIteratorNextInstruction::iterator() const
{
    return it_;
}

Value *PropertyIteratorNextInstruction::value() const
{
    return val_;
}

const Type *PropertyIteratorNextInstruction::type() const
{
    return Type::boolean();
}

PropertyGetInstruction::PropertyGetInstruction(Value *obj, uint64_t key, Value *res)
    : obj_(obj)
    , key_(key)
    , res_(res)
{
}

Value *PropertyGetInstruction::object() const
{
    return obj_;
}

uint64_t PropertyGetInstruction::key() const
{
    return key_;
}

Value *PropertyGetInstruction::result() const
{
    return res_;
}

const Type *PropertyGetInstruction::type() const
{
    return Type::boolean();
}

PropertyGetSlowInstruction::PropertyGetSlowInstruction(Value *obj, Value *key, Value *res)
    : obj_(obj)
    , key_(key)
    , res_(res)
{
}

Value *PropertyGetSlowInstruction::object() const
{
    return obj_;
}

Value *PropertyGetSlowInstruction::key() const
{
    return key_;
}

Value *PropertyGetSlowInstruction::result() const
{
    return res_;
}

const Type *PropertyGetSlowInstruction::type() const
{
    return Type::boolean();
}

PropertyPutInstruction::PropertyPutInstruction(Value *obj, uint64_t key, Value *val)
    : obj_(obj)
    , key_(key)
    , val_(val)
{
}

Value *PropertyPutInstruction::object() const
{
    return obj_;
}

uint64_t PropertyPutInstruction::key() const
{
    return key_;
}

Value *PropertyPutInstruction::value() const
{
    return val_;
}

const Type *PropertyPutInstruction::type() const
{
    return Type::boolean();
}

PropertyPutSlowInstruction::PropertyPutSlowInstruction(Value *obj, Value *key, Value *val)
    : obj_(obj)
    , key_(key)
    , val_(val)
{
}

Value *PropertyPutSlowInstruction::object() const
{
    return obj_;
}

Value *PropertyPutSlowInstruction::key() const
{
    return key_;
}

Value *PropertyPutSlowInstruction::value() const
{
    return val_;
}

const Type *PropertyPutSlowInstruction::type() const
{
    return Type::boolean();
}

PropertyDeleteInstruction::PropertyDeleteInstruction(Value *obj, uint64_t key, Value *res)
    : obj_(obj)
    , key_(key)
    , res_(res)
{
#ifdef DEBUG
    if (obj) { assert(obj->type()->is_value()); }
#endif
    assert(res->type()->is_value());
}

Value *PropertyDeleteInstruction::object() const
{
    return obj_;
}

uint64_t PropertyDeleteInstruction::key() const
{
    return key_;
}

Value *PropertyDeleteInstruction::result() const
{
    return res_;
}

const Type *PropertyDeleteInstruction::type() const
{
    return Type::boolean();
}

PropertyDeleteSlowInstruction::PropertyDeleteSlowInstruction(Value *obj, Value *key, Value *res)
    : obj_(obj)
    , key_(key)
    , res_(res)
{
#ifdef DEBUG
    if (obj) { assert(obj->type()->is_value()); }
#endif
    assert(key->type()->is_value());
    assert(res->type()->is_value());
}

Value *PropertyDeleteSlowInstruction::object() const
{
    return obj_;
}

Value *PropertyDeleteSlowInstruction::key() const
{
    return key_;
}

Value *PropertyDeleteSlowInstruction::result() const
{
    return res_;
}

const Type *PropertyDeleteSlowInstruction::type() const
{
    return Type::boolean();
}

EsNewArrayInstruction::EsNewArrayInstruction(size_t length, Value *vals)
    : length_(length)
    , vals_(vals)
{
}

size_t EsNewArrayInstruction::length() const
{
    return length_;
}

Value *EsNewArrayInstruction::values() const
{
    return vals_;
}

const Type *EsNewArrayInstruction::type() const
{
    return Type::value();
}

EsNewFunctionDeclarationInstruction::EsNewFunctionDeclarationInstruction(
    Function *fun, int param_count, bool is_strict)
    : fun_(fun)
    , param_count_(param_count)
    , is_strict_(is_strict)
{
}

Function *EsNewFunctionDeclarationInstruction::function() const
{
    return fun_;
}

int EsNewFunctionDeclarationInstruction::parameter_count() const
{
    return param_count_;
}

bool EsNewFunctionDeclarationInstruction::is_strict() const
{
    return is_strict_;
}

const Type *EsNewFunctionDeclarationInstruction::type() const
{
    return Type::value();
}

EsNewFunctionExpressionInstruction::EsNewFunctionExpressionInstruction(
    Function *fun, int param_count, bool is_strict)
    : fun_(fun)
    , param_count_(param_count)
    , is_strict_(is_strict)
{
}

Function *EsNewFunctionExpressionInstruction::function() const
{
    return fun_;
}

int EsNewFunctionExpressionInstruction::parameter_count() const
{
    return param_count_;
}

bool EsNewFunctionExpressionInstruction::is_strict() const
{
    return is_strict_;
}

const Type *EsNewFunctionExpressionInstruction::type() const
{
    return Type::value();
}

const Type *EsNewObjectInstruction::type() const
{
    return Type::value();
}

EsNewRegexInstruction::EsNewRegexInstruction(const String &pattern,
                                             const String &flags)
    : pattern_(pattern)
    , flags_(flags)
{
}

const String &EsNewRegexInstruction::pattern() const
{
    return pattern_;
}

const String &EsNewRegexInstruction::flags() const
{
    return flags_;
}

const Type *EsNewRegexInstruction::type() const
{
    return Type::value();
}

EsBinaryInstruction::EsBinaryInstruction(Operation op, Value *lval, Value *rval, Value *res)
    : op_(op)
    , lval_(lval)
    , rval_(rval)
    , res_(res)
{
}

EsBinaryInstruction::Operation EsBinaryInstruction::operation() const
{
    return op_;
}

Value *EsBinaryInstruction::left() const
{
    return lval_;
}

Value *EsBinaryInstruction::right() const
{
    return rval_;
}

Value *EsBinaryInstruction::result() const
{
    return res_;
}

const Type *EsBinaryInstruction::type() const
{
    return Type::boolean();
}

EsUnaryInstruction::EsUnaryInstruction(Operation op, Value *val, Value *res)
    : op_(op)
    , val_(val)
    , res_(res)
{
}

EsUnaryInstruction::Operation EsUnaryInstruction::operation() const
{
    return op_;
}

Value *EsUnaryInstruction::value() const
{
    return val_;
}

Value *EsUnaryInstruction::result() const
{
    return res_;
}

const Type *EsUnaryInstruction::type() const
{
    return Type::boolean();
}

}
