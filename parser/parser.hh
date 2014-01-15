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
#include <map>
#include <vector>
#include <gc/gc_allocator.h>
#include "ast.hh"
#include "exception.hh"
#include "lexer.hh"

namespace parser {

///< Name of root FunctionLiteral node.
#define PARSER_FUN_NAME_ROOT            "(root)"

class Parser
{
public:
    /**
     * @brief Types of code.
     */
    enum Code
    {
        ///< Program code.
        CODE_PROGRAM,
        ///< Function code.
        CODE_FUNCTION,
        ///< Eval code.
        CODE_EVAL
    };

private:
    class Scope
    {
    private:
        FunctionLiteral *fun_;
        Code code_;

        std::map<String, Declaration *,
                 std::less<String>,
                 gc_allocator<std::pair<String, Declaration *> > > decl_;   ///< Map for defining what's currently in scope.

    public:
        Scope(FunctionLiteral *fun, Code code)
            : fun_(fun)
            , code_(code) {}

        FunctionLiteral *function();

        void set_strict_mode(bool strict_mode);
        bool is_strict_mode() const;

        bool is_program_scope() const;
        bool is_function_scope() const;
        bool is_eval_scope() const;

        void push_back(Statement *stmt);
        void push_decl(String name, Declaration *decl);
    };

    std::vector<Scope *, gc_allocator<Scope *> > scopes_;

    void enter_scope(FunctionLiteral *fun, Code code = CODE_FUNCTION);
    void leave_scope();
    Scope *scope();

private:
    class TargetScope
    {
    public:
        static LabeledStatement *BARRIER;

    private:
        Parser *parser_;

    public:
        TargetScope(Parser *parser, LabeledStatement *stmt)
            : parser_(parser)
        {
            assert(parser_);
            parser_->targets_.push_back(stmt);
        }

        ~TargetScope()
        {
            assert(parser_);
            assert(!parser_->targets_.empty());
            parser_->targets_.pop_back();
        }
    };

    LabeledStatementVector targets_;

    /**
     * Find the labeled target statement given a label.
     * @note Labeled statements may carry multiple labels, only one is
     *       needed for finding the target.
     * @param [in] label Label of the target statement.
     * @return If found, the target statement, otherwise a NULL pointer is
     *         returned.
     */
    const LabeledStatement *find_target(String label) const;

    /**
     * Find the next anonymous target statement. For example, this is useful
     * when finding the target of an unlabeled continue and break statements.
     * @return If found, the target statement, otherwise a NULL pointer is
     *         returned.
     */
    const LabeledStatement *find_target() const;

private:
    Lexer &lexer_;      ///< Lexer used for parsing.
    Code code_;         ///< Type of code being parsed.
    bool strict_mode_;  ///< Is parsed code in strict mode.

    inline void expect(Token::Type expected_tok)
    {
        Token tok = lexer_.next();
        if (tok != expected_tok)
        {
            THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected '%S'.",
                                                         tok.string().data(),
                                                         Token::description(expected_tok).data()));
        }
    }

    inline void expect_semi()
    {
        // Implements automatic semicolon insertion according to 7.9.
        Token tmp = lexer_.peek();
        if (tmp == Token::SEMI)
        {
            lexer_.next();
            return;
        }

        if (tmp.is_separated_by_line_term() ||
            tmp == Token::RBRACE ||
            tmp == Token::EOI)
        {
            return;
        }

        expect(Token::SEMI);
    }

    inline bool next_if(Token::Type tok)
    {
        if (lexer_.peek() == tok)
        {
            lexer_.next();
            return true;
        }

        return false;
    }

    inline bool is_identifier(Token::Type tok)
    {
        return tok == Token::LIT_IDENTIFIER ||
               (!scope()->is_strict_mode() && tok == Token::FUTURE_STRICT_RESERVED_WORD);
    }

    FunctionLiteral *parse_program();

    /**
     * Parses a sequence of source elements and adds the to the current scope.
     * @pre scope() returns the current scope that the source elements
     *      should be added to.
     */
    void parse_source_elements(Token::Type break_tok);
    Statement *parse_source_element(LabelList &labels);
    Statement *parse_fun_decl();

    Statement *parse_stmt(LabelList &labels);
    Statement *parse_block_stmt(LabelList &labels);
    /**
     * @param [out] name If the function parsed exactly one variable, the name
     *                   of that variable will be written to this parameter.
     */
    Statement *parse_var_decl(bool no_in, String &var_name);
    Statement *parse_var_stmt();
    Statement *parse_empty_stmt();
    Statement *parse_if_stmt(LabelList &labels);
    Statement *parse_do_while_stmt(LabelList &labels);
    Statement *parse_while_stmt(LabelList &labels);
    Statement *parse_for_stmt(LabelList &labels);
    Statement *parse_continue_stmt();
    Statement *parse_break_stmt(LabelList &labels);
    Statement *parse_return_stmt();
    Statement *parse_with_stmt(LabelList &labels);
    Statement *parse_switch_stmt(LabelList &labels);
    SwitchStatement::CaseClause *parse_switch_case_clause();
    Statement *parse_throw_stmt();
    Statement *parse_try_stmt(LabelList &labels);
    Statement *parse_debugger_stmt();
    Statement *parse_expr_or_labeled_stmt(LabelList &labels);

    String parse_identifier_str(bool strict_mode);
    String parse_identifier_name_str();
    Expression *parse_identifier(bool strict_mode);
    Expression *parse_identifier_name();

    Expression *parse_reg_exp_lit();
    Expression *parse_array_lit();
    ObjectLiteral::Property *parse_obj_lit_get_set(ObjectLiteral *obj, bool is_setter);
    Expression *parse_obj_lit();
    Expression *parse_fun_lit(String name, int beg_pos);

    Expression *parse_expr(bool no_in);
    Expression *parse_assignment_expr(bool no_in);
    Expression *parse_cond_expr(bool no_in);
    Expression *parse_prim_expr();
    ExpressionVector parse_args_expr();
    Expression *parse_fun_expr();
    Expression *parse_member_with_new_pfx_expr(std::vector<int, gc_allocator<int> > &stack);
    Expression *parse_new_pfx_expr(std::vector<int, gc_allocator<int> > &stack);
    Expression *parse_new_expr();
    Expression *parse_member_expr();
    Expression *parse_lhs_expr();
    Expression *parse_unary_expr();
    Expression *parse_binary_expr(bool no_in, int prec);

public:
    Parser(Lexer &lexer, Code code = CODE_PROGRAM, bool strict_mode = false);

    FunctionLiteral *parse();
};

}
