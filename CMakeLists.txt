
cmake_minimum_required(VERSION 3.28)
set(CMAKE_C_COMPILER "/usr/bin/clang")
project(Klenalo LANGUAGES C)

set(CMAKE_C_STANDARD 23)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(SEND_ERROR "GNU extensions are used")
endif()

add_compile_options("-Wall")
add_compile_options("-Wextra")
add_compile_options("-Wpedantic")
add_compile_options("-O3")
#add_link_options("-s")

add_compile_definitions("-D_GNU_SOURCE -D_THREAD_SAFE")

file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS src/*.c src/*.h src/*/*.c src/*/*.h)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

include_directories(lib/nuklear/include)
target_link_libraries(${PROJECT_NAME} c m SDL2)
