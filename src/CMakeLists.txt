cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# general configuration for compilation:
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -O2 -g")


project(supera)

set(CMAKE_PACKAGE_DIR "./")
set(PROJECT_SOURCE_DIR "./")

set(PYBIND11_MKDOC_DOCSTRING_FILE docstrings.h)

include_directories("./")

message("    Configuring Supera build")

if(NOT WITHOUT_PYTHON)
    add_definitions(-DBUILD_PYTHON_BINDINGS)
    #########
    # Python
    #########
    find_package(Python COMPONENTS Interpreter Development)
    message("    Adding Python")
    message("    INCLUDE:  ${Python_INCLUDE_DIRS}")
    include_directories(${Python_INCLUDE_DIRS})

    if(NOT Python_EXECUTABLE)
    message( FATAL_ERROR "Python_EXECUTABLE not found. Cannot build Supera...")
    endif()

    ########
    # Numpy
    ########
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c
        "from __future__ import print_function\ntry: import numpy; print(numpy.get_include(), end='')\nexcept:pass\n"
        OUTPUT_VARIABLE __numpy_path)

    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c
        "from __future__ import print_function\ntry: import numpy; print(numpy.__version__, end='')\nexcept:pass\n"
        OUTPUT_VARIABLE __numpy_version)

    find_path(PYTHON_NUMPY_INCLUDE_DIR numpy/arrayobject.h
        HINTS "${__numpy_path}" "${PYTHON_INCLUDE_PATH}" NO_DEFAULT_PATH)

    if(PYTHON_NUMPY_INCLUDE_DIR)
        set(PYTHON_NUMPY_FOUND 1 CACHE INTERNAL "Python numpy found")
    else()
        message( FATAL_ERROR "Numpy could not be located. Cannot build Supera...")
    endif()

    message("    Adding Numpy")
    message("    INCLUDE: " ${PYTHON_NUMPY_INCLUDE_DIR})
    include_directories(${PYTHON_NUMPY_INCLUDE_DIR})

    ####################################
    # Add pybind11
    ####################################
    add_subdirectory(pybind11)
    message("    Adding pybind11")
    message("    INCLUDE: " ${PYBIND11_INCLUDE_DIR})
    include_directories(${PYBIND11_INCLUDE_DIR})

    ####################################
    # Check if pybind11_mkdoc is around
    ####################################
    execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import pybind11_mkdoc"
            RESULT_VARIABLE PYBIND11_MKDOC_EXIT_CODE
    )
    if (${PYBIND11_MKDOC_EXIT_CODE} EQUAL 0)
        message("Found pybind11_mkdoc.  Will use it to autogenerate Python docstrings.")
        set(USE_PYBIND11_MKDOC ON)
        add_definitions(-DUSE_MKDOC_DOCSTRINGS)
        include_directories("${CMAKE_INSTALL_PREFIX}")
    endif()

endif()

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

####################################
# Add HDF5
####################################
find_package(HDF5 REQUIRED)
message("    Adding HDF5")
message("    INCLUDE: " ${HDF5_INCLUDE_DIR})
include_directories(${HDF5_INCLUDE_DIR})

####################################
# Add larcv
####################################
#execute_process(COMMAND 
#    "${Python_EXECUTABLE}" -c "import larcv;print(larcv.get_lib_dir(),end='');"
#    OUTPUT_VARIABLE
#    LARCV_LIB_DIR)
#execute_process(COMMAND 
#    "${Python_EXECUTABLE}" -c "import larcv;print(larcv.get_includes(),end='');"
#    OUTPUT_VARIABLE
#    LARCV_INCLUDE_DIR)
#
#message("    Adding larcv")
#message("    INCLUDE: " ${LARCV_INCLUDE_DIR})
#message("    LIB:     " ${LARCV_LIB_DIR}    )
#include_directories(${LARCV_INCLUDE_DIR})

add_subdirectory(supera)

# if the build isn't being driven by scikit-build, we won't get the python __init__.py installed correctly
if(NOT SKBUILD)
    message("Not using scikit-build.  Installing supera's __init__.py by hand!...")
    install(FILES ../python/supera/__init__.py DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
