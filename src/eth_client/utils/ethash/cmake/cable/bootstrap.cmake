# Cable: CMake Bootstrap Library.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

# Bootstrap the Cable - CMake Bootstrap Library by including this file.
# e.g. include(cmake/cable/bootstrap.cmake).


# Cable version.
#
# This is internal variable automatically updated with external tools.
# Use CABLE_VERSION variable if you need this information.
set(version 0.2.14)

# For convenience, add the project CMake module dir to module path.
set(module_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if(EXISTS ${module_dir})
    list(APPEND CMAKE_MODULE_PATH ${module_dir})
endif()

if(CABLE_VERSION)
    # Some other instance of Cable was initialized in the top project.

    # Compare versions of the top project and this instances.
    if(CABLE_VERSION VERSION_LESS version)
        set(severity WARNING)
        set(comment "version older than ${version}")
    elseif(CABLE_VERSION VERSION_EQUAL version)
        set(severity STATUS)
        set(comment "same version")
    else()
        set(severity STATUS)
        set(comment "version newer than ${version}")
    endif()

    # Find the name of the top project.
    # Make sure the name is not overwritten by multiple nested projects.
    if(NOT DEFINED cable_top_project_name)
        set(cable_top_project_name ${PROJECT_NAME})
    endif()

    # Mark this project as nested.
    set(PROJECT_IS_NESTED TRUE)

    message(
        ${severity}
        "[cable ] Cable ${CABLE_VERSION} (${comment}) initialized in the `${cable_top_project_name}` parent project"
    )
    cable_debug("Project CMake modules directory: ${module_dir}")

    unset(version)
    unset(module_dir)
    unset(severity)
    unset(comment)
    return()
endif()


option(CABLE_DEBUG "Enable Cable debug logs" OFF)

function(cable_log)
    message(STATUS "[cable ] ${ARGN}")
endfunction()

function(cable_debug)
    if(CABLE_DEBUG)
        message(STATUS "[cable*] ${ARGN}")
    endif()
endfunction()

# Export Cable version.
set(CABLE_VERSION ${version})

# Mark this project as non-nested.
set(PROJECT_IS_NESTED FALSE)

# Add Cable modules to the CMake module path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

cable_log("Cable ${CABLE_VERSION} initialized")
cable_debug("Project CMake modules directory: ${module_dir}")

unset(version)
unset(module_dir)
