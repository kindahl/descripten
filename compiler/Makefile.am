bin_PROGRAMS = compiler

compiler_SOURCES = allocator.cc cc_generator.cc generator.cc ir_generator.cc main.cc \
				   name_generator.cc rope.cc
compiler_CXXFLAGS = -I.. \
					-DECMA262_EXT_FUNC_STMT
compiler_LDFLAGS = -L../common/.libs/ -L../ir/.libs/ -L../parser/.libs/ \
				   -lcommon -lir -lparser -ldl -lpthread

if PLATFORM_DARWIN
compiler_CXXFLAGS += -DPLATFORM_DARWIN
endif

if PLATFORM_LINUX
compiler_CXXFLAGS += -DPLATFORM_LINUX
endif
