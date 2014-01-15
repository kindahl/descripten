lib_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = cast.cc conversion.cc exception.cc lexical.cc \
					   string.cc stringbuilder.cc strings.cc unicode.cc \
					   dtoa.c
libcommon_la_CFLAGS = -DIEEE_8087 -DNO_HEX_FP -DNO_INFNAN_CHECK
libcommon_la_CXXFLAGS = -DECMA262_EXT_FUNC_STMT
libcommon_la_LDFLAGS = -version-info $(COMMON_VERSION)

#if DEBUG
#libcommon_la_CXXFLAGS = -DGC_DEBUG
#endif

if PLATFORM_DARWIN
libcommon_la_CXXFLAGS += -DPLATFORM_DARWIN
endif

if PLATFORM_LINUX
libcommon_la_CXXFLAGS += -DPLATFORM_LINUX
endif

if ARCH_X86_64
# dtoa requires Long to be 32-bits large.
libcommon_la_CFLAGS += -DLong=int
endif

library_includedir = $(includedir)/richmond/common
library_include_HEADERS = cast.hh conversion.hh core.hh exception.hh \
						  lexical.hh string.hh stringbuilder.hh types.hh \
						  unicode.hh
