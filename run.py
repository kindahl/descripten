#!/usr/bin/env python3

import getopt
import os
import platform
import subprocess
import sys

is_linux = platform.uname()[0] == 'Linux'
is_darwin = platform.uname()[0] == 'Darwin'

EXT='c'

RCC='compiler/compiler'
EVL='tools/evaluator'

GCC_COMPILE=str.join(' ', [
    #'/usr/lib/ccache/bin/g++' if is_linux else '/opt/local/libexec/ccache/g++',
    #'clang++',
    '/usr/lib/ccache/bin/gcc' if is_linux else '/opt/local/libexec/ccache/gcc',
    '-arch x86_64' if is_darwin else '',
    #'-std=c++11',
    '-std=c11',
    '-Werror=unused-variable',
    #'-O3',
    '-g',
    '-I' + os.path.join(os.getcwd(), './'),
    '-I' + os.path.join(os.getcwd(), './runtime/'),
    '-DPLATFORM_LINUX' if is_linux else '',
    '-DPLATFORM_DARWIN' if is_darwin else '',
    subprocess.check_output('pkg-config --cflags bdw-gc libpcre',
                            shell=True, universal_newlines=True).strip(),
    '-c'
])

GCC_LINK=str.join(' ', [
    #'/usr/lib/ccache/bin/g++' if is_linux else '/opt/local/libexec/ccache/g++',
    #'clang++',
    '/usr/lib/ccache/bin/gcc' if is_linux else '/opt/local/libexec/ccache/gcc',
    '-L' + os.path.join(os.getcwd(), './common/.libs/'),
    '-L' + os.path.join(os.getcwd(), './parser/.libs/'),
    '-L' + os.path.join(os.getcwd(), './runtime/.libs/'),
    '-lcommon', '-lparser', '-lruntime',
    subprocess.check_output('pkg-config --libs bdw-gc libpcre',
                            shell=True, universal_newlines=True).strip()
])

# setup environment variables
env_paths = [
    os.path.join(os.getcwd(), './common/.libs/'),
    os.path.join(os.getcwd(), './parser/.libs/'),
    os.path.join(os.getcwd(), './runtime/.libs/')
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

# compile single program.
def program_compile(path):
    path_src = os.path.splitext(path)[0] + '.' + EXT
    path_obj = os.path.splitext(path)[0] + '.o'
    path_bin = os.path.splitext(path)[0]

    cmdline = RCC + ' ' + path + ' -o ' + path_src
    if (subprocess.call(cmdline, shell=True) != 0):
        return 1

    res = subprocess.call(GCC_COMPILE + ' ' + path_src + ' -o ' + path_obj, shell=True)
    if (res != 0):
        return res

    return subprocess.call(GCC_LINK + ' ' + path_obj + ' -o ' + path_bin, shell=True)

# run single program through evaluator.
def program_eval(path):
    cmdline = EVL + ' ' + path
    return subprocess.call(cmdline, shell=True);

# runs a program.
def program_run(path, use_evaluator):
    path_bin = os.path.splitext(path)[0]

    if (use_evaluator):
        # evaluate program.
        if (program_eval(path) != 0):
            return 1
    else:
        # compile program.
        if (program_compile(path) != 0):
            return 1

        # run program.
        cmdline = path_bin
        if (subprocess.call(cmdline, shell=True) != 0):
            return 1

    return 0

# main entry.
def main():
    # parse command line options.
    use_evaluator = False

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'x', ['use-evaluator'])
    except getopt.GetoptError:
        print('error: invalid usage.')
        exit(1)

    for o, a in opts:
        if (o == '--use-evaluator'):
            use_evaluator = True
        else:
            print('warning: unknown command line option ' + o)

    path = os.path.abspath(args[0])
    if (not os.path.exists(path)):
        print('error: no such file')
        exit(1)

    program_run(path, use_evaluator)

if __name__ == "__main__":
    main()
