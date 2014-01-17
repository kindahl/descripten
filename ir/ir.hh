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
#include <cassert>
#include <vector>
#include <gc_cpp.h>
#include <gc/gc_allocator.h>
#include "common/core.hh"
#include "common/list.hh"
#include "common/proxy.hh"
#include "common/string.hh"

#ifdef DEBUG
#include <sstream>
#endif

namespace ir {

class Instruction;
class Value;

class Module;
class Function;
class Block;
class ArgumentsObjectInitInstruction;
class ArgumentsObjectLinkInstruction;
class ArrayInstruction;
class BinaryInstruction;
class BindExtraInitInstruction;
class BindExtraPtrInstruction;
class CallInstruction;
class CallKeyedInstruction;
class CallKeyedSlowInstruction;
class CallNamedInstruction;
class ValueInstruction;
class BranchInstruction;
class JumpInstruction;
class ReturnInstruction;
class MemoryAllocInstruction;
class MemoryStoreInstruction;
class MemoryElementPointerInstruction;
class StackAllocInstruction;
class StackFreeInstruction;
class StackPushInstruction;
class ContextSetStrictInstruction;
class ContextEnterCatchInstruction;
class ContextEnterWithInstruction;
class ContextLeaveInstruction;
class ContextLookupInstruction;
class ContextGetInstruction;
class ContextPutInstruction;
class ContextDeleteInstruction;
class ExceptionSaveStateInstruction;
class ExceptionLoadStateInstruction;
class ExceptionSetInstruction;
class ExceptionClearInstruction;
class InitArgumentsInstruction;
class InitArgumentsObjectInstruction;
class Declaration;
class Link;
class PropertyLoadInstruction;
class PropertyDefineDataInstruction;
class PropertyDefineAccessorInstruction;
class PropertyIteratorNewInstruction;
class PropertyIteratorNextInstruction;
class PropertyGetInstruction;
class PropertyGetSlowInstruction;
class PropertyPutInstruction;
class PropertyPutSlowInstruction;
class PropertyDeleteInstruction;
class PropertyDeleteSlowInstruction;
class EsNewArrayInstruction;
class EsNewFunctionDeclarationInstruction;
class EsNewFunctionExpressionInstruction;
class EsNewObjectInstruction;
class EsNewRegexInstruction;
class EsBinaryInstruction;
class EsUnaryInstruction;
class ArrayElementConstant;
class FramePointer;
class ValuePointer;
class NullConstant;
class BooleanConstant;
class DoubleConstant;
class StringifiedDoubleConstant;
class StringConstant;
class ValueConstant;

class Resource;
class StringResource;

typedef IntrusiveLinkedList<Block> BlockList;
typedef std::vector<Function *, gc_allocator<Function *> > FunctionVector;
typedef std::vector<Instruction *,
                    gc_allocator<Instruction *> > InstructionVector;
typedef std::set<Instruction *, std::less<Instruction *>,
                 gc_allocator<Instruction *> > InstructionSet;
typedef std::vector<Resource *, gc_allocator<Resource *> > ResourceVector;

/**
 * @brief Node meta data.
 */
class Meta
{
private:
    String name_;   ///< Source file name.
    int beg_;       ///< Start position in source file.
    int end_;       ///< End position in source file.

public:
    Meta(String name, int beg, int end)
        : name_(name)
        , beg_(beg)
        , end_(end) {}
    Meta(int beg, int end)
        : beg_(beg)
        , end_(end) {}

    inline int begin() const { return beg_; }
    inline int end() const { return end_; }
};

/**
 * @brief Value type.
 */
class Type
{
public:
    static const Type *_void();
    static const Type *boolean();
    static const Type *_double();
    static const Type *string();
    static const Type *value();
    static const Type *reference();

public:
    struct equal_to : std::binary_function<const Type *, const Type *, bool>
    {
        bool operator() (const Type *x, const Type *y) const
        {
            return x->equal_to(y);
        }
    };

    struct less : std::binary_function<const Type *, const Type *, bool>
    {
        bool operator() (const Type *x, const Type *y) const
        {
            return x->less_than(y);
        }
    };

public:
    enum Identifier
    {
        // Primitive types.
        ID_VOID,
        ID_BOOLEAN,
        ID_DOUBLE,
        ID_STRING,

        // Complex types.
        ID_VALUE,
        ID_REFERENCE,

        // Derived types.
        ID_ARRAY,
        ID_POINTER,
        ID_OPAQUE,
    };

private:
    Identifier id_; ///< Type identifier.

public:
    /**
     * Constructs a new void type.
     */
    Type()
        : id_(ID_VOID) {}

    /**
     * Constructs a new type.
     * @param [in] id Type identifier.
     */
    Type(Identifier id)
        : id_(id) {}

    /**
     * Destructs the object.
     */
    virtual ~Type() {}

    /**
     * @return Type identifier.
     */
    Identifier identifier() const { return id_; }

    /**
     * @return true if this type is a void type.
     */
    bool is_void() const { return id_ == ID_VOID; }

    /**
     * @return true if this type is a boolean type.
     */
    bool is_boolean() const { return id_ == ID_BOOLEAN; }

    /**
     * @return true if this type is a double type.
     */
    bool is_double() const { return id_ == ID_DOUBLE; }

    /**
     * @return true if this type is a string type.
     */
    bool is_string() const { return id_ == ID_STRING; }

    /**
     * @return true if this type is a value type.
     */
    bool is_value() const { return id_ == ID_VALUE; }

    /**
     * @return true if this type is a reference type.
     */
    bool is_reference() const { return id_ == ID_REFERENCE; }

    /**
     * @return true if this type is an array type.
     */
    bool is_array() const { return id_ == ID_ARRAY; }

    /**
     * @return true if this type is a pointer type.
     */
    bool is_pointer() const { return id_ == ID_POINTER; }

    /**
     * @return true if this type is an array type.
     */
    bool is_opaque() const { return id_ == ID_OPAQUE; }

#ifdef UNUSED
    /**
     * @return true if the right hand side is of the same type as this type.
     */
    bool operator==(const Type &rhs) const
    {
        return equal_to(&rhs);
    }

    /**
     * @return true if the right hand side is considered greater than this type.
     */
    bool operator<(const Type &rhs) const
    {
        return less_than(&rhs);
    }
#endif

    /**
     * Overridable function for testing type equality.
     * @param [in] rhs Right-hand-side to compare to.
     * @return true if the right hand side is of the same type as this type.
     */
    virtual bool equal_to(const Type *rhs) const
    {
        return id_ == rhs->id_;
    }

    /**
     * Overridable function for testing if this type is less than another type.
     * @param [in] rhs Right-hand-side to compare to.
     * @return true if the right hand side is considered greater than this type.
     */
    virtual bool less_than(const Type *rhs) const
    {
        return id_ < rhs->id_;
    }

#ifdef DEBUG
    virtual std::string as_string() const
    {
        switch (id_)
        {
            // Primitive types.
            case ID_VOID:
                return "void";
            case ID_BOOLEAN:
                return "boolean";
            case ID_DOUBLE:
                return "double";
            case ID_STRING:
                return "string";

            // Complex types.
            case ID_VALUE:
                return "value";
            case ID_REFERENCE:
                return "reference";

            // Derived types.
            case ID_ARRAY:
            case ID_POINTER:
            case ID_OPAQUE:
                assert(false);
                return "<error>";

            // Derived types.
            default:
                assert(false);
                return "<error>";
        }
    }
#endif
};

/**
 * @brief Reference type.
 */
class ReferenceType : public Type
{
private:
    const String name_;

public:
    /**
     * Constructs a new reference type.
     * @param [in] name Identifier of referred entity.
     */
    ReferenceType(const String &name)
        : Type(ID_REFERENCE)
        , name_(name) {}

    /**
     * @return Name of referred entity.
     */
    const String &name() const
    {
        return name_;
    }

#ifdef DEBUG
    virtual std::string as_string() const
    {
        std::stringstream str;
        str <<  "reference(" << name_.utf8() << ")";
        return str.str();
    }
#endif
};

/**
 * @brief Array type.
 */
class ArrayType : public Type
{
private:
    const Type *type_;
    size_t length_;

public:
    /**
     * Constructs a new array type.
     * @param [in] type Type of array elements.
     * @param [in] length Array length.
     */
    ArrayType(const Type *type, size_t length)
        : Type(ID_ARRAY)
        , type_(type)
        , length_(length) {}

    /**
     * @return Type of array elements.
     */
    const Type *type() const { return type_; }

    /**
     * @return Array length.
     */
    size_t length() const { return length_; }

    /**
     * @copydoc Type::less_than
     */
    virtual bool equal_to(const Type *rhs) const
    {
        if (identifier() != rhs->identifier())
            return false;

        const ArrayType *rhs_arr = static_cast<const ArrayType *>(rhs);
        return type_->equal_to(rhs_arr->type_) && length_ == rhs_arr->length_;
    }

    /**
     * @copydoc Type::less_than
     */
    virtual bool less_than(const Type *rhs) const
    {
        if (identifier() != rhs->identifier())
            return identifier() < rhs->identifier();

        const ArrayType *rhs_arr = static_cast<const ArrayType *>(rhs);

        if (!type_->equal_to(rhs_arr->type_))
            return type_->less_than(rhs_arr->type_);

        return length_ < rhs_arr->length_;
    }

#ifdef DEBUG
    virtual std::string as_string() const
    {
        std::stringstream str;
        str <<  type_->as_string() << "[" << length_ << "]";
        return str.str();
    }
#endif
};

/**
 * @brief Pointer type.
 */
class PointerType : public Type
{
private:
    const Type *type_;

public:
    /**
     * Constructs a new pointer type.
     * @param [in] type Type of referenced value.
     */
    PointerType(const Type *type)
        : Type(ID_POINTER)
        , type_(type) {}

    /**
     * @return Type of referenced value.
     */
    const Type *type() const { return type_; }

    /**
     * @copydoc Type::less_than
     */
    virtual bool equal_to(const Type *rhs) const
    {
        if (identifier() != rhs->identifier())
            return false;

        return type_->equal_to(static_cast<const PointerType *>(rhs)->type_);
    }

    /**
     * @copydoc Type::less_than
     */
    virtual bool less_than(const Type *rhs) const
    {
        if (identifier() != rhs->identifier())
            return identifier() < rhs->identifier();

        return type_->less_than(static_cast<const PointerType *>(rhs)->type_);
    }

#ifdef DEBUG
    virtual std::string as_string() const
    {
        return type_->as_string() + "*";
    }
#endif
};

/**
 * @brief Opaque type. Similar to forward declared structs in C.
 */
class OpaqueType : public Type
{
private:
    std::string name_;

public:
    /**
     * Constructs a new opaque type.
     * @param [in] opaque Name of opaque type.
     */
    OpaqueType(const std::string &name)
        : Type(ID_OPAQUE)
        , name_(name) {}

    /**
     * @return Opaque type name.
     */
    const std::string &name() const { return name_; }

    /**
     * @copydoc Type::less_than
     */
    virtual bool equal_to(const Type *rhs) const
    {
        if (identifier() != rhs->identifier())
            return false;

        return name_ == static_cast<const OpaqueType *>(rhs)->name_;
    }

    /**
     * @copydoc Type::less_than
     */
    virtual bool less_than(const Type *rhs) const
    {
        if (identifier() != rhs->identifier())
            return identifier() < rhs->identifier();

        std::less<std::string> cmp;
        return cmp(name_, static_cast<const OpaqueType *>(rhs)->name_);
    }

#ifdef DEBUG
    virtual std::string as_string() const
    {
        return std::string("opaque ") + name_;
    }
#endif
};

/**
 * @brief Resource root class.
 */
class Resource
{
public:
    /**
     * @brief Node visitor interface.
     */
    class Visitor
    {
    public:
        virtual ~Visitor() {}

        void visit(Resource *res) { res->accept(this); }

        virtual void visit_str_res(StringResource *res) = 0;
    };

protected:
    Resource() {}

public:
    virtual ~Resource() {}

    /**
     * Accept resource in visitor pattern.
     * @param [in] visitor The visitor.
     */
    virtual void accept(Visitor *visitor) = 0;
};

/**
 * @brief String resource.
 */
class StringResource : public Resource
{
private:
    String str_;
    uint32_t id_;

public:
    StringResource(const String &str, uint32_t id)
        : str_(str)
        , id_(id) {}

    const String &string() const
    {
        return str_;
    }

    uint32_t id() const
    {
        return id_;
    }

    /**
     * @copydoc Resource::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_str_res(this);
    }
};

/**
 * @brief Node root class.
 */
class Node
{
public:
    /**
     * @brief Node visitor interface.
     */
    class Visitor
    {
    public:
        virtual ~Visitor() {}

        void visit(Node *node) { node->accept(this); }

        virtual void visit_module(Module *module) = 0;
        virtual void visit_fun(Function *fun) = 0;
        virtual void visit_block(Block *block) = 0;
    };

private:
    Meta *meta_;

protected:
    Node()
        : meta_(NULL) {}

public:
    virtual ~Node() {}

    /**
     * Sets node meta data.
     * @param [in] meta Meta data to associate with node.
     */
    void set_meta(Meta *meta)
    {
        meta_ = meta;
    }

    /**
     * @return true if the node has associated meta data, false if it does not.
     */
    bool has_meta() const
    {
        return meta_ != NULL;
    }

    /**
     * @return Meta data associated with node.
     */
    Meta *meta() const
    {
        return meta_;
    }

    /**
     * Accept node in visitor pattern.
     * @param [in] visitor The visitor.
     */
    virtual void accept(Visitor *visitor) = 0;
};

/**
 * @brief Single compilation unit.
 */
class Module : public Node
{
private:
    FunctionVector functions_;
    ResourceVector resources_;

public:
    const FunctionVector &functions() const;

    void push_function(Function *fun);

    const ResourceVector &resources() const;

    void push_resource(Resource *res);

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_module(this);
    }
};

/**
 * @brief Function definition.
 *
 * Example:
 * define value %name { ... }
 */
class Function : public Node
{
private:
    bool is_global_;    ///< true if the function represents the program root.
    std::string name_;
    mutable BlockList blocks_;

public:
    Function(const std::string &name, bool is_global);

    /**
     * @return true if the program represents the program root, false if it
     *         does not.
     */
    bool is_global() const { return is_global_; }

    const std::string &name() const { return name_; }

    BlockList &mutable_blocks() { return blocks_; }

    const BlockList &blocks() const { return blocks_; }

    void push_block(Block *block);

    /**
     * @return Last block in function. This will never be NULL as functions
     *         are created with one initial block;
     */
    Block *last_block() const;

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_fun(this);
    }
};

/**
 * @brief Block containing a set of instructions.
 *
 * Blocks may have an optional label and its last instruction must be a
 * terminating instruction. Terminating instructions are not allowed anywhere
 * else but the last instruction in a block.
 */
class Block : public Node, public IntrusiveLinkedListNode<Block>
{
private:
    std::string label_;         ///< Optional label.
    InstructionVector instrs_;  ///< List of instructions.
    InstructionSet referrers_;  ///< List of instructions (not necessarily inside this block) referencing this block.

    void push_instr(Instruction *instr);

public:
    Block() {}
    Block(const std::string &label)
        : label_(label) {}

    bool empty() const { return instrs_.empty(); }

    /**
     * Adds a referrer to the block. A referrer is a terminating instruction
     * that refers to this block. The instruction can reside in any block,
     * including this block.
     * @param [in] instr Instruction referencing this block.
     */
    void add_referrer(Instruction *instr);

    /**
     * Removes a referrer from the block.
     * @param [in] instr Instruction that no longer refers to the block.
     * @see add_referrer
     */
    void remove_referrer(Instruction *instr);

    /**
     * @return List of instructions referencing this block.
     */
    const InstructionSet &referrers() const { return referrers_; }

    /**
     * @return Block label.
     */
    const std::string &label() const { return label_; }

    /**
     * @return Instructions contained within block.
     */
    const InstructionVector &instructions() const { return instrs_; }

    /**
     * @return Last instruction in block.
     * @pre The block contains at least one instruction.
     */
    Instruction *last_instr() const;

    Value *push_args_obj_init();
    Value *push_args_obj_link(Value *args, uint32_t index, Value *val);
    Value *push_arr_get(size_t index, Value *arr);
    Value *push_arr_put(size_t index, Value *arr, Value *val);
    Value *push_bin_add(Value *op1, Value *op2);
    Value *push_bin_sub(Value *op1, Value *op2);
    Value *push_bin_or(Value *op1, Value *op2);
    Value *push_bin_eq(Value *op1, Value *op2);
    Value *push_bnd_extra_init(uint32_t num_extra);
    Value *push_bnd_extra_ptr(uint32_t hops);
    Value *push_call(Value *fun, uint32_t argc, Value *res);
    Value *push_call_keyed(Value *obj, uint64_t key, uint32_t argc,
                           Value *res);
    Value *push_call_keyed_slow(Value *obj, Value *key, uint32_t argc,
                                Value *res);
    Value *push_call_named(uint64_t key, uint32_t argc, Value *res);
    Value *push_call_new(Value *fun, uint32_t argc, Value *res);
    Value *push_mem_store(Value *dst, Value *src);
    Value *push_mem_elm_ptr(Value *val, size_t index);
    Value *push_stk_alloc(const Proxy<size_t> &count);
    Value *push_stk_free(size_t count);
    Value *push_stk_push(Value *val);
#ifdef UNUSED
    Value *push_phi_2(Block *block1, Value *value1, Block *block2, Value *value2);
#endif
    Value *push_prp_def_data(Value *obj, Value *key, Value *val);
    Value *push_prp_def_accessor(Value *obj, uint64_t key,
                                 Value *fun, bool is_setter);
    Value *push_prp_it_new(Value *obj);
    Value *push_prp_it_next(Value *it, Value *val);
    Value *push_prp_get(Value *obj, uint64_t key, Value *res);
    Value *push_prp_get_slow(Value *obj, Value *key, Value *res);
    Value *push_prp_put(Value *obj, uint64_t key, Value *val);
    Value *push_prp_put_slow(Value *obj, Value *key, Value *val);
    Value *push_prp_del(Value *obj, uint64_t key, Value *res);
    Value *push_prp_del_slow(Value *obj, Value *key, Value *res);
    Value *push_trm_br(Value *cond, Block *true_block, Block *false_block);
    Value *push_trm_jmp(Block *block);
    Value *push_trm_ret(Value *val);
    Value *push_val_to_bool(Value *val);
    Value *push_val_to_double(Value *val, Value *res);
    Value *push_val_from_bool(Value *val, Value *res);  // FIXME: Rename args more appropriately val, bval
    Value *push_val_from_double(Value *val, Value *res);
    Value *push_val_from_str(Value *val, Value *res);
    Value *push_val_is_null(Value *val);
    Value *push_val_is_undefined(Value *val);
    Value *push_val_tst_coerc(Value *val);
    Value *push_meta_ctx_load(uint64_t key);
    Value *push_meta_prp_load(Value *obj, Value *key);
    Value *push_ctx_set_strict(bool strict);
    Value *push_ctx_enter_catch(uint64_t key);
    Value *push_ctx_enter_with(Value *val);
    Value *push_ctx_leave();
    Value *push_ctx_get(uint64_t key, Value *res, uint16_t cid);
    Value *push_ctx_put(uint64_t key, Value *val, uint16_t cid);
    Value *push_ctx_del(uint64_t key, Value *res);
    Value *push_ex_save_state(Value *res);
    Value *push_ex_load_state(Value *state);
    Value *push_ex_set(Value *val);
    Value *push_ex_clear();
    Value *push_init_args(Value *dst, uint32_t prmc);
    Value *push_decl_var(uint64_t key, bool is_strict);
    Value *push_decl_fun(uint64_t key, bool is_strict, Value *fun);
    Value *push_decl_prm(uint64_t key, bool is_strict,
                         size_t prm_index, Value *prm_array);
    Value *push_link_var(uint64_t key, bool is_strict, Value *var);
    Value *push_link_fun(uint64_t key, bool is_strict, Value *fun);
    Value *push_link_prm(uint64_t key, bool is_strict, Value *prm);

    Value *push_es_new_arr(size_t length, Value *vals, Value *res);
    Value *push_es_new_fun(Function *fun, uint32_t param_count, bool is_strict,
                           Value *res);
    Value *push_es_new_fun_expr(Function *fun, uint32_t param_count,
                                bool is_strict, Value *res);
    Value *push_es_new_obj(Value *res);
    Value *push_es_new_rex(const String &pattern, const String &flags,
                           Value *res);

    Value *push_es_bin_mul(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_div(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_mod(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_add(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_sub(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_ls(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_rss(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_rus(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_lt(Value *op1, Value *op, Value *res2);
    Value *push_es_bin_gt(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_lte(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_gte(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_in(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_instanceof(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_eq(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_neq(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_strict_eq(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_strict_neq(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_bit_and(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_bit_xor(Value *op1, Value *op2, Value *res);
    Value *push_es_bin_bit_or(Value *op1, Value *op2, Value *res);
    Value *push_es_unary_typeof(Value *op1, Value *res);
    Value *push_es_unary_neg(Value *op1, Value *res);
    Value *push_es_unary_bit_not(Value *op1, Value *res);
    Value *push_es_unary_log_not(Value *op1, Value *res);

    /**
     * @copydoc Node::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_block(this);
    }
};

/**
 * @brief Value.
 */
class Value
{
private:
    bool persistent_;   ///< true if the value will live through the entire function lifetime.

public:
    Value()
        : persistent_(false) {}
    virtual ~Value() {}

    /**
     * @return true if the value is persistent. Meaning that life time analysis
     *         the value cannot be performed. It will live through the lifetime
     *         of its function.
     */
    bool persistent() const { return persistent_; }

    /**
     * Makes the value persistent.
     * @see persistent()
     */
    void make_persistent() { persistent_ = true; }

    /**
     * @return Value type.
     */
    virtual const Type *type() const = 0;

    /**
     * @return true if the value is constant, false if it's not.
     * @see Constant.
     */
    virtual bool is_constant() const { return false; }
};

/**
 * @brief Instruction.
 */
class Instruction : public Value
{
public:
    /**
     * @brief Instruction visitor interface.
     */
    class Visitor
    {
    public:
        virtual ~Visitor() {}

        void visit(Instruction *instr) { instr->accept(this); }

        virtual void visit_instr_args_obj_init(ArgumentsObjectInitInstruction *instr) = 0;
        virtual void visit_instr_args_obj_link(ArgumentsObjectLinkInstruction *instr) = 0;
        virtual void visit_instr_arr(ArrayInstruction *instr) = 0;
        virtual void visit_instr_bin(BinaryInstruction *instr) = 0;
        virtual void visit_instr_bnd_extra_init(BindExtraInitInstruction *instr) = 0;
        virtual void visit_instr_bnd_extra_ptr(BindExtraPtrInstruction *instr) = 0;
        virtual void visit_instr_call(CallInstruction *instr) = 0;
        virtual void visit_instr_call_keyed(CallKeyedInstruction *instr) = 0;
        virtual void visit_instr_call_keyed_slow(CallKeyedSlowInstruction *instr) = 0;
        virtual void visit_instr_call_named(CallNamedInstruction *instr) = 0;
        virtual void visit_instr_val(ValueInstruction *instr) = 0;
        virtual void visit_instr_br(BranchInstruction *instr) = 0;
        virtual void visit_instr_jmp(JumpInstruction *instr) = 0;
        virtual void visit_instr_ret(ReturnInstruction *instr) = 0;
        virtual void visit_instr_mem_store(MemoryStoreInstruction *instr) = 0;
        virtual void visit_instr_mem_elm_ptr(MemoryElementPointerInstruction *instr) = 0;
        virtual void visit_instr_stk_alloc(StackAllocInstruction *instr) = 0;
        virtual void visit_instr_stk_free(StackFreeInstruction *instr) = 0;
        virtual void visit_instr_stk_push(StackPushInstruction *instr) = 0;
        virtual void visit_instr_ctx_set_strict(ContextSetStrictInstruction *instr) = 0;
        virtual void visit_instr_ctx_enter_catch(ContextEnterCatchInstruction *instr) = 0;
        virtual void visit_instr_ctx_enter_with(ContextEnterWithInstruction *instr) = 0;
        virtual void visit_instr_ctx_leave(ContextLeaveInstruction *instr) = 0;
        virtual void visit_instr_ctx_get(ContextGetInstruction *instr) = 0;
        virtual void visit_instr_ctx_put(ContextPutInstruction *instr) = 0;
        virtual void visit_instr_ctx_del(ContextDeleteInstruction *instr) = 0;
        virtual void visit_instr_ex_save_state(ExceptionSaveStateInstruction *instr) = 0;
        virtual void visit_instr_ex_load_state(ExceptionLoadStateInstruction *instr) = 0;
        virtual void visit_instr_ex_set(ExceptionSetInstruction *instr) = 0;
        virtual void visit_instr_ex_clear(ExceptionClearInstruction *instr) = 0;
        virtual void visit_instr_init_args(InitArgumentsInstruction *instr) = 0;
        virtual void visit_instr_decl(Declaration *instr) = 0;
        virtual void visit_instr_link(Link *instr) = 0;
        virtual void visit_instr_prp_def_data(PropertyDefineDataInstruction *instr) = 0;
        virtual void visit_instr_prp_def_accessor(PropertyDefineAccessorInstruction *instr) = 0;
        virtual void visit_instr_prp_it_new(PropertyIteratorNewInstruction *instr) = 0;
        virtual void visit_instr_prp_it_next(PropertyIteratorNextInstruction *instr) = 0;
        virtual void visit_instr_prp_get(PropertyGetInstruction *instr) = 0;
        virtual void visit_instr_prp_get_slow(PropertyGetSlowInstruction *instr) = 0;
        virtual void visit_instr_prp_put(PropertyPutInstruction *instr) = 0;
        virtual void visit_instr_prp_put_slow(PropertyPutSlowInstruction *instr) = 0;
        virtual void visit_instr_prp_del(PropertyDeleteInstruction *instr) = 0;
        virtual void visit_instr_prp_del_slow(PropertyDeleteSlowInstruction *instr) = 0;
        virtual void visit_instr_es_new_arr(EsNewArrayInstruction *instr) = 0;
        virtual void visit_instr_es_new_fun_decl(EsNewFunctionDeclarationInstruction *instr) = 0;
        virtual void visit_instr_es_new_fun_expr(EsNewFunctionExpressionInstruction *instr) = 0;
        virtual void visit_instr_es_new_obj(EsNewObjectInstruction *instr) = 0;
        virtual void visit_instr_es_new_rex(EsNewRegexInstruction *instr) = 0;
        virtual void visit_instr_es_bin(EsBinaryInstruction *instr) = 0;
        virtual void visit_instr_es_unary(EsUnaryInstruction *instr) = 0;
    };

protected:
    Instruction() {}
    virtual ~Instruction() {}

public:
    /**
     * @return true if the instruction is a terminating instruction.
     */
    virtual bool is_terminating() const { return false; }

    /**
     * Accept instruction in visitor pattern.
     * @param [in] visitor The visitor.
     */
    virtual void accept(Visitor *visitor) = 0;
};

/**
 * @brief Instruction for initializing the arguments object.
 */
class ArgumentsObjectInitInstruction : public Instruction
{
public:
    ArgumentsObjectInitInstruction() {}

    virtual const Type *type() const OVERRIDE {  return Type::value(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_args_obj_init(this);
    }
};

/**
 * @brief Instruction for linking an argument to the arguments object.
 */
class ArgumentsObjectLinkInstruction : public Instruction
{
private:
    Value *args_;
    uint32_t index_;
    Value *val_;

public:
    ArgumentsObjectLinkInstruction(Value *args, uint32_t index, Value *val)
        : args_(args)
        , index_(index)
        , val_(val) {}

    Value *arguments_object() const { return args_; }
    uint32_t argument_index() const { return index_; }
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_args_obj_link(this);
    }
};

/**
 * @brief Array instruction.
 */
class ArrayInstruction : public Instruction
{
public:
    enum Operation
    {
        GET,
        PUT
    };

private:
    Operation op_;
    size_t index_;
    Value *arr_;
    Value *val_;

public:
    ArrayInstruction(Operation op, size_t index, Value *arr);
    ArrayInstruction(Operation op, size_t index, Value *arr, Value *val);

    Operation operation() const { return op_; }
    size_t index() const { return index_; }
    Value *array() const { return arr_; }

    /**
     * @return Value operand.
     * @pre Array instruction is a put operation.
     */
    Value *value() const;

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_arr(this);
    }
};

/**
 * @brief Binary instruction.
 */
class BinaryInstruction : public Instruction
{
public:
    enum Operation
    {
        ADD,
        SUB,

        OR,

        EQ
    };

private:
    Operation op_;
    Value *lval_;
    Value *rval_;

public:
    BinaryInstruction(Operation op, Value *lval, Value *rval)
        : op_(op)
        , lval_(lval)
        , rval_(rval) {}

    Operation operation() const { return op_; }
    Value *left() const { return lval_; }
    Value *right() const { return rval_; }

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_bin(this);
    }
};

/**
 * @brief Extra bindings initialization instruction.
 */
class BindExtraInitInstruction : public Instruction
{
private:
    uint32_t num_extra_;

public:
    BindExtraInitInstruction(uint32_t num_extra)
        : num_extra_(num_extra) {}

    uint32_t num_extra() const { return num_extra_; }

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_bnd_extra_init(this);
    }
};

/**
 * @brief Extra bindings get instruction.
 */
class BindExtraPtrInstruction : public Instruction
{
private:
    uint32_t hops_;

public:
    BindExtraPtrInstruction(uint32_t hops)
        : hops_(hops) {}

    uint32_t hops() const { return hops_; }

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_bnd_extra_ptr(this);
    }
};

/**
 * @brief Call instruction.
 */
class CallInstruction : public Instruction
{
public:
    enum Operation
    {
        NORMAL, ///< Normal function call.
        NEW     ///< New call.
    };

private:
    Operation op_;
    Value *fun_;
    uint32_t argc_;
    Value *res_;

public:
    CallInstruction(Operation op, Value *fun, uint32_t argc, Value *res)
        : op_(op)
        , fun_(fun)
        , argc_(argc)
        , res_(res) {}

    Operation operation() const { return op_; }
    Value *function() const { return fun_; }
    uint32_t argc() const { return argc_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_call(this);
    }
};

/**
 * @brief Keyed call instruction.
 */
class CallKeyedInstruction : public Instruction
{
private:
    Value *obj_;
    uint64_t key_;
    uint32_t argc_;
    Value *res_;

public:
    CallKeyedInstruction(Value *obj, uint64_t key, uint32_t argc,
                         Value *res)
        : obj_(obj)
        , key_(key)
        , argc_(argc)
        , res_(res) {}

    Value *object() const { return obj_; }
    uint64_t key() const { return key_; }
    uint32_t argc() const { return argc_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_call_keyed(this);
    }
};

/**
 * @brief Keyed call instruction.
 */
class CallKeyedSlowInstruction : public Instruction
{
private:
    Value *obj_;
    Value *key_;
    uint32_t argc_;
    Value *res_;

public:
    CallKeyedSlowInstruction(Value *obj, Value *key, uint32_t argc,
                             Value *res)
        : obj_(obj)
        , key_(key)
        , argc_(argc)
        , res_(res) {}

    Value *object() const { return obj_; }
    Value *key() const { return key_; }
    uint32_t argc() const { return argc_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_call_keyed_slow(this);
    }
};

/**
 * @brief Named call instruction.
 */
class CallNamedInstruction : public Instruction
{
private:
    uint64_t key_;
    uint32_t argc_;
    Value *res_;

public:
    CallNamedInstruction(uint64_t key, uint32_t argc, Value *res)
        : key_(key)
        , argc_(argc)
        , res_(res) {}

    uint64_t key() const { return key_; }
    uint32_t argc() const { return argc_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_call_named(this);
    }
};

/**
 * @brief Value instruction.
 *
 * Example:
 * val.is_null %value
 */
class ValueInstruction : public Instruction
{
public:
    enum Operation
    {
        TO_BOOLEAN,
        TO_DOUBLE,

        FROM_BOOLEAN,
        FROM_DOUBLE,
        FROM_STRING,

        IS_NULL,
        IS_UNDEFINED,

        TEST_COERCIBILITY
    };

private:
    Operation op_;
    Value *val_;
    Value *res_;

public:
    ValueInstruction(Operation op, Value *val, Value *res);
    ValueInstruction(Operation op, Value *val);

    Operation operation() const { return op_; }
    Value *value() const { return val_; }

    /**
     * @return Result operation.
     * @pre Operation requires a value operand.
     */
    Value *result() const;

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_val(this);
    }
};

/**
 * @brief Terminate class of instructions.
 */
class TerminateInstruction : public Instruction
{
private:
    Block *block_;  ///< Block hosting the instruction.

protected:
    TerminateInstruction(Block *block)
        : block_(block) {}
    virtual ~TerminateInstruction() {}

public:
    /**
     * @return Block hosting the instruction.
     */
    Block *block() const { return block_; }

    /**
     * @copydoc Instruction::is_terminating
     */
    virtual bool is_terminating() const OVERRIDE { return true; }
};

/**
 * @Brief Branch instruction.
 *
 * Example:
 * br %value true_label false_label
 */
class BranchInstruction : public TerminateInstruction
{
private:
    Value *cond_;
    Block *true_block_;
    Block *false_block_;

public:
    BranchInstruction(Block *host, Value *cond,
                      Block *true_block, Block *false_block);

    Value *condition() const { return cond_; }
    Block *true_block() const { return true_block_; }
    Block *false_block() const { return false_block_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_br(this);
    }
};

/**
 * @brief Jump instruction.
 */
class JumpInstruction : public TerminateInstruction
{
private:
    Block *block_;

public:
    JumpInstruction(Block *host, Block *block)
        : TerminateInstruction(host)
        , block_(block) {}

    Block *block() const { return block_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_jmp(this);
    }
};

/**
 * @brief Return instruction.
 */
class ReturnInstruction : public TerminateInstruction
{
private:
    Value *val_;

public:
    ReturnInstruction(Block *host, Value *val)
        : TerminateInstruction(host)
        , val_(val) {}

    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ret(this);
    }
};

/**
 * @brief Memory store instruction.
 */
class MemoryStoreInstruction : public Instruction
{
private:
    Value *dst_;
    Value *src_;

public:
    MemoryStoreInstruction(Value *dst, Value *src)
        : dst_(dst)
        , src_(src) {}

    Value *destination() const { return dst_; }
    Value *source() const { return src_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_mem_store(this);
    }
};

/**
 * @brief Instruction for obtaining an element pointer.
 */
class MemoryElementPointerInstruction : public Instruction
{
private:
    Value *val_;
    size_t index_;

public:
    MemoryElementPointerInstruction(Value *val, size_t index);

    Value *value() const { return val_; }
    size_t index() const { return index_; }

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_mem_elm_ptr(this);
    }
};

/**
 * @brief Instruction for allocating memory on the stack.
 */
class StackAllocInstruction : public Instruction
{
private:
    Proxy<size_t> count_;

public:
    StackAllocInstruction(const Proxy<size_t> &count)
        : count_(count) {}

    /**
     * @return Number of values to allocate.
     */
    size_t count() const { return count_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_stk_alloc(this);
    }
};

/**
 * @brief Instruction for freeing memory from the stack.
 */
class StackFreeInstruction : public Instruction
{
private:
    size_t count_;

public:
    StackFreeInstruction(size_t count)
        : count_(count) {}

    /**
     * @return Number of values to free.
     */
    size_t count() const { return count_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_stk_free(this);
    }
};

/**
 * @brief Instruction for pushing an element to the stack.
 */
class StackPushInstruction : public Instruction
{
private:
    Value *val_;

public:
    StackPushInstruction(Value *val);

    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_stk_push(this);
    }
};

/**
 * @brief Meta instructions are never serialized.
 *
 * The compiler uses meta instructions to reason about the code and produce
 * non-meta instructions from it.
 */
class MetaInstruction : public Instruction
{
public:
    virtual ~MetaInstruction() {}

    /**
     * @copydoc Instruction::accept
     */
    virtual void accept(Visitor *visitor) OVERRIDE
    {
    }
};

/**
 * @brief Loads a property from the current execution context.
 */
class MetaContextLoadInstruction : public MetaInstruction
{
private:
    uint64_t key_;

public:
    MetaContextLoadInstruction(uint64_t key)
        : key_(key) {}

    uint64_t key() const { return key_; }

    virtual const Type *type() const OVERRIDE { return Type::reference(); }
};

/**
 * @brief Loads an object property.
 */
class MetaPropertyLoadInstruction : public MetaInstruction
{
private:
    Value *obj_;
    Value *key_;

public:
    MetaPropertyLoadInstruction(Value *obj, Value *key)
        : obj_(obj)
        , key_(key) {}

    Value *object() const { return obj_; }
    Value *key() const { return key_; }

    virtual const Type *type() const OVERRIDE { return Type::reference(); }
};

/**
 * @brief Instruction for setting strict mode.
 */
class ContextSetStrictInstruction : public Instruction
{
private:
    bool strict_;

public:
    ContextSetStrictInstruction(bool strict)
        : strict_(strict) {}

    bool strict() const { return strict_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_set_strict(this);
    }
};

/**
 * @brief Instruction for entering a new catch execution context.
 */
class ContextEnterCatchInstruction : public Instruction
{
private:
    uint64_t key_;

public:
    ContextEnterCatchInstruction(uint64_t key)
        : key_(key) {}

    uint64_t key() const { return key_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_enter_catch(this);
    }
};

/**
 * @brief Instruction for entering a new with execution context.
 */
class ContextEnterWithInstruction : public Instruction
{
private:
    Value *val_;

public:
    ContextEnterWithInstruction(Value *val)
        : val_(val) {}

    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_enter_with(this);
    }
};

/**
 * @brief Instruction for leaving the current execution context.
 */
class ContextLeaveInstruction : public Instruction
{
public:
    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_leave(this);
    }
};

/**
 * @brief Instruction for getting a value from the current context.
 */
class ContextGetInstruction : public Instruction
{
private:
    uint64_t key_;
    Value *res_;
    uint16_t cid_;

public:
    ContextGetInstruction(uint64_t key, Value *res, uint16_t cid)
        : key_(key)
        , res_(res)
        , cid_(cid) {}

    uint64_t key() const { return key_; }
    Value *result() const { return res_; }
    uint16_t cache_id() const { return cid_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_get(this);
    }
};

/**
 * @brief Instruction for setting a value in the current context.
 */
class ContextPutInstruction : public Instruction
{
private:
    uint64_t key_;
    Value *val_;
    uint16_t cid_;

public:
    ContextPutInstruction(uint64_t key, Value *val, uint16_t cid)
        : key_(key)
        , val_(val)
        , cid_(cid) {}

    uint64_t key() const { return key_; }
    Value *value() const { return val_; }
    uint16_t cache_id() const { return cid_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_put(this);
    }
};

/**
 * @brief Instruction for deleting a value from the current context.
 *
 * Example:
 * delete %name %result
 * delete %name %result
 */
class ContextDeleteInstruction : public Instruction
{
private:
    uint64_t key_;
    Value *res_;

public:
    ContextDeleteInstruction(uint64_t key, Value *res);

    uint64_t key() const { return key_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ctx_del(this);
    }
};

/**
 * @brief Instruction for savig the current exception state.
 */
class ExceptionSaveStateInstruction : public Instruction
{
private:
    Value *res_;

public:
    ExceptionSaveStateInstruction(Value *res)
        : res_(res) {}

    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ex_save_state(this);
    }
};

/**
 * @brief Instruction for saving the current exception state.
 */
class ExceptionLoadStateInstruction : public Instruction
{
private:
    Value *state_;

public:
    ExceptionLoadStateInstruction(Value *state);

    Value *state() const { return state_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ex_load_state(this);
    }
};

/**
 * @brief Instruction for setting a pending.
 */
class ExceptionSetInstruction : public Instruction
{
private:
    Value *val_;

public:
    ExceptionSetInstruction(Value *val);

    /**
    * @return Value to throw with exception.
    */
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ex_set(this);
    }
};

/**
 * @brief Instruction for clearing any pending exception.
 */
class ExceptionClearInstruction : public Instruction
{
public:
    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_ex_clear(this);
    }
};

/**
 * @brief Instruction for copying the function arguments into a buffer.
 */
class InitArgumentsInstruction : public Instruction
{
private:
    Value *dst_;
    uint32_t prmc_;

public:
    InitArgumentsInstruction(Value *dst, uint32_t prmc)
        : dst_(dst)
        , prmc_(prmc) {}

    Value *destination() const { return dst_; }
    uint32_t parameter_count() const { return prmc_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_init_args(this);
    }
};

/**
 * @brief Variable or function declaration.
 *
 * Example:
 * decl.fun "name" true %function
 * decl.var "name" true
 */
class Declaration : public Instruction
{
public:
    enum Kind
    {
        FUNCTION,
        VARIABLE,
        PARAMETER
    };

private:
    Kind kind_;
    uint64_t key_;
    bool is_strict_;
    Value *val_;        ///< Only valid for FUNCTION type.
    size_t prm_index_;  ///< Only valid for PARAMETER type.
    Value *prm_array_;  ///< Only valid for PARAMETER type.

public:
    Declaration(uint64_t key, bool is_strict)
        : kind_(VARIABLE)
        , key_(key)
        , is_strict_(is_strict)
        , val_(NULL)
        , prm_index_(-1)
        , prm_array_(NULL) {}
    Declaration(uint64_t key, bool is_strict, Value *val)
        : kind_(FUNCTION)
        , key_(key)
        , is_strict_(is_strict)
        , val_(val)
        , prm_index_(-1)
        , prm_array_(NULL) {}
    Declaration(uint64_t key, bool is_strict, size_t prm_index,
                Value *prm_array)
        : kind_(PARAMETER)
        , key_(key)
        , is_strict_(is_strict)
        , val_(NULL)
        , prm_index_(prm_index)
        , prm_array_(prm_array) {}

    Kind kind() const { return kind_; }
    uint64_t key() const { return key_; }
    bool is_strict() const { return is_strict_; }

    /**
     * @return Declared value.
     * @pre Type is FUNCTION.
     */
    Value *value() const;

    /**
     * @return Parameter index.
     * @pre type is PARAMETER.
     */
    size_t parameter_index() const;

    /**
     * @return Parameter array.
     * @pre type is PARAMETER.
     */
    Value *parameter_array() const;

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_decl(this);
    }
};

/**
 * @brief Variable or function declaration that is linked to external storage.
 *
 * Example:
 * link.fun "name" true %function
 * link.var "name" true %variable
 */
class Link : public Instruction
{
public:
    enum Kind
    {
        FUNCTION,
        VARIABLE,
        PARAMETER
    };

private:
    Kind kind_;
    uint64_t key_;
    bool is_strict_;
    Value *val_;

public:
    Link(Kind kind, uint64_t key, bool is_strict, Value *val)
        : kind_(kind)
        , key_(key)
        , is_strict_(is_strict)
        , val_(val) {}

    Kind kind() const { return kind_; }
    uint64_t key() const { return key_; }
    bool is_strict() const { return is_strict_; }
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_link(this);
    }
};

/**
 * @brief Defines a new property on an object.
 */
class PropertyDefineDataInstruction : public Instruction
{
private:
    Value *obj_;
    Value *key_;
    Value *val_;

public:
    PropertyDefineDataInstruction(Value *obj, Value *key, Value *val)
        : obj_(obj)
        , key_(key)
        , val_(val) {}

    Value *object() const { return obj_; }
    Value *key() const { return key_; }
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_def_data(this);
    }
};

/**
 * @brief Defines a new accessor property on an object.
 */
class PropertyDefineAccessorInstruction : public Instruction
{
private:
    Value *obj_;
    uint64_t key_;
    Value *fun_;
    bool is_setter_;

public:
    PropertyDefineAccessorInstruction(Value *obj, uint64_t key,
                                      Value *fun, bool is_setter)
        : obj_(obj)
        , key_(key)
        , fun_(fun)
        , is_setter_(is_setter) {}

    Value *object() const { return obj_; }
    uint64_t key() const { return key_; }
    Value *function() const { return fun_; }
    bool is_setter() const { return is_setter_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_def_accessor(this);
    }
};

/**
 * @brief Creates a new object property iterator.
 */
class PropertyIteratorNewInstruction : public Instruction
{
private:
    Value *obj_;

public:
    PropertyIteratorNewInstruction(Value *obj)
        : obj_(obj) {}

    Value *object() const { return obj_; }

    virtual const Type *type() const OVERRIDE;
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_it_new(this);
    }
};

/**
 * @brief Fetches the next property from an object property iterator.
 */
class PropertyIteratorNextInstruction : public Instruction
{
private:
    Value *it_;
    Value *val_;

public:
    PropertyIteratorNextInstruction(Value *it, Value *val)
        : it_(it)
        , val_(val) {}

    Value *iterator() const { return it_; }
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_it_next(this);
    }
};

/**
 * @brief Instruction for getting an object property value.
 */
class PropertyGetInstruction : public Instruction
{
private:
    Value *obj_;
    uint64_t key_;
    Value *res_;

public:
    PropertyGetInstruction(Value *obj, uint64_t key, Value *res)
        : obj_(obj)
        , key_(key)
        , res_(res) {}

    Value *object() const { return obj_; }
    uint64_t key() const { return key_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_get(this);
    }
};

/**
 * @brief Instruction for getting an object property value.
 */
class PropertyGetSlowInstruction : public Instruction
{
private:
    Value *obj_;
    Value *key_;
    Value *res_;

public:
    PropertyGetSlowInstruction(Value *obj, Value *key, Value *res)
        : obj_(obj)
        , key_(key)
        , res_(res) {}

    Value *object() const { return obj_; }
    Value *key() const { return key_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_get_slow(this);
    }
};

/**
 * @brief Instruction for setting an object property value.
 */
class PropertyPutInstruction : public Instruction
{
private:
    Value *obj_;
    uint64_t key_;
    Value *val_;

public:
    PropertyPutInstruction(Value *obj, uint64_t key, Value *val)
        : obj_(obj)
        , key_(key)
        , val_(val) {}

    Value *object() const { return obj_; }
    uint64_t key() const { return key_; }
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_put(this);
    }
};

/**
 * @brief Instruction for setting an object property value.
 */
class PropertyPutSlowInstruction : public Instruction
{
private:
    Value *obj_;
    Value *key_;
    Value *val_;

public:
    PropertyPutSlowInstruction(Value *obj, Value *key, Value *val)
        : obj_(obj)
        , key_(key)
        , val_(val) {}

    Value *object() const { return obj_; }
    Value *key() const { return key_; }
    Value *value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_put_slow(this);
    }
};

/**
 * @brief Instruction for deleting an object property.
 *
 * Example:
 * delete %object %name %result
 * delete %name %result
 */
class PropertyDeleteInstruction : public Instruction
{
private:
    Value *obj_;
    uint64_t key_;
    Value *res_;

public:
    PropertyDeleteInstruction(Value *obj, uint64_t key, Value *res);

    Value *object() const { return obj_; }
    uint64_t key() const { return key_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_del(this);
    }
};

/**
 * @brief Instruction for deleting an object property.
 *
 * Example:
 * delete %object %name %result
 * delete %name %result
 */
class PropertyDeleteSlowInstruction : public Instruction
{
private:
    Value *obj_;
    Value *key_;
    Value *res_;

public:
    PropertyDeleteSlowInstruction(Value *obj, Value *key, Value *res);

    Value *object() const { return obj_; }
    Value *key() const { return key_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_prp_del_slow(this);
    }
};

/**
 * @brief Instruction for creating a new array.
 */
class EsNewArrayInstruction : public Instruction
{
private:
    size_t length_;
    Value *vals_;
    Value *res_;

public:
    EsNewArrayInstruction(size_t length, Value *vals, Value *res)
        : length_(length)
        , vals_(vals)
        , res_(res) {}

    size_t length() const { return length_; }
    Value *values() const { return vals_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_new_arr(this);
    }
};

/**
 * @brief Instruction for creating a new function.
 */
class EsNewFunctionDeclarationInstruction : public Instruction
{
private:
    Function *fun_;
    uint32_t param_count_;
    bool is_strict_;
    Value *res_;

public:
    EsNewFunctionDeclarationInstruction(Function *fun, uint32_t param_count,
                                        bool is_strict, Value *res)
        : fun_(fun)
        , param_count_(param_count)
        , is_strict_(is_strict)
        , res_(res) {}

    Function *function() const { return fun_; }
    uint32_t parameter_count() const { return param_count_; }
    bool is_strict() const { return is_strict_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_new_fun_decl(this);
    }
};

/**
 * @brief Instruction for creating a new function.
 */
class EsNewFunctionExpressionInstruction : public Instruction
{
private:
    Function *fun_;
    uint32_t param_count_;
    bool is_strict_;
    Value *res_;

public:
    EsNewFunctionExpressionInstruction(Function *fun, uint32_t param_count,
                                       bool is_strict, Value *res)
        : fun_(fun)
        , param_count_(param_count)
        , is_strict_(is_strict)
        , res_(res) {}

    Function *function() const { return fun_; }
    uint32_t parameter_count() const { return param_count_; }
    bool is_strict() const { return is_strict_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_new_fun_expr(this);
    }
};

/**
 * @brief Instruction for creating a new object.
 */
class EsNewObjectInstruction : public Instruction
{
private:
    Value *res_;

public:
    EsNewObjectInstruction(Value *res)
        : res_(res) {}

    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_new_obj(this);
    }
};

/**
 * @brief Loads a regular expression object.
 */
class EsNewRegexInstruction : public Instruction
{
private:
    String pattern_;
    String flags_;
    Value *res_;

public:
    EsNewRegexInstruction(const String &pattern, const String &flags,
                          Value *res)
        : pattern_(pattern)
        , flags_(flags)
        , res_(res) {}

    const String &pattern() const { return pattern_; }
    const String &flags() const { return flags_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::_void(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_new_rex(this);
    }
};

/**
 * @brief Binary ECMAScript instruction.
 */
class EsBinaryInstruction : public Instruction
{
public:
    enum Operation
    {
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
        BIT_OR
    };

private:
    Operation op_;
    Value *lval_;
    Value *rval_;
    Value *res_;

public:
    EsBinaryInstruction(Operation op, Value *lval, Value *rval, Value *res)
        : op_(op)
        , lval_(lval)
        , rval_(rval)
        , res_(res) {}

    Operation operation() const { return op_; }
    Value *left() const { return lval_; }
    Value *right() const { return rval_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_bin(this);
    }
};

/**
 * @brief Unary instruction.
 */
class EsUnaryInstruction : public Instruction
{
public:
    enum Operation
    {
        TYPEOF,
        NEG,
        BIT_NOT,
        LOG_NOT
    };

private:
    Operation op_;
    Value *val_;
    Value *res_;

public:
    EsUnaryInstruction(Operation op, Value *val, Value *res)
        : op_(op)
        , val_(val)
        , res_(res) {}

    Operation operation() const { return op_; }
    Value *value() const { return val_; }
    Value *result() const { return res_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_instr_es_unary(this);
    }
};

/**
 * @brief Constant value.
 */
class Constant : public Value
{
public:
    /**
     * @brief Constant visitor interface.
     */
    class Visitor
    {
    public:
        virtual ~Visitor() {}

        void visit(Constant *constant) { constant->accept(this); }

        virtual void visit_const_arr_elm(ArrayElementConstant *instr) = 0;
        virtual void visit_const_fp(FramePointer *instr) = 0;
        virtual void visit_const_vp(ValuePointer *instr) = 0;
        virtual void visit_const_null(NullConstant *instr) = 0;
        virtual void visit_const_bool(BooleanConstant *instr) = 0;
        virtual void visit_const_double(DoubleConstant *instr) = 0;
        virtual void visit_const_strdouble(StringifiedDoubleConstant *instr) = 0;
        virtual void visit_const_str(StringConstant *instr) = 0;
        virtual void visit_const_val(ValueConstant *instr) = 0;
    };

public:
    virtual ~Constant() {}

    /**
     * @copydoc Value::is_constant
     */
    virtual bool is_constant() const OVERRIDE { return true; }

    /**
     * Accept node in visitor pattern.
     * @param [in] visitor The visitor.
     */
    virtual void accept(Visitor *visitor) = 0;
};

/**
 * @brief Array indexed element.
 */
class ArrayElementConstant : public Constant
{
private:
    Value *array_;
    int index_;     ///< Array index, may be negative.

public:
    ArrayElementConstant(Value *array, int index)
        : array_(array)
        , index_(index)
    {
        assert(array->type()->is_array() ||
               array->type()->is_pointer());
    }

    Value *array() const { return array_; }
    int index() const { return index_; }

    virtual const Type *type() const OVERRIDE
    {
        return static_cast<const ArrayType *>(array_->type())->type();
    }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_arr_elm(this);
    }
};

class FramePointer : public Constant
{
public:
    virtual const Type *type() const OVERRIDE
    {
        return new (GC)PointerType(Type::value());
    }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_fp(this);
    }
};

class ValuePointer : public Constant
{
public:
    virtual const Type *type() const OVERRIDE
    {
        return new (GC)PointerType(Type::value());
    }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_vp(this);
    }
};

/**
 * @brief Null value constant.
 */
class NullConstant : public Constant
{
private:
    const Type *type_;

public:
    /**
     * Creates a new NULL constant for a specific opaque type.
     * @param [in] opaque Name of opaque type.
     */
    NullConstant(const Type *type)
        : type_(type) {}

    virtual const Type *type() const OVERRIDE { return type_; }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_null(this);
    }
};

/**
 * @brief Boolean value constant.
 */
class BooleanConstant : public Constant
{
private:
    bool val_;

public:
    BooleanConstant(bool val)
        : val_(val) {}

    bool value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::boolean(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_bool(this);
    }
};

/**
 * @brief Double value constant.
 */
class DoubleConstant : public Constant
{
private:
    double val_;

public:
    DoubleConstant(double val)
        : val_(val) {}

    double value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_double(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_double(this);
    }
};

/**
 * @brief Double value constant in string format.
 */
class StringifiedDoubleConstant : public Constant
{
private:
    String val_;

public:
    StringifiedDoubleConstant(const String &val)
        : val_(val) {}

    /**
     * @return Double value as string.
     */
    const String &value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::_double(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_strdouble(this);
    }
};

/**
 * @brief String value constant.
 */
class StringConstant : public Constant
{
private:
    String val_;

public:
    StringConstant(const String &val)
        : val_(val) {}

    const String &value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::string(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_str(this);
    }
};

/**
 * @brief Value constant.
 */
class ValueConstant : public Constant
{
public:
    enum Value
    {
        VALUE_NOTHING,
        VALUE_UNDEFINED,
        VALUE_NULL,
        VALUE_TRUE,
        VALUE_FALSE
    };

private:
    Value val_;

public:
    ValueConstant(Value val)
        : val_(val) {}

    Value value() const { return val_; }

    virtual const Type *type() const OVERRIDE { return Type::value(); }
    virtual void accept(Visitor *visitor) OVERRIDE
    {
        visitor->visit_const_val(this);
    }
};

/**
 * @brief Temporary value.
 */
class Temporary : public Value
{
private:
    const Type *type_;

public:
    Temporary(const Type *type)
        : type_(type) {}

    virtual bool is_constant() const OVERRIDE { return false; }
    virtual const Type *type() const OVERRIDE { return type_; }
};

}

template <>
struct IntrusiveLinkedListTraits<ir::Block>
{
    static ir::Block *create_sentinel()
    {
        return new (GC)ir::Block();
    }

    static void destroy_sentinel(ir::Block *sentinel)
    {
    }
};
