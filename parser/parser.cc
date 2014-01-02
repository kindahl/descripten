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

#include <set>
#include <gc_cpp.h>
#include "parser.hh"

namespace parser {

LabeledStatement *Parser::TargetScope::BARRIER = NULL;

FunctionLiteral *Parser::Scope::function()
{
    assert(fun_);
    return fun_;
}

void Parser::Scope::set_strict_mode(bool strict_mode)
{
    fun_->set_strict_mode(strict_mode);
}

bool Parser::Scope::is_strict_mode() const
{
    return fun_->is_strict_mode();
}

bool Parser::Scope::is_program_scope() const
{
    return code_ == CODE_PROGRAM;
}

bool Parser::Scope::is_function_scope() const
{
    return code_ == CODE_FUNCTION;
}

bool Parser::Scope::is_eval_scope() const
{
    return code_ == CODE_EVAL;
}

void Parser::Scope::push_back(Statement *stmt)
{
    assert(fun_);
    fun_->push_back(stmt);
}

void Parser::Scope::push_decl(String name, Declaration *decl)
{
    assert(fun_);
    fun_->push_decl(decl);

    decl_[name] = decl;
}

Parser::Parser(Lexer &lexer, Code code, bool strict_mode)
    : lexer_(lexer)
    , code_(code)
    , strict_mode_(strict_mode)
{
}

void Parser::enter_scope(FunctionLiteral *fun, Code code)
{
    assert(fun);

    Scope *scope = new (GC)Scope(fun, code);
    scopes_.push_back(scope);
}

void Parser::leave_scope()
{
    assert(!scopes_.empty());
    scopes_.pop_back();
}

Parser::Scope *Parser::scope()
{
    assert(!scopes_.empty());
    return scopes_.back();
}

const LabeledStatement *Parser::find_target(String label) const
{
    LabeledStatementVector::const_reverse_iterator it;
    for (it = targets_.rbegin(); it != targets_.rend(); ++it)
    {
        const LabeledStatement *stmt = *it;
        if (stmt == TargetScope::BARRIER)
            return NULL;

        StringVector::const_iterator it_label;
        for (it_label = stmt->labels().begin(); it_label != stmt->labels().end(); ++it_label)
        {
            if (*it_label == label)
                return stmt;
        }
    }

    return NULL;
}

const LabeledStatement *Parser::find_target() const
{
    LabeledStatementVector::const_reverse_iterator it;
    for (it = targets_.rbegin(); it != targets_.rend(); ++it)
    {
        const LabeledStatement *stmt = *it;
        if (stmt == TargetScope::BARRIER)
            return NULL;

        return stmt;
    }

    return NULL;
}

FunctionLiteral *Parser::parse_program()
{
    FunctionLiteral *fun = new (GC)FunctionLiteral(Location(), _USTR(PARSER_FUN_NAME_ROOT));
    if (strict_mode_)
        fun->set_strict_mode(true);

    enter_scope(fun, code_);
    parse_source_elements(Token::EOI);
    leave_scope();

    return fun;
}

void Parser::parse_source_elements(Token::Type break_tok)
{
    bool directive_prologue = true;     // Parsing directive prologue.

    TokenVector skipped_tokens;

    while (lexer_.peek() != break_tok)
    {
        if (directive_prologue)
        {
            if (lexer_.peek() == Token::LIT_STRING)
            {
                Token tok = lexer_.next();

                // We only support the 'use strict' directive prologue clause.
                if (!tok.contains_esc_seq() && tok.string() == _USTR("use strict"))
                {
                    scope()->set_strict_mode(true);
                    expect_semi();

                    // We're now in strict mode, we must verify the parameters
                    // and function name that already have been parsed.
                    FunctionLiteral *fun = scope()->function();

                    if (fun->name() == _USTR("eval") || fun->name() == _USTR("arguments"))
                        THROW(ParseException, _USTR("function may not be named 'eval' or 'arguments' in strict mode."));

                    bool found_dup_params = false;
                    StringSet found_params;

                    StringVector::const_iterator it;
                    for (it = fun->parameters().begin(); it != fun->parameters().end(); ++it)
                    {
                        if (*it == _USTR("eval") || *it == _USTR("arguments"))
                            THROW(ParseException, _USTR("function argument may not be named 'eval' or 'arguments' in strict mode."));

                        if (!found_dup_params && found_params.count(*it) != 0)
                            found_dup_params = true;

                        found_params.insert(*it);
                    }

                    if (found_dup_params)
                        THROW(ParseException, _USTR("duplicate function parameters are not allowed in strict mode."));
                }
                else
                {
                    skipped_tokens.push_back(tok);
                    if (lexer_.peek() == Token::SEMI)
                        skipped_tokens.push_back(lexer_.next());
                }

                continue;
            }
            else
            {
                directive_prologue = false;

                // Put skipped tokens back into the lexer.
                TokenVector::reverse_iterator it;
                for (it = skipped_tokens.rbegin(); it != skipped_tokens.rend(); ++it)
                    lexer_.push(*it);
            }
        }

        LabelList labels;
        scope()->push_back(parse_source_element(labels));
    }

    if (directive_prologue)
    {
        TokenVector::reverse_iterator it;
        for (it = skipped_tokens.rbegin(); it != skipped_tokens.rend(); ++it)
            lexer_.push(*it);

        while (lexer_.peek() != break_tok)
        {
            LabelList labels;
            scope()->push_back(parse_source_element(labels));
        }
    }
}

Statement *Parser::parse_source_element(LabelList &labels)
{
    // 14
    // SourceElement :
    //     Statement
    //     FunctionDeclaration

    if (lexer_.peek() == Token::FUNCTION)
        return parse_fun_decl();
    else
        return parse_stmt(labels);
}

Statement *Parser::parse_fun_decl()
{
    // A.5
    // FunctionDeclaration :
    //     function Identifier ( FormalParameterListopt ) { FunctionBody } 

    int beg_pos = lexer_.position();

    expect(Token::FUNCTION);

    String name = parse_identifier_str(scope()->is_strict_mode());

    Expression *fun = parse_fun_lit(name, beg_pos);
    scope()->push_decl(name, static_cast<FunctionLiteral *>(fun));     // FIXME: Maybe parse functions should return the appropriate types.

    // FIXME: Reduce to single shared element.
    return new (GC)EmptyStatement();
}

Statement *Parser::parse_stmt(LabelList &labels)
{
    // A.4
    // Statement : 
    //     Block
    //     VariableStatement 
    //     EmptyStatement 
    //     ExpressionStatement 
    //     IfStatement 
    //     IterationStatement 
    //     ContinueStatement 
    //     BreakStatement 
    //     ReturnStatement 
    //     WithStatement 
    //     LabeledStatement
    //     SwitchStatement 
    //     ThrowStatement 
    //     TryStatement 
    //     DebuggerStatement 

    switch (lexer_.peek())
    {
        case Token::LBRACE:
            return parse_block_stmt(labels);
        case Token::VAR:
            return parse_var_stmt();
        case Token::SEMI:
            return parse_empty_stmt();
        case Token::IF:
            return parse_if_stmt(labels);

        // Iteration statements.
        case Token::DO:
            return parse_do_while_stmt(labels);
        case Token::WHILE:
            return parse_while_stmt(labels);
        case Token::FOR:
            return parse_for_stmt(labels);

        case Token::CONTINUE:
            return parse_continue_stmt();
        case Token::BREAK:
            return parse_break_stmt(labels);
        case Token::RETURN:
            return parse_return_stmt();
        case Token::WITH:
            return parse_with_stmt(labels);
        case Token::SWITCH:
            return parse_switch_stmt(labels);
        case Token::THROW:
            return parse_throw_stmt();
        case Token::TRY:
            return parse_try_stmt(labels);
        case Token::DEBUGGER:
            return parse_debugger_stmt();

        // Function declarations are not allowed outside source elements
        // according to ECMA-262, but it seems to be defacto standard to allow
        // them in statements as well.
#ifdef ECMA262_EXT_FUN_STMT
        case Token::FUNCTION:
            if (!scope()->is_strict_mode())
                return parse_fun_decl();
            else
                return parse_expr_or_labeled_stmt(labels);
#endif

        // ExpressionStatement or LabeledStatement.
        default:
            return parse_expr_or_labeled_stmt(labels);
    }

    assert(false);
    return NULL;
}

Statement *Parser::parse_block_stmt(LabelList &labels)
{
    // A.4
    // Block : 
    //     { StatementListopt }

    int beg_pos = lexer_.position();

    BlockStatement *block = new (GC)BlockStatement(Location(), labels);
    TargetScope ts(this, block);

    expect(Token::LBRACE);

    while (lexer_.peek() != Token::RBRACE)
    {
        LabelList nested_labels;
        block->push_back(parse_stmt(nested_labels));
    }

    lexer_.next();  // Consume '}'.

    int end_pos = lexer_.position();

    block->set_location(Location(beg_pos, end_pos));
    return block;
}

Statement *Parser::parse_var_decl(bool no_in, String &var_name)
{
    // A.4
    // VariableDeclarationList : 
    //     VariableDeclaration 
    //     VariableDeclarationList , VariableDeclaration 
    //
    // VariableDeclaration :
    //     Identifier Initialiseropt 
    //
    // Initialiser : 
    //     = AssignmentExpression 

    expect(Token::VAR);

    bool strict_mode = scope()->is_strict_mode();

    // We might need to output multiple assignment expressions, to make things
    // easier we create a new block instead of allowing the parse_* functions
    // to return multiple statements.
    //
    // WARNING: In Harmony blocks affect scoping so this might be a bad idea.
    BlockStatement *init_block = NULL;

    String name;

    size_t count = 0;
    do
    {
        if (count > 0)
            lexer_.next();  // Consume ','.

        int beg_pos = lexer_.position();

        // 12.2.1
        name = parse_identifier_str(strict_mode);
        if (strict_mode && (name == _USTR("eval") || name == _USTR("arguments")))
            THROW(ParseException, _USTR("variable may not be named 'eval' or 'arguments' in strict mode."));

        if (next_if(Token::ASSIGN))
        {
            int end_pos_lit = lexer_.position();

            Expression *value = parse_assignment_expr(no_in);

            if (init_block == NULL)
            {
                LabelList nested_labels;
                init_block = new (GC)BlockStatement(Location(), nested_labels);
                init_block->set_hidden(true);
            }

            int end_pos = lexer_.position();

            Expression *lit = new (GC)IdentifierLiteral(Location(beg_pos, end_pos_lit),
                                                        name);

            Expression *assignment = new (GC)AssignmentExpression(Location(beg_pos, end_pos),
                                                                  AssignmentExpression::ASSIGN,
                                                                  lit,
                                                                  value);

            init_block->push_back(new (GC)ExpressionStatement(assignment));
        }

        int end_pos = lexer_.position();

        VariableLiteral *var = new (GC)VariableLiteral(Location(beg_pos, end_pos), name);

        scope()->push_decl(name, var);

        count++;
    } while (lexer_.peek() == Token::COMMA);

    if (count == 1)
        var_name = name;

    if (init_block != NULL)
        return init_block;

    return new (GC)EmptyStatement();    // FIXME:
}

Statement *Parser::parse_var_stmt()
{
    // A.4
    // VariableStatement : 
    //     var VariableDeclarationList ; 

    String name;
    Statement *stmt = parse_var_decl(false, name);

    expect_semi();

    return stmt;
}

Statement *Parser::parse_empty_stmt()
{
    expect(Token::SEMI); // We don't allow automatic semicolon insertion here.

    return new (GC)EmptyStatement();    // FIXME:
}

Statement *Parser::parse_if_stmt(LabelList &labels)
{
    // A.4
    // IfStatement : 
    //     if ( Expression ) Statement else Statement 
    //     if ( Expression ) Statement 

    int beg_pos = lexer_.position();

    expect(Token::IF);
    expect(Token::LPAREN);

    Expression *cond = parse_expr(false);

    expect(Token::RPAREN);

    Statement *if_stmt = parse_stmt(labels);
    Statement *else_stmt = NULL;

    if (next_if(Token::ELSE))
        else_stmt = parse_stmt(labels);

    int end_pos = lexer_.position();

    return new (GC)IfStatement(Location(beg_pos, end_pos), cond, if_stmt, else_stmt);
}

Statement *Parser::parse_do_while_stmt(LabelList &labels)
{
    int beg_pos = lexer_.position();

    DoWhileStatement *loop = new (GC)DoWhileStatement(Location(), labels);
    TargetScope ts(this, loop);

    expect(Token::DO);

    LabelList nested_labels;
    loop->set_body(parse_stmt(nested_labels));

    expect(Token::WHILE);
    expect(Token::LPAREN);

    loop->set_condition(parse_expr(false));

    expect(Token::RPAREN);

    int end_pos = lexer_.position();

    loop->set_location(Location(beg_pos, end_pos));
    return loop;
}

Statement *Parser::parse_while_stmt(LabelList &labels)
{
    int beg_pos = lexer_.position();

    WhileStatement *loop = new (GC)WhileStatement(Location(), labels);
    TargetScope ts(this, loop);

    expect(Token::WHILE);
    expect(Token::LPAREN);

    loop->set_condition(parse_expr(false));

    expect(Token::RPAREN);

    LabelList nested_labels;
    loop->set_body(parse_stmt(nested_labels));

    int end_pos = lexer_.position();

    loop->set_location(Location(beg_pos, end_pos));
    return loop;
}

Statement *Parser::parse_for_stmt(LabelList &labels)
{
    // A.4
    // IterationStatement : 
    //     ...
    //     for (ExpressionNoInopt; Expressionopt ; Expressionopt ) Statement 
    //     for ( var VariableDeclarationListNoIn; Expressionopt ; Expressionopt ) Statement 
    //     for ( LeftHandSideExpression in Expression ) Statement 
    //     for ( var VariableDeclarationNoIn in Expression ) Statement 

    int beg_pos = lexer_.position();

    expect(Token::FOR);
    expect(Token::LPAREN);

    Statement *init = NULL;

    if (lexer_.peek() != Token::SEMI)
    {
        if (lexer_.peek() == Token::VAR)
        {
            String name;
            Statement *decl = parse_var_decl(true, name);

            if (next_if(Token::IN))
            {
                ForInStatement *loop = new (GC)ForInStatement(Location(), labels);
                TargetScope ts(this, loop);

                loop->set_declaration(new (GC)IdentifierLiteral(decl->location(), name));
                loop->set_enumerable(parse_expr(false));

                expect(Token::RPAREN);

                LabelList nested_labels;
                loop->set_body(parse_stmt(nested_labels));

                int end_pos = lexer_.position();

                loop->set_location(Location(beg_pos, end_pos));

                LabelList no_labels;
                BlockStatement *block = new (GC)BlockStatement(Location(), no_labels);
                block->push_back(decl);
                block->push_back(loop);

                return loop;
            }
            else
            {
                init = decl;
            }
        }
        else
        {
            Expression *expr = parse_expr(true);
            if (next_if(Token::IN))
            {
                if (!expr->is_left_hand_expr())
                    THROW(ParseException, _USTR("invalid left hand side in assignment"),
                          ParseException::KIND_REFERENCE);

                ForInStatement *loop = new (GC)ForInStatement(Location(), labels);
                TargetScope ts(this, loop);

                loop->set_declaration(expr);
                loop->set_enumerable(parse_expr(false));

                expect(Token::RPAREN);

                LabelList nested_labels;
                loop->set_body(parse_stmt(nested_labels));

                int end_pos = lexer_.position();

                loop->set_location(Location(beg_pos, end_pos));
                return loop;
            }
            else
            {
                init = new (GC)ExpressionStatement(expr);
            }
        }
    }

    ForStatement *loop = new (GC)ForStatement(Location(), labels);
    TargetScope ts(this, loop);

    loop->set_initializer(init);

    expect(Token::SEMI);

    if (lexer_.peek() != Token::SEMI)
        loop->set_condition(parse_expr(false));

    expect(Token::SEMI);

    if (lexer_.peek() != Token::RPAREN)
        loop->set_next(parse_expr(false));

    expect(Token::RPAREN);

    LabelList nested_labels;
    loop->set_body(parse_stmt(nested_labels));

    int end_pos = lexer_.position();

    loop->set_location(Location(beg_pos, end_pos));
    return loop;
}

Statement *Parser::parse_continue_stmt()
{
    // A.4
    // ContinueStatement :
    //     continue ;
    //     continue [no LineTerminator here] Identifier ;

    int beg_pos = lexer_.position();

    expect(Token::CONTINUE);

    const LabeledStatement *target = NULL;

    Token tmp = lexer_.peek();
    if (is_identifier(tmp) && !tmp.is_separated_by_line_term())
    {
        String label = parse_identifier_str(scope()->is_strict_mode());

        target = find_target(label);
        if (target == NULL)
            THROW(ParseException, StringBuilder::sprintf("unknown label '%S' in continue statement.", label.data()));
    }
    else if (!find_target())
    {
        THROW(ParseException, _USTR("non-labeled continue statements are only allowed in loops."));
    }

    expect_semi();

    int end_pos = lexer_.position();

    return new (GC)ContinueStatement(Location(beg_pos, end_pos), target);
}

Statement *Parser::parse_break_stmt(LabelList &labels)
{
    // A.4
    // BreakStatement :
    //     break ;
    //     break [no LineTerminator here] Identifier ;

    int beg_pos = lexer_.position();

    expect(Token::BREAK);

    const LabeledStatement *target = NULL;

    Token tmp = lexer_.peek();
    if (is_identifier(tmp) && !tmp.is_separated_by_line_term())
    {
        String label = parse_identifier_str(scope()->is_strict_mode());

        // Consider labeled break statements as empty statements if they target
        // themselves. For example, l0: break l0;
        if (labels.contains(label))
            return new (GC)EmptyStatement();

        target = find_target(label);
        if (target == NULL)
            THROW(ParseException, StringBuilder::sprintf("unknown label '%S' in break statement.", label.data()));
    }
    else if (!find_target())
    {
        THROW(ParseException, _USTR("mon-labeled break statements are only allowed in loops."));
    }

    expect_semi();

    int end_pos = lexer_.position();

    return new (GC)BreakStatement(Location(beg_pos, end_pos), target);
}

Statement *Parser::parse_return_stmt()
{
    // A.4
    // ReturnStatement : 
    //     return ; 
    //     return [no LineTerminator here] Expression ; 

    int beg_pos = lexer_.position();

    expect(Token::RETURN);

    Expression *expr = NULL;

    Token tmp = lexer_.peek();
    if (tmp != Token::SEMI && !tmp.is_separated_by_line_term())
        expr = parse_expr(false);

    expect_semi();

    // According 12.9, a program is considered syntactically incorrect if it
    // contains a return statement that's not within a function body.
    if (!scope()->is_function_scope())
        THROW(ParseException, _USTR("return statement can only be used in functions."));

    int end_pos = lexer_.position();

    return new (GC)ReturnStatement(Location(beg_pos, end_pos), expr);
}

Statement *Parser::parse_with_stmt(LabelList &labels)
{
    // A.4
    // WithStatement : 
    //     with ( Expression ) Statement 

    int beg_pos = lexer_.position();

    expect(Token::WITH);

    if (scope()->is_strict_mode())
        THROW(ParseException, _USTR("with statement is not allowed in strict mode."));

    expect(Token::LPAREN);
    Expression *expr = parse_expr(false);
    expect(Token::RPAREN);

    Statement *body = parse_stmt(labels);

    int end_pos = lexer_.position();

    return new (GC)WithStatement(Location(beg_pos, end_pos), expr, body);
}

Statement *Parser::parse_switch_stmt(LabelList &labels)
{
    // A.4
    // SwitchStatement : 
    //     switch ( Expression ) CaseBlock 
    //
    // CaseBlock : 
    //     { CaseClausesopt } 
    //     { CaseClausesopt DefaultClause CaseClausesopt } 

    int beg_pos = lexer_.position();

    SwitchStatement *stmt = new (GC)SwitchStatement(Location(), labels);
    TargetScope ts(this, stmt);

    expect(Token::SWITCH);
    expect(Token::LPAREN);
    stmt->set_expression(parse_expr(false));
    expect(Token::RPAREN);

    expect(Token::LBRACE);

    bool found_default = false;

    while (lexer_.peek() != Token::RBRACE)
    {
        SwitchStatement::CaseClause *clause = parse_switch_case_clause();
        if (clause->is_default())
        {
            if (found_default)
                THROW(ParseException, _USTR("multiple default clauses in switch statement."));

            found_default = true;
        }

        stmt->push_back(clause);
    }

    expect(Token::RBRACE);

    int end_pos = lexer_.position();

    stmt->set_location(Location(beg_pos, end_pos));
    return stmt;
}

SwitchStatement::CaseClause *Parser::parse_switch_case_clause()
{
    // A.4
    // CaseClause : 
    //     case Expression : StatementListopt 
    //
    // DefaultClause : 
    //     default : StatementListopt 

    Expression *label = NULL;
    if (next_if(Token::CASE))
        label = parse_expr(false);
    else
        expect(Token::DEFAULT);

    expect(Token::COLON);

    StatementVector stmts;
    while (lexer_.peek() != Token::CASE &&
           lexer_.peek() != Token::DEFAULT &&
           lexer_.peek() != Token::RBRACE)
    {
        LabelList nested_labels;
        stmts.push_back(parse_stmt(nested_labels));
    }

    return new (GC)SwitchStatement::CaseClause(label, stmts);
}

Statement *Parser::parse_throw_stmt()
{
    // A.4
    // ThrowStatement : 
    //     throw [no LineTerminator here] Expression ; 

    int beg_pos = lexer_.position();

    expect(Token::THROW);

    if (lexer_.peek().is_separated_by_line_term())
        THROW(ParseException, _USTR("illegal line break after throw keyword."));

    Expression *expr = parse_expr(false);

    expect_semi();

    int end_pos = lexer_.position();

    return new (GC)ThrowStatement(Location(beg_pos, end_pos), expr);
}

Statement *Parser::parse_try_stmt(LabelList &labels)
{
    // A.4
    // TryStatement : 
    //     try Block Catch 
    //     try Block Finally 
    //     try Block Catch Finally 
    //
    // Catch : 
    //     catch ( Identifier ) Block 
    //
    // Finally : 
    //     finally Block 

    int beg_pos = lexer_.position();

    TryStatement *stmt = new (GC)TryStatement(Location(), labels);
    TargetScope ts(this, stmt);

    expect(Token::TRY);

    LabelList nested_labels;
    stmt->set_try_block(parse_block_stmt(nested_labels));

    if (next_if(Token::CATCH))
    {
        expect(Token::LPAREN);

        bool strict_mode = scope()->is_strict_mode();
        String name = parse_identifier_str(strict_mode);
        if (strict_mode && (name == _USTR("eval") || name == _USTR("arguments")))
            THROW(ParseException, _USTR("catch identifier may not be named 'eval' or 'arguments' in strict mode."));

        expect(Token::RPAREN);

        LabelList nested_labels;
        stmt->set_catch_block(parse_block_stmt(nested_labels));
        stmt->set_catch_identifier(name);
    }

    if (next_if(Token::FINALLY))
    {
        LabelList nested_labels;
        stmt->set_finally_block(parse_block_stmt(nested_labels));
    }

    if (!stmt->has_catch_block() && !stmt->has_finally_block())
        THROW(ParseException, _USTR("no catch or finally after try block."));

    int end_pos = lexer_.position();

    stmt->set_location(Location(beg_pos, end_pos));
    return stmt;
}

Statement *Parser::parse_debugger_stmt()
{
    // A.4
    // DebuggerStatement : 
    //     debugger ; 

    int beg_pos = lexer_.position();

    expect(Token::DEBUGGER);
    expect_semi();

    int end_pos = lexer_.position();

    return new (GC)DebuggerStatement(Location(beg_pos, end_pos));
}

Statement *Parser::parse_expr_or_labeled_stmt(LabelList &labels)
{
    // A.4
    // LabelledStatement :
    //     Identifier : Statement

    if (is_identifier(lexer_.peek()))
    {
        Token tok = lexer_.next();

        if (lexer_.peek() == Token::COLON)
        {
            String label = tok.string();
            if (labels.contains(label) || find_target(label) != NULL)
                THROW(ParseException, _USTR("label redeclaration."));

            lexer_.next();  // Consume ':'.

            labels.push_back(label);
            return parse_stmt(labels);
        }

        // Return the token, we're not dealing with a label here.
        lexer_.push(tok);
    }

    Expression *expr = parse_expr(false);

    expect_semi();

    return new (GC)ExpressionStatement(expr);
}

String Parser::parse_identifier_str(bool strict_mode)
{
    Token tok = lexer_.next();
    if (tok == Token::LIT_IDENTIFIER ||
        (!strict_mode && tok == Token::FUTURE_STRICT_RESERVED_WORD))
    {
        return tok.string();
    }

    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected identifier.",
                                                 tok.string().data()));
}

String Parser::parse_identifier_name_str()
{
    // NOTE: Identifier name is not the same as identifier. The identifier name
    //       allows keywords.
    Token tok = lexer_.next();
    if (tok == Token::LIT_IDENTIFIER ||
        tok.is_keyword() ||
        tok.is_future_reserved_keyword() ||
        tok.is_future_strict_reserved_keyword())
    {
        return tok.string();
    }

    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected identifier name.",
                                                 tok.string().data()));
}

Expression *Parser::parse_identifier(bool strict_mode)
{
    Token tok = lexer_.next();
    if (tok == Token::LIT_IDENTIFIER ||
        (!strict_mode && tok == Token::FUTURE_STRICT_RESERVED_WORD))
    {
        if (tok.string() == _USTR("arguments"))
            scope()->function()->set_needs_args_obj(true);

        return new (GC)IdentifierLiteral(tok.location(), tok.string());
    }

    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected identifier.",
                                                 tok.string().data()));
}

Expression *Parser::parse_identifier_name()
{
    // NOTE: Identifier name is not the same as identifier. The identifier name
    //       allows keywords.
    Token tok = lexer_.next();
    if (tok == Token::LIT_IDENTIFIER ||
        tok.is_keyword() ||
        tok.is_future_reserved_keyword() ||
        tok.is_future_strict_reserved_keyword())
    {
        return new (GC)StringLiteral(tok.location(), tok.string());
    }

    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected identifier name.",
                                                 tok.string().data()));
}

Expression *Parser::parse_reg_exp_lit()
{
    // 7.8.5
    // RegularExpressionLiteral ::
    //     / RegularExpressionBody / RegularExpressionFlags
    //
    // RegularExpressionBody ::
    //     RegularExpressionFirstChar RegularExpressionChars
    //
    // RegularExpressionChars ::
    //     [empty]
    //     RegularExpressionChars RegularExpressionChar
    //
    // RegularExpressionFirstChar ::
    //     RegularExpressionNonTerminator but not one of \ or / or [
    //     RegularExpressionBackslashSequence
    //     RegularExpressionClass
    //
    // RegularExpressionBackslashSequence ::
    //     \ RegularExpressionNonTerminator
    //
    // RegularExpressionNonTerminator ::
    //     SourceCharacter but not LineTerminator
    //
    // RegularExpressionClass ::
    //     [ RegularExpressionClassChars ]
    //
    // RegularExpressionClassChars ::
    //     [empty]
    //     RegularExpressionClassChars RegularExpressionClassChar
    //
    // RegularExpressionClassChar ::
    //     RegularExpressionNonTerminator but not one of ] or \\ (single backslash)
    //     RegularExpressionBackslashSequence
    //
    // RegularExpressionFlags ::
    //     [empty]
    //     RegularExpressionFlags IdentifierPart

    Token tok = lexer_.next_reg_exp();
    if (tok != Token::_LIT_REGEXP)
    {
        if (tok == Token::ILLEGAL)
            THROW(ParseException, _USTR("illegal token, expected regular expression."));
        else
            THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected regular expression.",
                                                         tok.string().data()));
    }

    return new (GC)RegularExpression(tok.location(), tok.string());
}

Expression *Parser::parse_array_lit()
{
    // A.3
    // ArrayLiteral :
    //     [ Elisionopt ] 
    //     [ ElementList ] 
    //     [ ElementList , Elisionopt ] 

    int beg_pos = lexer_.position();

    expect(Token::LBRACK);

    ExpressionVector values;

    while (lexer_.peek() != Token::RBRACK)
    {
        if (lexer_.peek() == Token::COMMA)
            values.push_back(new (GC)NothingLiteral(Location()));
        else
            values.push_back(parse_assignment_expr(false));

        if (lexer_.peek() == Token::RBRACK)
            break;

        Token tok = lexer_.next();
        if (tok != Token::COMMA)
            THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected ',' or ']'.",
                                                         tok.string().data()));
    }

    expect(Token::RBRACK);

    int end_pos = lexer_.position();

    return new (GC)ArrayLiteral(Location(beg_pos, end_pos), values);
}

ObjectLiteral::Property *Parser::parse_obj_lit_get_set(ObjectLiteral *obj, bool is_setter)
{
    int beg_pos = lexer_.position();

    Token tok = lexer_.next();

    if (tok == Token::LIT_IDENTIFIER ||
        tok.is_keyword() ||
        tok.is_future_reserved_keyword() ||
        tok.is_future_strict_reserved_keyword() ||
        tok == Token::LIT_NUMBER ||
        tok == Token::LIT_STRING)
    {
        // In case of a number literal, fail for octals in strict mode.
        if (scope()->is_strict_mode() && tok.is_octal())
            THROW(ParseException, _USTR("octal number literals are not allowed in strict mode."));

        String name = tok.string();
        if (obj->contains_data_prop(name))
            THROW(ParseException, _USTR("object literal accessor properties may not share names with data properties."));

        Expression *fun = parse_fun_lit(name, beg_pos);

        return new (GC)ObjectLiteral::Property(is_setter, fun, name);
    }

    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S'.",
                                                 tok.string().data()));
}

Expression *Parser::parse_obj_lit()
{
    // A.3
    // ObjectLiteral : 
    //     { } 
    //     { PropertyNameAndValueList } 
    //     { PropertyNameAndValueList , } 
    //
    // PropertyNameAndValueList :
    //     PropertyAssignment 
    //     PropertyNameAndValueList , PropertyAssignment 
    //
    // PropertyAssignment :
    //     PropertyName : AssignmentExpression 
    //     get PropertyName ( ) { FunctionBody } 
    //     set PropertyName ( PropertySetParameterList ) { FunctionBody } 
    //
    // PropertyName :
    //     IdentifierName 
    //     StringLiteral 
    //     NumericLiteral 
    //
    // PropertySetParameterList : 
    //     Identifier 

    int beg_pos = lexer_.position();

    expect(Token::LBRACE);

    ObjectLiteral *obj = new (GC)ObjectLiteral(Location());

    while (lexer_.peek() != Token::RBRACE)
    {
        Expression *key = NULL;
        String key_str;

        switch (lexer_.peek())
        {
            case Token::LIT_IDENTIFIER:
            case Token::FUTURE_RESERVED_WORD:
            case Token::FUTURE_STRICT_RESERVED_WORD:
            {
                int beg_pos = lexer_.position();

                String id = parse_identifier_name_str();
                bool is_getter = id == _USTR("get");
                bool is_setter = id == _USTR("set");

                if ((is_getter || is_setter) && lexer_.peek() != Token::COLON)
                {
                    ObjectLiteral::Property *prop = parse_obj_lit_get_set(obj, is_setter);

                    if (obj->contains(prop))
                        THROW(ParseException, _USTR("accessor properties with duplicate in object literals are not allowed."));

                    obj->push_back(prop);

                    if (lexer_.peek() != Token::RBRACE)
                        expect(Token::COMMA);

                    continue;
                }

                int end_pos = lexer_.position();
                    
                // Not a getter or setter, simply a property named "get" or "set".
                key = new (GC)StringLiteral(Location(beg_pos, end_pos), id);
                key_str = id;
                break;
            }

            case Token::LIT_STRING:
            {
                Token tok = lexer_.next();
                key = new (GC)StringLiteral(tok.location(), tok.string());
                key_str = tok.string();
                break;
            }

            case Token::LIT_NUMBER:
            {
                Token tok = lexer_.next();
                if (scope()->is_strict_mode() && tok.is_octal())
                    THROW(ParseException, StringBuilder::sprintf("octal number literals are not allowed in strict mode."));

                key = new (GC)NumberLiteral(tok.location(), tok.string());
                key_str = tok.string();
                break;
            }

            default:
            {
                Token tok = lexer_.next();
                if (tok.is_keyword())
                {
                    key = new (GC)StringLiteral(tok.location(), tok.string());
                    key_str = tok.string();
                }
                else
                {
                    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S'.",
                                                                 tok.string().data()));
                }
                break;
            }
        }

        expect(Token::COLON);

        Expression *val = parse_assignment_expr(false);

        if (obj->contains_accessor_prop(key_str))
            THROW(ParseException, _USTR("object literal accessor properties may not share names with data properties."));

        ObjectLiteral::Property *prop = new (GC)ObjectLiteral::Property(key, val);

        // Check if the property is already defined.
        if (scope()->is_strict_mode() && obj->contains(prop))
            THROW(ParseException, _USTR("duplicate data properties in object literals are not allowed in strict mode."));

        obj->push_back(prop);

        if (lexer_.peek() != Token::RBRACE)
            expect(Token::COMMA);
    }

    lexer_.next();  // Consume '}'.

    int end_pos = lexer_.position();

    obj->set_location(Location(beg_pos, end_pos));
    return obj;
}

Expression *Parser::parse_fun_lit(String name, int beg_pos)
{
    // <INTERNAL> : 
    //     ( FormalParameterListopt ) { FunctionBody }

    if (scope()->is_strict_mode() && (name == _USTR("eval") || name == _USTR("arguments")))
        THROW(ParseException, _USTR("function may not be named 'eval' or 'arguments' in strict mode."));

    // Function literal breaks label scoping.
    TargetScope ts(this, TargetScope::BARRIER);

    expect(Token::LPAREN);

    FunctionLiteral *fun = new (GC)FunctionLiteral(Location(), name);
    if (scope()->is_strict_mode())     // Strict mode is inherited.
        fun->set_strict_mode(true);

    bool has_dup_params = false;
    while (lexer_.peek() != Token::RPAREN)
    {
        String name = parse_identifier_str(scope()->is_strict_mode());
        if (scope()->is_strict_mode())
        {
            // Check for duplicate parameters.
            if (!has_dup_params && fun->has_param(name))
                has_dup_params = true;
        }

        fun->push_param(name);

        if (lexer_.peek() == Token::RPAREN)
            break;

        Token tok = lexer_.next();
        if (tok != Token::COMMA)
        {
            THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected ',' or ')'.",
                                                         tok.string().data()));
        }
    }

    expect(Token::RPAREN);
    expect(Token::LBRACE);

    enter_scope(fun);
    parse_source_elements(Token::RBRACE);
    leave_scope();

    int end_pos = lexer_.next();
    fun->set_location(Location(beg_pos, end_pos));

    // Verify parameter names.
    if (scope()->is_strict_mode() || fun->is_strict_mode())
    {
        StringVector::const_iterator it;
        for (it = fun->parameters().begin(); it != fun->parameters().end(); ++it)
        {
            if (*it == _USTR("eval") || *it == _USTR("arguments"))
                THROW(ParseException, _USTR("function argument may not be named 'eval' or 'arguments' in strict mode."));
        }

        if (has_dup_params)
            THROW(ParseException, _USTR("duplicate function parameters are not allowed in strict mode."));
    }

    // If the function contains a parameter or function declaration with the
    // name "arguments" it'll override the "official" arguments object. As a
    // result we don't have to bother creating it in the first place.
    //
    // The specification is a little bit fuzzy on the subject. ECMA-262: 10.6
    // states that an arguments object shouldn't be created "unless (as
    // specified in 10.5) the identifier arguments occurs as an Identifier in
    // the function‘s FormalParameterList or occurs as the Identifier of a
    // VariableDeclaration or FunctionDeclaration contained in the function
    // code.". This suggests that variable declarations should override the
    // arguments object, however the pseudo code in section 10.5 suggests that
    // only parameters and function declarations should override the arugments
    // object.
    if (fun->needs_args_obj())
    {
        StringVector::const_iterator it_prm;
        for (it_prm = fun->parameters().begin(); it_prm != fun->parameters().end(); ++it_prm)
        {
            if (*it_prm == _USTR("arguments"))
            {
                fun->set_needs_args_obj(false);
                break;
            }
        }

        DeclarationVector::const_iterator it_decl;
        for (it_decl = fun->declarations().begin(); it_decl != fun->declarations().end(); ++it_decl)
        {
            if (!(*it_decl)->is_function())
                continue;

            if ((*it_decl)->name() == _USTR("arguments"))
            {
                fun->set_needs_args_obj(false);
                break;
            }
        }
    }

    return fun;
}

Expression *Parser::parse_expr(bool no_in)
{
    // A.3
    // Expression :
    //     AssignmentExpression 
    //     Expression , AssignmentExpression 

    Expression *left = parse_assignment_expr(no_in);
    while (lexer_.peek() == Token::COMMA)
    {
        lexer_.next();  // Consume ','.
        Expression *right = parse_assignment_expr(no_in);

        int beg_pos = left->location().begin();
        int end_pos = right->location().end();

        left = new (GC)BinaryExpression(Location(beg_pos, end_pos), BinaryExpression::COMMA, left, right);
    }

    return left;
}

Expression *Parser::parse_assignment_expr(bool no_in)
{
    // A.3
    // AssignmentExpression :
    //     ConditionalExpression 
    //     LeftHandSideExpression = AssignmentExpression 
    //     LeftHandSideExpression AssignmentOperator AssignmentExpression 

    Expression *left = parse_cond_expr(no_in);
    if (!lexer_.peek().is_assignment())
        return left;

    // Make sure the expression is a valid left hand side expression.
    if (!left->is_left_hand_expr())
    {
        // FIXME: Re-evaluate the comment below after upgrading test suite.
        // S7.5.1_A1.1
        // S7.5.1_A1.2
        // S7.5.1_A1.3
        // S7.5.2_A1.18
        //  States that test should fail at execution so maybe a ReferenceError
        //  would be the proper thing to throw? On the other hand, I don't think
        //  they should parser in the first place given the ES5.1 grammar.
        THROW(ParseException, _USTR("invalid left hand side in assignment"),
              ParseException::KIND_REFERENCE);
    }

    // Check for "arguments" and "eval" in strict mode.
    if (scope()->is_strict_mode())
    {
        IdentifierLiteral *lit = dynamic_cast<IdentifierLiteral *>(left);
        if (lit && (lit->value() == _USTR("arguments") || lit->value() == _USTR("eval")))
            THROW(ParseException, _USTR("assignments to 'arguments' and 'eval' is not allowed in strict mode."));
    }

    Token assign_tok = lexer_.next();

    Expression *right = parse_assignment_expr(no_in);

    AssignmentExpression::Operation op = AssignmentExpression::NONE;
    switch (assign_tok)
    {
        case Token::ASSIGN:
            op = AssignmentExpression::ASSIGN;
            break;
        case Token::ASSIGN_ADD:
            op = AssignmentExpression::ASSIGN_ADD;
            break;
        case Token::ASSIGN_SUB:
            op = AssignmentExpression::ASSIGN_SUB;
            break;
        case Token::ASSIGN_MUL:
            op = AssignmentExpression::ASSIGN_MUL;
            break;
        case Token::ASSIGN_MOD:
            op = AssignmentExpression::ASSIGN_MOD;
            break;
        case Token::ASSIGN_LS:
            op = AssignmentExpression::ASSIGN_LS;
            break;
        case Token::ASSIGN_RSS:
            op = AssignmentExpression::ASSIGN_RSS;
            break;
        case Token::ASSIGN_RUS:
            op = AssignmentExpression::ASSIGN_RUS;
            break;
        case Token::ASSIGN_BIT_AND:
            op = AssignmentExpression::ASSIGN_BIT_AND;
            break;
        case Token::ASSIGN_BIT_OR:
            op = AssignmentExpression::ASSIGN_BIT_OR;
            break;
        case Token::ASSIGN_BIT_XOR:
            op = AssignmentExpression::ASSIGN_BIT_XOR;
            break;
        case Token::ASSIGN_DIV:
            op = AssignmentExpression::ASSIGN_DIV;
            break;
        default:
            assert(false);
            break;
    }

    int beg_pos = left->location().begin();
    int end_pos = right->location().end();

    return new (GC)AssignmentExpression(Location(beg_pos, end_pos), op, left, right);
}

Expression *Parser::parse_cond_expr(bool no_in)
{
    // A.3
    // ConditionalExpression : 
    //     LogicalORExpression 
    //     LogicalORExpression ? AssignmentExpression : AssignmentExpression 

    Expression *expr = parse_binary_expr(no_in, Token::precedence(Token::LOG_OR));
    if (next_if(Token::COND))
    {
        Expression *left = parse_assignment_expr(false);
        expect(Token::COLON);
        Expression *right = parse_assignment_expr(no_in);

        int beg_pos = left->location().begin();
        int end_pos = right->location().end();

        expr = new (GC)ConditionalExpression(Location(beg_pos, end_pos), expr, left, right);
    }

    return expr;
}

Expression *Parser::parse_prim_expr()
{
    // A.3
    // PrimaryExpression :
    //     this 
    //     Identifier 
    //     Literal 
    //     ArrayLiteral 
    //     ObjectLiteral 
    //     ( Expression ) 

    // A.2
    // Literal :
    //     NullLiteral 
    //     BooleanLiteral 
    //     NumericLiteral 
    //     StringLiteral 
    //     RegularExpressionLiteral 

    switch (lexer_.peek())
    {
        case Token::THIS:
        {
            Token tok = lexer_.next();
            return new (GC)ThisLiteral(tok.location());
        }

        case Token::LIT_IDENTIFIER:
        case Token::FUTURE_STRICT_RESERVED_WORD:
        {
            if (lexer_.peek() == Token::FUTURE_STRICT_RESERVED_WORD &&
                scope()->is_strict_mode())
                break;

            return parse_identifier(scope()->is_strict_mode());
        }

        case Token::LIT_NULL:
        {
            Token tok = lexer_.next();
            return new (GC)NullLiteral(tok.location());
        }

        case Token::LIT_TRUE:
        case Token::LIT_FALSE:
        {
            Token tok = lexer_.next();
            return new (GC)BoolLiteral(tok.location(), tok == Token::LIT_TRUE);
        }

        case Token::LIT_NUMBER:
        {
            Token tok = lexer_.next();
            if (scope()->is_strict_mode() && tok.is_octal())
                THROW(ParseException, StringBuilder::sprintf("octal number literals are not allowed in strict mode."));

            return new (GC)NumberLiteral(tok.location(), tok.string());
        }

        case Token::LIT_STRING:
        {
            Token tok = lexer_.next();
            return new (GC)StringLiteral(tok.location(), tok.string());
        }

        case Token::DIV:
        case Token::ASSIGN_DIV:
            return parse_reg_exp_lit();

        case Token::LBRACK:
            return parse_array_lit();

        case Token::LBRACE:
            return parse_obj_lit();

        case Token::LPAREN:
        {
            lexer_.next();

            Expression *expr = parse_expr(false);

            expect(Token::RPAREN);

            return expr;
        }
    }

    Token tok = lexer_.next();

    THROW(ParseException, StringBuilder::sprintf("unexpected token '%S'.",
                                                 tok.string().data()));
}

ExpressionVector Parser::parse_args_expr()
{
    // A.3
    // Arguments :
    //     ()
    //     ( ArgumentList )

    expect(Token::LPAREN);

    ExpressionVector args;
    while (lexer_.peek() != Token::RPAREN)
    {
        Expression *expr = parse_assignment_expr(false);
        args.push_back(expr);

        if (lexer_.peek() == Token::RPAREN)
            break;

        Token tok = lexer_.next();
        if (tok != Token::COMMA)
        {
            THROW(ParseException, StringBuilder::sprintf("unexpected token '%S', expected ',' or ')'.",
                                                         tok.string().data()));
        }
    }

    expect(Token::RPAREN);
    return args;
}

Expression *Parser::parse_fun_expr()
{
    // A.3
    // FunctionExpression : 
    //     function Identifieropt ( FormalParameterListopt ) { FunctionBody } 

    int beg_pos = lexer_.position();

    expect(Token::FUNCTION);

    String name;
    if (is_identifier(lexer_.peek()))   // Optional.
        name = parse_identifier_str(scope()->is_strict_mode());

    Expression *fun = parse_fun_lit(name, beg_pos);
    static_cast<FunctionLiteral *>(fun)->set_type(FunctionLiteral::TYPE_EXPRESSION);

    int end_pos = lexer_.position();
    return new (GC)FunctionExpression(Location(beg_pos, end_pos), static_cast<FunctionLiteral *>(fun)); // FIXME: Maybe parse functions should return the appropriate types.
}

Expression *Parser::parse_member_with_new_pfx_expr(std::vector<int, gc_allocator<int> > &stack)
{
    // A.3
    // MemberExpression : 
    //     PrimaryExpression 
    //     FunctionExpression 
    //     MemberExpression [ Expression ] 
    //     MemberExpression . IdentifierName 
    //     new MemberExpression Arguments 

    Expression *expr = NULL;
    if (lexer_.peek() == Token::FUNCTION)
        expr = parse_fun_expr();
    else
        expr = parse_prim_expr();

    while (true)
    {
        switch (lexer_.peek())
        {
            case Token::LBRACK:
            {
                lexer_.next();
                Expression *key = parse_expr(false);

                int beg_pos = expr->location().begin();
                int end_pos = lexer_.position();
                expr = new (GC)PropertyExpression(Location(beg_pos, end_pos), expr, key);

                expect(Token::RBRACK);
                break;
            }

            case Token::LPAREN:
            {
                if (stack.empty())
                    return expr;

                ExpressionVector args = parse_args_expr();

                int beg_pos = stack.back();
                int end_pos = lexer_.position();

                stack.pop_back();

                expr = new (GC)CallNewExpression(Location(beg_pos, end_pos), expr, args);
                break;
            }

            case Token::DOT:
            {
                lexer_.next();

                Expression *name = parse_identifier_name();

                int beg_pos = expr->location().begin();
                int end_pos = lexer_.position();
                expr = new (GC)PropertyExpression(Location(beg_pos, end_pos), expr, name);
                break;
            }

            default:
                return expr;
        }
    }

    assert(false);
    return NULL;
}

Expression *Parser::parse_new_pfx_expr(std::vector<int, gc_allocator<int> > &stack)
{
    expect(Token::NEW);

    stack.push_back(lexer_.position());

    Expression *expr = NULL;
    if (lexer_.peek() == Token::NEW)
        expr = parse_new_pfx_expr(stack);
    else
        expr = parse_member_with_new_pfx_expr(stack);

    if (!stack.empty())
    {
        ExpressionVector args;

        int beg_pos = stack.back();
        int end_pos = lexer_.position();

        stack.pop_back();

        expr = new (GC)CallNewExpression(Location(beg_pos, end_pos), expr, args);
    }

    return expr;
}

Expression *Parser::parse_new_expr()
{
    std::vector<int, gc_allocator<int> > stack;
    return parse_new_pfx_expr(stack);
}

Expression *Parser::parse_member_expr()
{
    std::vector<int, gc_allocator<int> > stack;
    return parse_member_with_new_pfx_expr(stack);
}

Expression *Parser::parse_lhs_expr()
{
    // LeftHandSideExpression : 
    //     NewExpression 
    //     CallExpression 

    Expression *expr = NULL;
    if (lexer_.peek() == Token::NEW)
        expr = parse_new_expr();
    else
        expr = parse_member_expr();

    while (true)
    {
        switch (lexer_.peek())
        {
            case Token::LBRACK:
            {
                lexer_.next();
                Expression *key = parse_expr(false);

                int beg_pos = expr->location().begin();
                int end_pos = lexer_.position();
                expr = new (GC)PropertyExpression(Location(beg_pos, end_pos), expr, key);

                expect(Token::RBRACK);
                break;
            }

            case Token::LPAREN:
            {
                int beg_pos = lexer_.position();

                ExpressionVector args = parse_args_expr();

                int end_pos = lexer_.position();
                expr = new (GC)CallExpression(Location(beg_pos, end_pos), expr, args);
                break;
            }

            case Token::DOT:
            {
                lexer_.next();

                Expression *name = parse_identifier_name();

                int beg_pos = expr->location().begin();
                int end_pos = lexer_.position();
                expr = new (GC)PropertyExpression(Location(beg_pos, end_pos), expr, name);
                break;
            }

            default:
                return expr;
        }
    }

    assert(false);
    return NULL;
}

Expression *Parser::parse_unary_expr()
{
    // A.3
    // UnaryExpression : 
    //     PostfixExpression 
    //     delete UnaryExpression 
    //     void UnaryExpression 
    //     typeof UnaryExpression 
    //     ++ UnaryExpression 
    //     -- UnaryExpression 
    //     + UnaryExpression 
    //     - UnaryExpression 
    //     ~ UnaryExpression 
    //     ! UnaryExpression 

    if (lexer_.peek().is_unary())
    {
        int beg_pos = lexer_.position();

        Token op_tok = lexer_.next();

        Expression *expr = parse_unary_expr();

        UnaryExpression::Operation op = UnaryExpression::NONE;
        switch (op_tok)
        {
            case Token::DELETE:
                if (scope()->is_strict_mode() && dynamic_cast<IdentifierLiteral *>(expr))
                    THROW(ParseException, _USTR("delete operator is not allowed on variable references, function arguments and function names in strict mode."));

                op = UnaryExpression::DELETE;
                break;
            case Token::VOID:
                op = UnaryExpression::VOID;
                break;
            case Token::TYPEOF:
                op = UnaryExpression::TYPEOF;
                break;
            case Token::INC:
                op = UnaryExpression::PRE_INC;
                break;
            case Token::DEC:
                op = UnaryExpression::PRE_DEC;
                break;
            case Token::ADD:
                op = UnaryExpression::PLUS;
                break;
            case Token::SUB:
                op = UnaryExpression::MINUS;
                break;
            case Token::BIT_NOT:
                op = UnaryExpression::BIT_NOT;
                break;
            case Token::LOG_NOT:
                op = UnaryExpression::LOG_NOT;
                break;
            default:
                assert(false);
                break;
        }

        if (op_tok == Token::INC || op_tok == Token::DEC)
        {
            if (!expr->is_left_hand_expr())
                THROW(ParseException, _USTR("invalid left hand side in prefix operation."),
                      ParseException::KIND_REFERENCE);

            // Check for "arguments" and "eval" in strict mode.
            if (scope()->is_strict_mode())
            {
                IdentifierLiteral *lit = dynamic_cast<IdentifierLiteral *>(expr);
                if (lit && (lit->value() == _USTR("arguments") || lit->value() == _USTR("eval")))
                    THROW(ParseException, _USTR("prefix increment/decrement not allowed 'arguments' and 'eval' in strict mode."));
            }
        }

        int end_pos = expr->location().end();
        return new (GC)UnaryExpression(Location(beg_pos, end_pos), op, expr);
    }

    // Postfix expression.
    Expression *expr = parse_lhs_expr();

    if (!lexer_.peek().is_separated_by_line_term() &&
        (lexer_.peek() == Token::INC || lexer_.peek() == Token::DEC))
    {
        if (!expr->is_left_hand_expr())
            THROW(ParseException, _USTR("invalid left hand side in postfix operation."), ParseException::KIND_REFERENCE);

        // Check for "arguments" and "eval" in strict mode.
        if (scope()->is_strict_mode())
        {
            IdentifierLiteral *lit = dynamic_cast<IdentifierLiteral *>(expr);
            if (lit && (lit->value() == _USTR("arguments") || lit->value() == _USTR("eval")))
                THROW(ParseException, _USTR("postfix increment/decrement not allowed 'arguments' and 'eval' in strict mode."));
        }

        Token op_tok = lexer_.next();

        int beg_pos = expr->location().begin();
        int end_pos = lexer_.position();

        expr = new (GC)UnaryExpression(Location(beg_pos, end_pos),
                                       op_tok == Token::INC ? UnaryExpression::POST_INC
                                                            : UnaryExpression::POST_DEC,
                                       expr);
    }

    return expr;
}

Expression *Parser::parse_binary_expr(bool no_in, int prec)
{
    Expression *left = parse_unary_expr();

    for (int cur_prec = lexer_.peek().precedence(no_in); cur_prec <= prec; cur_prec++)
    {
        while (cur_prec == lexer_.peek().precedence(no_in))
        {
            Token::Type op_tok = lexer_.next();

            Expression *right = parse_binary_expr(no_in, cur_prec - 1);

            BinaryExpression::Operation op = BinaryExpression::NONE;
            switch (op_tok)
            {
                case Token::MUL:
                    op = BinaryExpression::MUL;
                    break;
                case Token::DIV:
                    op = BinaryExpression::DIV;
                    break;
                case Token::MOD:
                    op = BinaryExpression::MOD;
                    break;
                case Token::ADD:
                    op = BinaryExpression::ADD;
                    break;
                case Token::SUB:
                    op = BinaryExpression::SUB;
                    break;
                case Token::LS:
                    op = BinaryExpression::LS;
                    break;
                case Token::RSS:
                    op = BinaryExpression::RSS;
                    break;
                case Token::RUS:
                    op = BinaryExpression::RUS;
                    break;
                case Token::LT:
                    op = BinaryExpression::LT;
                    break;
                case Token::GT:
                    op = BinaryExpression::GT;
                    break;
                case Token::LTE:
                    op = BinaryExpression::LTE;
                    break;
                case Token::GTE:
                    op = BinaryExpression::GTE;
                    break;
                case Token::IN:
                    op = BinaryExpression::IN;
                    break;
                case Token::INSTANCEOF:
                    op = BinaryExpression::INSTANCEOF;
                    break;
                case Token::EQ:
                    op = BinaryExpression::EQ;
                    break;
                case Token::NEQ:
                    op = BinaryExpression::NEQ;
                    break;
                case Token::STRICT_EQ:
                    op = BinaryExpression::STRICT_EQ;
                    break;
                case Token::STRICT_NEQ:
                    op = BinaryExpression::STRICT_NEQ;
                    break;
                case Token::BIT_AND:
                    op = BinaryExpression::BIT_AND;
                    break;
                case Token::BIT_XOR:
                    op = BinaryExpression::BIT_XOR;
                    break;
                case Token::BIT_OR:
                    op = BinaryExpression::BIT_OR;
                    break;
                case Token::LOG_AND:
                    op = BinaryExpression::LOG_AND;
                    break;
                case Token::LOG_OR:
                    op = BinaryExpression::LOG_OR;
                    break;
                default:
                    assert(false);
                    break;
            }

            int beg_pos = left->location().begin();
            int end_pos = right->location().end();

            left = new (GC)BinaryExpression(Location(beg_pos, end_pos), op, left, right);
        }
    }

    return left;
}

FunctionLiteral *Parser::parse()
{
    return parse_program();
}

}
