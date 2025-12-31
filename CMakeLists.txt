cmake_minimum_required(VERSION 3.15)

# Some distros ship the binary as `ninja` (not `ninja-build`). If CMake picked a
# non-existent program, fix it up automatically.
if(CMAKE_GENERATOR MATCHES "Ninja")
    if(CMAKE_MAKE_PROGRAM AND NOT EXISTS "${CMAKE_MAKE_PROGRAM}")
        find_program(NINJA_EXE NAMES ninja ninja-build)
        if(NINJA_EXE)
            set(CMAKE_MAKE_PROGRAM "${NINJA_EXE}" CACHE FILEPATH "Ninja build program" FORCE)
        endif()
    endif()
endif()
project(VTFEditReloaded LANGUAGES C CXX)

option(VTFLIB_USE_COMPRESSONATOR "Link against AMD Compressonator if available" ON)
option(BUILD_VTFCMD "Build VTFCmd CLI" ON)
option(BUILD_VTFEDIT_QT "Build cross-platform Qt GUI (VTFEditQt)" OFF)

add_subdirectory(VTFLib)
if(BUILD_VTFCMD)
    add_subdirectory(VTFCmd)
endif()
if(BUILD_VTFEDIT_QT)
    add_subdirectory(VTFEditQt)
endif()
