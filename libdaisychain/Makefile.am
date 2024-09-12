ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = src/libdaisychain.la

src_libdaisychain_la_SOURCES = \
    $(top_srcdir)/../3rdparty/easyloggingpp/src/easylogging++.h \
    $(top_srcdir)/../3rdparty/easyloggingpp/src/easylogging++.cc \
	src/logger.h \
	src/logger.cpp \
	src/commandlinenode.h \
	src/commandlinenode.cpp \
	src/concatnode.h \
	src/concatnode.cpp \
	src/distronode.h \
	src/distronode.cpp \
	src/filelistnode.h \
	src/filelistnode.cpp \
	src/filternode.h \
	src/filternode.cpp \
	src/remotenode.h \
	src/remotenode.cpp \
	src/watchnode.h \
	src/watchnode.cpp \
	src/graph.h \
	src/graph.cpp \
	src/node.h

src_libdaisychain_la_CPPFLAGS = -I$(top_srcdir) \
                               -I$(top_srcdir)/../3rdparty/easyloggingpp/src \
                               -I$(top_srcdir)/../3rdparty/nlohmann_json/single_include \
                               -I./src \
                               -DELPP_THREAD_SAFE=1 \
                               -DELPP_FRESH_LOG_FILE=1 \
                               -DELPP_FEATURE_PERFORMANCE_TRACKING=1 \
                               -DELPP_DEFAULT_LOG_FILE=\"/var/tmp/daisy.log\"
