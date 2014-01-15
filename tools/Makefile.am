bin_PROGRAMS = evaluator

evaluator_SOURCES = evaluator.cc
evaluator_CXXFLAGS = -I.. \
					 -DECMA262_EXT_FUNC_STMT
evaluator_LDFLAGS = -L../common/.libs/ -L../parser/.libs/ -L../runtime/.libs/ \
					-lcommon -lparser -lruntime -ldl -lpthread

if PLATFORM_DARWIN
evaluator_CXXFLAGS += -DPLATFORM_DARWIN
endif

if PLATFORM_LINUX
evaluator_CXXFLAGS += -DPLATFORM_LINUX
endif
