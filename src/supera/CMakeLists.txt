#################
# Building Supera
#################

add_subdirectory(base)
add_subdirectory(data)
add_subdirectory(algorithm)
add_subdirectory(process)

add_library(supera SHARED
    $<TARGET_OBJECTS:base>
    $<TARGET_OBJECTS:data>
    $<TARGET_OBJECTS:algorithm>
    $<TARGET_OBJECTS:process>
)

target_link_libraries(supera 
    ${PYTHON_LIBRARIES}
    #-L${LARCV_LIB_DIR} -llarcv3
    )

if(NOT WITHOUT_PYTHON)
    ###############################
    # Python binding (via pybind11)
    ###############################

    pybind11_add_module(pysupera SHARED supera_pybind.cxx)

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
endif()


###########
# Install
###########

install(TARGETS supera
    LIBRARY DESTINATION ${CMAKE_PACKAGE_DIR}/lib
    ARCHIVE DESTINATION ${CMAKE_PACKAGE_DIR}/lib
    RUNTIME DESTINATION ${CMAKE_PACKAGE_DIR}/bin
    INCLUDES DESTINATION ${CMAKE_PACKAGE_DIR}/include
    )

if(NOT WITHOUT_PYTHON)
    install(TARGETS pysupera
        LIBRARY DESTINATION ${CMAKE_PACKAGE_DIR}
        ARCHIVE DESTINATION ${CMAKE_PACKAGE_DIR}
        )
endif()

