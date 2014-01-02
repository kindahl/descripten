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
#include <ostream>
#include "common/core.hh"
#include "ast.hh"
#include "visitor.hh"

namespace parser {

class Printer : public Visitor
{
private:
    std::ostream &out_;

    bool indent_enabled_;
    std::string indent_;

    inline void inc_indent()
    {
        indent_ += "    ";
    }

    inline void dec_indent()
    {
        assert(indent_.size() > 3);
        indent_.resize(indent_.size() - 4);
    }

    inline std::string indent()
    {
        if (!indent_enabled_)
            return "";

        return indent_;
    }

    inline std::string new_line()
    {
        if (!indent_enabled_)
            return " ";

        return "\n";
    }

public:
    Printer(std::ostream &out);
    virtual ~Printer();

    virtual void visit_binary_expr(BinaryExpression *expr) OVERRIDE;
    virtual void visit_unary_expr(UnaryExpression *expr) OVERRIDE;
    virtual void visit_assign_expr(AssignmentExpression *expr) OVERRIDE;
    virtual void visit_cond_expr(ConditionalExpression *expr) OVERRIDE;
    virtual void visit_prop_expr(PropertyExpression *expr) OVERRIDE;
    virtual void visit_call_expr(CallExpression *expr) OVERRIDE;
    virtual void visit_call_new_expr(CallNewExpression *expr) OVERRIDE;
    virtual void visit_regular_expr(RegularExpression *expr) OVERRIDE;
    virtual void visit_fun_expr(FunctionExpression *expr) OVERRIDE;

    virtual void visit_this_lit(ThisLiteral *lit) OVERRIDE;
    virtual void visit_ident_lit(IdentifierLiteral *lit) OVERRIDE;
    virtual void visit_null_lit(NullLiteral *lit) OVERRIDE;
    virtual void visit_bool_lit(BoolLiteral *lit) OVERRIDE;
    virtual void visit_num_lit(NumberLiteral *lit) OVERRIDE;
    virtual void visit_str_lit(StringLiteral *lit) OVERRIDE;
    virtual void visit_fun_lit(FunctionLiteral *lit) OVERRIDE;
    virtual void visit_var_lit(VariableLiteral *lit) OVERRIDE;
    virtual void visit_array_lit(ArrayLiteral *lit) OVERRIDE;
    virtual void visit_obj_lit(ObjectLiteral *lit) OVERRIDE;
    virtual void visit_nothing_lit(NothingLiteral *lit) OVERRIDE;

    virtual void visit_empty_stmt(EmptyStatement *stmt) OVERRIDE;
    virtual void visit_expr_stmt(ExpressionStatement *stmt) OVERRIDE;
    virtual void visit_block_stmt(BlockStatement *stmt) OVERRIDE;
    virtual void visit_if_stmt(IfStatement *stmt) OVERRIDE;
    virtual void visit_do_while_stmt(DoWhileStatement *stmt) OVERRIDE;
    virtual void visit_while_stmt(WhileStatement *stmt) OVERRIDE;
    virtual void visit_for_in_stmt(ForInStatement *stmt) OVERRIDE;
    virtual void visit_for_stmt(ForStatement *stmt) OVERRIDE;
    virtual void visit_cont_stmt(ContinueStatement *stmt) OVERRIDE;
    virtual void visit_break_stmt(BreakStatement *stmt) OVERRIDE;
    virtual void visit_ret_stmt(ReturnStatement *stmt) OVERRIDE;
    virtual void visit_with_stmt(WithStatement *stmt) OVERRIDE;
    virtual void visit_switch_stmt(SwitchStatement *stmt) OVERRIDE;
    virtual void visit_throw_stmt(ThrowStatement *stmt) OVERRIDE;
    virtual void visit_try_stmt(TryStatement *stmt) OVERRIDE;
    virtual void visit_dbg_stmt(DebuggerStatement *stmt) OVERRIDE;
};

}
