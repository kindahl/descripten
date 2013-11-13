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
#include <set>
#include <unordered_map>
#include "config.hh"
#include "common/string.hh"
#include "common/proxy.hh"
#include "parser/ast.hh"
#include "parser/visitor.hh"
#include "analyzer.hh"
#include "ir.hh"

/** Name of the function representing the global program. */
#define RUNTIME_GLOBAL_FUNCTION_NAME    "_global_main"      // FIXME: Should move to config header.

namespace ir {

class TemplateBlock;

// FIXME: Extract to own header.
class Scope
{
public:
    enum Type
    {
        TYPE_DEFAULT,
        TYPE_ITERATION,
        TYPE_SWITCH,
        TYPE_FUNCTION,
        TYPE_WITH
    };

private:
    Type type_;                     ///< Scope type.
    std::set<std::string> labels_;  ///< Labels referring to this scope.

    Block *cnt_target_;             ///< Block to jump to when continuing in this scope. This is only valid for iteration statements.
    Block *brk_target_;             ///< Block to jump to when breaking in this scope. This is only valid for iteration statements.

    TemplateBlock *epilogue_;       ///< Epilogue to execute when leaving this scope.

    std::map<String, Value *,
             std::less<String>,
             gc_allocator<std::pair<String, Value *> > > local_map_;    ///< Local variables declared in scope, mapped to index in locals stack.

    typedef std::map<uint64_t, uint16_t> CacheMap;
    CacheMap cache_map_;

    uint16_t next_cache_id_;

    typedef std::map<int, Value *,
                     std::less<int>,
                     gc_allocator<std::pair<int, Value *> > > StackMap;
    StackMap stack_map_;

    typedef std::set<Value *, std::less<Value *>, gc_allocator<Value *> > TemporarySet;
    TemporarySet used_temporaries_;

    /** List of allocated, but currently free temporary slots. */
    std::vector<Value *, gc_allocator<Value *> > free_temporaries_;

    int max_temporaries_;

    /** Number of values to allocate in the stack frame. */
    ProxySource<size_t> call_frame_value_count_;

public:
    /**
     * Creates a new scope for an iteration statement.
     * @param [in] cnt_target Block to jump to when executing a continue statement.
     * @param [in] brk_target Block to jump to when executing a break statement.
     */
    Scope(Block *cnt_target,
          Block *brk_target)
        : type_(TYPE_ITERATION)
        , cnt_target_(cnt_target)
        , brk_target_(brk_target)
        , epilogue_(NULL)
        , next_cache_id_(0)
        , max_temporaries_(0) {}

    /**
     * Creates a new scope for a breakable statement.
     * @param [in] brk_target Block to jump to when executing a break statement.
     */
    Scope(Type type, Block *brk_target)
        : type_(type)
        , cnt_target_(NULL)
        , brk_target_(brk_target)
        , epilogue_(NULL)
        , next_cache_id_(0)
        , max_temporaries_(0) {}

    /**
     * Creates a new scope for a non-iteration statement.
     * @param [in] is_function_scope true if the scope is a root function scope.
     */
    Scope(Type type)
        : type_(type)
        , cnt_target_(NULL)
        , brk_target_(NULL)
        , epilogue_(NULL)
        , next_cache_id_(0)
        , max_temporaries_(0) {}

    ProxySource<size_t> &call_frame_value_count()
    {
        return call_frame_value_count_;
    }

    void commit()
    {
        call_frame_value_count_.set_value(num_locals() + num_temporaries());
    }

    Type type() const
    {
        return type_;
    }

    bool has_label(const std::string &label) const
    {
        return labels_.count(label) == 1;
    }

    void push_label(const std::string &label)
    {
        labels_.insert(label);
    }

    Block *continue_target() const
    {
        return cnt_target_;
    }

    Block *break_target() const
    {
        return brk_target_;
    }

    bool has_epilogue() const
    {
        return epilogue_ != NULL;
    }

    TemplateBlock *epilogue()
    {
        assert(epilogue_);
        return epilogue_;
    }

    void set_epilogue(TemplateBlock *epilogue)
    {
        epilogue_ = epilogue;
    }

    size_t num_locals() const
    {
        return local_map_.size();
    }

    void add_local(const String &name, Value *val)
    {
        local_map_[name] = val;
    }

    bool has_local(const String &name)
    {
        return local_map_.count(name) > 0;
    }

    Value *get_local(const String &name)
    {
        return local_map_[name];
    }

    size_t num_temporaries() const
    {
        return max_temporaries_;
    }

    /**
     * Get a new temporary value from the pool of temporaries.
     */
    Value *get_temporary()
    {
        if (!free_temporaries_.empty())
        {
            Value *tmp = free_temporaries_.back();
            free_temporaries_.pop_back();
            used_temporaries_.insert(tmp);
            return tmp;
        }

        Value *tmp = new (GC)ArrayElementConstant(
            new (GC)ValuePointer(), num_locals() + max_temporaries_++);
        used_temporaries_.insert(tmp);

        return tmp;
    }

    /**
     * Returns a temporay into the pool of temporaries. If @tmp is not a
     * temporary, this function will do nothing.
     */
    void put_temporary(Value *tmp)
    {
        TemporarySet::iterator it = used_temporaries_.find(tmp);
        assert(it != used_temporaries_.end());

        free_temporaries_.push_back(*it);
        used_temporaries_.erase(it);
    }

    uint16_t get_ctx_cache_id(uint64_t key)
    {
        CacheMap::iterator it = cache_map_.find(key);
        if (it == cache_map_.end())
        {
            uint16_t cid = next_cache_id_++;
            if (next_cache_id_ >= FEATURE_CONTEXT_CACHE_SIZE)
                next_cache_id_ = 0;

            cache_map_.insert(std::make_pair(key, cid));
            return cid;
        }

        return it->second;
    }

    void add_scope_stack(int hops, Value *val)
    {
        stack_map_[hops] = val;
    }

    Value *get_scope_stack(int hops)
    {
        StackMap::iterator it = stack_map_.find(hops);
        assert(it != stack_map_.end());
        return it->second;
    }
};

class Compiler;

class ValueAllocator
{
public:
    virtual ~ValueAllocator() {}

    virtual Value *get() = 0;
    virtual void release(Value *value) = 0;
};

class FixedValueAllocator : public ValueAllocator
{
private:
    Value *val_;

public:
    FixedValueAllocator(Value *val)
        : val_(val) {}

    virtual Value *get() OVERRIDE
    {
        return val_;
    }

    virtual void release(Value *value) OVERRIDE
    {
    }
};

class SingleValueAllocator : public ValueAllocator
{
private:
    ValueAllocator &allocator_;
    Value *value_;

public:
    SingleValueAllocator(ValueAllocator &allocator)
        : allocator_(allocator)
        , value_(NULL) {}

    virtual Value *get() OVERRIDE
    {
        if (value_ == NULL)
            value_ = allocator_.get();

        return value_;
    }

    virtual void release(Value *value) OVERRIDE
    {
        assert(value_);

        allocator_.release(value);
        value_ = NULL;
    }
};

class TemporaryValueAllocator : public ValueAllocator
{
private:
    TemporaryValueAllocator *parent_;
    Compiler *compiler_;
    Scope *scope_;

    std::set<Value *, std::less<Value *>, gc_allocator<Value *> > temporaries_;

public:
    TemporaryValueAllocator(Compiler *compiler);
    ~TemporaryValueAllocator();

    TemporaryValueAllocator *parent();

    virtual Value *get() OVERRIDE;
    virtual void release(Value *value) OVERRIDE;
};

class ValueHandle
{
private:
    ValueAllocator *allocator_;
    Value *value_;

    ValueHandle(ValueAllocator *allocator)
        : allocator_(allocator)
        , value_(NULL) {}

public:
    ValueHandle()
        : allocator_(NULL)
        , value_(NULL) {}
    ValueHandle(Value *value)
        : allocator_(NULL)
        , value_(value) {}

    static ValueHandle lazy(ValueAllocator *allocator)
    {
        return ValueHandle(allocator);
    }

    Value *get()
    {
        if (value_ == NULL && allocator_ == NULL)
            return NULL;

        if (value_ == NULL)
            value_ = allocator_->get();

        return value_;
    }

    void release()
    {
        if (allocator_)
        {
            allocator_->release(value_);
            value_ = NULL;
        }
    }

    operator Value *()
    {
        return get();
    }

    Value *operator->()
    {
        assert(value_);
        return value_;
    }
};

/**
 * @brief Compiles a syntax tree into an intermediate representation.
 */
class Compiler : public parser::ValueVisitor2<ValueHandle, Function *, ValueAllocator *>
{
public:
    friend class FinallyTemplateBlock;  // FIXME: Ugly.

    friend class TemporaryValueAllocator;

private:
    /**
     * Maps strings to string ids. A string id is an unsigned 32-bit integer
     * that is unique for all instances of strings that are equal. For example,
     * String foo1("foo") and String foo2("foo") will share the same id. The
     * ids are directly comparable to interned StringIds and can be seen as the
     * compile-time counterpart of interned strings.
     */
    typedef std::unordered_map<String, uint32_t, String::Hash,
                               std::equal_to<String>,
                               gc_allocator<std::pair<String, uint32_t> > > StringIdMap;
    StringIdMap strings_;

private:
    bool is_in_epilogue_;

    typedef std::vector<Scope *, gc_allocator<Scope *> > ScopeVector;
    ScopeVector scopes_;

    Scope *current_fun_scope(bool accept_with);
    Value *get_local(const String &name, Function *fun);

private:
    Analyzer analyzer_;

    Module *module_;

private:
    typedef std::vector<TemplateBlock *,
                        gc_allocator<TemplateBlock *> > ExceptionActionVector;
    ExceptionActionVector exception_actions_;   ///< Actions to perform when an exception is thrown.

    inline TemplateBlock *exception_action() const
    {
        assert(!exception_actions_.empty());
        return exception_actions_.back();
    }

private:
    std::vector<TemporaryValueAllocator *> temporaries_stack_;

private:
    Scope *unroll_for_continue(Function *fun, const std::string &label = "");
    Scope *unroll_for_break(Function *fun, const std::string &label = "");
    Scope *unroll_for_return(Function *fun);

    /**
     * Returns the id for all strings equal to the specified string. If no
     * other equal strings have been classified a new unique id will be
     * returned and used for future equals.
     *
     * @param [in] str String to get id for.
     * @return String id.
     */
    uint32_t get_str_id(const String &str);

    /**
     * Returns the cache id for a context access.
     * @param [in] key Key to access in context.
     * @return Cache id for a context identifier.
     */
    uint16_t get_ctx_cid(uint64_t key);

    /**
     * Returns a raw unsigned 64-bit property key identifying an indexed
     * property.
     * @param [in] index Property index.
     * @return Property key for specified indexed property.
     */
    uint64_t get_prp_key(uint32_t index);


    /**
     * Returns a raw unsigned 64-bit property key identifying a named property.
     * @param [in] str_id Property string id.
     * @return Property key for specified named property.
     */
    uint64_t get_prp_key(const String &str);

    /**
     * Emits a property get instruction. There are different property get
     * instructions that perform differently on different input. This function
     * selects the best property get instruction given the meta property load
     * instruction.
     * @param [in] dst Destination value where the result of the property get
     *                 instruction should be written.
     * @param [in] fun Current function.
     * @param [in] prp_load Meta instruction describing the load semantics.
     * @return Result of property get instruction.
     */
    Value *expand_prp_get(Value *dst, Function *fun,
                          MetaPropertyLoadInstruction *prp_load);

    /**
     * Emits a property put instruction. There are different property put
     * instructions that perform differently on different input. This function
     * selects the best property put instruction given the meta property load
     * instruction.
     * @param [in] dst Destination value where the result of the property put
     *                 instruction should be written.
     * @param [in] fun Current function.
     * @param [in] prp_load Meta instruction describing the load semantics.
     * @return Result of property put instruction.
     */
    Value *expand_prp_put(Value *val, Function *fun,
                          MetaPropertyLoadInstruction *prp_load);

    /**
     * Extracts the value from a reference. This essentially implements the
     * ECMA-262 function GetValue.
     * @param [in] ref Reference to get value from.
     * @param [in] dst Destination value where the dereferenced ref should be
     *             written.
     * @param [in] fun Current function.
     * @param [in] expt_block Block to jump to in case of failure.
     * @return If ref is a reference the dereferenced value is returned, if v
     *         is not a reference v is returned.
     */
    ValueHandle expand_ref_get_inplace(ValueHandle &ref,
                                       ValueHandle &dst,
                                       Function *fun, Block *expt_block);

    ValueHandle expand_ref_get_inplace_lazy(ValueHandle &ref,
                                            Function *fun, Block *expt_block,
                                            ValueAllocator &allocator);

    /**
     * Extracts the value from a reference. This essentially implements the
     * ECMA-262 function GetValue.
     * @param [in] ref Reference to get value from.
     * @param [in] dst Destination value where the dereferenced ref should be
     *             written.
     * @param [in] fun Current function.
     * @param [in] done_block Block to jump to in case of success.
     * @param [in] expt_block Block to jump to in case of failure.
     * @return dst.
     */
    ValueHandle expand_ref_get(ValueHandle &ref, ValueHandle &dst,
                               Function *fun,
                               Block *done_block, Block *expt_block);

    ValueHandle expand_ref_get_lazy(ValueHandle &ref, Function *fun,
                                    Block *done_block, Block *expt_block,
                                    ValueAllocator &allocator);

    /**
     * Writes a value to a reference. This essentially implements the ECMA-262
     * function PutValue.
     * @param [in] ref Reference to write value to.
     * @param [in] val Value to write.
     * @param [in] fun Current function.
     * @param [in] expt_block Block to jump to in case of failure.
     */
    void expand_ref_put_inplace(ValueHandle &ref, ValueHandle &val,
                                Function *fun, Block *expt_block);

    /**
     * Writes a value to a reference. This essentially implements the ECMA-262
     * function PutValue.
     * @param [in] ref Reference to write value to.
     * @param [in] val Value to write.
     * @param [in] fun Current function.
     * @param [in] done_block Block to jump to in case of success.
     * @param [in] expt_block Block to jump to in case of failure.
     */
    void expand_ref_put(ValueHandle &ref, ValueHandle &val,
                        Function *fun, Block *done_block, Block *expt_block);

    void reset();

    Function *parse_fun(const parser::FunctionLiteral *lit, bool is_global);

private:
    virtual ValueHandle parse_binary_expr(parser::BinaryExpression *expr,
                                          Function *fun,
                                          ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_unary_expr(parser::UnaryExpression *expr,
                                         Function *fun,
                                         ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_assign_expr(parser::AssignmentExpression *expr,
                                          Function *fun,
                                          ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_cond_expr(parser::ConditionalExpression *expr,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_prop_expr(parser::PropertyExpression *expr,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_call_expr(parser::CallExpression *expr,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_call_new_expr(parser::CallNewExpression *expr,
                                            Function *fun,
                                            ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_regular_expr(parser::RegularExpression *expr,
                                           Function *fun,
                                           ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_fun_expr(parser::FunctionExpression *expr,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;

    virtual ValueHandle parse_this_lit(parser::ThisLiteral *lit,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_ident_lit(parser::IdentifierLiteral *lit,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_null_lit(parser::NullLiteral *lit,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_bool_lit(parser::BoolLiteral *lit,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_num_lit(parser::NumberLiteral *lit,
                                      Function *fun,
                                      ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_str_lit(parser::StringLiteral *lit,
                                      Function *fun,
                                      ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_fun_lit(parser::FunctionLiteral *lit,
                                      Function *fun,
                                      ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_var_lit(parser::VariableLiteral *lit,
                                      Function *fun,
                                      ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_array_lit(parser::ArrayLiteral *lit,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_obj_lit(parser::ObjectLiteral *lit,
                                      Function *fun,
                                      ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_nothing_lit(parser::NothingLiteral *lit,
                                          Function *fun,
                                          ValueAllocator *rva) OVERRIDE;

    virtual ValueHandle parse_empty_stmt(parser::EmptyStatement *stmt,
                                         Function *fun,
                                         ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_expr_stmt(parser::ExpressionStatement *stmt,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_block_stmt(parser::BlockStatement *stmt,
                                         Function *fun,
                                         ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_if_stmt(parser::IfStatement *stmt,
                                      Function *fun,
                                      ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_do_while_stmt(parser::DoWhileStatement *stmt,
                                            Function *fun,
                                            ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_while_stmt(parser::WhileStatement *stmt,
                                         Function *fun,
                                         ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_for_in_stmt(parser::ForInStatement *stmt,
                                          Function *fun,
                                          ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_for_stmt(parser::ForStatement *stmt,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_cont_stmt(parser::ContinueStatement *stmt,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_break_stmt(parser::BreakStatement *stmt,
                                         Function *fun,
                                         ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_ret_stmt(parser::ReturnStatement *stmt,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_with_stmt(parser::WithStatement *stmt,
                                        Function *fun,
                                        ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_switch_stmt(parser::SwitchStatement *stmt,
                                          Function *fun,
                                          ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_throw_stmt(parser::ThrowStatement *stmt,
                                         Function *fun,
                                         ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_try_stmt(parser::TryStatement *stmt,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;
    virtual ValueHandle parse_dbg_stmt(parser::DebuggerStatement *stmt,
                                       Function *fun,
                                       ValueAllocator *rva) OVERRIDE;

public:
    Compiler();
    ~Compiler();

    /**
     * Compiles the the tree, starting  with the specific root function.
     * @param [in] root Function to compile.
     */
    Module *compile(const parser::FunctionLiteral *root);
};

}
