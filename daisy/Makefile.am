ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS       = src/daisy
src_daisy_SOURCES  = src/main.cpp
src_daisy_CPPFLAGS = -I$(top_srcdir)/../libdaisychain/src \
                     -I$(top_srcdir)/../3rdparty/easyloggingpp/src \
                     -I$(top_srcdir)/../3rdparty/tclap/include \
                     -I$(top_srcdir)/../3rdparty/nlohmann_json/single_include \
                     -I../libdaisychain/src

src_daisy_LDADD    = ../libdaisychain/src/libdaisychain.la
