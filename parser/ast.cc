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

#include "ast.hh"
#include "visitor.hh"

namespace parser {

String Declaration::name() const
{
    return is_function() ? as_function()->name() : as_variable()->name();
}

void BinaryExpression::accept(Visitor *visitor)
{
    visitor->visit_binary_expr(this);
}

void UnaryExpression::accept(Visitor *visitor)
{
    visitor->visit_unary_expr(this);
}

void AssignmentExpression::accept(Visitor *visitor)
{
    visitor->visit_assign_expr(this);
}

void ConditionalExpression::accept(Visitor *visitor)
{
    visitor->visit_cond_expr(this);
}

void PropertyExpression::accept(Visitor *visitor)
{
    visitor->visit_prop_expr(this);
}

void CallExpression::accept(Visitor *visitor)
{
    visitor->visit_call_expr(this);
}

void CallNewExpression::accept(Visitor *visitor)
{
    visitor->visit_call_new_expr(this);
}

void RegularExpression::accept(Visitor *visitor)
{
    visitor->visit_regular_expr(this);
}

void FunctionExpression::accept(Visitor *visitor)
{
    visitor->visit_fun_expr(this);
}

void ThisLiteral::accept(Visitor *visitor)
{
    visitor->visit_this_lit(this);
}

void IdentifierLiteral::accept(Visitor *visitor)
{
    visitor->visit_ident_lit(this);
}

void NullLiteral::accept(Visitor *visitor)
{
    visitor->visit_null_lit(this);
}

void BoolLiteral::accept(Visitor *visitor)
{
    visitor->visit_bool_lit(this);
}

void NumberLiteral::accept(Visitor *visitor)
{
    visitor->visit_num_lit(this);
}

void StringLiteral::accept(Visitor *visitor)
{
    visitor->visit_str_lit(this);
}

void FunctionLiteral::accept(Visitor *visitor)
{
    visitor->visit_fun_lit(this);
}

void VariableLiteral::accept(Visitor *visitor)
{
    visitor->visit_var_lit(this);
}

void ArrayLiteral::accept(Visitor *visitor)
{
    visitor->visit_array_lit(this);
}

void ObjectLiteral::accept(Visitor *visitor)
{
    visitor->visit_obj_lit(this);
}

void NothingLiteral::accept(Visitor *visitor)
{
    visitor->visit_nothing_lit(this);
}

void EmptyStatement::accept(Visitor *visitor)
{
    visitor->visit_empty_stmt(this);
}

void ExpressionStatement::accept(Visitor *visitor)
{
    visitor->visit_expr_stmt(this);
}

void BlockStatement::accept(Visitor *visitor)
{
    visitor->visit_block_stmt(this);
}

void IfStatement::accept(Visitor *visitor)
{
    visitor->visit_if_stmt(this);
}

void DoWhileStatement::accept(Visitor *visitor)
{
    visitor->visit_do_while_stmt(this);
}

void WhileStatement::accept(Visitor *visitor)
{
    visitor->visit_while_stmt(this);
}

void ForInStatement::accept(Visitor *visitor)
{
    visitor->visit_for_in_stmt(this);
}

void ForStatement::accept(Visitor *visitor)
{
    visitor->visit_for_stmt(this);
}

void ContinueStatement::accept(Visitor *visitor)
{
    visitor->visit_cont_stmt(this);
}

void BreakStatement::accept(Visitor *visitor)
{
    visitor->visit_break_stmt(this);
}

void ReturnStatement::accept(Visitor *visitor)
{
    visitor->visit_ret_stmt(this);
}

void WithStatement::accept(Visitor *visitor)
{
    visitor->visit_with_stmt(this);
}

void SwitchStatement::accept(Visitor *visitor)
{
    visitor->visit_switch_stmt(this);
}

void ThrowStatement::accept(Visitor *visitor)
{
    visitor->visit_throw_stmt(this);
}

void TryStatement::accept(Visitor *visitor)
{
    visitor->visit_try_stmt(this);
}

void DebuggerStatement::accept(Visitor *visitor)
{
    visitor->visit_dbg_stmt(this);
}

}
