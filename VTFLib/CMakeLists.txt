set(VTFLIB_SOURCES
    VMTFile.cpp
    VMTIntegerNode.cpp
    VMTStringNode.cpp
    VMTGroupNode.cpp
    VMTNode.cpp
    VMTSingleNode.cpp
    VMTValueNode.cpp
    VMTWrapper.cpp
    VTFFile.cpp
    VTFLib.cpp
    VTFMathlib.cpp
    VTFWrapper.cpp
    Error.cpp
    FileReader.cpp
    FileWriter.cpp
    MemoryReader.cpp
    MemoryWriter.cpp
    ProcReader.cpp
    ProcWriter.cpp
    Proc.cpp
    Float16.cpp
)

add_library(vtflib STATIC ${VTFLIB_SOURCES})
target_include_directories(vtflib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}/thirdparty/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(vtflib PUBLIC VTFLIB_STATIC)

set_target_properties(vtflib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Compressonator (optional)
if(VTFLIB_USE_COMPRESSONATOR)
    set(_compressonator_search_paths
        ${PROJECT_SOURCE_DIR}/thirdparty/lib
        /usr/local/lib
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Precompiled Linux static libs live under lib/linux/<arch>/.
        list(APPEND _compressonator_search_paths
            ${PROJECT_SOURCE_DIR}/lib/linux/${CMAKE_SYSTEM_PROCESSOR}
            ${PROJECT_SOURCE_DIR}/lib/linux/x86_64
        )
    endif()

    if(WIN32)
        # The bundled Windows libs are named `Compressonator_MT{d}.lib`.
        find_library(COMPRESSONATOR_LIB_RELEASE NAMES Compressonator_MT Compressonator compressonator
            PATHS ${_compressonator_search_paths}
        )
        find_library(COMPRESSONATOR_LIB_DEBUG NAMES Compressonator_MTd Compressonator compressonator
            PATHS ${_compressonator_search_paths}
        )
        if(COMPRESSONATOR_LIB_RELEASE OR COMPRESSONATOR_LIB_DEBUG)
            if(COMPRESSONATOR_LIB_RELEASE AND COMPRESSONATOR_LIB_DEBUG)
                target_link_libraries(vtflib PRIVATE
                    optimized ${COMPRESSONATOR_LIB_RELEASE}
                    debug ${COMPRESSONATOR_LIB_DEBUG}
                )
            else()
                target_link_libraries(vtflib PRIVATE ${COMPRESSONATOR_LIB_RELEASE} ${COMPRESSONATOR_LIB_DEBUG})
            endif()
        endif()
    else()
        find_library(COMPRESSONATOR_LIB NAMES Compressonator compressonator
            PATHS ${_compressonator_search_paths}
        )
        if(COMPRESSONATOR_LIB)
            target_link_libraries(vtflib PRIVATE ${COMPRESSONATOR_LIB})
        endif()
    endif()

    if(COMPRESSONATOR_LIB OR COMPRESSONATOR_LIB_RELEASE OR COMPRESSONATOR_LIB_DEBUG)
        # Link all CMP_Core variants (base, SSE, AVX)
        find_library(CMP_CORE_LIB NAMES CMP_Core
            PATHS ${_compressonator_search_paths}
        )
        find_library(CMP_CORE_SSE_LIB NAMES CMP_Core_SSE
            PATHS ${_compressonator_search_paths}
        )
        find_library(CMP_CORE_AVX_LIB NAMES CMP_Core_AVX
            PATHS ${_compressonator_search_paths}
        )
        if(CMP_CORE_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_LIB})
        endif()
        if(CMP_CORE_SSE_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_SSE_LIB})
        endif()
        if(CMP_CORE_AVX_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_AVX_LIB})
        endif()
        target_compile_definitions(vtflib PRIVATE VTFLIB_HAS_COMPRESSONATOR)
        target_include_directories(vtflib PRIVATE ${PROJECT_SOURCE_DIR}/lib/include)
    else()
        message(WARNING "Compressonator library not found; DXT compression will be disabled.")
    endif()
endif()

if(NOT WIN32)
    target_link_libraries(vtflib PRIVATE m)
endif()
