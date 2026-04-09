AUTOMAKE_OPTIONS = foreign 
AM_CXXFLAGS = -I/usr/local/include

lib_LTLIBRARIES = libDentryExtractor.la

libDentryExtractor_la_SOURCES = Dentry.cpp DentryExtractor.cpp

include_HEADERS = Dentry.hpp DentryExtractor.hpp

libDentryExtractor_la_LDFLAGS = -L/usr/local/lib -lPipelineExpatParser -version-info 0:0:0
