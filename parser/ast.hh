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
#include <algorithm>
#include <cassert>
#include <vector>
#include <gc/gc_allocator.h>
#include "common/core.hh"
#include "common/string.hh"
#include "location.hh"

namespace parser {

class Visitor;

class LabelList
{
private:
    StringVector labels_;

public:
    void push_back(String label)
    {
        labels_.push_back(label);
    }

    bool empty() const
    {
        return labels_.empty();
    }

    bool contains(String label) const
    {
        return std::find(labels_.begin(), labels_.end(), label) != labels_.end();
    }

    size_t size() const
    {
        return labels_.size();
    }

    String first() const
    {
        assert(!empty());
        return labels_.front();
    }

    StringVector::const_iterator begin() const
    {
        return labels_.begin();
    }

    StringVector::const_iterator end() const
    {
        return labels_.end();
    }
};

class FunctionLiteral;
class VariableLiteral;

class Declaration
{
private:
    FunctionLiteral *fun_;
    VariableLiteral *var_;

public:
    Declaration(FunctionLiteral *fun)
        : fun_(fun)
        , var_(NULL) {}

    Declaration(VariableLiteral *var)
        : fun_(NULL)
        , var_(var) {}

    virtual ~Declaration() {}

    bool is_function() const
    {
        return fun_ != NULL;
    }

    bool is_variable() const
    {
        return var_ != NULL;
    }

    FunctionLiteral *as_function() const
    {
        return fun_;
    }

    VariableLiteral *as_variable() const
    {
        return var_;
    }

    String name() const;
};

/**
 * Set of declarations.
 */
typedef std::set<Declaration *, std::less<Declaration *>, gc_allocator<Declaration *> > DeclarationSet;

/**
 * Vector of declarations.
 */
typedef std::vector<Declaration *, gc_allocator<Declaration *> > DeclarationVector;

/**
 * @brief AST node root type.
 */
class Node
{
private:
    Location loc_;      ///< Position in source code.

public:
    Node(Location loc)
        : loc_(loc)
    {
    }

    virtual ~Node() {}

    Location location() const
    {
        return loc_;
    }

    void set_location(Location loc)
    {
        loc_ = loc;
    }

    /**
     * Accept node in visitor pattern.
     * @param [in] visitor The visitor.
     */
    virtual void accept(Visitor *visitor) = 0;
};

class Expression : public Node
{
public:
    Expression(Location loc)
        : Node(loc) {}
    virtual ~Expression() {}

    /**
     * @return true if the expression is a valid left hand side expresion and
     *         false otherwise.
     */
    virtual bool is_left_hand_expr() const
    {
        return false;
    }
};

/**
 * Vector of expressions.
 */
typedef std::vector<Expression *, gc_allocator<Expression *> > ExpressionVector;

class Statement : public Node
{
public:
    Statement(Location loc)
        : Node(loc) {}
    virtual ~Statement() {}
};

/**
 * Vector of statements.
 */
typedef std::vector<Statement *, gc_allocator<Statement *> > StatementVector;

class LabeledStatement : public Statement
{
private:
    LabelList labels_;

public:
    LabeledStatement(Location loc, LabelList &labels)
        : Statement(loc)
        , labels_(labels) {}

    const LabelList &labels() const
    {
        return labels_;
    }
};

/**
 * Vector of labeled statements.
 */
typedef std::vector<LabeledStatement *,
                    gc_allocator<LabeledStatement *> > LabeledStatementVector;

class BinaryExpression : public Expression
{
public:
    enum Operation
    {
        // Not used, indicates uninitialized value.
        NONE,

        COMMA,

        // Arithmetic.
        MUL,
        DIV,
        MOD,
        ADD,
        SUB,
        LS,
        RSS,
        RUS,

        // Relational.
        LT,
        GT,
        LTE,
        GTE,
        IN,
        INSTANCEOF,

        // Equality.
        EQ,
        NEQ,
        STRICT_EQ,
        STRICT_NEQ,

        // Bitwise.
        BIT_AND,
        BIT_XOR,
        BIT_OR,

        // Logical.
        LOG_AND,
        LOG_OR,
    };

private:
    Operation op_;
    Expression *left_;
    Expression *right_;

public:
    BinaryExpression(Location loc, Operation op, Expression *left, Expression *right)
        : Expression(loc)
        , op_(op)
        , left_(left)
        , right_(right) {}

    Operation operation() const
    {
        return op_;
    }

    Expression *left() const
    {
        return left_;
    }

    Expression *right() const
    {
        return right_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class UnaryExpression : public Expression
{
public:
    enum Operation
    {
        // Not used, indicates uninitialized value.
        NONE,

        DELETE,
        VOID,
        TYPEOF,
        PRE_INC,
        PRE_DEC,
        POST_INC,
        POST_DEC,
        PLUS,
        MINUS,
        BIT_NOT,
        LOG_NOT
    };

private:
    Operation op_;
    Expression *expr_;

public:
    UnaryExpression(Location loc, Operation op, Expression *expr)
        : Expression(loc)
        , op_(op)
        , expr_(expr) {}

    Operation operation() const
    {
        return op_;
    }

    Expression *expression() const
    {
        return expr_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class AssignmentExpression : public Expression
{
public:
    enum Operation
    {
        // Not used, indicates uninitialized value.
        NONE,

        ASSIGN,
        ASSIGN_ADD,
        ASSIGN_SUB,
        ASSIGN_MUL,
        ASSIGN_MOD,
        ASSIGN_LS,
        ASSIGN_RSS,
        ASSIGN_RUS,
        ASSIGN_BIT_AND,
        ASSIGN_BIT_OR,
        ASSIGN_BIT_XOR,
        ASSIGN_DIV
    };

private:
    Operation op_;
    Expression *lhs_;
    Expression *rhs_;

public:
    AssignmentExpression(Location loc, Operation op, Expression *lhs, Expression *rhs)
        : Expression(loc)
        , op_(op)
        , lhs_(lhs)
        , rhs_(rhs) {}

    Operation operation() const
    {
        return op_;
    }

    Expression *lhs() const
    {
        return lhs_;
    }

    Expression *rhs() const
    {
        return rhs_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ConditionalExpression : public Expression
{
private:
    Expression *cond_;
    Expression *left_;
    Expression *right_;

public:
    ConditionalExpression(Location loc, Expression *cond, Expression *left, Expression *right)
        : Expression(loc)
        , cond_(cond)
        , left_(left)
        , right_(right) {}

    Expression *condition() const
    {
        return cond_;
    }

    Expression *left() const
    {
        return left_;
    }

    Expression *right() const
    {
        return right_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class PropertyExpression : public Expression
{
private:
    Expression *obj_;
    Expression *key_;

public:
    PropertyExpression(Location loc, Expression *obj, Expression *key)
        : Expression(loc)
        , obj_(obj)
        , key_(key) {}

    Expression *obj() const
    {
        return obj_;
    }

    Expression *key() const
    {
        return key_;
    }

    /**
     * @copydoc Expression::is_left_hand_expr
     */
    bool is_left_hand_expr() const
    {
        return true;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class CallExpression : public Expression
{
private:
    Expression *expr_;
    ExpressionVector args_;

public:
    CallExpression(Location loc, Expression *expr, ExpressionVector args)
        : Expression(loc)
        , expr_(expr)
        , args_(args) {}

    Expression *expression() const
    {
        return expr_;
    }

    const ExpressionVector &arguments() const
    {
        return args_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class CallNewExpression : public Expression
{
private:
    Expression *expr_;
    ExpressionVector args_;

public:
    CallNewExpression(Location loc, Expression *expr, ExpressionVector args)
        : Expression(loc)
        , expr_(expr)
        , args_(args) {}

    Expression *expression() const
    {
        return expr_;
    }

    const ExpressionVector &arguments() const
    {
        return args_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class RegularExpression : public Expression
{
private:
    String expr_;

public:
    RegularExpression(Location loc, String expr)
        : Expression(loc)
        , expr_(expr)
    {
    }

    String as_string() const
    {
        return expr_;
    }

    String pattern() const
    {
        bool in_char_class = false;

        size_t i = 1;
        for (; i < expr_.length(); i++)
        {
            uni_char c = expr_[i];
            if (c == '\\')
            {
                i++;
                continue;
            }

            if (c == '[')
                in_char_class = true;
            if (c == ']')
                in_char_class = false;

            if (c == '/' && !in_char_class)
                break;
        }

        if (i > 1)
            return expr_.substr(1, i - 1);
        else
            return String();
    }

    String flags() const
    {
        bool in_char_class = false;

        size_t i = 1;
        for (; i < expr_.length(); i++)
        {
            uni_char c = expr_[i];
            if (c == '\\')
            {
                i++;
                continue;
            }

            if (c == '[')
                in_char_class = true;
            if (c == ']')
                in_char_class = false;

            if (c == '/' && !in_char_class)
                break;
        }

        return expr_.skip(i + 1);
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class FunctionExpression  : public Expression
{
private:
    FunctionLiteral *fun_;

public:
    FunctionExpression(Location loc, FunctionLiteral *fun)
        : Expression(loc)
        , fun_(fun) {}

    const FunctionLiteral *function() const
    {
        return fun_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ThisLiteral : public Expression
{
public:
    ThisLiteral(Location loc)
        : Expression(loc) {}

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class IdentifierLiteral : public Expression
{
private:
    String value_;

public:
    IdentifierLiteral(Location loc, String value)
        : Expression(loc)
        , value_(value) {}

    String value() const
    {
        return value_;
    }

    /**
     * @copydoc Expression::is_left_hand_expr
     */
    bool is_left_hand_expr() const
    {
        return true;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class NullLiteral : public Expression
{
public:
    NullLiteral(Location loc)
        : Expression(loc) {}

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class BoolLiteral : public Expression
{
private:
    bool value_;

public:
    BoolLiteral(Location loc, bool value)
        : Expression(loc)
        , value_(value) {}

    bool value() const
    {
        return value_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class NumberLiteral : public Expression
{
private:
    String value_;

public:
    NumberLiteral(Location loc, String value)
        : Expression(loc)
        , value_(value) {}

    String as_string() const
    {
        return value_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class StringLiteral : public Expression
{
private:
    String value_;

public:
    StringLiteral(Location loc, String value)
        : Expression(loc)
        , value_(value) {}

    String value() const
    {
        return value_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class FunctionLiteral : public Expression, public Declaration
{
public:
    /**
     * @brief Contexts in which the literal is present.
     */
    enum Type
    {
        TYPE_DECLARATION,   ///< The literal is a function declaration.
        TYPE_EXPRESSION     ///< The literal is a function expression.
    };

private:
    bool strict_mode_;
    bool needs_args_obj_;   ///< true if calling the function requires the arguments object to be created.
    String name_;           ///< Function name, may be empty for anonymous functions.
    StringVector params_;
    StatementVector body_;
    DeclarationVector decl_;
    Type type_;

public:
    FunctionLiteral(Location loc, String name)
        : Expression(loc)
        , Declaration(this)
        , strict_mode_(false)
        , needs_args_obj_(false)
        , name_(name)
        , type_(TYPE_DECLARATION) {}

    String name() const
    {
        return name_;
    }

    const StringVector &parameters() const
    {
        return params_;
    }

    const StatementVector &body() const
    {
        return body_;
    }

    StatementVector &body()
    {
        return body_;
    }

    const DeclarationVector &declarations() const
    {
        return decl_;
    }

    DeclarationVector &declarations()
    {
        return decl_;
    }

    void set_strict_mode(bool strict_mode)
    {
        strict_mode_ = strict_mode;
    }

    bool is_strict_mode() const
    {
        return strict_mode_;
    }

    void set_needs_args_obj(bool needs_args_obj)
    {
        needs_args_obj_ = needs_args_obj;
    }

    bool needs_args_obj() const
    {
        return needs_args_obj_;
    }

    bool has_param(const String &p)
    {
        return std::find(params_.begin(), params_.end(), p) != params_.end();
    }

    void push_back(Statement *stmt)
    {
        body_.push_back(stmt);
    }

    void push_decl(Declaration *decl)
    {
        decl_.push_back(decl);
    }

    void push_param(String p)
    {
        params_.push_back(p);
    }

    Type type() const
    {
        return type_;
    }

    void set_type(Type type)
    {
        type_ = type;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class VariableLiteral : public Expression, public Declaration
{
private:
    String name_;

public:
    VariableLiteral(Location loc, String name)
        : Expression(loc)
        , Declaration(this)
        , name_(name) {}

    String name() const
    {
        return name_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ArrayLiteral : public Expression
{
private:
    ExpressionVector values_;

public:
    ArrayLiteral(Location loc, ExpressionVector values)
        : Expression(loc)
        , values_(values) {}

    const ExpressionVector &values() const
    {
        return values_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ObjectLiteral : public Expression
{
public:
    // FIXME: Subclass.
    class Property
    {
    public:
        enum Type
        {
            DATA,
            GETTER,
            SETTER
        };

    private:
        Type type_;
        Expression *key_;
        Expression *val_;
        String accessor_name_;

    public:
        Property(Expression *key, Expression *val)
            : type_(DATA)
            , key_(key)
            , val_(val) {}

        Property(bool is_setter, Expression *val, const String &accessor_name)
            : type_(is_setter ? SETTER : GETTER)
            , key_(NULL)
            , val_(val)
            , accessor_name_(accessor_name) {}

        Type type() const
        {
            return type_;
        }

        Expression *key() const
        {
            return key_;
        }

        Expression *val() const
        {
            return val_;
        }

        const String &accessor_name() const
        {
            return accessor_name_;
        }
    };

    typedef std::vector<ObjectLiteral::Property *,
                        gc_allocator<ObjectLiteral::Property *> > PropertyVector;

private:
    PropertyVector props_;

public:
    ObjectLiteral(Location loc)
        : Expression(loc) {}

    const PropertyVector &properties() const
    {
        return props_;
    }

    bool contains_data_prop(const String &prop_name) const
    {
        PropertyVector::const_iterator it;
        for (it = props_.begin(); it != props_.end(); ++it)
        {
            Property *cur_prop = *it;
            if (cur_prop->type() != Property::DATA)
                continue;

            Expression *cur_key = cur_prop->key();
            StringLiteral *cur_str_lit = dynamic_cast<StringLiteral *>(cur_key);
            NumberLiteral *cur_num_lit = dynamic_cast<NumberLiteral *>(cur_key);

            if (cur_str_lit)
            {
                if (cur_str_lit->value() == prop_name)
                    return true;
            }
            if (cur_num_lit)
            {
                if (cur_num_lit->as_string() == prop_name)
                    return true;
            }
        }

        return false;
    }

    bool contains_accessor_prop(const String &accessor_name)
    {
        PropertyVector::const_iterator it;
        for (it = props_.begin(); it != props_.end(); ++it)
        {
            Property *cur_prop = *it;
            if (cur_prop->type() == Property::DATA)
                continue;

            if (cur_prop->accessor_name() == accessor_name)
                return true;
        }

        return false;
    }

    bool contains(Property *prop) const
    {
        StringLiteral *data_str_lit = NULL;
        NumberLiteral *data_num_lit = NULL;

        if (prop->type() == Property::DATA)
        {
            data_str_lit = dynamic_cast<StringLiteral *>(prop->key());
            data_num_lit = dynamic_cast<NumberLiteral *>(prop->key());
            assert((data_str_lit && !data_num_lit) || (!data_str_lit && data_num_lit));
        }

        PropertyVector::const_iterator it;
        for (it = props_.begin(); it != props_.end(); ++it)
        {
            Property *cur_prop = *it;
            if (cur_prop->type() != prop->type())
                continue;

            if (cur_prop->type() == Property::DATA)
            {
                Expression *cur_key = cur_prop->key();
                StringLiteral *cur_str_lit = dynamic_cast<StringLiteral *>(cur_key);
                NumberLiteral *cur_num_lit = dynamic_cast<NumberLiteral *>(cur_key);

                if (cur_str_lit && data_str_lit)
                {
                    if (cur_str_lit->value() == data_str_lit->value())
                        return true;
                }
                if (cur_num_lit && data_num_lit)
                {
                    if (cur_num_lit->as_string() == data_num_lit->as_string())
                        return true;
                }
            }
            else
            {
                if (cur_prop->accessor_name() == prop->accessor_name())
                    return true;
            }
        }

        return false;
    }

    void push_back(ObjectLiteral::Property *prop)
    {
        props_.push_back(prop);
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class NothingLiteral : public Expression
{
public:
    NothingLiteral(Location loc)
        : Expression(loc) {}

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class EmptyStatement : public Statement
{
public:
    EmptyStatement()
        : Statement(Location()) {}

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ExpressionStatement : public Statement
{
private:
    Expression *expr_;

public:
    ExpressionStatement(Expression *expr)
        : Statement(expr->location())
        , expr_(expr) {}

    Expression *expression() const
    {
        return expr_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class BlockStatement : public LabeledStatement
{
private:
    bool hidden_;
    StatementVector body_;

public:
    BlockStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    void set_hidden(bool hidden)
    {
        hidden_ = hidden;
    }

    bool is_hidden() const
    {
        return hidden_;
    }

    const StatementVector &body() const
    {
        return body_;
    }

    void push_back(Statement *stmt)
    {
        body_.push_back(stmt);
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class IfStatement : public Statement
{
private:
    Expression *cond_;
    Statement *if_stmt_;
    Statement *else_stmt_;

public:
    IfStatement(Location loc, Expression *cond, Statement *if_stmt, Statement *else_stmt)
        : Statement(loc)
        , cond_(cond)
        , if_stmt_(if_stmt)
        , else_stmt_(else_stmt) {}

    Expression *condition() const
    {
        return cond_;
    }

    Statement *if_statement() const
    {
        return if_stmt_;
    }

    Statement *else_statement() const
    {
        return else_stmt_;
    }

    bool has_else() const
    {
        return else_stmt_ != NULL;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class DoWhileStatement : public LabeledStatement
{
private:
    Expression *cond_;
    Statement *body_;

public:
    DoWhileStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    bool has_condition() const
    {
        return cond_ != NULL;
    }

    Expression *condition() const
    {
        return cond_;
    }

    void set_condition(Expression *cond)
    {
        cond_ = cond;
    }

    Statement *body() const
    {
        return body_;
    }

    void set_body(Statement *body)
    {
        body_ = body;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class WhileStatement : public LabeledStatement
{
private:
    Expression *cond_;
    Statement *body_;

public:
    WhileStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    Expression *condition() const
    {
        return cond_;
    }

    void set_condition(Expression *cond)
    {
        cond_ = cond;
    }

    Statement *body() const
    {
        return body_;
    }

    void set_body(Statement *body)
    {
        body_ = body;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ForInStatement : public LabeledStatement
{
private:
    Expression *decl_;
    Expression *enum_;
    Statement *body_;

public:
    ForInStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    Expression *declaration() const
    {
        return decl_;
    }

    void set_declaration(Expression *decl)
    {
        decl_ = decl;
    }

    Expression *enumerable() const
    {
        return enum_;
    }

    void set_enumerable(Expression *enumerable)
    {
        enum_ = enumerable;
    }

    Statement *body() const
    {
        return body_;
    }

    void set_body(Statement *body)
    {
        body_ = body;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ForStatement : public LabeledStatement
{
private:
    Statement *init_;
    Expression *cond_;
    Expression *next_;
    Statement *body_;

public:
    ForStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    bool has_initializer() const
    {
        return init_ != NULL;
    }

    Statement *initializer() const
    {
        return init_;
    }

    void set_initializer(Statement *init)
    {
        init_ = init;
    }

    bool has_condition() const
    {
        return cond_ != NULL;
    }

    Expression *condition() const
    {
        return cond_;
    }

    void set_condition(Expression *cond)
    {
        cond_ = cond;
    }

    bool has_next() const
    {
        return next_ != NULL;
    }

    Expression *next() const
    {
        return next_;
    }

    void set_next(Expression *next)
    {
        next_ = next;
    }

    Statement *body() const
    {
        return body_;
    }

    void set_body(Statement *body)
    {
        body_ = body;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ContinueStatement : public Statement
{
private:
    const LabeledStatement *target_;

public:
    ContinueStatement(Location loc, const LabeledStatement *target)
        : Statement(loc)
        , target_(target) {}

    const LabeledStatement *target() const
    {
        return target_;
    }

    bool has_target() const
    {
        return target_ != NULL;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class BreakStatement : public Statement
{
private:
    const LabeledStatement *target_;

public:
    BreakStatement(Location loc, const LabeledStatement *target)
        : Statement(loc)
        , target_(target) {}

    const LabeledStatement *target() const
    {
        return target_;
    }

    bool has_target() const
    {
        return target_ != NULL;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ReturnStatement : public Statement
{
private:
    Expression *expr_;

public:
    ReturnStatement(Location loc, Expression *expr)
        : Statement(loc)
        , expr_(expr) {}

    Expression *expression() const
    {
        return expr_;
    }

    bool has_expression() const
    {
        return expr_ != NULL;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class WithStatement : public Statement
{
private:
    Expression *expr_;
    Statement *body_;

public:
    WithStatement(Location loc, Expression *expr, Statement *body)
        : Statement(loc)
        , expr_(expr)
        , body_(body) {}

    Expression *expression() const
    {
        return expr_;
    }

    Statement *body() const
    {
        return body_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class SwitchStatement : public LabeledStatement
{
public:
    class CaseClause
    {
    private:
        Expression *label_;     ///< If NULL the clause is a default clause.
        StatementVector stmts_;

    public:
        CaseClause(Expression *label, StatementVector stmts)
            : label_(label)
            , stmts_(stmts) {}

        bool is_default() const
        {
            return label_ == NULL;
        }

        Expression *label() const
        {
            return label_;
        }

        const StatementVector &body() const
        {
            return stmts_;
        }
    };

    typedef std::vector<CaseClause *, gc_allocator<CaseClause *> > CaseClauseVector;

private:
    Expression *expr_;
    CaseClauseVector cases_;

public:
    SwitchStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    Expression *expression() const
    {
        return expr_;
    }

    void set_expression(Expression *expr)
    {
        expr_ = expr;
    }

    const CaseClauseVector &cases() const
    {
        return cases_;
    }

    void push_back(CaseClause *clause)
    {
        cases_.push_back(clause);
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class ThrowStatement : public Statement
{
private:
    Expression *expr_;

public:
    ThrowStatement(Location loc, Expression *expr)
        : Statement(loc)
        , expr_(expr) {}

    Expression *expression() const
    {
        return expr_;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class TryStatement : public LabeledStatement
{
private:
    Statement *try_block_;
    Statement *catch_block_;
    Statement *finally_block_;
    String catch_ident_;

public:
    TryStatement(Location loc, LabelList &labels)
        : LabeledStatement(loc, labels) {}

    Statement *try_block() const
    {
        return try_block_;
    }

    void set_try_block(Statement *try_block)
    {
        try_block_ = try_block;
    }

    Statement *catch_block() const
    {
        return catch_block_;
    }

    void set_catch_block(Statement *catch_block)
    {
        catch_block_ = catch_block;
    }

    Statement *finally_block() const
    {
        return finally_block_;
    }

    void set_finally_block(Statement *finally_block)
    {
        finally_block_ = finally_block;
    }

    bool has_catch_block() const
    {
        return catch_block_ != NULL;
    }

    bool has_finally_block() const
    {
        return finally_block_ != NULL;
    }

    String catch_identifier() const
    {
        return catch_ident_;
    }

    void set_catch_identifier(String ident)
    {
        catch_ident_ = ident;
    }

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

class DebuggerStatement : public Statement
{
public:
    DebuggerStatement(Location loc)
        : Statement(loc) {}

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE;
};

}
