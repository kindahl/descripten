#!/usr/bin/env python3

import fileinput
import getopt
import glob
import os
import platform
import queue
import re
import subprocess
import sys
import threading
import time

is_linux = platform.uname()[0] == 'Linux'
is_darwin = platform.uname()[0] == 'Darwin'

RCC='../compiler/compiler'
EVL='../tools/evaluator'

GCC_COMPILE=str.join(' ', [
    '/usr/lib/ccache/bin/g++' if is_linux else '/opt/local/libexec/ccache/g++',
    '-arch x86_64' if is_darwin else '',
    '-std=c++11',
    #'-O3',
    '-g',
    '-I' + os.path.join(os.getcwd(), '../'),
    '-I' + os.path.join(os.getcwd(), '../runtime/'),
    '-DPLATFORM_LINUX' if is_linux else '',
    '-DPLATFORM_DARWIN' if is_darwin else '',
    subprocess.check_output('pkg-config --cflags bdw-gc libpcre',
                            shell=True, universal_newlines=True).strip(),
    '-c'
])

GCC_LINK=str.join(' ', [
    '/usr/lib/ccache/bin/g++' if is_linux else '/opt/local/libexec/ccache/g++',
    '-L' + os.path.join(os.getcwd(), '../common/.libs/'),
    '-L' + os.path.join(os.getcwd(), '../parser/.libs/'),
    '-L' + os.path.join(os.getcwd(), '../runtime/.libs/'),
    '-lcommon', '-lparser', '-lruntime',
    subprocess.check_output('pkg-config --libs bdw-gc libpcre',
                            shell=True, universal_newlines=True).strip(),
    'main.o'
])

# setup environment variables
env_paths = [
    os.path.join(os.getcwd(), '../common/.libs/'),
    os.path.join(os.getcwd(), '../parser/.libs/'),
    os.path.join(os.getcwd(), '../runtime/.libs/')
]

env_vars = [
    'DYLD_LIBRARY_PATH',
    'LD_LIBRARY_PATH'
]

for env in env_vars:
    env_val_sfx = ''
    if (env in os.environ):
        env_val_sfx = ':' + os.environ[env]
    os.environ[env] = str.join(':', env_paths) + env_val_sfx

# blacklisted tests that should be skipped.
test_blacklist = [
    'test262/test/suite/ch07/7.8/7.8.5/S7.8.5_A1.4_T1.js',  # FIXME: regular expression.
    'test262/test/suite/ch07/7.8/7.8.5/S7.8.5_A1.4_T2.js',  # FIXME: regular expression.
    'test262/test/suite/ch07/7.8/7.8.5/S7.8.5_A2.4_T1.js',  # FIXME: regular expression.
    'test262/test/suite/ch07/7.8/7.8.5/S7.8.5_A2.4_T2.js',  # FIXME: regular expression.
    'test262/test/suite/ch11/11.2/11.2.1/S11.2.1_A4_T1.js', # optional according to standard.
    'test262/test/suite/ch11/11.2/11.2.1/S11.2.1_A4_T9.js', # optional according to standard.
    'test262/test/suite/ch13/13.2/S13.2.1_A7_T3.js',        # claims x=x should throw which cannot be confirmed with the standard.
    'test262/test/suite/ch15/15.1/15.1.3/15.1.3.1/S15.1.3.1_A2.5_T1.js',    # passes, but takes 5+ mins to run.
    'test262/test/suite/ch15/15.1/15.1.3/15.1.3.2/S15.1.3.2_A2.5_T1.js',    # passes, but takes 5+ mins to run.
    'test262/test/suite/ch15/15.2/15.2.2/S15.2.2.1_A2_T5.js',           # optional according to standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.3/15.2.3.3-4-12.js',    # optional according to standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.3/15.2.3.3-4-164.js',   # RegExp.prototype.compile is not specified in standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.3/15.2.3.3-4-119.js',   # optional according to standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.3/15.2.3.3-4-155.js',   # optional according to standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.3/15.2.3.3-4-13.js',    # optional according to standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.3/15.2.3.3-4-137.js',   # optional according to standard.
    'test262/test/suite/ch15/15.2/15.2.3/15.2.3.14/15.2.3.14-5-13.js',  # assumes Object.keys(O) array to be sorted.
    'test262/test/suite/ch15/15.5/15.5.4/15.5.4.11/S15.5.4.11_A5_T1.js',# non-standard octal literals.
    'test262/test/suite/ch15/15.10/15.10.1/S15.10.1_A1_T3.js',          # regular expression syntax accepted by pcre.
    'test262/test/suite/ch15/15.10/15.10.2/S15.10.2_A1_T1.js',          # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.5/S15.10.2.5_A1_T4.js',  # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.6/S15.10.2.6_A4_T7.js',  # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.7/S15.10.2.7_A2_T1.js',  # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.9/S15.10.2.9_A1_T4.js',  # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.10/S15.10.2.10_A2.1_T3.js',  # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.12/S15.10.2.12_A2_T2.js',    # FIXME: regular expression.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.12/S15.10.2.12_A2_T5.js',    # in PCRE \v belongs to the \S class (not considered a white space).
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.12/S15.10.2.12_A1_T5.js',    # in PCRE \v belongs to the \S class (not considered a white space).
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.12/S15.10.2.12_A2_T1.js',    # \S class problem.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.12/S15.10.2.12_A1_T1.js',    # \s class problem.
    'test262/test/suite/ch15/15.10/15.10.2/15.10.2.12/S15.10.2.12_A1_T2.js',    # \s class problem.
    'test262/test/suite/ch15/15.10/15.10.6/15.10.6.2/S15.10.6.2_A1_T6.js',      # FIXME: regular expression.
]

# tests marked @negative but that should parse and fail at runtime.
test_fail = [
    'test262/test/suite/ch07/7.3/S7.3_A2.1_T1.js',
    'test262/test/suite/ch07/7.3/S7.3_A2.2_T1.js',
    'test262/test/suite/ch07/7.3/S7.3_A2.3.js',
    'test262/test/suite/ch07/7.3/S7.3_A2.4.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.1_T1.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.1_T2.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.2_T1.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.2_T2.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.3_T1.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.3_T2.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.4_T1.js',
    'test262/test/suite/ch07/7.3/S7.3_A3.4_T2.js',
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T1.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T2.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T3.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T4.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T5.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T6.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T7.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.8/7.8.3/S7.8.3_A4.1_T8.js',  # v8 fails with reference error, documentation hints that it should be a syntax error.
    'test262/test/suite/ch07/7.9/S7.9_A7_T7.js',
    'test262/test/suite/ch08/8.4/S8.4_A7.1.js',
    'test262/test/suite/ch08/8.4/S8.4_A7.2.js',
    'test262/test/suite/ch08/8.4/S8.4_A7.3.js',
    'test262/test/suite/ch08/8.4/S8.4_A7.4.js',
    'test262/test/suite/ch08/8.6/8.6.2/S8.6.2_A7.js',
    'test262/test/suite/ch08/8.7/8.7.2/8.7.2-3-a-1gs.js',
    'test262/test/suite/ch08/8.7/8.7.2/8.7.2-3-a-2gs.js',
    'test262/test/suite/ch10/10.4/10.4.2/10.4.2.1-1gs.js',
    'test262/test/suite/ch10/10.6/10.6-2gs.js',
    'test262/test/suite/ch11/11.3/11.3.1/S11.3.1_A1.1_T1.js',
    'test262/test/suite/ch11/11.3/11.3.1/S11.3.1_A1.1_T2.js',
    'test262/test/suite/ch11/11.3/11.3.1/S11.3.1_A1.1_T3.js',
    'test262/test/suite/ch11/11.3/11.3.1/S11.3.1_A1.1_T4.js',
    'test262/test/suite/ch11/11.3/11.3.2/S11.3.2_A1.1_T1.js',
    'test262/test/suite/ch11/11.3/11.3.2/S11.3.2_A1.1_T2.js',
    'test262/test/suite/ch11/11.3/11.3.2/S11.3.2_A1.1_T3.js',
    'test262/test/suite/ch11/11.3/11.3.2/S11.3.2_A1.1_T4.js',
    'test262/test/suite/ch11/11.4/11.4.2/S11.4.2_A2_T2.js',
    'test262/test/suite/ch11/11.13/11.13.1/11.13.1-4-28gs.js',
    'test262/test/suite/ch11/11.13/11.13.1/11.13.1-4-29gs.js',
    'test262/test/suite/ch12/12.5/S12.5_A2.js',     # not sure about this.
    #'test262/test/suite/ch12/12.7/S12.7_A1_T1.js',  # not sure about this - checked in compiler.
    #'test262/test/suite/ch12/12.7/S12.7_A1_T4.js',  # not sure about this - checked in compiler.
    #'test262/test/suite/ch12/12.7/S12.7_A8_T2.js',  # not sure about this - checked in compiler.
    #'test262/test/suite/ch12/12.8/S12.8_A1_T1.js',  # not sure about this - checked in compiler.
    #'test262/test/suite/ch12/12.8/S12.8_A1_T3.js',  # not sure about this - checked in compiler.
    #'test262/test/suite/ch12/12.8/S12.8_A8_T2.js',  # not sure about this - checked in compiler.
    'test262/test/suite/ch12/12.13/S12.13_A1.js',
    'test262/test/suite/ch13/13.0/13_4-17gs.js',
    'test262/test/suite/ch15/15.1/S15.1_A1_T1.js',
    'test262/test/suite/ch15/15.1/S15.1_A1_T2.js',
    'test262/test/suite/ch15/15.1/S15.1_A2_T1.js',
    'test262/test/suite/ch15/15.1/15.1.2/15.1.2.1/S15.1.2.1_A2_T2.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.3/S15.2.4.3_A12.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.3/S15.2.4.3_A13.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.4/S15.2.4.4_A12.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.4/S15.2.4.4_A13.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.4/S15.2.4.4_A14.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.4/S15.2.4.4_A15.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.5/S15.2.4.5_A12.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.5/S15.2.4.5_A13.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.6/S15.2.4.6_A12.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.6/S15.2.4.6_A13.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.7/S15.2.4.7_A12.js',
    'test262/test/suite/ch15/15.2/15.2.4/15.2.4.7/S15.2.4.7_A13.js',
    'test262/test/suite/ch15/15.3/15.3.2/15.3.2.1/15.3.2.1-10-4gs.js',
    'test262/test/suite/ch15/15.3/15.3.2/15.3.2.1/15.3.2.1-10-6gs.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.2/S15.3.4.2_A12.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.2/S15.3.4.2_A13.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.2/S15.3.4.2_A14.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.2/S15.3.4.2_A15.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.2/S15.3.4.2_A16.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.3/S15.3.4.3_A13.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.3/S15.3.4.3_A14.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.3/S15.3.4.3_A15.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.4/S15.3.4.4_A13.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.4/S15.3.4.4_A14.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.4/S15.3.4.4_A15.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.5/S15.3.4.5_A13.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.5/S15.3.4.5_A14.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.5/S15.3.4.5_A15.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.5/S15.3.4.5_A1.js',
    'test262/test/suite/ch15/15.3/15.3.4/15.3.4.5/S15.3.4.5_A2.js',
    'test262/test/suite/ch15/15.3/15.3.5/15.3.5-1gs.js',
    'test262/test/suite/ch15/15.3/15.3.5/15.3.5-2gs.js'
]

# for statistics.
stat_ok = 0
stat_skip = 0
stat_fail = 0

path_lib = os.path.join(os.getcwd(), 'test262', 'test', 'harness')
path_inc = os.path.join(os.getcwd(), 'test262', 'external', 'contributions', 'Google', 'sputniktests', 'lib')

# utility function shamelessly copied from sputnik.py
# Copyright 2009 the Sputnik authors.  All rights reserved.
# This code is governed by the BSD license found in the LICENSE file.
def GetDaylightSavingsTimes():
    # Is the given floating-point time in DST?
    def IsDst(t):
        return time.localtime(t)[-1]

    # Binary search to find an interval between the two times no greater than
    # delta where DST switches, returning the midpoint.
    def FindBetween(start, end, delta):
        while end - start > delta:
            middle = (end + start) / 2
            if IsDst(middle) == IsDst(start):
                start = middle
            else:
                end = middle
        return (start + end) / 2

    now = time.time()
    one_month = (30 * 24 * 60 * 60)
    # First find a date with different daylight savings.  To avoid corner cases
    # we try four months before and after today.
    after = now + 4 * one_month
    before = now - 4 * one_month
    if IsDst(now) == IsDst(before) and IsDst(now) == IsDst(after):
        logger.warning("Was unable to determine DST info.")
        return None

    # Determine when the change occurs between now and the date we just found
    # in a different DST.
    if IsDst(now) != IsDst(before):
        first = FindBetween(before, now, 1)
    else:
        first = FindBetween(now, after, 1)

    # Determine when the change occurs between three and nine months from the
    # first.
    second = FindBetween(first + 3 * one_month, first + 9 * one_month, 1)
    # Find out which switch is into and which if out of DST
    if IsDst(first - 1) and not IsDst(first + 1):
        start = second
        end = first
    else:
        start = first
        end = second
    return (start, end)

# utility function shamelessly copied from sputnik.py
# Copyright 2009 the Sputnik authors.  All rights reserved.
# This code is governed by the BSD license found in the LICENSE file.
def GetDaylightSavingsAttribs():
    times = GetDaylightSavingsTimes()
    if not times:
        return None
    (start, end) = times
    def DstMonth(t):
        return time.localtime(t)[1] - 1
    def DstHour(t):
        return time.localtime(t - 1)[3] + 1
    def DstSunday(t):
        if time.localtime(t)[2] > 15:
          return "'last'"
        else:
          return "'first'"
    def DstMinutes(t):
        return (time.localtime(t - 1)[4] + 1) % 60

    attribs = { }
    attribs['start_month'] = DstMonth(start)
    attribs['end_month'] = DstMonth(end)
    attribs['start_sunday'] = DstSunday(start)
    attribs['end_sunday'] = DstSunday(end)
    attribs['start_hour'] = DstHour(start)
    attribs['end_hour'] = DstHour(end)
    attribs['start_minutes'] = DstMinutes(start)
    attribs['end_minutes'] = DstMinutes(end)
    return attribs

# get source for include file.
def get_sys_include(name):
    inc_path = os.path.join(path_lib, name)

    if (os.path.exists(inc_path)):
        with open(inc_path) as inc:
            return inc.readlines()

    raise RuntimeError("couldn't open system include file '" + name + "'.")


def get_tz_include():
    dst_attribs = GetDaylightSavingsAttribs()
    if not dst_attribs:
        return ''

    lines = []
    for key in sorted(dst_attribs.keys()):
        lines.append('var $DST_%s = %s;\n' % (key, str(dst_attribs[key])))

    localtz = time.timezone / -3600
    lines.append('var $LocalTZ = %i;\n' % localtz)
    return lines

def get_usr_include(name):
    tz_src = []
    if (name == 'Date_library.js'):
        tz_src = get_tz_include()

    inc_path = os.path.join(path_inc, name)
    if (not os.path.exists(inc_path)):
        inc_path = os.path.join(path_lib, name)

    if (os.path.exists(inc_path)):
        with open(inc_path) as inc:
            return tz_src + inc.readlines()

    raise RuntimeError("couldn't open user include file '" + name + "'.")

def get_sys_includes():
    #return get_sys_include('cth.js') + \
    #       get_sys_include('sta.js') + \
    #       get_sys_include('ed.js')
    return ''

def get_source(path):
    p = re.compile('\\$INCLUDE\\s*\\(\\s*[\'"](\\w*\\.\\w*)[\'"]\\s*\\)')

    src_pp = ''
    with open(path) as src:
        for src_line in src.readlines():
            did_include = False
            if (src_line.strip()[0:8] == '$INCLUDE'):
                res = p.search(src_line)
                if (res != None):
                    src_pp = src_pp + '// >>>> ' + res.group(1) + '\n'
                    src_pp = src_pp + ''.join(get_usr_include(res.group(1)))
                    src_pp = src_pp + '// <<<< ' + res.group(1) + '\n'
                    did_include = True
            if (not did_include):
                src_pp = src_pp + src_line

    return ''.join(get_sys_includes()) + '\n' + src_pp

# preprocess single test.
def test_pp(path):
    path_pp = os.path.splitext(path)[0] + '.pp'

    with open(path) as src:
        src_lines = src.readlines()

        with open(path_pp, 'w') as dst:
            dst.write(get_source(path))

# compile single test.
def test_compile(path, verbose=True):
    path_cpp = os.path.splitext(path)[0] + '.cc'
    path_obj = os.path.splitext(path)[0] + '.o'
    path_bin = os.path.splitext(path)[0]

    # check if we need to preprocess the file.
    if (not subprocess.call('grep -q \'$INCLUDE\' ' + path, shell=True)):
        test_pp(path)
        path_pp = os.path.splitext(path)[0] + '.pp'
        cmdline = RCC + ' ' + path_pp + ' -o ' + path_cpp
    else:
        cmdline = RCC + ' ' + path + ' -o ' + path_cpp

    if (not verbose):
        cmdline += ' > /dev/null 2>&1'

    if (subprocess.call(cmdline, shell=True) != 0):
        return 1

    res = subprocess.call(GCC_COMPILE + ' ' + path_cpp + ' -o ' + path_obj, shell=True)
    if (res != 0):
        return res

    return subprocess.call(GCC_LINK + ' ' + path_obj + ' -o ' + path_bin, shell=True)

# run single test through evaluator.
def test_eval(path, verbose=True):
    # check if we need to preprocess the file.
    if (not subprocess.call('grep -q \'$INCLUDE\' ' + path, shell=True)):
        test_pp(path)
        path_pp = os.path.splitext(path)[0] + '.pp'
        cmdline = EVL + ' ' + path_pp
    else:
        cmdline = EVL + ' ' + path

    if (not verbose):
        cmdline += ' > /dev/null 2>&1'

    return subprocess.call(cmdline, shell=True);

# runs a single test.
def test_run(path, use_evaluator, verbose=True):
    global stat_ok
    global stat_skip
    global stat_fail

    path_bin = os.path.splitext(path)[0]

    # check if the test is blacklisted.
    if (os.path.relpath(path) in test_blacklist):
        print('[SKIP] ' + os.path.basename(path))
        stat_skip += 1
        return 0

    expected_res = int(not subprocess.call(['grep', '-q', '@negative', path]))
    if (use_evaluator):
        # evaluate test.
        if (test_eval(path, verbose) != expected_res):
            print('[FAIL] ' + os.path.basename(path))
            stat_fail += 1
            return 1
    else:
        # compile test.
        if (expected_res == 1 and os.path.relpath(path) in test_fail):
            expected_res = 0

        if (test_compile(path, verbose) != expected_res):
            print('[FAIL] ' + os.path.basename(path))
            stat_fail += 1
            return 1

        # run test.
        if (expected_res == 0):
            expected_res = int(os.path.relpath(path) in test_fail)

            cmdline = path_bin
            #if (expected_res != 0):
            #    cmdline += ' > /dev/null 2>&1'
            if (not verbose):
                cmdline += ' > /dev/null 2>&1'

            t0 = time.clock()

            if (subprocess.call(cmdline, shell=True) != expected_res):
                print('[FAIL] ' + os.path.basename(path))
                stat_fail += 1
                return 1

            t1 = time.clock()
            elapsed = t1 - t0
            if (elapsed > 1.0):
                with open(path_bin + '.time', 'w') as time_file:
                    time_file.write(str(t1 - t0) + '\n')

    print('[ OK ] ' + os.path.basename(path))
    stat_ok += 1
    return 0

# runs all tests.
def run_tests(abort_on_fail, num_worker_threads, start_at, use_evaluator):
    in_comment = False

    abort = threading.Event()

    # function for performing work in a single thread.
    def do_work(path):
        if (test_run(path, use_evaluator, False) != 0):
            if (abort_on_fail):
                abort.set()
                return 1
        return 0

    # delegates work to the worker function.
    def worker():
        while True:
            path = q.get()
            do_work(path)
            q.task_done()

    q = queue.Queue(num_worker_threads)

    for i in range(num_worker_threads):
        t = threading.Thread(target=worker)
        t.daemon = True
        t.start()

    # prepare start point
    found_start = len(start_at) == 0

    for line in fileinput.input('run-test262.input'):
        test_dir = line.strip()
        if (test_dir[0] == '#'):
            continue

        if (test_dir[0] == '{'):
            in_comment = True
        elif (test_dir[0] == '}'):
            in_comment = False

        if (in_comment):
            continue

        for path in glob.glob(os.path.join(os.getcwd(), test_dir, '*.js')):
            # check if we have found the start.
            if (not found_start):
                if (os.path.basename(path) == start_at):
                    found_start = True
                else:
                    continue

            if (abort.is_set()):
                break

            q.put(path)

    q.join()
    return 0

# main entry.
def main():
    # parse command line options.
    num_worker_threads = 8
    start_at = ''
    use_evaluator = False

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'x', ['num-threads=', 'start-at=', 'use-evaluator'])
    except getopt.GetoptError:
        print('error: invalid usage.')
        exit(1)

    if (subprocess.call(GCC_COMPILE + ' main.cc -o main.o', shell=True) != 0):
        print('error: unable to compile main.cc.')
        exit(1)

    for o, a in opts:
        if (o == '--num-threads'):
            num_worker_threads = int(a)
        elif (o == '--start-at'):
            start_at = a
            if (len(args) > 0):
                print('error: --start-at cannot be used with a single test case.')
                exit(1)
        elif (o == '--use-evaluator'):
            use_evaluator = True
        else:
            print('warning: unknown command line option ' + o)

    # test if we should run all tests or a single one.
    if (len(args) > 0):
        if (os.path.exists(args[0])):
            path = os.path.abspath(args[0])
        else:
            cmdline = 'find ' + os.path.join(os.getcwd(), 'test262/test/suite/') + ' -name ' + args[0]
            path = subprocess.check_output(cmdline, shell=True).decode("utf-8").strip()

        if (not os.path.exists(path)):
            print('error: no such file')
            exit(1)

        test_run(path, use_evaluator)
    else:
        run_tests(True, num_worker_threads, start_at, use_evaluator)

    global stat_ok
    global stat_skip
    global stat_fail

    print('')
    print('summary:')
    print('  ' + str(stat_ok) + ' succeeded.');
    print('  ' + str(stat_skip) + ' skipped.');
    print('  ' + str(stat_fail) + ' failed.');

if __name__ == "__main__":
    main()
