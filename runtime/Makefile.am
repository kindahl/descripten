lib_LTLIBRARIES = libruntime.la

libruntime_la_SOURCES = algorithm.cc api.cc context.cc conversion.cc \
						date.cc debug.cc environment.cc error.cc eval.cc \
						global.cc json.cc map.cc messages.cc native.cc \
						object.cc operation.cc runtime.cc platform.cc \
						profiler.cc property.cc property_array.cc \
						property_key.cc prototype.cc resources.cc shape.cc \
						standard.cc test.cc types.cc unique.cc uri.cc \
						utility.cc value.cc
libruntime_la_CXXFLAGS = -I.. \
						 -DECMA262_EXT_FUNC_STMT
libruntime_la_LDFLAGS = -version-info $(PEREGRINE_VERSION)

#if DEBUG
#libruntime_la_CXXFLAGS += -DGC_DEBUG
#endif

if PLATFORM_DARWIN
libruntime_la_CXXFLAGS += -DPLATFORM_DARWIN
endif

if PLATFORM_LINUX
libruntime_la_CXXFLAGS += -DPLATFORM_LINUX
endif

library_includedir = $(includedir)/runtime
library_include_HEADERS = api.hh context.hh utility.hh \
						  operation.hh
