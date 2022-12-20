cmake_minimum_required(VERSION 3.23)

project(AttendanceServer VERSION 1.0.0)

if (MSVC)
    # 4142 - benign redefinition of type, doesn't matter
    add_compile_options(-wd4142)
endif()

include_directories(deps/mongoose)
add_library(mongoose STATIC deps/mongoose/mongoose.c deps/mongoose/mongoose.h)

set(HEADERS server.h types.h)
set(SOURCES server.c)
add_executable(AttendanceServer ${HEADERS} ${SOURCES})
target_link_libraries(AttendanceServer PRIVATE mongoose)
set_target_properties(AttendanceServer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT AttendanceServer)

