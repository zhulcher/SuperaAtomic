set(name test)

# Get all the source files:
file(GLOB_RECURSE SOURCES *.cxx)
file(GLOB HEADERS *.h)

# update the list inherited from src/supera
list(APPEND ALL_HEADERS ${HEADERS})
set(ALL_HEADERS "${ALL_HEADERS}" PARENT_SCOPE)

# Build the object files
add_library(${name} OBJECT ${SOURCES})

# if the docstring generator is available, use it first
if(USE_PYBIND11_MKDOC)
    add_dependencies(${name} mkdoc_docstring_file)
endif()
