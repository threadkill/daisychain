cmake_minimum_required (VERSION 3.12.0)


include_directories (
    src
)


set (libdaisychain_SOURCES
    ../3rdparty/easyloggingpp/src/easylogging++.cc
    src/logger.cpp
    src/commandlinenode.cpp
    src/concatnode.cpp
    src/distronode.cpp
    src/remotenode.cpp
    src/filternode.cpp
    src/filelistnode.cpp
    src/watchnode.cpp
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

