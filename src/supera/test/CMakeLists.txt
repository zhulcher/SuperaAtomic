set(name test)

# Get all the source files:
file(GLOB_RECURSE SOURCES *.cxx)
file(GLOB HEADERS *.h)

# Build the object files
add_library(${name} OBJECT ${SOURCES})

