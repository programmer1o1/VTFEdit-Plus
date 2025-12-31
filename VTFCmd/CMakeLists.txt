add_executable(vtfcmd
    vtfcmd.c
    enumerations.c
    image_io.c
)

target_include_directories(vtfcmd
    PRIVATE
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}/thirdparty/include
)

target_link_libraries(vtfcmd PRIVATE vtflib)

set(VTFCMD_IMAGE_BACKEND "auto" CACHE STRING "VTFCmd image backend: auto, devil, stb")
set_property(CACHE VTFCMD_IMAGE_BACKEND PROPERTY STRINGS auto devil stb)

set(_vtfcmd_use_devil 0)

if(VTFCMD_IMAGE_BACKEND STREQUAL "stb")
    set(_vtfcmd_use_devil 0)
else()
    # DevIL (OpenIL) - we only need IL/il.h + libIL for vtfcmd.
    set(IL_INCLUDE_DIR "" CACHE PATH "DevIL include dir (contains IL/il.h)")
    set(IL_LIBRARY "" CACHE FILEPATH "DevIL library (libIL.so/libIL.a)")

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PC_IL QUIET IL)
        if(PC_IL_FOUND)
            if(NOT IL_INCLUDE_DIR)
                find_path(IL_INCLUDE_DIR NAMES IL/il.h PATHS ${PC_IL_INCLUDE_DIRS})
            endif()
            if(NOT IL_LIBRARY)
                find_library(IL_LIBRARY NAMES IL PATHS ${PC_IL_LIBRARY_DIRS} PATH_SUFFIXES lib lib64)
            endif()
        endif()
    endif()

    if(NOT IL_INCLUDE_DIR)
        find_path(IL_INCLUDE_DIR NAMES IL/il.h)
    endif()
    if(NOT IL_LIBRARY)
        find_library(IL_LIBRARY NAMES IL)
    endif()

    if(IL_INCLUDE_DIR AND NOT EXISTS "${IL_INCLUDE_DIR}/IL/il.h")
        message(FATAL_ERROR "IL_INCLUDE_DIR was set to '${IL_INCLUDE_DIR}' but '${IL_INCLUDE_DIR}/IL/il.h' was not found.")
    endif()
    if(IL_LIBRARY AND NOT EXISTS "${IL_LIBRARY}")
        message(FATAL_ERROR "IL_LIBRARY was set to '${IL_LIBRARY}' but that file does not exist. Use the full path to libIL.so (or libIL.so.1) on your system.")
    endif()

    if(IL_INCLUDE_DIR AND IL_LIBRARY)
        set(_vtfcmd_use_devil 1)
        target_include_directories(vtfcmd PRIVATE ${IL_INCLUDE_DIR})
        target_link_libraries(vtfcmd PRIVATE ${IL_LIBRARY})
    else()
        if(VTFCMD_IMAGE_BACKEND STREQUAL "devil")
            message(FATAL_ERROR "VTFCMD_IMAGE_BACKEND=devil but DevIL (OpenIL) was not found. Install the DevIL development package (headers + libIL) or use -DVTFCMD_IMAGE_BACKEND=stb.")
        endif()
        set(_vtfcmd_use_devil 0)
    endif()
endif()

target_compile_definitions(vtfcmd PRIVATE VTFCMD_USE_DEVIL=${_vtfcmd_use_devil})

if(NOT _vtfcmd_use_devil)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include/stb_image.h" OR NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include/stb_image_write.h")
        message(FATAL_ERROR "VTFCmd stb backend selected but stb headers are missing. Add stb_image.h and stb_image_write.h to thirdparty/include/ or use -DVTFCMD_IMAGE_BACKEND=devil.")
    endif()
endif()

if(NOT WIN32)
    target_link_libraries(vtfcmd PRIVATE m)
endif()
