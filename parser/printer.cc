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
#include "printer.hh"

namespace parser {

Printer::Printer(std::ostream &out)
    : out_(out)
    , indent_enabled_(true)
{
}

Printer::~Printer()
{
}

void Printer::visit_binary_expr(BinaryExpression *expr)
{
    static const char *op[] =
    {
        ",",
        // Arithmetic.
        "*", "/", "%", "+", "-", "<<", ">>", ">>>",
        // Relational.
        ",", ">", "<=", ">=", "in", "instanceof",
        // Equality.
        "==", "!=", "===", "!==",
        // Bitwise.
        "&", "^", "|",
        // Logical.
        "&&", "||"
    };

    assert(expr->operation() <= 24);

    out_ << "(";
    visit(expr->left());
    out_ << " " << op[expr->operation()] << " ";
    visit(expr->right());
    out_ << ")";
}

void Printer::visit_unary_expr(UnaryExpression *expr)
{
    static const char *op[] =
    {
        "delete ", "void ", "typeof ",
        "++", "--", "++", "--", "+", "-", "~", "!"
    };

    assert(expr->operation() <= 11);

    if (expr->operation() != UnaryExpression::POST_INC &&
        expr->operation() != UnaryExpression::POST_DEC)
    {
        out_ << op[expr->operation()];
        visit(expr->expression());
    }
    else
    {
        visit(expr->expression());
        out_ << op[expr->operation()];
    }
}

void Printer::visit_assign_expr(AssignmentExpression *expr)
{
    static const char *op[] =
    {
        "=", "+=", "-=", "*=", "%=", "<<=", ">>=", ">>>=", "&=", "|=", "^=", "/="
    };

    assert(expr->operation() <= 12);

    visit(expr->lhs());
    out_ << " " << op[expr->operation()] << " ";
    visit(expr->rhs());
}

void Printer::visit_cond_expr(ConditionalExpression *expr)
{
    visit(expr->condition());
    out_ << " ? ";
    visit(expr->left());
    out_ << " : ";
    visit(expr->right());
}

void Printer::visit_prop_expr(PropertyExpression *expr)
{
    visit(expr->object());
    out_ << "[";
    visit(expr->key());
    out_ << "]";
}

void Printer::visit_call_expr(CallExpression *expr)
{
    visit(expr->expression());

    out_ << "(";
    ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        if (it != expr->arguments().begin())
            out_ << ", ";

        visit(*it);
    }
    out_ << ")";
}

void Printer::visit_call_new_expr(CallNewExpression *expr)
{
    out_ << "new (";
    visit(expr->expression());
    out_ << ")";

    out_ << "(";
    ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
    {
        if (it != expr->arguments().begin())
            out_ << ", ";

        visit(*it);
    }
    out_ << ")";
}

void Printer::visit_regular_expr(RegularExpression *expr)
{
    out_ << expr->as_string().utf8();
}

void Printer::visit_fun_expr(FunctionExpression *expr)
{
    if (!expr->function()->name().empty())
        out_ << expr->function()->name().utf8();
    else
        out_ << "<anonymous>";
}

void Printer::visit_this_lit(ThisLiteral *lit)
{
    out_ << "this";
}

void Printer::visit_ident_lit(IdentifierLiteral *lit)
{
    out_ << lit->value().utf8();
}

void Printer::visit_null_lit(NullLiteral *lit)
{
    out_ << "null";
}

void Printer::visit_bool_lit(BoolLiteral *lit)
{
    out_ << (lit->value() ? "true" : "false");
}

void Printer::visit_num_lit(NumberLiteral *lit)
{
    out_ << lit->as_string().utf8();
}

void Printer::visit_str_lit(StringLiteral *lit)
{
    out_ << "'" << lit->value().utf8() << "'";
}

void Printer::visit_fun_lit(FunctionLiteral *lit)
{
    out_ << indent() << "function";

    if (!lit->name().empty())
        out_ << " " << lit->name().utf8();

    out_ << "(";
    StringVector::const_iterator it_prm;
    for (it_prm = lit->parameters().begin(); it_prm != lit->parameters().end(); ++it_prm)
    {
        if (it_prm != lit->parameters().begin())
            out_ << ", ";

        out_ << (*it_prm).utf8();
    }
    out_ << ") " << new_line();

    out_ << indent() << "{" << new_line();
    inc_indent();

    DeclarationVector::const_iterator it_decl;
    for (it_decl = lit->declarations().begin(); it_decl != lit->declarations().end(); ++it_decl)
    {
        const Declaration *decl = *it_decl;
        if (decl->is_function())
            visit(decl->as_function());
        else if (decl->is_variable())
            visit(decl->as_variable());
        else
        {
            assert(false);
        }
    }

    StatementVector::const_iterator it_stmt;
    for (it_stmt = lit->body().begin(); it_stmt != lit->body().end(); ++it_stmt)
        visit(*it_stmt);

    dec_indent();
    out_ << indent() << "}" << new_line();
}

void Printer::visit_var_lit(VariableLiteral *lit)
{
    out_ << indent() << "var ";
    out_ << lit->name().utf8();
    out_ << ";" << new_line();
}

void Printer::visit_array_lit(ArrayLiteral *lit)
{
    out_ << "[";

    ExpressionVector::const_iterator it;
    for (it = lit->values().begin(); it != lit->values().end(); ++it)
    {
        if (it != lit->values().begin())
            out_ << ", ";
        visit(*it);
    }

    out_ << "]";
}

void Printer::visit_obj_lit(ObjectLiteral *lit)
{
    out_ << "{";

    ObjectLiteral::PropertyVector::const_iterator it;
    for (it = lit->properties().begin(); it != lit->properties().end(); ++it)
    {
        const ObjectLiteral::Property *prop = *it;

        if (it != lit->properties().begin())
            out_ << ", ";

        if (prop->type() == ObjectLiteral::Property::DATA)
        {
            visit(prop->key());
            out_ << ": ";
            visit(prop->value());
        }
        else
        {
            visit(prop->value());
        }
    }

    out_ << "}";
}

void Printer::visit_nothing_lit(NothingLiteral *lit)
{
}

void Printer::visit_empty_stmt(EmptyStatement *stmt)
{
    out_ << indent() << ";" << new_line();
}

void Printer::visit_expr_stmt(ExpressionStatement *stmt)
{
    out_ << indent();
    visit(stmt->expression());
    out_ << ";" << new_line();
}

void Printer::visit_block_stmt(BlockStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "{" << new_line();
    inc_indent();

    StatementVector::const_iterator it_stmt;
    for (it_stmt = stmt->body().begin(); it_stmt != stmt->body().end(); ++it_stmt)
        visit(*it_stmt);

    dec_indent();
    out_ << indent() << "}" << new_line();
}

void Printer::visit_if_stmt(IfStatement *stmt)
{
    out_ << indent() << "if (";
    visit(stmt->condition());
    out_ << ")" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->if_statement()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->if_statement());
    if (do_indent)
        dec_indent();

    if (stmt->has_else())
    {
        bool do_indent = dynamic_cast<BlockStatement *>(stmt->else_statement()) == NULL;
        if (do_indent)
            inc_indent();
        visit(stmt->else_statement());
        if (do_indent)
            dec_indent();
    }
}

void Printer::visit_do_while_stmt(DoWhileStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "do" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->body()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->body());
    if (do_indent)
        dec_indent();

    out_ << indent() << "while (";

    visit(stmt->condition());

    out_ << ");" << new_line();
}

void Printer::visit_while_stmt(WhileStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "while (";
    visit(stmt->condition());
    out_ << ")" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->body()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->body());
    if (do_indent)
        dec_indent();
}

void Printer::visit_for_in_stmt(ForInStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "for (";
    visit(stmt->declaration());
    out_ << " in ";
    visit(stmt->enumerable());
    out_ << ")" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->body()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->body());
    if (do_indent)
        dec_indent();
}

void Printer::visit_for_stmt(ForStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "for (";
    visit(stmt->initializer());
    out_ << "; ";
    visit(stmt->condition());
    out_ << "; ";
    visit(stmt->next());
    out_ << ")" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->body()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->body());
    if (do_indent)
        dec_indent();
}

void Printer::visit_cont_stmt(ContinueStatement *stmt)
{
    out_ << indent() << "continue";

    if (stmt->has_target())
        out_ << " " << stmt->target()->labels().first().utf8();     // Any label will do.

    out_ << ";" << new_line();
}

void Printer::visit_break_stmt(BreakStatement *stmt)
{
    out_ << indent() << "break";

    if (stmt->has_target())
        out_ << " " << stmt->target()->labels().first().utf8();     // Any label will do.

    out_ << ";" << new_line();
}

void Printer::visit_ret_stmt(ReturnStatement *stmt)
{
    out_ << indent() << "return";

    if (stmt->has_expression())
    {
        out_ << " ";
        visit(stmt->expression());
    }

    out_ << ";" << new_line();
}

void Printer::visit_with_stmt(WithStatement *stmt)
{
    out_ << indent() << "with (";
    visit(stmt->expression());
    out_ << ")" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->body()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->body());
    if (do_indent)
        dec_indent();
}

void Printer::visit_switch_stmt(SwitchStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "switch (";
    visit(stmt->expression());
    out_ << ")" << new_line();

    out_ << indent() << "{" << new_line();
    inc_indent();

    SwitchStatement::CaseClauseVector::const_iterator it;
    for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
    {
        if (it != stmt->cases().begin())
            out_ << new_line();

        const SwitchStatement::CaseClause *clause = *it;

        if (clause->is_default())
        {
            out_ << "default:" << new_line();
        }
        else
        {
            out_ << "case ";
            visit(clause->label());
            out_ << ":";
        }

        inc_indent();

        StatementVector::const_iterator it_stmt;
        for (it_stmt = clause->body().begin(); it_stmt != clause->body().end(); ++it_stmt)
            visit(*it_stmt);

        dec_indent();
    }

    dec_indent();
    out_ << indent() << "}" << new_line();
}

void Printer::visit_throw_stmt(ThrowStatement *stmt)
{
    out_ << indent() << "throw ";
    visit(stmt->expression());
    out_ << ";" << new_line();
}

void Printer::visit_try_stmt(TryStatement *stmt)
{
    StringVector::const_iterator it_label;
    for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        out_ << (*it_label).utf8() << ": ";

    out_ << indent() << "try" << new_line();

    bool do_indent = dynamic_cast<BlockStatement *>(stmt->try_block()) == NULL;
    if (do_indent)
        inc_indent();
    visit(stmt->try_block());
    if (do_indent)
        dec_indent();

    if (stmt->has_catch_block())
    {
        out_ << indent() << "catch (" << stmt->catch_identifier().utf8() << ")" << new_line();

        do_indent = dynamic_cast<BlockStatement *>(stmt->catch_block()) == NULL;
        if (do_indent)
            inc_indent();
        visit(stmt->catch_block());
        if (do_indent)
            dec_indent();
    }

    if (stmt->has_finally_block())
    {
        out_ << indent() << "finally" << new_line();

        do_indent = dynamic_cast<BlockStatement *>(stmt->finally_block()) == NULL;
        if (do_indent)
            inc_indent();
        visit(stmt->finally_block());
        if (do_indent)
            dec_indent();
    }
}

void Printer::visit_dbg_stmt(DebuggerStatement *stmt)
{
    out_ << "debugger;" << new_line();
}

}
