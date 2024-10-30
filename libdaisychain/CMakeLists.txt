cmake_minimum_required (VERSION 3.12.0)


include_directories (
    src
)

set (libdaisychain_SOURCES
    ../3rdparty/easyloggingpp/src/easylogging++.cc
    src/signalhandler.h
    src/utils.h
    src/utils_win.h
    src/logger.h
    src/logger.cpp
    src/node.h
    src/commandlinenode.h
    src/commandlinenode.cpp
    src/concatnode.h
    src/concatnode.cpp
    src/distronode.h
    src/distronode.cpp
    src/remotenode.h
    src/remotenode.cpp
    src/filternode.h
    src/filternode.cpp
    src/filelistnode.h
    src/filelistnode.cpp
    src/watchnode.h
    src/watchnode.cpp
    src/graph.h
    src/graph.cpp
)

add_library (daisychain SHARED ${libdaisychain_SOURCES})
add_library (daisychain_static STATIC ${libdaisychain_SOURCES})
set_target_properties (
        daisychain_static
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET "hidden"
)
