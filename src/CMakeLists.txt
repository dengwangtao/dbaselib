file(GLOB_RECURSE DBASE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

file(GLOB_RECURSE DBASE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

file(GLOB_RECURSE DBASE_SOURCES_C CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)
set_source_files_properties(${DBASE_SOURCES_C} PROPERTIES LANGUAGE CXX)

if(DBASE_BUILD_SHARED)
    add_library(dbase SHARED)
else()
    add_library(dbase STATIC)
endif()

add_library(dbase::dbase ALIAS dbase)

target_sources(dbase
    PRIVATE
        ${DBASE_SOURCES}
        ${DBASE_SOURCES_C}
)

target_include_directories(dbase
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dbase/stl>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/asio>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

dbase_apply_options(dbase)
dbase_apply_warnings(dbase)

target_compile_definitions(dbase
    PUBLIC
        ASIO_STANDALONE
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<CONFIG:Debug>:DBASE_DEBUG>
)

set_target_properties(dbase PROPERTIES
    OUTPUT_NAME dbase
    DEBUG_POSTFIX d
    POSITION_INDEPENDENT_CODE ON
    VISIBILITY_INLINES_HIDDEN ON
)
