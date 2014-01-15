#!/bin/bash

export DYLD_LIBRARY_PATH=$PWD/../common/.libs/:$DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$PWD/../parser/.libs/:$DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$PWD/../runtime/.libs/:$DYLD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD/../common/.libs/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD/../parser/.libs/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD/../runtime/.libs/:$LD_LIBRARY_PATH

echo common:
./bin/test-common
echo

echo parser:
./bin/test-parser
echo

echo runtime:
./bin/test-runtime
