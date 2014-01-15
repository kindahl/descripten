/*
 *  profiler.hh
 *  runtime
 *
 *  Created by Christian Kindahl on 7/30/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

namespace profiler
{

struct Statistics
{
    uint64_t ctx_access_cnt_;       ///< Number of context accesses.
    uint64_t ctx_cache_hits_;       ///< Number of hits in context cache.
    uint64_t ctx_cache_misses_;     ///< Number of misses in context cache.
    uint64_t prp_access_cnt_;       ///< Number of property accesses.
    uint64_t prp_cache_hits_;       ///< Number of hits in property cache.
    uint64_t prp_cache_misses_;     ///< Number of misses in property cache.

    Statistics()
        : ctx_access_cnt_(0)
        , ctx_cache_hits_(0)
        , ctx_cache_misses_(0)
        , prp_access_cnt_(0)
        , prp_cache_hits_(0)
        , prp_cache_misses_(0) {}
};

extern Statistics stats;

void print_results();

}
