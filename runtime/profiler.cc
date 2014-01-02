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
