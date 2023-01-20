cmake_minimum_required(VERSION 3.23)

project(AttendanceServer VERSION 1.0.0)

if (MSVC)
    # 4142 - benign redefinition of type, doesn't matter
    add_compile_options(-wd4142)
endif()

include_directories(deps/cjson
                    deps/curl/include
                    deps/mbedtls/include
                    deps/mongoose
                    deps/tomlc99)

add_compile_definitions(MG_ENABLE_MBEDTLS=1)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING_STATIC OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/cjson)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(GEN_FILES ON CACHE BOOL "" FORCE)
add_subdirectory(deps/mbedtls)
set(MBEDTLS_INCLUDE_DIRS deps/mbedtls/include CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARY mbedtls CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY mbedx509 CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "" FORCE)
set(CURL_USE_LIBSSH2 TRUE CACHE BOOL "" FORCE)
set(CURL_ENABLE_SSL TRUE CACHE BOOL "" FORCE)
set(CURL_USE_MBEDTLS TRUE CACHE BOOL "" FORCE)
set(CURL_ENABLE_EXPORT_TARGET FALSE CACHE BOOL "" FORCE)
add_subdirectory(deps/curl)
add_library(mongoose STATIC deps/mongoose/mongoose.h deps/mongoose/mongoose.c)
add_library(tomlc99 STATIC deps/tomlc99/toml.h deps/tomlc99/toml.c)

set(HEADERS server.h types.h)
set(SOURCES server.c)
set(DATA index.html)
add_executable(AttendanceServer ${HEADERS} ${SOURCES} ${DATA})
target_link_libraries(AttendanceServer PRIVATE cjson libcurl mbedtls mbedcrypto mbedx509 mongoose tomlc99)
set_target_properties(AttendanceServer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT AttendanceServer)
