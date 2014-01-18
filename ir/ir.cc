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

void Function::push_block(Block *block)
{
#ifdef DEBUG
    // We do not allow empty blocks, but we cannot check that the new @a block
    // is not empty becase the compiler works by adding empty blocks and then
    // populating them. Though we cannot check @a block, we can make sure that
    // we're not leaving any empty blocks by checking the preivously last
    // block.
    if (!blocks_.empty())
    {
        assert(!last_block()->empty());
        assert(last_block()->last_instr()->is_terminating());
    }
#endif
    blocks_.push_back(block);
}

void Block::add_referrer(Instruction *instr)
{
    referrers_.insert(instr);
}

void Block::remove_referrer(Instruction *instr)
{
    referrers_.erase(instr);
}

Block *Function::last_block() const
{
    assert(!blocks_.empty());
    return &blocks_.back();
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

Value *Block::push_args_obj_link(Value *args, uint32_t index, Value *val)
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

Value *Block::push_bnd_extra_init(uint32_t num_extra)
{
    Instruction *instr =
        new (GC)BindExtraInitInstruction(num_extra);
    push_instr(instr);
    return instr;
}

Value *Block::push_bnd_extra_ptr(uint32_t hops)
{
    Instruction *instr =
        new (GC)BindExtraPtrInstruction(hops);
    push_instr(instr);
    return instr;
}

Value *Block::push_call(Value *fun, uint32_t argc, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)CallInstruction(CallInstruction::NORMAL, fun, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_keyed(Value *obj, uint64_t key, uint32_t argc, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)CallKeyedInstruction(obj, key, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_keyed_slow(Value *obj, Value *key, uint32_t argc,
                                   Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)CallKeyedSlowInstruction(obj, key, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_named(uint64_t key, uint32_t argc, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)CallNamedInstruction(key, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_call_new(Value *fun, uint32_t argc, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)CallInstruction(CallInstruction::NEW, fun, argc, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_mem_store(Value *dst, Value *src)
{
    assert(dst);

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

Value *Block::push_stk_alloc(const Proxy<size_t> &count)
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
    assert(res);
    Instruction *instr = new (GC)PropertyGetInstruction(obj, key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_get_slow(Value *obj, Value *key, Value *res)
{
    assert(res);
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
    assert(res);
    Instruction *instr =
        new (GC)PropertyDeleteInstruction(obj, key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_prp_del_slow(Value *obj, Value *key, Value *res)
{
    assert(res);
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
    assert(res);
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::TO_DOUBLE, val, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_from_bool(Value *val, Value *res)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::FROM_BOOLEAN, val, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_from_double(Value *val, Value *res)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::FROM_DOUBLE, val, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_val_from_str(Value *val, Value *res)
{
    Instruction *instr =
        new (GC)ValueInstruction(ValueInstruction::FROM_STRING, val, res);
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
    assert(res);
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
    assert(res);
    Instruction *instr =
        new (GC)ContextDeleteInstruction(key, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_ex_save_state(Value *res)
{
    Instruction *instr = new (GC)ExceptionSaveStateInstruction(res);
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

Value *Block::push_init_args(Value *dst, uint32_t prmc)
{
    assert(dst);
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
                            size_t prm_index, Value *prm_array)
{
    Instruction *instr = new (GC)Declaration(key, is_strict, prm_index,
                                             prm_array);
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

Value *Block::push_es_new_arr(size_t length, Value *vals, Value *res)
{
    Instruction *instr =
        new (GC)EsNewArrayInstruction(length, vals, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_fun(Function *fun, uint32_t param_count,
                              bool is_strict, Value *res)
{
    Instruction *instr =
        new (GC)EsNewFunctionDeclarationInstruction(fun, param_count,
                                                    is_strict, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_fun_expr(Function *fun, uint32_t param_count,
                                   bool is_strict, Value *res)
{
    Instruction *instr =
        new (GC)EsNewFunctionExpressionInstruction(fun, param_count,
                                                   is_strict, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_obj(Value *res)
{
    Instruction *instr = new (GC)EsNewObjectInstruction(res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_new_rex(const String &pattern,
                              const String &flags,
                              Value *res)
{
    Instruction *instr = new (GC)EsNewRegexInstruction(pattern, flags, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_mul(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::MUL, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_div(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::DIV, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_mod(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::MOD, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_add(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::ADD, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_sub(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::SUB, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_ls(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::LS, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_rss(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::RSS, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_rus(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::RUS, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_lt(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::LT, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_gt(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::GT, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_lte(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::LTE, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_gte(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::GTE, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_in(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::IN, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_instanceof(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::INSTANCEOF, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_eq(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::EQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_neq(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::NEQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_strict_eq(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::STRICT_EQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_strict_neq(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::STRICT_NEQ, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_bin_bit_and(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::BIT_AND, op1, op2, res);
    push_instr(instr);
    return instr;
}


Value *Block::push_es_bin_bit_xor(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::BIT_XOR, op1, op2, res);
    push_instr(instr);
    return instr;
}


Value *Block::push_es_bin_bit_or(Value *op1, Value *op2, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsBinaryInstruction(EsBinaryInstruction::BIT_OR, op1, op2, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_typeof(Value *op1, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::TYPEOF, op1, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_neg(Value *op1, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::NEG, op1, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_bit_not(Value *op1, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::BIT_NOT, op1, res);
    push_instr(instr);
    return instr;
}

Value *Block::push_es_unary_log_not(Value *op1, Value *res)
{
    assert(res);
    Instruction *instr =
        new (GC)EsUnaryInstruction(EsUnaryInstruction::LOG_NOT, op1, res);
    push_instr(instr);
    return instr;
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

Value *ArrayInstruction::value() const
{
    assert(op_ == PUT);
    return val_;
}

const Type *ArrayInstruction::type() const
{
    assert(arr_->type()->is_array() ||
           arr_->type()->is_pointer());

    if (op_ == PUT)
        return Type::_void();

    return arr_->type()->is_array()
        ? static_cast<const ArrayType *>(arr_->type())->type()
        : static_cast<const PointerType *>(arr_->type())->type();
}

const Type *BinaryInstruction::type() const
{
    return op_ == EQ ? Type::boolean() : lval_->type();
}

const Type *BindExtraInitInstruction::type() const
{
    return new (GC)PointerType(Type::value());
}

const Type *BindExtraPtrInstruction::type() const
{
    return new (GC)PointerType(Type::value());
}

ValueInstruction::ValueInstruction(Operation op, Value *val, Value *res)
    : op_(op)
    , val_(val)
    , res_(res)
{
    assert(op == FROM_BOOLEAN || op == FROM_DOUBLE || op == FROM_STRING ||
           op == TO_DOUBLE);

#ifdef DEBUG
    if (op == FROM_BOOLEAN) { assert(val->type()->is_boolean()); }
    if (op == FROM_DOUBLE) { assert(val->type()->is_double()); }
    if (op == FROM_STRING) { assert(val->type()->is_string()); }
    if (op == TO_DOUBLE)
    {
        assert(res->type()->is_double());
        assert(val->type()->is_value());
    }
#endif
}

ValueInstruction::ValueInstruction(Operation op, Value *val)
    : op_(op)
    , val_(val)
    , res_(NULL)
{
#ifdef DEBUG
    if (op == TO_BOOLEAN) { assert(val->type()->is_value()); }
    if (op == IS_NULL) { assert(val->type()->is_value()); }
    if (op == IS_UNDEFINED) { assert(val->type()->is_value()); }
    if (op == TEST_COERCIBILITY) { assert(val->type()->is_value()); }
#endif
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
            return Type::boolean();
            return Type::boolean();

        case FROM_BOOLEAN:
        case FROM_DOUBLE:
        case FROM_STRING:
            return Type::_void();

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

MemoryElementPointerInstruction::MemoryElementPointerInstruction(Value *val, size_t index)
    : val_(val)
    , index_(index)
{
    assert(val_->type()->is_array() ||
           val_->type()->is_pointer());
}

const Type *MemoryElementPointerInstruction::type() const
{
    assert(val_->type()->is_array() ||
           val_->type()->is_pointer());
    return new (GC)PointerType(static_cast<const PointerType *>(val_->type())->type());
}

StackPushInstruction::StackPushInstruction(Value *val)
    : val_(val)
{
    assert(val_->type()->is_value());
}

ContextDeleteInstruction::ContextDeleteInstruction(uint64_t key, Value *res)
    : key_(key)
    , res_(res)
{
    assert(res->type()->is_value());
}

ExceptionLoadStateInstruction::ExceptionLoadStateInstruction(Value *state)
    : state_(state)
{
    assert(state->type()->is_value());
}

ExceptionSetInstruction::ExceptionSetInstruction(Value *val)
    : val_(val)
{
    assert(val->type()->is_value());
}

Value *Declaration::value() const
{
    assert(kind_ == FUNCTION);
    return val_;
}

size_t Declaration::parameter_index() const
{
    assert(kind_ == PARAMETER);
    return prm_index_;
}

Value *Declaration::parameter_array() const
{
    assert(kind_ == PARAMETER);
    return prm_array_;
}

const Type *PropertyIteratorNewInstruction::type() const
{
    return new (GC)OpaqueType("EsPropertyIterator");
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

}
