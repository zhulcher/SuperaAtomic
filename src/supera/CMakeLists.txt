#################
# Building Supera
#################

set(ALL_HEADERS "")

# this needs to be done before the add_subdirectory() calls
# for the include_directories() addition to be inherited, it seems
if(USE_PYBIND11_MKDOC)
    get_filename_component(curdir ${CMAKE_CURRENT_BINARY_DIR} NAME)
    message("adding output dir to c++ include path: ${PROJECT_BINARY_DIR}/${curdir}")
    include_directories(${PROJECT_BINARY_DIR}/${curdir})
endif()

add_subdirectory(base)
add_subdirectory(data)
add_subdirectory(algorithm)
add_subdirectory(process)
#add_subdirectory(test)

add_library(supera SHARED
    $<TARGET_OBJECTS:base>
    $<TARGET_OBJECTS:data>
    $<TARGET_OBJECTS:algorithm>
    $<TARGET_OBJECTS:process>
#    $<TARGET_OBJECTS:test>
)

target_link_libraries(supera 
    ${PYTHON_LIBRARIES}
    yaml-cpp
    #-L${LARCV_LIB_DIR} -llarcv3
    )

if(NOT WITHOUT_PYTHON)
    ###############################
    # Python binding (via pybind11)
    ###############################

    pybind11_add_module(pysupera MODULE supera_pybind.cxx)

    # Add a shared library
    set_target_properties(pysupera PROPERTIES SKIP_BUILD_RPATH FALSE)

    # If building with setuptools, CMake will not be performing the install
    set_target_properties(pysupera PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

    target_link_libraries(pysupera PRIVATE supera)

    if(UNIX AND NOT APPLE)
       set_target_properties(pysupera PROPERTIES INSTALL_RPATH "$ORIGIN/lib/")
    elseif(APPLE)
        set_target_properties(pysupera PROPERTIES INSTALL_RPATH "@loader_path/lib/")
    endif()

    set_target_properties(pysupera PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

    # this is extremely fragile and kind of icky.
    # I think it only works if you're using g++ as the CMake compiler
    # due to the command used to work out where the system include paths are.
    if(USE_PYBIND11_MKDOC)
        message("Setting up pybind11_mkdoc to generate docstrings...")
        # work out the system include paths if you're using something other than Clang to compile...
        if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES Clang)
            # I think in practice this only works with GCC.
            # If we get defect reports maybe we can pursue further...
            execute_process(
                    COMMAND bash -c "${CMAKE_CXX_COMPILER} -Wp,-v -x c++ -fsyntax-only /dev/null 2>&1 | sed -n 's/^ \\(.*\\)/\\1/pg'"
                    OUTPUT_VARIABLE mkdoc_includes
            )
            string(STRIP ${mkdoc_includes} mkdoc_includes)
            string(REPLACE "\n" ";" mkdoc_includes "${mkdoc_includes}")
            list(TRANSFORM mkdoc_includes PREPEND "-I")
            message("using extra system include path to mkdoc: ${mkdoc_includes}")
        endif()
        add_custom_command(OUTPUT ${PYBIND11_MKDOC_DOCSTRING_FILE}
                           COMMAND "${Python_EXECUTABLE}" -m "pybind11_mkdoc" "-std=c++${CMAKE_CXX_STANDARD}" "${mkdoc_includes}" "-I$<JOIN:$<TARGET_PROPERTY:supera,INCLUDE_DIRECTORIES>,;-I>" -o ${PYBIND11_MKDOC_DOCSTRING_FILE} ${ALL_HEADERS}
                           COMMENT "Generating pybind header ${PYBIND11_MKDOC_DOCSTRING_FILE} from Doxygen docstrings"
                           DEPENDS ${ALL_HEADERS}
                           COMMAND_EXPAND_LISTS )
        add_custom_target(mkdoc_docstring_file
                          ALL DEPENDS ${PYBIND11_MKDOC_DOCSTRING_FILE}
                          SOURCES ${ALL_HEADERS} )

        add_dependencies(pysupera mkdoc_docstring_file)
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###########
# Install
###########
install(TARGETS supera
    LIBRARY DESTINATION supera/lib
    ARCHIVE DESTINATION supera/lib
    RUNTIME DESTINATION supera/bin
    INCLUDES DESTINATION supera/include
    )

if(NOT WITHOUT_PYTHON)
    install(TARGETS pysupera
        LIBRARY DESTINATION supera
        ARCHIVE DESTINATION supera
        INCLUDES DESTINATION supera/include
    )
endif()

