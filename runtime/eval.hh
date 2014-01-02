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
#include <vector>
#include <gc_cpp.h>
#include <gc/gc_allocator.h>
#include "common/core.hh"
#include "parser/ast.hh"
#include "parser/visitor.hh"
#include "api.hh"

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

class EsCallFrame;

/**
 * @brief Reference specification type.
 */
class EsReference
{
private:
    bool strict_;
    String name_;
    EsObject *base_;

public:
    /**
     * Creates an empty reference.
     */
    EsReference()
        : strict_(false)
        , base_(NULL)
    {
    }

    /**
     * Creates a reference with an undefined base value.
     * @param [in] name Reference name.
     * @param [in] strict true for a strict reference, false otherwise.
     */
    EsReference(const String &name, bool strict)
        : strict_(strict)
        , name_(name)
        , base_(NULL)
    {
    }

    /**
     * Creates a reference with a primitive or object base value.
     * @param [in] name Reference name.
     * @param [in] strict true for a strict reference, false otherwise.
     * @param [in] base Base value.
     */
    EsReference(const String &name, bool strict, EsObject *base)
        : strict_(strict)
        , name_(name)
        , base_(base)
    {
        assert(base);
    }

    /**
     * @return true if the reference is a strict reference and false otherwise.
     */
    bool is_strict_reference() const
    {
        return strict_;
    }

    /**
     * @return The name of the referenced entity.
     */
    const String &get_referenced_name() const
    {
        return name_;
    }

    /**
     * @return The base entity the reference points at.
     */
    EsObject *get_base() const
    {
        return base_;
    }
};

/**
 * @brief Represents a value or a reference.
 */
class EsReferenceOrValue
{
private:
    /**
     * @brief Content types.
     */
    enum Type
    {
        TYPE_EMPTY,     ///< The object contains nothing.
        TYPE_VALUE,     ///< The object is a value.
        TYPE_REFERENCE  ///< The object is a reference.
    } type_;

    EsValue val_;       ///< Value, only valid if type_ == TYPE_VALUE.
    EsReference ref_;   ///< Reference, only valid if type_ == TYPE_REFERENCE.

public:
    /**
     * Constructs an any object containing nothing.
     */
    EsReferenceOrValue() : type_(TYPE_EMPTY) {}

    /**
     * Constructs an object containing a value.
     * @param [in] val Value.
     */
    EsReferenceOrValue(const EsValue &val) : type_(TYPE_VALUE), val_(val) {}

    /**
     * Constructs and object containing a reference.
     * @param [in] ref Reference.
     */
    EsReferenceOrValue(const EsReference &ref) : type_(TYPE_REFERENCE), ref_(ref) {}

    /**
     * @return true if the object contains nothing, else false is returned.
     */
    bool empty() const { return type_ == TYPE_EMPTY || (type_ == TYPE_VALUE && val_.is_nothing()); }

    /**
     * @return true if the object contains a value, else false is returned.
     */
    bool is_value() const { return type_ == TYPE_VALUE; }

    /**
     * @return true if the object contains a reference, else false is returned.
     */
    bool is_reference() const { return type_ == TYPE_REFERENCE; }

    /**
     * @return Value contained in object.
     * @pre Object contains a value; is_value() returns true.
     */
    const EsValue &value() const { assert(is_value()); return val_; }

    /**
     * @return Reference contained in object.
     * @pre Object contains a reference; is_reference() returns true.
     */
    const EsReference &reference() const { assert(is_reference()); return ref_; }
};

/**
 * @brief ECMAScript completion type (8.9).
 */
class Completion
{
public:
    enum Type
    {
        TYPE_NORMAL,
        TYPE_BREAK,
        TYPE_CONTINUE,
        TYPE_RETURN,
        TYPE_THROW
    };

private:
    /** ValueVisitor is granted access to Completion(). */
    friend class ValueVisitor<Completion>;

    Type type_;
    EsReferenceOrValue value_;
    String target_;

    Completion()
        : type_(TYPE_NORMAL) {}

public:
    Completion(Type type, EsReferenceOrValue value)
        : type_(type)
        , value_(value) {}

    Completion(Type type, EsReferenceOrValue value, const String &target)
        : type_(type)
        , value_(value)
        , target_(target) {}

    Type type() const { return type_; }
    EsReferenceOrValue value() const { return value_; }
    String target() const { return target_; }

    bool is_abrupt() const
    {
        // 8.9: The term "abrupt completion" refers to any completion with a type other than normal.
        return type_ != TYPE_NORMAL;
    }

    void clear()
    {
        type_ = TYPE_NORMAL;
        value_ = EsReferenceOrValue();
        target_ = String();
    }
};

class Evaluator : public ValueVisitor<Completion>
{
public:
    /**
     * @brief Type of code being evaluated.
     */
    enum Type
    {
        TYPE_EVAL,      ///< "eval" code.
        TYPE_FUNCTION,  ///< Function code.
        TYPE_PROGRAM    ///< Program code.
    };

    /**
     * @brief Scope types.
     */
    enum Scope
    {
        SCOPE_DEFAULT,
        SCOPE_ITERATION,
        SCOPE_SWITCH,
        SCOPE_FUNCTION,
        SCOPE_WITH
    };

private:
    class AutoScope
    {
    private:
        Evaluator *eval_;

    public:
        AutoScope(Evaluator *eval, Scope scope)
            : eval_(eval)
        {
            assert(eval);
            eval->scopes_.push_back(scope);
        }

        ~AutoScope()
        {
            assert(eval_);
            assert(!eval_->scopes_.empty());
            eval_->scopes_.pop_back();
        }
    };

private:
    FunctionLiteral *code_;
    Type type_;
    EsCallFrame &frame_;

    std::vector<Scope> scopes_;

    bool is_in_iteration() const;
    bool is_in_switch() const;

    bool expand_ref_get(const EsReferenceOrValue &any, EsValue &value);
    bool expand_ref_put(const EsReferenceOrValue &any, const EsValue &value);

    void parse_fun_decls(const DeclarationVector &decls);

private:
    virtual Completion parse_binary_expr(BinaryExpression *expr) OVERRIDE;
    virtual Completion parse_unary_expr(UnaryExpression *expr) OVERRIDE;
    virtual Completion parse_assign_expr(AssignmentExpression *expr) OVERRIDE;
    virtual Completion parse_cond_expr(ConditionalExpression *expr) OVERRIDE;
    virtual Completion parse_prop_expr(PropertyExpression *expr) OVERRIDE;
    virtual Completion parse_call_expr(CallExpression *expr) OVERRIDE;
    virtual Completion parse_call_new_expr(CallNewExpression *expr) OVERRIDE;
    virtual Completion parse_regular_expr(RegularExpression *expr) OVERRIDE;
    virtual Completion parse_fun_expr(FunctionExpression *expr) OVERRIDE;

    virtual Completion parse_this_lit(ThisLiteral *lit) OVERRIDE;
    virtual Completion parse_ident_lit(IdentifierLiteral *lit) OVERRIDE;
    virtual Completion parse_null_lit(NullLiteral *lit) OVERRIDE;
    virtual Completion parse_bool_lit(BoolLiteral *lit) OVERRIDE;
    virtual Completion parse_num_lit(NumberLiteral *lit) OVERRIDE;
    virtual Completion parse_str_lit(StringLiteral *lit) OVERRIDE;
    virtual Completion parse_fun_lit(FunctionLiteral *lit) OVERRIDE;
    virtual Completion parse_var_lit(VariableLiteral *lit) OVERRIDE;
    virtual Completion parse_array_lit(ArrayLiteral *lit) OVERRIDE;
    virtual Completion parse_obj_lit(ObjectLiteral *lit) OVERRIDE;
    virtual Completion parse_nothing_lit(NothingLiteral *lit) OVERRIDE;

    virtual Completion parse_empty_stmt(EmptyStatement *stmt) OVERRIDE;
    virtual Completion parse_expr_stmt(ExpressionStatement *stmt) OVERRIDE;
    virtual Completion parse_block_stmt(BlockStatement *stmt) OVERRIDE;
    virtual Completion parse_if_stmt(IfStatement *stmt) OVERRIDE;
    virtual Completion parse_do_while_stmt(DoWhileStatement *stmt) OVERRIDE;
    virtual Completion parse_while_stmt(WhileStatement *stmt) OVERRIDE;
    virtual Completion parse_for_in_stmt(ForInStatement *stmt) OVERRIDE;
    virtual Completion parse_for_stmt(ForStatement *stmt) OVERRIDE;
    virtual Completion parse_cont_stmt(ContinueStatement *stmt) OVERRIDE;
    virtual Completion parse_break_stmt(BreakStatement *stmt) OVERRIDE;
    virtual Completion parse_ret_stmt(ReturnStatement *stmt) OVERRIDE;
    virtual Completion parse_with_stmt(WithStatement *stmt) OVERRIDE;
    virtual Completion parse_switch_stmt(SwitchStatement *stmt) OVERRIDE;
    virtual Completion parse_throw_stmt(ThrowStatement *stmt) OVERRIDE;
    virtual Completion parse_try_stmt(TryStatement *stmt) OVERRIDE;
    virtual Completion parse_dbg_stmt(DebuggerStatement *stmt) OVERRIDE;

public:
    Evaluator(FunctionLiteral *code, Type type, EsCallFrame &frame);
    bool exec(EsContext *ctx);
};
