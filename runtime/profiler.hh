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
