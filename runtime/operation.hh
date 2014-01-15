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
#include <stdint.h>
#include "value_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESA_BOOL            uint8_t
#define ESA_TRUE            1
#define ESA_FALSE           0

#define ESA_FUN_PTR(name)   bool (* name)(EsContext *ctx, uint32_t argc,\
                                          EsValueData *fp, EsValueData *vp)

struct EsContext;
struct EsPropertyIterator;
struct EsString;

void esa_str_intern(const EsString *str, uint32_t id);

bool esa_val_to_bool(EsValueData val_data);
bool esa_val_to_num(EsValueData val_data, double *num);
const EsString *esa_val_to_str(EsValueData val_data);
EsObject *esa_val_to_obj(EsValueData val_data);
bool esa_val_chk_coerc(EsValueData val_data);

// FIXME: Rename to esa_frm_alloc instead? Since we're allocating in the call frame.
void esa_stk_alloc(uint32_t count);
void esa_stk_free(uint32_t count);
void esa_stk_push(EsValueData val_data);

void esa_init_args(EsValueData dst_data[], uint32_t argc,
                   const EsValueData argv_data[], uint32_t prmc);

/**
 * Constructs and initializes the arguments object for the specified execution
 * context. The arguments object will be initialized with all arguments in
 * @p argv. However, no parameter will be linked to any argument.
 *
 * After the arguments object has been created arguments can be linked to
 * parameters using esa_args_obj_link().
 *
 * @param [in] ctx Context to which the arguments object should be linked.
 * @param [in] argc Number of arguments.
 * @param [in] fp Frame pointer.
 * @param [in] vp Value pointer.
 * @return Pointer to arguments object wrapped in a value.
 *
 * @see esa_args_obj_link()
 */
EsValueData esa_args_obj_init(EsContext *ctx, uint32_t argc,
                              EsValueData *fp_data, EsValueData *vp_data);
void esa_args_obj_link(EsValueData args_data, uint32_t i,
                       EsValueData *val_data);

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
 * All values allocated in extra bindings memory will be default initialized to
 * undefined.
 *
 * NOTE: A function scope may contain a combination of regular bindings and
 *       extra bindings.
 *
 * @param [in] ctx Current context.
 * @param [in] num_extra Number of extra bindings.
 * @return Pointer to extra bindings in memory. Is guaranteed to have enough
 *         memory for @a num_extra bindings.
 */
EsValueData *esa_bnd_extra_init(EsContext *ctx, uint32_t num_extra);
EsValueData *esa_bnd_extra_ptr(uint32_t argc, EsValueData *fp_data,
                               EsValueData *vp_data, uint32_t hops);

// Context related functions.
bool esa_ctx_decl_fun(EsContext *ctx, bool is_eval, bool is_strict,
                      uint64_t fn, EsValueData fo_data);
bool esa_ctx_decl_var(EsContext *ctx, bool is_eval, bool is_strict,
                      uint64_t vn);
bool esa_ctx_decl_prm(EsContext *ctx, bool is_strict, uint64_t pn,
                      EsValueData po_data);
void esa_ctx_link_fun(EsContext *ctx, uint64_t fn, EsValueData *fo_data);    // May not be called from eval context.
void esa_ctx_link_var(EsContext *ctx, uint64_t vn, EsValueData *vo_data);    // May not be called from eval context.
void esa_ctx_link_prm(EsContext *ctx, uint64_t vn, EsValueData *po_data);    // May not be called from eval context.
bool esa_ctx_get(EsContext *ctx, uint64_t raw_key, EsValueData *result_data,
                 uint16_t cid);
bool esa_ctx_put(EsContext *ctx, uint64_t raw_key, EsValueData val_data,
                 uint16_t cid);
bool esa_ctx_del(EsContext *ctx, uint64_t raw_key, EsValueData *result_data);
void esa_ctx_set_strict(EsContext *ctx, bool strict);
bool esa_ctx_enter_with(EsContext *ctx, EsValueData val_data);
bool esa_ctx_enter_catch(EsContext *ctx, uint64_t raw_key);
void esa_ctx_leave();
EsContext *esa_ctx_running();

EsValueData esa_ex_save_state(EsContext *ctx);
void esa_ex_load_state(EsContext *ctx, EsValueData state_data);
EsValueData esa_ex_get(EsContext *ctx);
void esa_ex_set(EsContext *ctx, EsValueData exception_data);
void esa_ex_clear(EsContext *ctx);

// Object related functions.
EsPropertyIterator *esa_prp_it_new(EsValueData val_data);
bool esa_prp_it_next(EsPropertyIterator *it, EsValueData *result_data);
bool esa_prp_def_data(EsValueData obj_data, EsValueData key_data,
                      EsValueData val_data);
bool esa_prp_def_accessor(EsValueData obj_data, uint64_t raw_key,
                          EsValueData fun_data, bool is_setter);
bool esa_prp_get_slow(EsValueData src_data, EsValueData key_data,
                      EsValueData *result_data, uint16_t cid);
bool esa_prp_get(EsValueData src_data, uint64_t raw_key,
                 EsValueData *result_data, uint16_t cid);
bool esa_prp_put_slow(EsContext *ctx, EsValueData dst_data,
                      EsValueData key_data, EsValueData val_data,
                      uint16_t cid);
bool esa_prp_put(EsContext *ctx, EsValueData dst_data, uint64_t raw_key,
                 EsValueData val_data, uint16_t cid);
bool esa_prp_del_slow(EsContext *ctx, EsValueData src_data,
                      EsValueData key_data, EsValueData *result_data);
bool esa_prp_del(EsContext *ctx, EsValueData src_data, uint64_t raw_key,
                 EsValueData *result_data);

bool esa_call(EsValueData fun_data, uint32_t argc, EsValueData *result_data);
bool esa_call_keyed_slow(EsValueData src_data, EsValueData key_data,
                         uint32_t argc, EsValueData *result_data);
bool esa_call_keyed(EsValueData src_data, uint64_t raw_key, uint32_t argc,
                    EsValueData *result_data);
bool esa_call_named(uint64_t raw_key, uint32_t argc, EsValueData *result_data);
bool esa_call_new(EsValueData fun_data, uint32_t argc,
                  EsValueData *result_data);

// Literal functions.
const EsString *esa_new_str(const void *str, uint32_t len);
EsValueData esa_new_arr(uint32_t count, EsValueData items_data[]);
EsValueData esa_new_obj();
EsValueData esa_new_reg_exp(const EsString *pattern, const EsString *flags);
EsValueData esa_new_fun_decl(EsContext *ctx, ESA_FUN_PTR(fun),
                             bool strict, uint32_t prmc);
EsValueData esa_new_fun_expr(EsContext *ctx, ESA_FUN_PTR(fun),
                             bool strict, uint32_t prmc);

// Unary functions.
bool esa_u_typeof(EsValueData val_data, EsValueData *result_data);
bool esa_u_not(EsValueData expr_data, EsValueData *result_data);
bool esa_u_bit_not(EsValueData expr_data, EsValueData *result_data);
bool esa_u_add(EsValueData expr_data, EsValueData *result_data);
bool esa_u_sub(EsValueData expr_data, EsValueData *result_data);

// Binary functions.
bool esa_b_or(EsValueData lval_data, EsValueData rval_data,
              EsValueData *result_data);
bool esa_b_xor(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_and(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_shl(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_sar(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_shr(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_add(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_sub(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_mul(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_div(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_b_mod(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);

// Comparative functions.
bool esa_c_in(EsValueData lval_data, EsValueData rval_data,
              EsValueData *result_data);
bool esa_c_instance_of(EsValueData lval_data, EsValueData rval_data,
                       EsValueData *result_data);
bool esa_c_strict_eq(EsValueData lval_data, EsValueData rval_data,
                     EsValueData *result_data);
bool esa_c_strict_neq(EsValueData lval_data, EsValueData rval_data,
                      EsValueData *result_data);
bool esa_c_eq(EsValueData lval_data, EsValueData rval_data,
              EsValueData *result_data);
bool esa_c_neq(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_c_lt(EsValueData lval_data, EsValueData rval_data,
              EsValueData *result_data);
bool esa_c_lte(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);
bool esa_c_gt(EsValueData lval_data, EsValueData rval_data,
              EsValueData *result_data);
bool esa_c_gte(EsValueData lval_data, EsValueData rval_data,
               EsValueData *result_data);

#ifdef __cplusplus
}
#endif
