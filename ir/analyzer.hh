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
#include <map>
#include <set>
#include "common/string.hh"
#include "parser/ast.hh"
#include "parser/visitor.hh"

namespace ir {

class AnalyzedVariable
{
public:
    enum Type
    {
        TYPE_PARAMETER,
        TYPE_DECLARATION,
        TYPE_CALLEE
    };

    enum Storage
    {
        STORAGE_UNALLOCATED,
        STORAGE_LOCAL,          // FIXME: STORAGE_STACK?
        STORAGE_LOCAL_EXTRA,    // FIXME: STORAGE_HEAP?
        STORAGE_CONTEXT
    };

public:
    struct Comparator
    {
        bool operator()(const AnalyzedVariable *v1, const AnalyzedVariable *v2) const
        {
            return v1->name() < v2->name();
        }
    };

private:
    Type type_;
    Storage storage_;

    String name_;

    /** Pointer to declaration object. Only valuid for declarative
     * variables. */
    parser::Declaration *decl_;

    /** Zero-based index of parameter in parameter list. Only valid for
     * parameter variables. */
    int param_index_;

public:
    AnalyzedVariable(const String &name, int index)
        : type_(TYPE_PARAMETER)
        , storage_(STORAGE_UNALLOCATED)
        , name_(name)
        , decl_(NULL)
        , param_index_(index) {}

    AnalyzedVariable(const String &name)
        : type_(TYPE_CALLEE)
        , storage_(STORAGE_UNALLOCATED)
        , name_(name)
        , decl_(NULL)
        , param_index_(-1) {}

    AnalyzedVariable(parser::Declaration *decl)
        : type_(TYPE_DECLARATION)
        , storage_(STORAGE_UNALLOCATED)
        , name_(decl->name())
        , decl_(decl)
        , param_index_(-1) {}

    Type type() const
    {
        return type_;
    }

    Storage storage() const
    {
        return storage_;
    }

    const String &name() const
    {
        return name_;
    }

    const parser::Declaration *declaration() const
    {
        return decl_;
    }

    void set_parameter_index(int param_index)
    {
        param_index_ = param_index;
    }

    int parameter_index() const
    {
        return param_index_;
    }

    bool is_parameter() const
    {
        return type_ == TYPE_PARAMETER;
    }

    bool is_declaration() const
    {
        return type_ == TYPE_DECLARATION;
    }

    bool is_callee() const
    {
        return type_ == TYPE_CALLEE;
    }

    bool is_allocated() const
    {
        return storage_ != STORAGE_UNALLOCATED;
    }

    void allocate_to(Storage storage)
    {
        storage_ = storage;
    }

    bool operator<(const AnalyzedVariable &rhs) const
    {
        return name_ < rhs.name_;
    }
};

typedef std::set<AnalyzedVariable *, AnalyzedVariable::Comparator, gc_allocator<AnalyzedVariable *> > AnalyzedVariableSet;

/**
 * @brief Data collected on functions.
 */
class AnalyzedFunction
{
private:
    parser::FunctionLiteral *fun_;  ///< Function literal to which this object is associated.
    AnalyzedVariableSet vars_;      ///< AnalyzedVariable declarations.

    /** true if the function must register variables in the execution context
     * because a call to eval might want to access them dynamicall by name. */
    bool tainted_by_eval_;

    std::set<int> referenced_scopes_;

public:
    AnalyzedFunction(parser::FunctionLiteral *fun)
        : fun_(fun)
        , tainted_by_eval_(false) {}

    parser::FunctionLiteral *literal() const
    {
        return fun_;
    }

    bool tainted_by_eval() const
    {
        return tainted_by_eval_;
    }

    void set_tainted_by_eval(bool tainted_by_eval)
    {
        tainted_by_eval_ = tainted_by_eval;
    }

    const std::set<int> &referenced_scopes() const
    {
        return referenced_scopes_;
    }

    void link_referenced_scope(int scope)
    {
        referenced_scopes_.insert(scope);
    }

    AnalyzedVariableSet &variables()
    {
        return vars_;
    }

    AnalyzedVariable *find_variable(const String &name)
    {
        AnalyzedVariableSet::iterator it_var;
        for (it_var = vars_.begin(); it_var != vars_.end(); ++it_var)
        {
            AnalyzedVariable *var = *it_var;

            if (var->name() == name)
                return var;
        }

        return NULL;
    }

    void add_variable(AnalyzedVariable *var)
    {
        std::pair<AnalyzedVariableSet::iterator, bool> res = vars_.insert(var);
        if (!res.second)
        {
            vars_.erase(res.first);
            vars_.insert(var);
        }
    }

    size_t num_locals() const
    {
        size_t locals = 0;

        AnalyzedVariableSet::iterator it_var;
        for (it_var = vars_.begin(); it_var != vars_.end(); ++it_var)
        {
            AnalyzedVariable *var = *it_var;

            if (var->storage() == AnalyzedVariable::STORAGE_LOCAL)
                locals++;
        }

        return locals;
    }

    size_t num_extra() const
    {
        size_t extra = 0;

        AnalyzedVariableSet::iterator it_var;
        for (it_var = vars_.begin(); it_var != vars_.end(); ++it_var)
        {
            AnalyzedVariable *var = *it_var;

            if (var->storage() == AnalyzedVariable::STORAGE_LOCAL_EXTRA)
                extra++;
        }

        return extra;
    }
};

class Analyzer : public parser::Visitor
{
public:
    typedef std::map<parser::FunctionLiteral *, AnalyzedFunction,
                     std::less<parser::FunctionLiteral *>,
                     gc_allocator<std::pair<parser::FunctionLiteral *,
                                            AnalyzedFunction> > > AnalyzedFunctionMap;

private:
    /**
     * @brief Object representing a lexical context.
     */
    class LexicalEnvironment
    {
    public:
        enum Type
        {
            TYPE_OBJECT,        ///< Object based context.
            TYPE_DECLARATIVE,   ///< Declarative context.
        };

    private:
        Type type_;
        parser::FunctionLiteral *fun_;  ///< Function literal associated with context.

    public:
        LexicalEnvironment(Type type, parser::FunctionLiteral *fun)
            : type_(type)
            , fun_(fun) {}

        bool is_obj() { return type_ == TYPE_OBJECT; }
        bool is_decl() { return type_ == TYPE_DECLARATIVE; }

        parser::FunctionLiteral *function() const { return fun_; }
    };

    typedef std::vector<LexicalEnvironment, gc_allocator<LexicalEnvironment> > LexicalEnvironmentVector;
    LexicalEnvironmentVector lex_envs_;

private:
    AnalyzedFunctionMap functions_;

    void reset();

private:
    void visit_fun(parser::FunctionLiteral *lit);

    virtual void visit_binary_expr(parser::BinaryExpression *expr) override;
    virtual void visit_unary_expr(parser::UnaryExpression *expr) override;
    virtual void visit_assign_expr(parser::AssignmentExpression *expr) override;
    virtual void visit_cond_expr(parser::ConditionalExpression *expr) override;
    virtual void visit_prop_expr(parser::PropertyExpression *expr) override;
    virtual void visit_call_expr(parser::CallExpression *expr) override;
    virtual void visit_call_new_expr(parser::CallNewExpression *expr) override;
    virtual void visit_regular_expr(parser::RegularExpression *expr) override;
    virtual void visit_fun_expr(parser::FunctionExpression *expr) override;

    virtual void visit_this_lit(parser::ThisLiteral *lit) override;
    virtual void visit_ident_lit(parser::IdentifierLiteral *lit) override;
    virtual void visit_null_lit(parser::NullLiteral *lit) override;
    virtual void visit_bool_lit(parser::BoolLiteral *lit) override;
    virtual void visit_num_lit(parser::NumberLiteral *lit) override;
    virtual void visit_str_lit(parser::StringLiteral *lit) override;
    virtual void visit_fun_lit(parser::FunctionLiteral *lit) override;
    virtual void visit_var_lit(parser::VariableLiteral *lit) override;
    virtual void visit_array_lit(parser::ArrayLiteral *lit) override;
    virtual void visit_obj_lit(parser::ObjectLiteral *lit) override;
    virtual void visit_nothing_lit(parser::NothingLiteral *lit) override;

    virtual void visit_empty_stmt(parser::EmptyStatement *stmt) override;
    virtual void visit_expr_stmt(parser::ExpressionStatement *stmt) override;
    virtual void visit_block_stmt(parser::BlockStatement *stmt) override;
    virtual void visit_if_stmt(parser::IfStatement *stmt) override;
    virtual void visit_do_while_stmt(parser::DoWhileStatement *stmt) override;
    virtual void visit_while_stmt(parser::WhileStatement *stmt) override;
    virtual void visit_for_in_stmt(parser::ForInStatement *stmt) override;
    virtual void visit_for_stmt(parser::ForStatement *stmt) override;
    virtual void visit_cont_stmt(parser::ContinueStatement *stmt) override;
    virtual void visit_break_stmt(parser::BreakStatement *stmt) override;
    virtual void visit_ret_stmt(parser::ReturnStatement *stmt) override;
    virtual void visit_with_stmt(parser::WithStatement *stmt) override;
    virtual void visit_switch_stmt(parser::SwitchStatement *stmt) override;
    virtual void visit_throw_stmt(parser::ThrowStatement *stmt) override;
    virtual void visit_try_stmt(parser::TryStatement *stmt) override;
    virtual void visit_dbg_stmt(parser::DebuggerStatement *stmt) override;
    
public:
    AnalyzedFunction *lookup(parser::FunctionLiteral *fun);

    /**
     * Analyzes code given AST through the specific root function.
     */
    void analyze(parser::FunctionLiteral *root);
};

}
