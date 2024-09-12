cmake_minimum_required (VERSION 3.12.0)


include_directories (
    src
)


set (daisy_SOURCES
    src/main.cpp
)


add_executable (daisy ${daisy_SOURCES})
target_link_libraries (daisy PRIVATE daisychain_static)
set_target_properties (daisy PROPERTIES CXX_VISIBILITY_PRESET "hidden")
install (TARGETS daisy DESTINATION ${CMAKE_INSTALL_PREFIX})
