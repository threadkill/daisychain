cmake_minimum_required (VERSION 3.12.0)

option (BUILD_PYTHON_MODULE "Build pydaisychain Python module" ON)

if (BUILD_PYTHON_MODULE)
    if (TARGET Python::Module)
        include_directories (
            ${Python_INCLUDE_DIRS}
            src
        )

        set (pydaisychain_SOURCES
            src/pydaisychain.cpp
        )

        pybind11_add_module (pydaisychain MODULE ${pydaisychain_SOURCES})
        target_link_libraries (pydaisychain PRIVATE daisychain_static)
        install (TARGETS pydaisychain DESTINATION lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages)
    endif()
endif()
