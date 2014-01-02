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

/**
 * Override macro wrapping the "override" declaration in C++11.
 */
#ifndef OVERRIDE
#  if defined(_MSC_VER)
#    define OVERRIDE override
#  elif defined(__clang__)
#    define OVERRIDE override
#  elif defined(__GNUC__)
#    include <features.h>
#    if __GNUC_PREREQ(4, 7)
#      define OVERRIDE override
#    else
#      define OVERRIDE
#    endif
#  else
#    define OVERRIDE
#  endif
#endif

/**
 * Final macro wrapping the "final" declaration in C++11.
 */
#ifndef FINAL
#  if defined(_MSC_VER)
#    define FINAL final
#  elif defined(__clang__)
#    define FINAL final
#  elif defined(__GNUC__)
#    include <features.h>
#    if __GNUC_PREREQ(4, 7)
#      define FINAL final
#    else
#      define FINAL
#    endif
#  else
#    define FINAL
#  endif
#endif
