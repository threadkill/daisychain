cmake_minimum_required (VERSION 3.12.0)
project (daisychain)

cmake_policy (SET CMP0063 NEW)
cmake_policy (SET CMP0127 NEW)

set (CMAKE_CXX_STANDARD 20)
add_compile_definitions (ELPP_THREAD_SAFE=1)
add_compile_definitions (ELPP_FRESH_LOG_FILE=1)
add_compile_definitions (ELPP_FEATURE_PERFORMANCE_TRACKING=1)
add_compile_definitions (ELPP_DEFAULT_LOG_FILE="/var/tmp/daisy.log")

include_directories (
    3rdparty/easyloggingpp/src
    3rdparty/nlohmann_json/include
    3rdparty/pybind11/include
    3rdparty/pybind11_json/include
    3rdparty/tclap/include
    libdaisychain/src
)

add_subdirectory (3rdparty/pybind11)
add_subdirectory (libdaisychain)
add_subdirectory (python)
add_subdirectory (daisy)
add_subdirectory (chain)
