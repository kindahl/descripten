/*
 *  profiler.cc
 *  runtime
 *
 *  Created by Christian Kindahl on 7/30/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include "profiler.hh"

namespace profiler
{
    Statistics stats;

    void print_results()
    {
        if (stats.ctx_access_cnt_ > 0)
        {
            std::cout << "context cache hits: "
                      << ((100 * stats.ctx_cache_hits_) / stats.ctx_access_cnt_)
                      << "% (" << stats.ctx_cache_hits_
                      << " / " << stats.ctx_access_cnt_ << ")" << std::endl;
            std::cout << "context cache misses: "
                      << ((100 * stats.ctx_cache_misses_) / stats.ctx_access_cnt_)
                      << "% (" << stats.ctx_cache_misses_
                      << " / " << stats.ctx_access_cnt_ << ")" << std::endl;
        }

        if (stats.prp_access_cnt_ > 0)
        {
            std::cout << "property cache hits: "
                      << ((100 * stats.prp_cache_hits_) / stats.prp_access_cnt_)
                      << "% (" << stats.prp_cache_hits_
                      << " / " << stats.prp_access_cnt_ << ")" << std::endl;
            std::cout << "property cache misses: "
                      << ((100 * stats.prp_cache_misses_) / stats.prp_access_cnt_)
                      << "% (" << stats.prp_cache_misses_
                      << " / " << stats.prp_access_cnt_ << ")" << std::endl;
        }
    }
}
