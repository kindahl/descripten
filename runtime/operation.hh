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

#pragma once
#include "api.hh"

class EsContext;
class EsFunction;
class EsPropertyIterator;
class EsValue;

void data_reg_str(const String &str, uint32_t id);

void op_init_args(EsValue dst[], int argc, const EsValue argv[], int prmc);

/**
 * Constructs and initializes the arguments object for the specified execution
 * context. The arguments object will be initialized with all arguments in
 * @p argv. However, no parameter will be linked to any argument.
 *
 * After the arguments object has been created arguments can be linked to
 * parameters using op_args_obj_link().
 *
 * @param [in] ctx Context to which the arguments object should be linked.
 * @param [in] callee Callee object to expose through <em>arguments.callee</em>
 *                    for non-strict contexts.
 * @param [in] argc Number of arguments.
 * @param [in] argv Argument vector.
 * @return Pointer to arguments object wrapped in a value.
 *
 * @see op_args_obj_link()
 */
EsValue op_args_obj_init(EsContext *ctx, EsFunction *callee,
                         int argc, const EsValue argv[]);
void op_args_obj_link(EsValue &args, int i, EsValue *val);

/**
 * Initalizes extra bindings. Extra bindings are bindings that are accessed
 * outside the local function scope. For example the variable 'x' in the
 * following code:
 *
 * function foo() {
 *   var x = 42;
 *   function bar() {
 *     print(x);
 *   }
 * }
 *
 * NOTE: A function scope may contain a combination of regular bindings and
 *       extra bindings.
 *
 * @param [in] ctx Current context.
 * @param [in] num_extra Number of extra bindings.
 * @return Pointer to extra bindings in memory. Is guaranteed to have enough
 *         memory for @a num_extra bindings.
 */
EsValue *op_bnd_extra_init(EsContext *ctx, int num_extra);
EsValue *op_bnd_extra_ptr(EsFunction *fun, int hops);

// Context related functions.
bool op_ctx_decl_fun(EsContext *ctx, bool is_eval, bool is_strict,
                     uint64_t fn, const EsValue &fo);
bool op_ctx_decl_var(EsContext *ctx, bool is_eval, bool is_strict,
                     uint64_t vn);
bool op_ctx_decl_prm(EsContext *ctx, bool is_strict, uint64_t pn,
                     const EsValue &po);
void op_ctx_link_fun(EsContext *ctx, uint64_t fn, EsValue *fo); // May not be called from eval context.
void op_ctx_link_var(EsContext *ctx, uint64_t vn, EsValue *vo); // May not be called from eval context.
void op_ctx_link_prm(EsContext *ctx, uint64_t vn, EsValue *po); // May not be called from eval context.
bool op_ctx_get(EsContext *ctx, uint64_t raw_key, EsValue &result,
                uint16_t cid);
bool op_ctx_put(EsContext *ctx, uint64_t raw_key, const EsValue &val,
                uint16_t cid);
bool op_ctx_del(EsContext *ctx, uint64_t raw_key, EsValue &result);
EsValue op_ctx_this(EsContext *ctx);
void op_ctx_set_strict(EsContext *ctx, bool strict);
bool op_ctx_enter_with(EsContext *ctx, const EsValue &val);
bool op_ctx_enter_catch(EsContext *ctx, uint64_t raw_key);
void op_ctx_leave();
EsContext *op_ctx_running();

EsValue op_ex_save_state(EsContext *ctx);
void op_ex_load_state(EsContext *ctx, const EsValue &state);
void op_ex_set(EsContext *ctx, const EsValue &exception);
void op_ex_clear(EsContext *ctx);

// Object related functions.
EsPropertyIterator *op_prp_it_new(const EsValue &val);
bool op_prp_it_next(EsPropertyIterator *it, EsValue &val);
bool op_prp_def_data(EsValue &obj_val, const EsValue &key,
                     const EsValue &val);
bool op_prp_def_accessor(EsValue &obj_val, uint64_t raw_key,
                         const EsValue &fun, bool is_setter);
bool op_prp_get(const EsValue &obj_val, const EsValue &key_val,
                EsValue &result, uint16_t cid);
bool op_prp_get(const EsValue &obj_val, uint64_t raw_key,
                EsValue &result, uint16_t cid);
bool op_prp_put(EsContext *ctx, const EsValue &obj_val, const EsValue &key_val,
                const EsValue &val, uint16_t cid);
bool op_prp_put(EsContext *ctx, const EsValue &obj_val, uint64_t raw_key,
                const EsValue &val, uint16_t cid);
bool op_prp_del(EsContext *ctx, EsValue &obj_val, const EsValue &key_val,
                EsValue &result);
bool op_prp_del(EsContext *ctx, EsValue &obj_val, uint64_t raw_key,
                EsValue &result);

bool op_call(const EsValue &fun, int argc, EsValue argv[], EsValue &result);
bool op_call_keyed(const EsValue &obj_val, const EsValue &key_val, int argc,
                   EsValue argv[], EsValue &result);
bool op_call_keyed(const EsValue &obj_val, uint64_t raw_key, int argc,
                   EsValue argv[], EsValue &result);
bool op_call_named(uint64_t raw_key, int argc,
                   EsValue argv[], EsValue &result);
bool op_call_new(const EsValue &fun, int argc, EsValue argv[],
                 EsValue &result);

// Literal functions.
EsValue op_new_obj();
EsValue op_new_arr(int count, EsValue items[]);
EsValue op_new_reg_exp(const String &pattern, const String &flags);
EsValue op_new_fun_decl(EsContext *ctx, ES_API_FUN_PTR(fun),
                        bool strict, int prmc);
EsValue op_new_fun_expr(EsContext *ctx, ES_API_FUN_PTR(fun),
                        bool strict, int prmc);

// Unary functions.
bool op_u_typeof(const EsValue &val, EsValue &result);
bool op_u_not(const EsValue &expr, EsValue &result);
bool op_u_bit_not(const EsValue &expr, EsValue &result);
bool op_u_add(const EsValue &expr, EsValue &result);
bool op_u_sub(const EsValue &expr, EsValue &result);

// Binary functions.
bool op_b_or(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_xor(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_and(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_shl(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_sar(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_shr(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_add(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_sub(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_mul(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_div(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_b_mod(const EsValue &lval, const EsValue &rval, EsValue &result);

// Comparative functions.
bool op_c_in(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_instance_of(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_strict_eq(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_strict_neq(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_eq(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_neq(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_lt(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_gt(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_lte(const EsValue &lval, const EsValue &rval, EsValue &result);
bool op_c_gte(const EsValue &lval, const EsValue &rval, EsValue &result);
