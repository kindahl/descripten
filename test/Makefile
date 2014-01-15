arch := $(shell uname -m)
platform := $(shell uname -s)

# Variables.
ifeq ($(platform),Darwin)
    CXXTESTGEN=cxxtestgen.pl
    archflags = -arch x86_64
    platformflags = -DPLATFORM_DARWIN
endif

ifeq ($(platform),Linux)
    CXXTESTGEN=cxxtestgen
    archflags =
    platformflags = -DPLATFORM_LINUX
endif

CXX=g++
CXXFLAGS=$(archflags) $(platformflags) \
         -L../common/.libs/ -L../parser/.libs/ -L../runtime/.libs/ \
         $(shell pkg-config --cflags --libs bdw-gc libpcre) \
         -DDEBUG -DUNITTEST -I.. -std=c++11
CXXFLAGS_COMMON=$(CXXFLAGS) -lcommon -lm
CXXFLAGS_PARSER=$(CXXFLAGS) -lcommon -lparser
CXXFLAGS_RUNTIME=$(CXXFLAGS) -lcommon -lparser -lruntime -lm

# Targets.
all: lexer parser test

test: bin/test-common bin/test-parser bin/test-runtime
	

clean:
	rm -f bin/test-common
	rm -f bin/test-parser
	rm -f bin/test-runtime
	rm -f test-common.cc
	rm -f test-parser.cc
	rm -f test-runtime.cc

bin/test-common: test-common.cc
	$(CXX) $(CXXFLAGS_COMMON) test-common.cc -o bin/test-common

test-common.cc: src/common/list.hh src/common/string.hh \
				src/common/stringbuilder.hh src/common/unicode.hh
	$(CXXTESTGEN) --error-printer -o test-common.cc \
		src/common/list.hh src/common/string.hh src/common/stringbuilder.hh \
		src/common/unicode.hh

bin/test-parser: test-parser.cc
	$(CXX) $(CXXFLAGS_PARSER) test-parser.cc -o bin/test-parser

test-parser.cc: src/parser/stream.hh
	$(CXXTESTGEN) --error-printer -o test-parser.cc src/parser/stream.hh

bin/test-runtime: test-runtime.cc
	$(CXX) $(CXXFLAGS_RUNTIME) test-runtime.cc -o bin/test-runtime

test-runtime.cc: src/runtime/map.hh src/runtime/property_array.hh \
				 src/runtime/shape.hh src/runtime/string.hh \
				 src/runtime/value.hh
	$(CXXTESTGEN) --error-printer -o test-runtime.cc \
		src/runtime/map.hh src/runtime/property_array.hh src/runtime/shape.hh \
		src/runtime/string.hh src/runtime/value.hh

lexer:
	$(CXX) $(CXXFLAGS_PARSER) lexer.cc -o bin/lexer

parser:
	$(CXX) $(CXXFLAGS_PARSER) parser.cc -o bin/parser
