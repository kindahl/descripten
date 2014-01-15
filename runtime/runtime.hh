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
#include "operation.hh"
#include "value_data.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EsContext;

typedef void (*GlobalDataEntry)();
typedef bool (*GlobalMainEntry)(EsContext *ctx, uint32_t argc,
                                EsValueData *fp, EsValueData *vp);

bool esr_init(GlobalDataEntry data_entry);
bool esr_run(GlobalMainEntry main_entry);
const char *esr_error();

#ifdef __cplusplus
}
#endif
