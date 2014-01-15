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

#include <gc.h>
#include "config.hh"
#include "common/exception.hh"
#include "context.hh"
#include "conversion.hh"
#include "frame.hh"
#include "global.hh"
#include "property_key.hh"
#include "prototype.hh"
#include "runtime.hh"
#include "utility.hh"

#ifdef PROFILE
#include "profiler.hh"
#endif

std::string err_msg_;

bool esr_init(GlobalDataEntry data_entry)
{
    // Initialize garbage collector.
    GC_INIT();

    g_call_stack.init();

    data_entry();

    property_keys.initialize();

    try
    {
        // Create objects.
        es_global_create();
        es_proto_create();
        
        // Initialize objects.
        es_global_init();
        es_proto_init();
    }
    catch (Exception &e)
    {
        err_msg_ = e.what();
        return false;
    }
    
    return true;
}

bool esr_run(GlobalMainEntry main_entry)
{
    bool result = true;

    EsContextStack::instance().push_global(false);

    try
    {
        EsCallFrame frame = EsCallFrame::push_global();
        result = main_entry(EsContextStack::instance().top(), 0,
                            frame.fp(), frame.vp());
        if (!result)
        {
            assert(EsContextStack::instance().top()->has_pending_exception());
            EsValue e = EsContextStack::instance().top()->get_pending_exception();

            const EsString *err_msg = e.to_stringT();
            assert(err_msg);

            err_msg_ = err_msg->utf8();
        }
    }
    catch (EsValue &e)
    {
        assert(false);

        const EsString *err_msg = e.to_stringT();
        assert(err_msg);

        err_msg_ = err_msg->utf8();

        result = false;
    }
    catch (Exception &e)
    {
        assert(false);
        err_msg_ = e.what();
        
        result = false;
    }

    // Make sure that we have not been sloppy with the stack.
    //assert(g_call_stack.size() == 0);
    // FIXME: Make sure stack is empty and do a final collect.

#ifdef PROFILE
    profiler::print_results();
#endif
    return result;
}

const char *esr_error()
{
    return err_msg_.c_str();
}
