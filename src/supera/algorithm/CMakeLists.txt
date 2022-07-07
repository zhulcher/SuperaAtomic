set(name algorithm)

# Get all the source files:
file(GLOB SOURCES *.cxx)
file(GLOB HEADERS *.h)

# update the list inherited from src/supera
list(APPEND ALL_HEADERS ${HEADERS})
set(ALL_HEADERS "${ALL_HEADERS}" PARENT_SCOPE)

# Add a shared library
add_library(${name} OBJECT ${SOURCES})

# if the docstring generator is available, use it first
if(USE_PYBIND11_MKDOC)
    add_dependencies(${name} mkdoc_docstring_file)
endif()

install (FILES ${HEADERS}
    DESTINATION ${CMAKE_PACKAGE_DIR}/include/supera/${name})
