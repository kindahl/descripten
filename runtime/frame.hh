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
#include <vector>
#include "value.hh"

class EsFunction;

/**
 * @brief Represents a call frame on the internal ECMAScript call stack.
 *
 * This is not to be mixed up with the call stack used by native C++ programs.
 * ECMAScript programs executing through this engine will have two paralell
 * call stacks: One native call stack for generated native code, this is what's
 * typically thought of as the call stack for standard C-programs. In adition
 * to this there will be a parallel stack to support the ECMAScript language.
 * This stack will contain all managed pointers that needs to be monitored by
 * the garbage collector.
 * 
 * Frame Layout:
 * fp -> [0] argument 0.
 *       [1] argument 1.
 *       ...
 *       [n] argument n.
 *       [a] pointer to the callee.
 *       [b] pointer to this object.
 *       [c] pointer to result value.
 * vp -> [0] local 0.
 *       [1] local 1.
 *       ...
 *       [m] local m.
 *
 * fp: Pointer to first value in stack frame.
 * vp: Pointer to first local in stack frame.
 * 
 * In the above description a local refers to any kind of value allocated by
 * the called function for whatever use it deems necessary. It may be local
 * variables or temporaries. The called function may grow or shrink the stack
 * frame as it deems necessary.
 *
 * Arguments are stored in the beginning of the stack frame. The number of
 * arguments included in the frame equals the number of arguments passed in
 * the function call, unless some argument corresponding to a formal parameter
 * was ommited. In that case the arguments region will be padded to cover all
 * formal parameters.
 *
 * In short this means that the number of arguments accessible on the stack is
 * greater than or equal to the number of format parameters of the called
 * function. Padded arguments will be initialized to undefined as per
 * specification.
 */
class EsCallFrame
{
public:
    enum
    {
        CALLEE = -3,
        /**
         * The this value. This may be the untainted this argument passed to
         * the call frame constructor or the this binding, derived from the
         * this argument.
         */
        THIS = -2,
        RESULT = -1,

        //NUM_PRE_VP_CONSTANTS = 3
    };

    /**
     * Convenience class to enable range-based for loop iteration of arguments.
     */
    class Arguments
    {
    private:
        EsValue *begin_ptr_;
        EsValue *end_ptr_;

    public:
        Arguments(EsValue *fp, uint32_t argc)
            : begin_ptr_(argc > 0 ? fp : NULL)
            , end_ptr_(argc > 0 ? fp + argc : NULL) {}

        inline EsValue *begin() { return begin_ptr_; }
        inline EsValue *end() { return end_ptr_; }
    };

private:
    size_t pos_;    ///< Frame position on call stack.
    uint32_t argc_; ///< Number of arguments passed to function.
    EsValue *fp_;   ///< Pointer to beginning of frame.
    EsValue *vp_;   ///< Pointer to beginning of locals.

    EsCallFrame(size_t pos, uint32_t argc, EsValue *fp, EsValue *vp);

    /**
     * @return The number of allocated arguments.
     * @note The number of allocated arguments may exceed the number of passed
     *       arguments.
     */
#ifdef DEBUG
    inline uint32_t num_alloc_args() const
    {
        return static_cast<uint32_t>(vp_ - fp_) - 3;
    }
#endif

public:
    EsCallFrame(EsCallFrame &&other);
    ~EsCallFrame();

    // FIXME: Argument ordering.
    static EsCallFrame push_eval_direct(EsFunction *callee,
                                        const EsValue &this_binding);
    static EsCallFrame push_eval_indirect(EsFunction *callee);
    static EsCallFrame push_function_excl_args(uint32_t argc,
                                               EsFunction *callee,
                                               const EsValue &this_arg);
    static EsCallFrame push_function(uint32_t argc, EsFunction *callee,
                                     const EsValue &this_arg);
    static EsCallFrame push_global();
    static EsCallFrame wrap(uint32_t argc, EsValue *fp, EsValue *vp);

    inline Arguments arguments() const { return Arguments(fp_, argc_); }

    inline uint32_t argc() const { return argc_; }
    inline EsValue *fp() { return fp_; }
    inline EsValue *vp() { return vp_; }

    inline const EsValue &arg(uint32_t index) { assert(index < num_alloc_args()); return fp_[index]; }
    inline const EsValue &callee() const { return vp_[CALLEE]; }
    inline const EsValue &this_value() const { return vp_[THIS]; }
    inline const EsValue &result() const { return vp_[RESULT]; }

    inline void set_this_value(const EsValue &val) { vp_[THIS] = val; }
    inline void set_result(const EsValue &val) { vp_[RESULT] = val; }
};

class EsCallStack
{
private:
    std::vector<EsValue, gc_allocator<EsValue>> stack_;

public:
    EsCallStack();

    void init();

    /**
     * @return Top value in stack or NULL if stack is empty.
     */
    /*inline EsValue *top()
    {
        return stack_.empty() ? NULL : &stack_.back();
    }*/

    /**
     * @return Pointer to the next value that will be pushed to the stack.
     */
    inline EsValue *next()
    {
        return stack_.empty() ? stack_.data() : &stack_.back() + 1;
    }

    inline size_t size() const
    {
        return stack_.size();
    }

    inline void resize(size_t size)
    {
        stack_.resize(size, EsValue::undefined);
    }

    inline void alloc(size_t count)
    {
        assert(stack_.size() + count < stack_.capacity());
        stack_.resize(stack_.size() + count, EsValue::undefined);
    }

    inline void free(size_t count)
    {
        assert(stack_.size() >= count);
        stack_.resize(stack_.size() - count, EsValue::undefined);
    }

    inline void push(const EsValue &val)
    {
        assert(stack_.size() < stack_.capacity());
        stack_.push_back(val);
    }

    inline EsValue pop()
    {
        assert(stack_.size() > 0);
        EsValue res = stack_.back();
        stack_.pop_back();
        return res;
    }
};

class EsCallStackGuard
{
private:
    size_t count_;

public:
    EsCallStackGuard(size_t count)
        : count_(count) {}
    ~EsCallStackGuard();

    inline void release()
    {
        count_ = 0;
    }
};

extern EsCallStack g_call_stack;
