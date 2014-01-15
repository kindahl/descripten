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

#include <limits>
#include <stdint.h>
#include "frame.hh"
#include "global.hh"
#include "object.hh"

EsCallStack g_call_stack;

EsCallFrame::EsCallFrame(size_t pos, int argc, EsValue *fp, EsValue *vp)
    : pos_(pos)
    , argc_(argc)
    , fp_(fp)
    , vp_(vp)
{
}

EsCallFrame::EsCallFrame(EsCallFrame &&other)
{
    pos_ = other.pos_;
    argc_ = other.argc_;
    fp_ = other.fp_;
    vp_ = other.vp_;

    other.pos_ = std::numeric_limits<size_t>::max();
}

EsCallFrame EsCallFrame::push_eval_direct(EsFunction *callee,
                                          const EsValue &this_binding)
{
    EsCallFrame frame(g_call_stack.size(),
                      0,
                      g_call_stack.next(),
                      g_call_stack.next() + 3);

    // Allocate space for: callee, this and result.
    g_call_stack.alloc(3);

    frame.vp_[CALLEE] = EsValue::from_obj(callee);
    frame.vp_[THIS] = this_binding;

    //return std::move(frame);
    return frame;
}

EsCallFrame EsCallFrame::push_eval_indirect(EsFunction *callee)
{
    EsCallFrame frame(g_call_stack.size(),
                      0,
                      g_call_stack.next(),
                      g_call_stack.next() + 3);

    // Allocate space for: callee, this and result.
    g_call_stack.alloc(3);

    frame.vp_[CALLEE] = EsValue::from_obj(callee);
    frame.vp_[THIS] = EsValue::from_obj(es_global_obj());

    //return std::move(frame);
    return frame;
}

EsCallFrame EsCallFrame::push_function_excl_args(int argc, EsFunction *callee,
                                                 const EsValue &this_arg)
{
    // Allocate any default (unspecified) arguments.
    assert(callee);
    int argc_def = 0;
    if (static_cast<int>(callee->length()) > argc)
        argc_def = callee->length() - argc;

    EsCallFrame frame(g_call_stack.size() - argc,
                      argc,
                      g_call_stack.next() - argc,
                      g_call_stack.next() + argc_def + 3);


    // Allocate space for: default arguments, callee, this and result.
    g_call_stack.alloc(argc_def + 3);

    frame.vp_[CALLEE] = EsValue::from_obj(callee);

    if (callee->needs_this_binding())
    {
        // 10.4.3
        if (callee->is_strict())
            frame.vp_[THIS] = this_arg;
        else if (this_arg.is_null() || this_arg.is_undefined())
            frame.vp_[THIS] = EsValue::from_obj(es_global_obj());
        else if (!this_arg.is_object())
            // Will never throw given the if expression above this one.
            frame.vp_[THIS] = EsValue::from_obj(this_arg.to_objectT());
        else
            frame.vp_[THIS] = this_arg;
    }
    else
    {
        frame.vp_[THIS] = this_arg;
    }

    //return std::move(frame);
    return frame;
}

EsCallFrame EsCallFrame::push_function(int argc, EsFunction *callee,
                                       const EsValue &this_arg)
{
    // Allocate any default (unspecified) arguments.
    assert(callee);
    int argc_def = 0;
    if (static_cast<int>(callee->length()) > argc)
        argc_def = callee->length() - argc;

    EsCallFrame frame(g_call_stack.size(),
                      argc,
                      g_call_stack.next(),
                      g_call_stack.next() + argc + argc_def + 3);

    // Allocate space for: arguments, callee, this and result.
    g_call_stack.alloc(argc + argc_def + 3);

    frame.vp_[CALLEE] = EsValue::from_obj(callee);

    if (callee->needs_this_binding())
    {
        // 10.4.3
        if (callee->is_strict())
            frame.vp_[THIS] = this_arg;
        else if (this_arg.is_null() || this_arg.is_undefined())
            frame.vp_[THIS] = EsValue::from_obj(es_global_obj());
        else if (!this_arg.is_object())
            // Will never throw given the if expression above this one.
            frame.vp_[THIS] = EsValue::from_obj(this_arg.to_objectT());
        else
            frame.vp_[THIS] = this_arg;
    }
    else
    {
        frame.vp_[THIS] = this_arg;
    }

    //return std::move(frame);
    return frame;
}

EsCallFrame EsCallFrame::push_global()
{
    EsCallFrame frame(g_call_stack.size(),
                      0,
                      g_call_stack.next(),
                      g_call_stack.next() + 3);

    // Allocate space for: callee, this and result.
    g_call_stack.alloc(3);

    frame.vp_[CALLEE] = EsValue::null;  // FIXME: Should we create an object for the program?
    frame.vp_[THIS] = EsValue::from_obj(es_global_obj());

    //return std::move(frame);
    return frame;
}

EsCallFrame EsCallFrame::wrap(int argc, EsValue *fp, EsValue *vp)
{
    return EsCallFrame(std::numeric_limits<size_t>::max(), argc, fp, vp);
}

EsCallFrame::~EsCallFrame()
{
    // If the position is set to max we have wrapped a stack frame and
    // shouldn't pop it.
    if (pos_ != std::numeric_limits<size_t>::max())
        g_call_stack.resize(pos_);
}

EsCallStack::EsCallStack()
{
}

void EsCallStack::init()
{
    stack_.reserve(8192);
}

EsCallStackGuard::~EsCallStackGuard()
{
    if (count_ > 0)
        g_call_stack.free(count_);
}
