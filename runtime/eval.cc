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

#include <string.h>
#include <gc_cpp.h>
#include "common/cast.hh"
#include "context.hh"
#include "conversion.hh"
#include "error.hh"
#include "eval.hh"
#include "frame.hh"
#include "operation.hh"
#include "property.hh"
#include "utility.hh"

using parser::Declaration;
using parser::ExpressionVector;
using parser::StatementVector;

// NOTE: Copied from header to satisfy Eclipse CDT.
using parser::ArrayLiteral;
using parser::AssignmentExpression;
using parser::BinaryExpression;
using parser::BlockStatement;
using parser::BoolLiteral;
using parser::BreakStatement;
using parser::CallExpression;
using parser::CallNewExpression;
using parser::ConditionalExpression;
using parser::ContinueStatement;
using parser::DebuggerStatement;
using parser::DeclarationVector;
using parser::DoWhileStatement;
using parser::EmptyStatement;
using parser::ExpressionStatement;
using parser::ForInStatement;
using parser::ForStatement;
using parser::FunctionExpression;
using parser::FunctionLiteral;
using parser::IdentifierLiteral;
using parser::IfStatement;
using parser::LabeledStatement;
using parser::LabeledStatementVector;
using parser::ObjectLiteral;
using parser::NothingLiteral;
using parser::NullLiteral;
using parser::NumberLiteral;
using parser::PropertyExpression;
using parser::RegularExpression;
using parser::ReturnStatement;
using parser::StringLiteral;
using parser::SwitchStatement;
using parser::ThisLiteral;
using parser::ThrowStatement;
using parser::TryStatement;
using parser::UnaryExpression;
using parser::VariableLiteral;
using parser::WhileStatement;
using parser::WithStatement;

using parser::ValueVisitor;

Evaluator::Evaluator(FunctionLiteral *code, Type type, EsCallFrame &frame)
    : code_(code)
    , type_(type)
    , frame_(frame)
{
    assert(code_);
}

bool Evaluator::is_in_iteration() const
{
    std::vector<Scope>::const_reverse_iterator it;
    for (it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        if (*it == SCOPE_ITERATION)
            return true;
    }

    return false;
}

bool Evaluator::is_in_switch() const
{
    std::vector<Scope>::const_reverse_iterator it;
    for (it = scopes_.rbegin(); it != scopes_.rend(); ++it)
    {
        if (*it == SCOPE_SWITCH)
            return true;
    }

    return false;
}

bool Evaluator::expand_ref_get(const EsReferenceOrValue &any, EsValue &value)
{
    if (any.is_value())
    {
        value = any.value();
        return true;
    }

    const EsReference &ref = any.reference();
    if (ref.get_base())
    {
        return op_prp_get(EsValue::from_obj(ref.get_base()),
                          EsPropertyKey::from_str(ref.get_referenced_name()).as_raw(),
                          value, 0);
    }
    else
    {
        return op_ctx_get(EsContextStack::instance().top(),
                          EsPropertyKey::from_str(ref.get_referenced_name()).as_raw(), value, 0);
    }
}

bool Evaluator::expand_ref_put(const EsReferenceOrValue &any, const EsValue &value)
{
    assert(any.is_reference());

    const EsReference &ref = any.reference();
    if (ref.get_base())
    {
        // If we have a base object it must be a property reference.
        return op_prp_put(EsContextStack::instance().top(),
                          EsValue::from_obj(ref.get_base()),
                          EsPropertyKey::from_str(ref.get_referenced_name()).as_raw(),
                          value, 0);
    }
    else
    {
        return op_ctx_put(EsContextStack::instance().top(),
                          EsPropertyKey::from_str(ref.get_referenced_name()).as_raw(), value, 0);
    }
}

Completion Evaluator::parse_binary_expr(BinaryExpression *expr)
{
    EsValue r = EsValue::undefined;

    Completion lhs_res = parse(expr->left());
    if (lhs_res.is_abrupt())
        return lhs_res;

    EsValue lval;
    if (!expand_ref_get(lhs_res.value(), lval))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    switch (expr->operation())
    {
        case BinaryExpression::COMMA:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            r = rval;
            break;
        }

        // Arithmetic.
        case BinaryExpression::MUL:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_mul(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::DIV:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_div(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::MOD:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_mod(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::ADD:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_add(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::SUB:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_sub(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::LS:  // <<
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_shl(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::RSS: // >>
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_sar(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::RUS: // >>>
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_shr(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }

        // Relational.
        case BinaryExpression::LT:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_lt(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::GT:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_gt(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::LTE:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_lte(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::GTE:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_gte(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::IN:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_in(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::INSTANCEOF:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_instance_of(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }

        // Equality.
        case BinaryExpression::EQ:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_eq(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::NEQ:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_neq(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::STRICT_EQ:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_strict_eq(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::STRICT_NEQ:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_c_strict_neq(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }

        // Bitwise.
        case BinaryExpression::BIT_AND:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_and(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::BIT_XOR:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_xor(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case BinaryExpression::BIT_OR:
        {
            Completion rhs_res = parse(expr->right());
            if (rhs_res.is_abrupt())
                return rhs_res;

            EsValue rval;
            if (!expand_ref_get(rhs_res.value(), rval))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_b_or(lval, rval, r))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }

        // Logical.
        case BinaryExpression::LOG_AND:
        {
            // Short-circuit evaluation.
            if (!lval.to_boolean())
            {
                r = lval;
            }
            else
            {
                Completion rhs_res = parse(expr->right());
                if (rhs_res.is_abrupt())
                    return rhs_res;

                // FIXME: rval necessary?
                EsValue rval;
                if (!expand_ref_get(rhs_res.value(), rval))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                r = rval;
            }
            break;
        }
        case BinaryExpression::LOG_OR:
        {
            // Short-circuit evaluation.
            if (lval.to_boolean())
            {
                r = lval;
            }
            else
            {
                Completion rhs_res = parse(expr->right());
                if (rhs_res.is_abrupt())
                    return rhs_res;

                // FIXME: rval necessary?
                EsValue rval;
                if (!expand_ref_get(rhs_res.value(), rval))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                r = rval;
            }
            break;
        }

        default:
            assert(false);
            break;
    }

    return Completion(Completion::TYPE_NORMAL, r);
}

Completion Evaluator::parse_unary_expr(UnaryExpression *expr)
{
    EsValue t;

    if (expr->operation() == UnaryExpression::DELETE)
    {
        if (PropertyExpression *prop =
            dynamic_cast<PropertyExpression *>(expr->expression()))
        {
            Completion key_res = parse(prop->key());
            if (key_res.is_abrupt())
                return key_res;
            EsReferenceOrValue key = key_res.value();

            Completion obj_res = parse(prop->obj());
            if (obj_res.is_abrupt())
                return obj_res;
            EsReferenceOrValue obj = obj_res.value();

            EsValue key_val;
            if (!expand_ref_get(key, key_val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            EsValue obj_val;
            if (!expand_ref_get(obj, obj_val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!obj_val.chk_obj_coercibleT())
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_prp_del(EsContextStack::instance().top(), obj_val, key_val, t))
            {
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            }
        }
        else if (IdentifierLiteral * ident =
            dynamic_cast<IdentifierLiteral *>(expr->expression()))
        {
            if (!op_ctx_del(EsContextStack::instance().top(), EsPropertyKey::from_str(ident->value()).as_raw(), t))
            {
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            }
        }
        else
        {
            t = EsValue::from_bool(true);
        }

        return Completion(Completion::TYPE_NORMAL, t);
    }

    Completion expr_res = parse(expr->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    switch (expr->operation())
    {
        case UnaryExpression::VOID:
        {
            EsValue tmp;
            if (!expand_ref_get(expr_res.value(), tmp))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            t = EsValue::undefined;
            break;
        }
        case UnaryExpression::TYPEOF:
        {
            EsValue v;
            if (expr_res.value().is_reference())
            {
                if (!expand_ref_get(expr_res.value(), v))
                {
                    op_ex_clear(EsContextStack::instance().top());
                    v = EsValue::undefined;
                }
            }
            else
            {
                v = expr_res.value().value();
            }

            if (!op_u_typeof(v, t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case UnaryExpression::PRE_INC:
        {
            EsValue rval;
            double old_val = 0.0;
            if (!expand_ref_get(expr_res.value(), rval) || !rval.to_number(old_val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            double new_val = old_val + 1.0f;

            t = EsValue::from_num(new_val);
            if (!expand_ref_put(expr_res.value(), t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case UnaryExpression::PRE_DEC:
        {
            EsValue rval;
            double old_val = 0.0;
            if (!expand_ref_get(expr_res.value(), rval) || !rval.to_number(old_val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            double new_val = old_val - 1.0f;

            t = EsValue::from_num(new_val);
            if (!expand_ref_put(expr_res.value(), t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case UnaryExpression::POST_INC:
        {
            EsValue rval;
            double old_val = 0.0;
            if (!expand_ref_get(expr_res.value(), rval) || !rval.to_number(old_val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            double new_val = old_val + 1.0f;

            if (!expand_ref_put(expr_res.value(), EsValue::from_num(new_val)))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            t = EsValue::from_num(old_val);
            break;
        }
        case UnaryExpression::POST_DEC:
        {
            EsValue rval;
            double old_val = 0.0;
            if (!expand_ref_get(expr_res.value(), rval) || !rval.to_number(old_val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            double new_val = old_val - 1.0f;

            if (!expand_ref_put(expr_res.value(), EsValue::from_num(new_val)))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            t = EsValue::from_num(old_val);
            break;
        }
        case UnaryExpression::PLUS:
        {
            EsValue rval;
            if (!expand_ref_get(expr_res.value(), rval) || !op_u_add(rval, t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case UnaryExpression::MINUS:
        {
            EsValue rval;
            if (!expand_ref_get(expr_res.value(), rval) || !op_u_sub(rval, t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case UnaryExpression::BIT_NOT:
        {
            EsValue rval;
            if (!expand_ref_get(expr_res.value(), rval) || !op_u_bit_not(rval, t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }
        case UnaryExpression::LOG_NOT:
        {
            EsValue rval;
            if (!expand_ref_get(expr_res.value(), rval) || !op_u_not(rval, t))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            break;
        }

        default:
            assert(false);
            break;
    }

    return Completion(Completion::TYPE_NORMAL, t);
}

Completion Evaluator::parse_assign_expr(AssignmentExpression *expr)
{
    EsValue t;

    Completion lhs_res = parse(expr->lhs());
    if (lhs_res.is_abrupt())
        return lhs_res;
    EsReferenceOrValue l = lhs_res.value();

    Completion rhs_res = parse(expr->rhs());
    if (rhs_res.is_abrupt())
        return rhs_res;
    EsReferenceOrValue r = rhs_res.value();

    if (expr->operation() == AssignmentExpression::ASSIGN)
    {
        if (!expand_ref_get(r, t) || !expand_ref_put(l, t))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());
    }
    else
    {
        EsValue lval, rval;
        if (!expand_ref_get(l, lval) || !expand_ref_get(r, rval))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        switch (expr->operation())
        {
            case AssignmentExpression::ASSIGN_ADD:
                if (!op_b_add(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_SUB:
                if (!op_b_sub(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_MUL:
                if (!op_b_mul(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_MOD:
                if (!op_b_mod(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_LS:
                if (!op_b_shl(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_RSS:
                if (!op_b_sar(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_RUS:
                if (!op_b_shr(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_BIT_AND:
                if (!op_b_and(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_BIT_OR:
                if (!op_b_or(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_BIT_XOR:
                if (!op_b_xor(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;
            case AssignmentExpression::ASSIGN_DIV:
                if (!op_b_div(lval, rval, t) || !expand_ref_put(l, t))
                    return Completion(Completion::TYPE_THROW,
                                      EsContextStack::instance().top()->get_pending_exception());
                break;

            default:
                assert(false);
                break;
        }
    }

    return Completion(Completion::TYPE_NORMAL, t);
}

Completion Evaluator::parse_cond_expr(ConditionalExpression *expr)
{
    Completion cond_res = parse(expr->condition());
    if (cond_res.is_abrupt())
        return cond_res;

    EsValue cond;
    if (!expand_ref_get(cond_res.value(), cond))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    if (cond.to_boolean())
        return parse(expr->left());
    else
        return parse(expr->right());
}

Completion Evaluator::parse_prop_expr(PropertyExpression *expr)
{
    Completion key_res = parse(expr->key());
    if (key_res.is_abrupt())
        return key_res;
    EsReferenceOrValue key = key_res.value();

    Completion obj_res = parse(expr->obj());
    if (obj_res.is_abrupt())
        return obj_res;
    EsReferenceOrValue obj_ref = obj_res.value();

    EsValue key_val;
    if (!expand_ref_get(key, key_val))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    EsValue obj_val;
    if (!expand_ref_get(obj_ref, obj_val))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    EsObject *obj = obj_val.to_objectT();
    if (!obj)
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    String key_str;
    if (!key_val.to_string(key_str))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    EsReference t(key_str, EsContextStack::instance().top()->is_strict(), obj);
    return Completion(Completion::TYPE_NORMAL, t);
}

Completion Evaluator::parse_call_expr(CallExpression *expr)
{
    ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        Completion arg_res = parse(*it);
        if (arg_res.is_abrupt())
            return arg_res;

        EsValue val;
        if (!expand_ref_get(arg_res.value(), val))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        // FIXME: Handle exceptions properly.
        op_stk_push(val);
    }

    int argc = static_cast<int>(expr->arguments().size());

    EsValue r;
    bool success = false;

    if (PropertyExpression *prop =
        dynamic_cast<PropertyExpression *>(expr->expression()))
    {
        Completion key_res = parse(prop->key());
        if (key_res.is_abrupt())
            return key_res;
        EsReferenceOrValue key = key_res.value();

        Completion obj_res = parse(prop->obj());
        if (obj_res.is_abrupt())
            return obj_res;
        EsReferenceOrValue obj = obj_res.value();

        EsValue key_val;
        if (!expand_ref_get(key, key_val))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        EsValue obj_val;
        if (!expand_ref_get(obj, obj_val))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        if (!obj_val.chk_obj_coercibleT())
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        success = op_call_keyed(obj_val, key_val, argc, r);
    }
    else if (IdentifierLiteral * ident =
        dynamic_cast<IdentifierLiteral *>(expr->expression()))
    {
        success =
            op_call_named(EsPropertyKey::from_str(ident->value()).as_raw(),
                          argc, r);
    }
    else
    {
        Completion expr_res = parse(expr->expression());
        if (expr_res.is_abrupt())
            return expr_res;

        EsReferenceOrValue fun = expr_res.value();

        assert(!fun.is_reference());
        success = op_call(fun.value(), argc, r);
    }

    return Completion(success ? Completion::TYPE_NORMAL : Completion::TYPE_THROW,
                      success ? r : EsContextStack::instance().top()->get_pending_exception());
}

Completion Evaluator::parse_call_new_expr(CallNewExpression *expr)
{
    Completion expr_res = parse(expr->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    EsReferenceOrValue fun_ref = expr_res.value();

    ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        Completion arg_res = parse(*it);
        if (arg_res.is_abrupt())
            return arg_res;

        EsValue val;
        if (!expand_ref_get(arg_res.value(), val))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        // FIXME: Handle exceptions properly.
        op_stk_push(val);
    }

    EsValue fun;
    if (!expand_ref_get(fun_ref, fun))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    int argc = static_cast<int>(expr->arguments().size());

    EsValue r;
    bool success = op_call_new(fun, argc, r);
    return Completion(success ? Completion::TYPE_NORMAL : Completion::TYPE_THROW,
                      success ? r : EsContextStack::instance().top()->get_pending_exception());
}

Completion Evaluator::parse_regular_expr(RegularExpression *expr)
{
    EsValue r = op_new_reg_exp(expr->pattern(), expr->flags());
    return Completion(Completion::TYPE_NORMAL, r);
}

Completion Evaluator::parse_fun_expr(FunctionExpression *expr)
{
    const FunctionLiteral *lit = expr->function();

    return parse(const_cast<FunctionLiteral *>(lit));
}

Completion Evaluator::parse_this_lit(ThisLiteral *lit)
{
    return Completion(Completion::TYPE_NORMAL, frame_.this_value());
}

Completion Evaluator::parse_ident_lit(IdentifierLiteral *lit)
{
    EsContext *ctx = EsContextStack::instance().top();

    EsReference t(lit->value(), ctx->is_strict());
    return Completion(Completion::TYPE_NORMAL, t);
}

Completion Evaluator::parse_null_lit(NullLiteral *lit)
{
    return Completion(Completion::TYPE_NORMAL, EsValue::null);
}

Completion Evaluator::parse_bool_lit(BoolLiteral *lit)
{
    return Completion(Completion::TYPE_NORMAL, EsValue::from_bool(lit->value()));
}

Completion Evaluator::parse_num_lit(NumberLiteral *lit)
{
    return Completion(Completion::TYPE_NORMAL, EsValue::from_num(es_str_to_num(lit->as_string())));
}

Completion Evaluator::parse_str_lit(StringLiteral *lit)
{
    return Completion(Completion::TYPE_NORMAL, EsValue::from_str(lit->value()));
}

Completion Evaluator::parse_fun_lit(FunctionLiteral *lit)
{
    AutoScope scope(this, SCOPE_FUNCTION);

    EsFunction *fun = NULL;

    if (lit->type() == FunctionLiteral::TYPE_DECLARATION)
    {
        fun = EsFunction::create_inst(EsContextStack::instance().top()->var_env(), lit);
    }
    else
    {
        if (!lit->name().empty())
        {
            EsLexicalEnvironment *fun_env = es_new_decl_env(EsContextStack::instance().top()->lex_env());

            fun = EsFunction::create_inst(fun_env, lit);

            assert(fun_env->env_rec()->is_decl_env());
            EsDeclarativeEnvironmentRecord *env =
                static_cast<EsDeclarativeEnvironmentRecord *>(fun_env->env_rec());

            env->create_immutable_binding(EsPropertyKey::from_str(lit->name()), EsValue::from_obj(fun));
        }
        else
        {
            fun = EsFunction::create_inst(EsContextStack::instance().top()->lex_env(), lit);
        }
    }

    assert(fun);
    return Completion(Completion::TYPE_NORMAL, EsValue::from_obj(fun));
}

Completion Evaluator::parse_var_lit(VariableLiteral *lit)
{
    // Dealt with in parse_fun_decls().
    return Completion(Completion::TYPE_NORMAL, EsValue::nothing);
}

Completion Evaluator::parse_array_lit(ArrayLiteral *lit)
{
    EsValueVector items;

    ExpressionVector::const_iterator it;
    for (it = lit->values().begin(); it != lit->values().end(); ++it)
    {
        Completion val_res = parse(*it);
        if (val_res.is_abrupt())
            return val_res;

        EsValue val;
        if (!expand_ref_get(val_res.value(), val))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        items.push_back(val);
    }
    
    return Completion(Completion::TYPE_NORMAL, op_new_arr(static_cast<int>(items.size()),
                                                          &items[0]));
}

Completion Evaluator::parse_obj_lit(ObjectLiteral *lit)
{
    EsValue new_obj = op_new_obj();

    ObjectLiteral::PropertyVector::const_iterator it;
    for (it = lit->properties().begin(); it != lit->properties().end(); ++it)
    {
        const ObjectLiteral::Property *prop = *it;

        if (prop->type() == ObjectLiteral::Property::DATA)
        {
            Completion key_res = parse(prop->key());
            if (key_res.is_abrupt())
                return key_res;

            EsValue key;
            if (!expand_ref_get(key_res.value(), key))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            Completion val_res = parse(prop->val());
            if (val_res.is_abrupt())
                return val_res;

            EsValue val;
            if (!expand_ref_get(val_res.value(), val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_prp_def_data(new_obj, key, val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
        }
        else
        {
            Completion val_res = parse(prop->val());
            if (val_res.is_abrupt())
                return val_res;

            EsValue val;
            if (!expand_ref_get(val_res.value(), val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!op_prp_def_accessor(new_obj, EsPropertyKey::from_str(prop->accessor_name()).as_raw(),
                                     val, prop->type() == ObjectLiteral::Property::SETTER))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
        }
    }

    return Completion(Completion::TYPE_NORMAL, new_obj);
}

Completion Evaluator::parse_nothing_lit(NothingLiteral *lit)
{
    return Completion(Completion::TYPE_NORMAL, EsValue::nothing);
}

Completion Evaluator::parse_empty_stmt(EmptyStatement *stmt)
{
    // 12.3.
    return Completion(Completion::TYPE_NORMAL, EsValue::nothing);
}

Completion Evaluator::parse_expr_stmt(ExpressionStatement *stmt)
{
    // 12.4.
    Completion expr_res = parse(stmt->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    EsValue val;
    if (!expand_ref_get(expr_res.value(), val))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    return Completion(Completion::TYPE_NORMAL, val);
}

Completion Evaluator::parse_block_stmt(BlockStatement *stmt)
{
    AutoScope scope(this, SCOPE_DEFAULT);

    // 12.1.
    EsReferenceOrValue v;

    Completion stmt_res(Completion::TYPE_THROW, EsReferenceOrValue());

    StatementVector::const_iterator it_stmt;
    for (it_stmt = stmt->body().begin(); it_stmt != stmt->body().end(); ++it_stmt)
    {
        // FIXME: Catch exception and return according to 12.1:4?
        stmt_res = parse(*it_stmt);
        if (stmt_res.type() == Completion::TYPE_THROW)
            return stmt_res;
        if (!stmt_res.value().empty())
            v = stmt_res.value();

        if (stmt_res.is_abrupt())
            return Completion(stmt_res.type(), v, stmt_res.target());
    }

    if (stmt->is_hidden())
        return Completion(Completion::TYPE_NORMAL, EsValue::nothing);

    if (stmt->body().empty())
        return Completion(Completion::TYPE_NORMAL, EsValue::nothing);
    else
        return Completion(stmt_res.type(), v, stmt_res.target());
}

Completion Evaluator::parse_if_stmt(IfStatement *stmt)
{
    // 12.5.
    Completion cond_res = parse(stmt->condition());
    if (cond_res.is_abrupt())
        return cond_res;

    EsValue cond;
    if (!expand_ref_get(cond_res.value(), cond))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    if (cond.to_boolean())
        return parse(stmt->if_statement());
    else if (stmt->has_else())
        return parse(stmt->else_statement());
    else
        return Completion(Completion::TYPE_NORMAL, EsValue::nothing);
}

Completion Evaluator::parse_do_while_stmt(DoWhileStatement *stmt)
{
    AutoScope scope(this, SCOPE_ITERATION);

    // 12.6.1.
    EsReferenceOrValue v;

    bool iterating = true;
    while (iterating)
    {
        Completion body_res = parse(stmt->body());

        if (!body_res.value().empty())
            v = body_res.value();

        if (body_res.type() != Completion::TYPE_CONTINUE ||
            !(body_res.target().empty() || stmt->labels().contains(body_res.target())))
        {
            if (body_res.type() == Completion::TYPE_BREAK &&
                (body_res.target().empty() || stmt->labels().contains(body_res.target())))
            {
                return Completion(Completion::TYPE_NORMAL, v);
            }
            if (body_res.is_abrupt())
                return body_res;
        }

        if (stmt->has_condition())
        {
            Completion cond_res = parse(stmt->condition());
            if (cond_res.is_abrupt())
                return cond_res;

            EsValue cond;
            if (!expand_ref_get(cond_res.value(), cond))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!cond.to_boolean())
                iterating = false;
        }
    }

    return Completion(Completion::TYPE_NORMAL, v);
}

Completion Evaluator::parse_while_stmt(WhileStatement *stmt)
{
    AutoScope scope(this, SCOPE_ITERATION);

    // 12.6.2.
    EsReferenceOrValue v;

    while (true)
    {
        Completion cond_res = parse(stmt->condition());
        if (cond_res.is_abrupt())
            return cond_res;

        EsValue cond;
        if (!expand_ref_get(cond_res.value(), cond))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        if (!cond.to_boolean())
            return Completion(Completion::TYPE_NORMAL, v);

        Completion body_res = parse(stmt->body());

        if (!body_res.value().empty())
            v = body_res.value();

        if (body_res.type() != Completion::TYPE_CONTINUE ||
            !(body_res.target().empty() || stmt->labels().contains(body_res.target())))
        {
            if (body_res.type() == Completion::TYPE_BREAK &&
                (body_res.target().empty() || stmt->labels().contains(body_res.target())))
            {
                return Completion(Completion::TYPE_NORMAL, v);
            }
            if (body_res.is_abrupt())
                return body_res;
        }
    }

    assert(false);
}

Completion Evaluator::parse_for_in_stmt(ForInStatement *stmt)
{
    AutoScope scope(this, SCOPE_ITERATION);

    // 12.6.4.
    Completion enum_res = parse(stmt->enumerable());
    if (enum_res.is_abrupt())
        return enum_res;

    EsValue expr_val;
    if (!expand_ref_get(enum_res.value(), expr_val))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    if (expr_val.is_null() || expr_val.is_undefined())
        return Completion(Completion::TYPE_NORMAL, EsValue::nothing);

    EsObject *obj = expr_val.to_objectT();
    if (!obj)
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    EsReferenceOrValue v;

    EsObject::Iterator it = obj->begin_recursive();
    EsObject::Iterator it_end = obj->end_recursive();

    while (true)
    {
        EsValue p;
        bool found_p = false;

        // Find the next enumerable property.
        for (; it != it_end && !found_p; ++it)
        {
            EsPropertyKey key = *it;

            EsPropertyReference prop = obj->get_property(key);
            if (!prop || !prop->is_enumerable())    // The property might have been deleted.
                continue;

            p.set_str(key.to_string());
            found_p = true;
        }

        if (!found_p)
            return Completion(Completion::TYPE_NORMAL, v);

        Completion decl_res = parse(stmt->declaration());
        if (decl_res.is_abrupt())
            return decl_res;

        if (!expand_ref_put(decl_res.value(), p))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());

        Completion body_res = parse(stmt->body());

        if (!body_res.value().empty())
            v = body_res.value();

        if (body_res.type() == Completion::TYPE_BREAK &&
            (body_res.target().empty() || stmt->labels().contains(body_res.target())))
        {
            return Completion(Completion::TYPE_NORMAL, v);
        }
        if (body_res.type() != Completion::TYPE_CONTINUE ||
            !(body_res.target().empty() || stmt->labels().contains(body_res.target())))
        {
            if (body_res.is_abrupt())
                return body_res;
        }
    }

    assert(false);
}

Completion Evaluator::parse_for_stmt(ForStatement *stmt)
{
    AutoScope scope(this, SCOPE_ITERATION);

    // 12.6.3
    if (stmt->has_initializer())
    {
        Completion init_res = parse(stmt->initializer());
        if (init_res.is_abrupt())
            return init_res;

        EsValue initializer;
        if (!expand_ref_get(init_res.value(), initializer))
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());
    }

    EsReferenceOrValue v;

    while (true)
    {
        if (stmt->has_condition())
        {
            Completion cond_res = parse(stmt->condition());
            if (cond_res.is_abrupt())
                return cond_res;

            EsValue cond;
            if (!expand_ref_get(cond_res.value(), cond))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            if (!cond.to_boolean())
                return Completion(Completion::TYPE_NORMAL, v);
        }

        Completion body_res = parse(stmt->body());

        if (!body_res.value().empty())
            v = body_res.value();

        if (body_res.type() == Completion::TYPE_BREAK &&
            (body_res.target().empty() || stmt->labels().contains(body_res.target())))
        {
            return Completion(Completion::TYPE_NORMAL, v);
        }

        if (body_res.type() != Completion::TYPE_CONTINUE ||
            !(body_res.target().empty() || stmt->labels().contains(body_res.target())))
        {
            if (body_res.is_abrupt())
                return body_res;
        }

        if (stmt->has_next())
        {
            Completion next_res = parse(stmt->next());
            if (next_res.is_abrupt())
                return next_res;

            EsValue next;
            if (!expand_ref_get(next_res.value(), next))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
        }
    }

    assert(false);
}

Completion Evaluator::parse_cont_stmt(ContinueStatement *stmt)
{
    // 12.7.
    String target;

    if (stmt->has_target())
    {
        target = stmt->target()->labels().first();  // Any label from the list is fine.
    }
    else
    {
        if (!is_in_iteration())
        {
            ES_THROW(EsError, _USTR("error: non-labeled continue statements are only allowed in loops."));
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());
        }
    }

    return Completion(Completion::TYPE_CONTINUE, EsValue::nothing, target);
}

Completion Evaluator::parse_break_stmt(BreakStatement *stmt)
{
    // 12.8.
    String target;

    if (stmt->has_target())
    {
        target = stmt->target()->labels().first();  // Any label from the list is fine.
    }
    else
    {
        if (!is_in_iteration() && !is_in_switch())
        {
            ES_THROW(EsError, _USTR("error: non-labeled break statements are only allowed in loops and switch statements."));
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());
        }
    }

    return Completion(Completion::TYPE_BREAK, EsValue::nothing, target);
}

Completion Evaluator::parse_ret_stmt(ReturnStatement *stmt)
{
    // 12.9.
    if (!stmt->has_expression())
        return Completion(Completion::TYPE_RETURN, EsValue::undefined);

    Completion expr_res = parse(stmt->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    EsValue expr;
    if (!expand_ref_get(expr_res.value(), expr))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    return Completion(Completion::TYPE_RETURN, expr);
}

Completion Evaluator::parse_with_stmt(WithStatement *stmt)
{
    AutoScope scope(this, SCOPE_WITH);

    // 12.10.
    Completion expr_res = parse(stmt->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    EsValue expr;
    if (!expand_ref_get(expr_res.value(), expr))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    if (!op_ctx_enter_with(EsContextStack::instance().top(), expr))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());
    Completion body_res = parse(stmt->body());
    op_ctx_leave();
    return body_res;
}

Completion Evaluator::parse_switch_stmt(SwitchStatement *stmt)
{
    AutoScope scope(this, SCOPE_SWITCH);

    // 12.11.
    EsReferenceOrValue v;

    Completion expr_res = parse(stmt->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    EsValue expr_val;
    if (!expand_ref_get(expr_res.value(), expr_val))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    bool found_case = false;

    // First, process all the non-default clauses.
    SwitchStatement::CaseClauseVector::const_iterator it;
    for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
    {
        const SwitchStatement::CaseClause *clause = *it;

        if (!clause->is_default() && !found_case)
        {
            Completion clause_res = parse(clause->label());
            if (clause_res.is_abrupt())
                return clause_res;

            EsValue label;
            if (!expand_ref_get(clause_res.value(), label))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            EsValue val;
            if (!op_c_strict_eq(label, expr_val, val))
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());

            found_case = val.to_boolean();
        }

        if (found_case)
        {
            StatementVector::const_iterator it_stmt;
            for (it_stmt = clause->body().begin(); it_stmt != clause->body().end(); ++it_stmt)
            {
                Completion body_res = parse(*it_stmt);
                if (!body_res.value().empty())
                    v = body_res.value();

                if (body_res.type() == Completion::TYPE_BREAK &&
                    (body_res.target().empty() || stmt->labels().contains(body_res.target())))
                {
                    return Completion(Completion::TYPE_NORMAL, body_res.value());
                }

                if (body_res.is_abrupt())
                    return Completion(body_res.type(), v , body_res.target());
            }
        }
    }

    // Lastly, process the default clause, if present.
    if (!found_case)
    {
        for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
        {
            const SwitchStatement::CaseClause *clause = *it;

            if (clause->is_default())
            {
                StatementVector::const_iterator it_stmt;
                for (it_stmt = clause->body().begin(); it_stmt != clause->body().end(); ++it_stmt)
                {
                    Completion body_res = parse(*it_stmt);
                    if (!body_res.value().empty())
                        v = body_res.value();

                    if (body_res.type() == Completion::TYPE_BREAK &&
                        (body_res.target().empty() || stmt->labels().contains(body_res.target())))
                    {
                        return Completion(Completion::TYPE_NORMAL, body_res.value());
                    }

                    if (body_res.is_abrupt())
                        return Completion(body_res.type(), v , body_res.target());
                }
            }
        }
    }

    return Completion(Completion::TYPE_NORMAL, v);
}

Completion Evaluator::parse_throw_stmt(ThrowStatement *stmt)
{
    Completion expr_res = parse(stmt->expression());
    if (expr_res.is_abrupt())
        return expr_res;

    EsValue expr_val;
    if (!expand_ref_get(expr_res.value(), expr_val))
        return Completion(Completion::TYPE_THROW,
                          EsContextStack::instance().top()->get_pending_exception());

    op_ex_set(EsContextStack::instance().top(), expr_val);

    return Completion(Completion::TYPE_THROW, expr_val);
}

Completion Evaluator::parse_try_stmt(TryStatement *stmt)
{
    AutoScope scope(this, SCOPE_DEFAULT);

    if (stmt->has_catch_block() && stmt->has_finally_block())
    {
        Completion b = parse(stmt->try_block());
        Completion c = b;
        if (b.type() == Completion::TYPE_THROW)
        {
            if (!op_ctx_enter_catch(EsContextStack::instance().top(),
                                    EsPropertyKey::from_str(stmt->catch_identifier()).as_raw()))
            {
                return Completion(Completion::TYPE_THROW,
                                  EsContextStack::instance().top()->get_pending_exception());
            }

            c = parse(stmt->catch_block());
            op_ctx_leave();
        }

        Completion fin_res = parse(stmt->finally_block());
        if (fin_res.type() == Completion::TYPE_NORMAL)
            return c;
        else
            return fin_res;
    }
    else if (stmt->has_catch_block())
    {
        Completion try_res = parse(stmt->try_block());
        if (try_res.type() != Completion::TYPE_THROW)
            return try_res;

        if (!op_ctx_enter_catch(EsContextStack::instance().top(),
                                EsPropertyKey::from_str(stmt->catch_identifier()).as_raw()))
        {
            return Completion(Completion::TYPE_THROW,
                              EsContextStack::instance().top()->get_pending_exception());
        }

        Completion catch_res = parse(stmt->catch_block());
        op_ctx_leave();
        return catch_res;
    }
    else if (stmt->has_finally_block())
    {
        Completion b = parse(stmt->try_block());
        EsValue b_ex_state = op_ex_save_state(EsContextStack::instance().top());

        Completion fin_res = parse(stmt->finally_block());
        if (fin_res.type() == Completion::TYPE_NORMAL)
        {
            op_ex_load_state(EsContextStack::instance().top(), b_ex_state);
            return b;
        }
        else
        {
            return fin_res;
        }
    }

    assert(false);
    return Completion(Completion::TYPE_THROW, EsValue::nothing);
}

Completion Evaluator::parse_dbg_stmt(DebuggerStatement *stmt)
{
    return Completion(Completion::TYPE_NORMAL, EsReferenceOrValue());
}

void Evaluator::parse_fun_decls(const DeclarationVector &decls)
{
    // Visit functions first to comply with Declaration Binding instantiation (10.5).
    DeclarationVector::const_iterator it_decl;
    for (it_decl = decls.begin(); it_decl != decls.end(); ++it_decl)
    {
        const Declaration *decl = *it_decl;
        if (decl->is_function())
        {
            FunctionLiteral *lit = decl->as_function();
            EsFunction *fun = parse(lit).value().value().as_function();

            op_ctx_decl_fun(EsContextStack::instance().top(),
                            type_ == TYPE_EVAL,
                            code_->is_strict_mode(),
                            EsPropertyKey::from_str(lit->name()).as_raw(),
                            EsValue::from_obj(fun));
        }
    }

    for (it_decl = decls.begin(); it_decl != decls.end(); ++it_decl)
    {
        const Declaration *decl = *it_decl;
        if (decl->is_variable())
        {
            VariableLiteral *var = decl->as_variable();
            parse(var);

            op_ctx_decl_var(EsContextStack::instance().top(),
                            type_ == TYPE_EVAL,
                            code_->is_strict_mode(),
                            EsPropertyKey::from_str(var->name()).as_raw());
        }
    }
}

bool Evaluator::exec(EsContext *ctx)
{
    AutoScope scope(this, SCOPE_FUNCTION);

    int argc = frame_.argc();
    EsValue *argv = frame_.fp();

    // Function prologue: arguments object and parameters.
    if (type_ == TYPE_FUNCTION && code_->needs_args_obj())
    {
        // If we have an arguments object we must allocate the arguments vector
        // on the heap since the arguments object might outlive the function
        // context. As a result we cannot let the arguments object reference
        // the arguments vector directly.
        EsValue *argv_heap = new (GC)EsValue[argc];
        memcpy(argv_heap, argv, sizeof(EsValue) * argc);

        String *prmv = code_->parameters().empty() ? NULL :
            const_cast<String *>(&code_->parameters()[0]);
        EsValue args = op_args_obj_init(ctx, argc, frame_.fp(), frame_.vp());
        assert(args.is_object());

        EsArguments *args_obj = safe_cast<EsArguments *>(args.as_object());

        StringSet mapped_names;

        int prmc = static_cast<int>(code_->parameters().size());
        for (int i = std::min(argc, prmc) - 1; i >= 0; i--)
        {
            String name = prmv[i];
            if (!ctx->is_strict() && mapped_names.count(name) == 0)
            {
                mapped_names.insert(name);

                args_obj->link_parameter(i, &argv_heap[i]);
            }
        }

        for (int i = 0; i < prmc; i++)
        {
            if (i < argc)
            {
                // Link the parameter to the coresponding slot in the arguments
                // vector. Updating an argument through the arguments object
                // should reflect in the parameter and vice versa.
                op_ctx_link_var(EsContextStack::instance().top(),
                                EsPropertyKey::from_str(code_->parameters()[i]).as_raw(),
                                &argv_heap[i]);
            }
            else
            {
                op_ctx_decl_prm(EsContextStack::instance().top(),
                                code_->is_strict_mode(),
                                EsPropertyKey::from_str(code_->parameters()[i]).as_raw(),
                                EsValue::undefined);
            }
        }
    }
    else
    {
        // Function prologue: parameters.
        for (int i = 0; i < static_cast<int>(code_->parameters().size()); i++)
        {
            op_ctx_decl_prm(EsContextStack::instance().top(),
                            code_->is_strict_mode(),
                            EsPropertyKey::from_str(code_->parameters()[i]).as_raw(),
                            i < argc ? argv[i] : EsValue::undefined);
        }
    }

    // Function prologue: declarations.
    parse_fun_decls(code_->declarations());
    
    // Only used for eval.
    EsValue v = EsValue::undefined;

    // Function body, 13.2.1
    StatementVector::const_iterator it_stmt;
    for (it_stmt = code_->body().begin(); it_stmt != code_->body().end(); ++it_stmt)
    {
        Completion stmt_res = parse(*it_stmt);
        switch (stmt_res.type())
        {
            case Completion::TYPE_NORMAL:
                break;
            case Completion::TYPE_BREAK:
                break;
            case Completion::TYPE_CONTINUE:
                break;
            case Completion::TYPE_RETURN:
                frame_.set_result(stmt_res.value().value());
                return true;
            case Completion::TYPE_THROW:
                assert(EsContextStack::instance().top()->has_pending_exception());
                return false;

            default:
                assert(false);
                break;
        }

        // Only used for eval.
        if (type_ == TYPE_EVAL)
        {
            if (!stmt_res.value().empty())
                v = stmt_res.value().value();
        }
    }

    frame_.set_result(type_ == TYPE_EVAL ? v : EsValue::undefined);
    return true;
}
