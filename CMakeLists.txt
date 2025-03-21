
cmake_minimum_required(VERSION 3.28)
set(CMAKE_C_COMPILER "/usr/bin/clang")
project(Klenalo LANGUAGES C)

set(CMAKE_C_STANDARD 23)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(SEND_ERROR "Extensions are used")
endif()

add_compile_options("-Wall")
add_compile_options("-Wextra")
add_compile_options("-Wpedantic")
add_compile_options("-O3")
add_compile_options(-ffast-math -fsemantic-interposition)
#add_link_options("-s")

add_compile_options(-g)
#add_compile_options(-fsanitize=memory)
add_compile_options(-fstack-protector-all)
add_compile_options(-fstack-check)
add_compile_options(-fstack-clash-protection)
add_compile_options(-fsanitize=address) # abort_on_error=1:detect_leaks=0 as SDL has memory leaks
add_link_options(-fsanitize=address)
#add_compile_options(-fsanitize=memory)
#add_link_options(-fsanitize=memory)
add_compile_options(-fno-omit-frame-pointer)
#add_compile_options(-fsanitize=thread)
#add_link_options(-fsanitize=thread)
add_compile_options(-fsanitize=leak)
add_compile_options(-fsanitize=undefined)
add_compile_options(-fsanitize-address-use-after-scope)
add_compile_options(-fsanitize-trap=all)
add_compile_options(-fcf-protection=full)
add_compile_options(-fsanitize=bounds -fsanitize=null -fsanitize=object-size -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=signed-integer-overflow)
#add_compile_options(-fhardened)
add_compile_options(-fPIE)
add_link_options(-pie -Wl,-z,relro,-z,now) #add_link_options(-pie -z relro)
# linker (man ld) -rpath=dir # gdb info symbol <addr>

# -finstrument-functions-once for tests

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--wrap=lv_malloc_core,--wrap=lv_realloc_core,--wrap=lv_free_core,--wrap=FT_New_Face")

add_compile_options(-Wno-language-extension-token)
add_compile_options(-Wno-gnu-folding-constant)
add_compile_options(-Wno-logical-op-parentheses)
add_compile_options(-Wno-gnu-pointer-arith)
add_compile_options(-Wno-zero-length-array)
add_compile_options(-Wno-gnu-empty-struct)
add_compile_options(-fblocks)
add_compile_options(-fms-extensions -Wno-microsoft-anon-tag)

add_compile_definitions("-D_GNU_SOURCE -D_THREAD_SAFE -D_FORTIFY_SOURCE=3")

file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS src/*.c src/*.h src/*/*.c src/*/*.h)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

include_directories(lib/lvgl/include)
include_directories(lib/sdl3net/include)
include_directories(/usr/include/freetype2)
target_link_libraries(${PROJECT_NAME} c m SDL3 ${PROJECT_SOURCE_DIR}/lib/lvgl/bin/liblvgl.a freetype sodium ${PROJECT_SOURCE_DIR}/lib/sdl3net/bin/libSDL3_net.so BlocksRuntime) # SDL3_net) TODO

#file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

#add_compile_definitions(TESTING)

# mkdir build && (cd build; cmake .. && cd .. && cmake --build build -j 18)
# objdump -x Klenalo | grep NEEDED
