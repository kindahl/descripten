lib_LTLIBRARIES = libparser.la

libparser_la_SOURCES = ast.cc exception.cc lexer.cc parser.cc printer.cc \
					   stream.cc token.cc utility.cc visitor.cc
libparser_la_CXXFLAGS = -I.. -Wall -Wno-switch \
						-DECMA262_EXT_FUN_STMT
libparser_la_LDFLAGS = -version-info $(PARSER_VERSION)

if PLATFORM_DARWIN
libparser_la_CXXFLAGS += -DPLATFORM_DARWIN
endif

if PLATFORM_LINUX
libparser_la_CXXFLAGS += -DPLATFORM_LINUX
endif

library_includedir = $(includedir)/richmond/parser
library_include_HEADERS = exception.hh lexer.hh parser.hh printer.hh \
						  stream.hh token.hh types.hh utility.hh visitor.hh
