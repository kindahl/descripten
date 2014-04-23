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

EXT='c'

RCC='../compiler/compiler'
EVL='../tools/evaluator'

GCC_COMPILE=str.join(' ', [
    #'/usr/lib/ccache/bin/g++',
    #'clang++',
    '/usr/lib/ccache/bin/gcc',
    '-arch x86_64' if is_darwin else '',
    #'-std=c++11',
    '-std=c11',
    '-Werror=unused-variable',
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
    #'/usr/lib/ccache/bin/g++',
    #'clang++',
    '/usr/lib/ccache/bin/gcc',
    '-L' + os.path.join(os.getcwd(), '../common/.libs/'),
    '-L' + os.path.join(os.getcwd(), '../parser/.libs/'),
    '-L' + os.path.join(os.getcwd(), '../runtime/.libs/'),
    '-lcommon', '-lparser', '-lruntime',
    subprocess.check_output('pkg-config --libs bdw-gc libpcre',
                            shell=True, universal_newlines=True).strip()
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

# compile single test.
def test_compile(paths):
    if (len(paths) == 1):
        path = ''.join(paths)
        path_src = os.path.splitext(path)[0] + '.' + EXT
        path_obj = os.path.splitext(path)[0] + '.o'
        path_bin = os.path.splitext(path)[0]
    else:
        path = ' '.join(paths)
        path_src = 'v8/all.' + EXT
        path_obj = 'v8/all.o'
        path_bin = 'v8/all'

    cmdline = RCC + ' v8/base.js ' + path + ' v8/run.js -o ' + path_src
    if (subprocess.call(cmdline, shell=True) == 1):
        return 1

    res = subprocess.call(GCC_COMPILE + ' ' + path_src + ' -o ' + path_obj, shell=True)
    if (res != 0):
        return res

    return subprocess.call(GCC_LINK + ' ' + path_obj + ' -o ' + path_bin, shell=True)

# runs a single test.
def run_test(path):
    path_bin = os.path.splitext(path)[0]

    # compile test.
    paths = [ path ]
    if (test_compile(paths) != 0):
        return 1

    # run test.
    if (subprocess.call(path_bin, shell=True) != 0):
        return 1

    return 0

# runs all tests.
def run_tests():
    in_comment = False

    tests = []
    for line in fileinput.input('run-v8.input'):
        test_file = line.strip()
        if (test_file[0] == '#'):
            continue

        if (test_file[0] == '{'):
            in_comment = True
        elif (test_file[0] == '}'):
            in_comment = False

        if (in_comment):
            continue

        tests.append(test_file);

    if (len(tests) == 1):
        run_test(''.join(tests))
    else:
        # compile test.
        if (test_compile(tests) != 0):
            return 1

        # run test.
        if (subprocess.call('v8/all', shell=True) != 0):
            return 1

    return 0

# main entry.
def main():
    # parse command line options.
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'x', [])
    except getopt.GetoptError:
        print('error: invalid usage.')
        exit(1)

    # test if we should run all tests or a single one.
    if (len(args) > 0):
        if (os.path.exists(args[0])):
            path = os.path.abspath(args[0])
        else:
            cmdline = 'find ' + os.path.join(os.getcwd(), 'v8/') + ' -name ' + args[0]
            path = subprocess.check_output(cmdline, shell=True).decode("utf-8").strip()

        if (not os.path.exists(path)):
            print('error: no such file')
            exit(1)

        run_test(path)
    else:
        run_tests()

if __name__ == "__main__":
    main()
