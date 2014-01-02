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
#include "analyzer.hh"

namespace ir {

void Analyzer::reset()
{
    functions_.clear();
}

void Analyzer::visit_fun(parser::FunctionLiteral *lit)
{
    AnalyzedFunctionMap::iterator it_fun = functions_.find(lit);
    if (it_fun == functions_.end())
    {
        functions_.insert(std::make_pair(lit, AnalyzedFunction(lit)));
        it_fun = functions_.find(lit);

        assert(it_fun != functions_.end());
    }

    AnalyzedFunction &fun = it_fun->second;

    int prm_index = 0;
    StringVector::const_iterator it_prm;
    for (it_prm = lit->parameters().begin(); it_prm != lit->parameters().end(); ++it_prm, ++prm_index)
    {
        // We'll always use the last declared parameter if multiple parameters
        // exist with the same name.
        AnalyzedVariable *var = new (GC)AnalyzedVariable(*it_prm, prm_index);
        fun.add_variable(var);

        // If the function needs an arguments object we must store the
        // parameters in the extra space. The reason is that the arguments
        // object may be referenced outside the function scope, and since the
        // arguments object refers to actual arguments using pointers we must
        // make sure they're stored on the heap.
        if (lit->needs_args_obj())
            var->allocate_to(AnalyzedVariable::STORAGE_LOCAL_EXTRA);
    }

    if (lit->type() == parser::FunctionLiteral::TYPE_EXPRESSION &&
        !lit->name().empty())
    {
        AnalyzedVariable *var = new (GC)AnalyzedVariable(lit->name());
        fun.add_variable(var);
    }

    // In the first pass we only gather and setup the variables.
    parser::DeclarationVector::const_iterator it_decl;
    for (it_decl = lit->declarations().begin(); it_decl != lit->declarations().end(); ++it_decl)
    {
        const parser::Declaration *decl = *it_decl;
        if (!decl->is_variable())
            continue;

        AnalyzedVariable *var = new (GC)AnalyzedVariable(*it_decl);
        fun.add_variable(var);
    }

    for (it_decl = lit->declarations().begin(); it_decl != lit->declarations().end(); ++it_decl)
    {
        const parser::Declaration *decl = *it_decl;
        if (!decl->is_function())
            continue;

        AnalyzedVariable *var = new (GC)AnalyzedVariable(*it_decl);
        fun.add_variable(var);
    }

    // In the second pass we visit the declarations.
    for (it_decl = lit->declarations().begin(); it_decl != lit->declarations().end(); ++it_decl)
    {
        const parser::Declaration *decl = *it_decl;
        if (!decl->is_variable())
            continue;

        visit(decl->as_variable());
    }

    for (it_decl = lit->declarations().begin(); it_decl != lit->declarations().end(); ++it_decl)
    {
        const parser::Declaration *decl = *it_decl;
        if (!decl->is_function())
            continue;

        visit(decl->as_function());
    }

    // Function body.
    parser::StatementVector::const_iterator it_stmt;
    for (it_stmt = lit->body().begin(); it_stmt != lit->body().end(); ++it_stmt)
        visit(*it_stmt);
}

void Analyzer::visit_binary_expr(parser::BinaryExpression *expr)
{
    visit(expr->left());

    switch (expr->operation())
    {
        case parser::BinaryExpression::COMMA:
        {
            visit(expr->right());
            break;
        }

        // Arithmetic.
        case parser::BinaryExpression::MUL:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::DIV:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::MOD:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::ADD:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::SUB:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::LS:  // <<
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::RSS: // >>
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::RUS: // >>>
        {
            visit(expr->right());
            break;
        }

        // Relational.
        case parser::BinaryExpression::LT:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::GT:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::LTE:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::GTE:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::IN:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::INSTANCEOF:
        {
            visit(expr->right());
            break;
        }

        // Equality.
        case parser::BinaryExpression::EQ:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::NEQ:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::STRICT_EQ:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::STRICT_NEQ:
        {
            visit(expr->right());
            break;
        }

        // Bitwise.
        case parser::BinaryExpression::BIT_AND:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::BIT_XOR:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::BIT_OR:
        {
            visit(expr->right());
            break;
        }

        // Logical.
        case parser::BinaryExpression::LOG_AND:
        {
            visit(expr->right());
            break;
        }
        case parser::BinaryExpression::LOG_OR:
        {
            visit(expr->right());
            break;
        }

        default:
            assert(false);
            break;
    }
}

void Analyzer::visit_unary_expr(parser::UnaryExpression *expr)
{
    visit(expr->expression());
    switch (expr->operation())
    {
        case parser::UnaryExpression::DELETE:
            break;
        case parser::UnaryExpression::VOID:
            break;
        case parser::UnaryExpression::TYPEOF:
            break;
        case parser::UnaryExpression::PRE_INC:
            break;
        case parser::UnaryExpression::PRE_DEC:
            break;
        case parser::UnaryExpression::POST_INC:
            break;
        case parser::UnaryExpression::POST_DEC:
            break;
        case parser::UnaryExpression::PLUS:
            break;
        case parser::UnaryExpression::MINUS:
            break;
        case parser::UnaryExpression::BIT_NOT:
            break;
        case parser::UnaryExpression::LOG_NOT:
            break;

        default:
            assert(false);
            break;
    }
}

void Analyzer::visit_assign_expr(parser::AssignmentExpression *expr)
{
    visit(expr->lhs());
    visit(expr->rhs());

    switch (expr->operation())
    {
        case parser::AssignmentExpression::ASSIGN:
            break;
        case parser::AssignmentExpression::ASSIGN_ADD:
            break;
        case parser::AssignmentExpression::ASSIGN_SUB:
            break;
        case parser::AssignmentExpression::ASSIGN_MUL:
            break;
        case parser::AssignmentExpression::ASSIGN_MOD:
            break;
        case parser::AssignmentExpression::ASSIGN_LS:
            break;
        case parser::AssignmentExpression::ASSIGN_RSS:
            break;
        case parser::AssignmentExpression::ASSIGN_RUS:
            break;
        case parser::AssignmentExpression::ASSIGN_BIT_AND:
            break;
        case parser::AssignmentExpression::ASSIGN_BIT_OR:
            break;
        case parser::AssignmentExpression::ASSIGN_BIT_XOR:
            break;
        case parser::AssignmentExpression::ASSIGN_DIV:
            break;
            
        default:
            assert(false);
            break;
    }
}

void Analyzer::visit_cond_expr(parser::ConditionalExpression *expr)
{
    visit(expr->condition());
    visit(expr->left());
    visit(expr->right());
}

void Analyzer::visit_prop_expr(parser::PropertyExpression *expr)
{
    visit(expr->key());

    visit(expr->object());
}

void Analyzer::visit_call_expr(parser::CallExpression *expr)
{
    parser::ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
        visit(*it);

    visit(expr->expression());
}

void Analyzer::visit_call_new_expr(parser::CallNewExpression *expr)
{
    parser::ExpressionVector::const_iterator it;
    for (it = expr->arguments().begin(); it != expr->arguments().end(); ++it)
        visit(*it);

    visit(expr->expression());
}

void Analyzer::visit_regular_expr(parser::RegularExpression *expr)
{
}

void Analyzer::visit_fun_expr(parser::FunctionExpression *expr)
{
    visit(const_cast<parser::FunctionLiteral *>(expr->function()));
}

void Analyzer::visit_this_lit(parser::ThisLiteral *lit)
{
}

void Analyzer::visit_ident_lit(parser::IdentifierLiteral *lit)
{
    assert(!lex_envs_.empty());
    LexicalEnvironment &cur_lex_env = lex_envs_.back();

    // Check for eval taint.
    if (lit->value() == String("eval"))
    {
        LexicalEnvironmentVector::reverse_iterator it = lex_envs_.rbegin();
        for (; it != lex_envs_.rend(); ++it)
        {
            LexicalEnvironment &context = *it;

            AnalyzedFunctionMap::iterator it_fun = functions_.find(context.function());
            assert(it_fun != functions_.end());

            it_fun->second.set_tainted_by_eval(true);
        }
    }

    // Update the identifier lookup map.
    bool found_obj_env = false;
    int hops = 0;

    LexicalEnvironmentVector::reverse_iterator it = lex_envs_.rbegin();
    for (; it != lex_envs_.rend(); ++it, hops++)
    {
        LexicalEnvironment &context = *it;
        if (context.is_obj())
            found_obj_env = true;

        AnalyzedFunctionMap::iterator it_fun = functions_.find(context.function());
        assert(it_fun != functions_.end());

        AnalyzedVariable *var = it_fun->second.find_variable(lit->value());
        if (var)
        {
            // The variable can be allocated locally only if the following is
            // true:
            //  * The variable is declared in the current context.
            //  * The current context is a declarative context.
            //  * The variable is not accessed from another context.
            if (found_obj_env)
            {
                var->allocate_to(AnalyzedVariable::STORAGE_CONTEXT);
                return;
            }
            else if (it == lex_envs_.rbegin())
            {
                if (!var->is_allocated())
                {
                    var->allocate_to(AnalyzedVariable::STORAGE_LOCAL);
                    return;
                }
            }
            else
            {
                if (var->storage() != AnalyzedVariable::STORAGE_CONTEXT)
                {
                    var->allocate_to(AnalyzedVariable::STORAGE_LOCAL_EXTRA);
                    lookup(cur_lex_env.function())->link_referenced_scope(hops);
                    return;
                }
            }
        }
    }
}

void Analyzer::visit_null_lit(parser::NullLiteral *lit)
{
}

void Analyzer::visit_bool_lit(parser::BoolLiteral *lit)
{
}

void Analyzer::visit_num_lit(parser::NumberLiteral *lit)
{
}

void Analyzer::visit_str_lit(parser::StringLiteral *lit)
{
}

void Analyzer::visit_fun_lit(parser::FunctionLiteral *lit)
{
    lex_envs_.push_back(LexicalEnvironment(LexicalEnvironment::TYPE_DECLARATIVE, lit));

    visit_fun(lit);

    lex_envs_.pop_back();
}

void Analyzer::visit_var_lit(parser::VariableLiteral *lit)
{
    // Dealt with in visit_fun().
}

void Analyzer::visit_array_lit(parser::ArrayLiteral *lit)
{
    parser::ExpressionVector::const_iterator it;
    for (it = lit->values().begin(); it != lit->values().end(); ++it)
        visit(*it);
}

void Analyzer::visit_obj_lit(parser::ObjectLiteral *lit)
{
    parser::ObjectLiteral::PropertyVector::const_iterator it;
    for (it = lit->properties().begin(); it != lit->properties().end(); ++it)
    {
        const parser::ObjectLiteral::Property *prop = *it;
        
        if (prop->type() == parser::ObjectLiteral::Property::DATA)
        {
            visit(prop->key());
            visit(prop->value());
        }
        else
        {
            visit(prop->value());
        }
    }
}

void Analyzer::visit_nothing_lit(parser::NothingLiteral *lit)
{
}

void Analyzer::visit_empty_stmt(parser::EmptyStatement *stmt)
{
}

void Analyzer::visit_expr_stmt(parser::ExpressionStatement *stmt)
{
    visit(stmt->expression());
}

void Analyzer::visit_block_stmt(parser::BlockStatement *stmt)
{
    parser::StatementVector::const_iterator it_stmt;
    for (it_stmt = stmt->body().begin(); it_stmt != stmt->body().end(); ++it_stmt)
        visit(*it_stmt);
}

void Analyzer::visit_if_stmt(parser::IfStatement *stmt)
{
    visit(stmt->condition());
    visit(stmt->if_statement());

    if (stmt->has_else())
        visit(stmt->else_statement());
}

void Analyzer::visit_do_while_stmt(parser::DoWhileStatement *stmt)
{
    visit(stmt->body());
    
    if (stmt->has_condition())
        visit(stmt->condition());
}

void Analyzer::visit_while_stmt(parser::WhileStatement *stmt)
{
    visit(stmt->condition());
    visit(stmt->body());
}

void Analyzer::visit_for_in_stmt(parser::ForInStatement *stmt)
{
    visit(stmt->enumerable());
    visit(stmt->declaration());
    visit(stmt->body());
}

void Analyzer::visit_for_stmt(parser::ForStatement *stmt)
{
    if (stmt->has_initializer())
        visit(stmt->initializer());

    if (stmt->has_condition())
        visit(stmt->condition());

    visit(stmt->body());

    if (stmt->has_next())
        visit(stmt->next());
}

void Analyzer::visit_cont_stmt(parser::ContinueStatement *stmt)
{
}

void Analyzer::visit_break_stmt(parser::BreakStatement *stmt)
{
}

void Analyzer::visit_ret_stmt(parser::ReturnStatement *stmt)
{
    if (stmt->has_expression())
        visit(stmt->expression());
}

void Analyzer::visit_with_stmt(parser::WithStatement *stmt)
{
    assert(!lex_envs_.empty());
    LexicalEnvironment &cur_lex_env = lex_envs_.back();

    lex_envs_.push_back(LexicalEnvironment(LexicalEnvironment::TYPE_OBJECT, cur_lex_env.function()));

    visit(stmt->expression());

    visit(stmt->body());

    lex_envs_.pop_back();
}

void Analyzer::visit_switch_stmt(parser::SwitchStatement *stmt)
{
    visit(stmt->expression());

    parser::SwitchStatement::CaseClauseVector::const_iterator it;
    for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
    {
        const parser::SwitchStatement::CaseClause *clause = *it;

        if (!clause->is_default())
            visit(clause->label());

        parser::StatementVector::const_iterator it_stmt;
        for (it_stmt = clause->body().begin(); it_stmt != clause->body().end(); ++it_stmt)
            visit(*it_stmt);
    }

    for (it = stmt->cases().begin(); it != stmt->cases().end(); ++it)
    {
        const parser::SwitchStatement::CaseClause *clause = *it;

        if (clause->is_default())
        {
            parser::StatementVector::const_iterator it_stmt;
            for (it_stmt = clause->body().begin(); it_stmt != clause->body().end(); ++it_stmt)
                visit(*it_stmt);
        }
    }
}

void Analyzer::visit_throw_stmt(parser::ThrowStatement *stmt)
{
    visit(stmt->expression());
}

void Analyzer::visit_try_stmt(parser::TryStatement *stmt)
{
    visit(stmt->try_block());

    if (stmt->has_catch_block())
        visit(stmt->catch_block());

    if (stmt->has_finally_block())
        visit(stmt->finally_block());
}

void Analyzer::visit_dbg_stmt(parser::DebuggerStatement *stmt)
{
}

AnalyzedFunction *Analyzer::lookup(parser::FunctionLiteral *fun)
{
    AnalyzedFunctionMap::iterator it = functions_.find(fun);
    if (it == functions_.end())
        return NULL;

    return &it->second;
}

void Analyzer::analyze(parser::FunctionLiteral *root)
{
    assert(root);
    reset();

    lex_envs_.push_back(LexicalEnvironment(LexicalEnvironment::TYPE_OBJECT, root));

    visit_fun(root);

    // Some unallocated variables might need to be allocated to the context.
    AnalyzedFunctionMap::iterator it_fun;
    for (it_fun = functions_.begin(); it_fun != functions_.end(); ++it_fun)
    {
        AnalyzedFunction &fun = it_fun->second;

        // We don't want to allocate unused variables if they're never
        // accessed by eval. The exception is the global scope since such
        // "variables" might be dynamically enumerated.
        bool is_global = fun.literal() == root;
        if (!is_global && !fun.tainted_by_eval())
            continue;

        AnalyzedVariableSet::iterator it_var;
        for (it_var = fun.variables().begin(); it_var != fun.variables().end(); ++it_var)
        {
            AnalyzedVariable *var = *it_var;
            if (!var->is_allocated())
                var->allocate_to(AnalyzedVariable::STORAGE_CONTEXT);
        }
    }

    lex_envs_.pop_back();
}

}
