lib_LTLIBRARIES = libir.la

libir_la_SOURCES = analyzer.cc compiler.cc ir.cc name_generator.cc \
				   optimizer.cc template.cc
libir_la_CXXFLAGS = -I.. -Wall -Wno-switch
libir_la_LDFLAGS = -version-info $(IR_VERSION)

if PLATFORM_DARWIN
libir_la_CXXFLAGS += -DPLATFORM_DARWIN
endif

if PLATFORM_LINUX
libir_la_CXXFLAGS += -DPLATFORM_LINUX
endif

library_includedir = $(includedir)/richmond/ir
library_include_HEADERS = analyzer.hh compiler.hh ir.hh name_generator.hh \
						  optimizer.hh template.hh utility.hh
