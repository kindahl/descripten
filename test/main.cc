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
#include "runtime.hh"

void _global_data();
bool _global_main(EsContext *ctx, int argc,
                  EsValue *fp, EsValue *vp);

int main(int argc, const char * arv[])
{
    if (!runtime::init(_global_data))
    {
        std::cerr << runtime::error() << std::endl;
        return 1;
    }
    
    if (!runtime::run(_global_main))
    {
        std::cerr << runtime::error() << std::endl;
        return 1;
    }

    return 0;
}
