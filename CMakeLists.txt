
cmake_minimum_required(VERSION 4.0)
set(CMAKE_C_COMPILER "/usr/bin/clang")
project(Klenalo LANGUAGES C VERSION 0.1)

#add_subdirectory(collections)

set(CMAKE_C_STANDARD 23) # 2Y | 26 - not yet supported
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(SEND_ERROR "Extensions are used")
endif()

add_compile_options(-std=gnu2y)

add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Wpedantic)
add_compile_options(-Wformat -Wformat=2 -Wconversion -Wimplicit-fallthrough -Werror=format-security -Wstrict-prototypes -Wdeprecated-non-prototype -Wold-style-definition -Wthread-safety)
add_compile_options(-O3)
add_compile_options(-ffast-math -fsemantic-interposition)
add_compile_options(-Werror=return-type)
#add_link_options("-s")
add_compile_options(-fvisibility=hidden)

add_compile_options(-g3)
#add_compile_options(-fsanitize=memory)
add_compile_options(-fstack-protector-all)
add_compile_options(-fstack-check)
add_compile_options(-fstack-clash-protection)
add_compile_options(-fsanitize=address) # abort_on_error=1:detect_leaks=0 as SDL has memory leaks
#add_link_options(-fsanitize=address)
#add_compile_options(-fsanitize=memory)
#add_link_options(-fsanitize=memory)
add_compile_options(-fno-omit-frame-pointer)
#add_compile_options(-fsanitize=thread)
#add_link_options(-fsanitize=thread)
add_compile_options(-fsanitize=undefined)
add_compile_options(-fsanitize-address-use-after-scope)
add_compile_options(-fsanitize-trap=all)
add_compile_options(-fcf-protection=full)
add_compile_options(-fsanitize=bounds -fsanitize=null -fsanitize=object-size -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=signed-integer-overflow -fsanitize=float-cast-overflow -fsanitize=pointer-overflow)
#add_compile_options(-fhardened)
add_compile_options(-fPIE)
add_compile_options(-fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing -ftrivial-auto-var-init=zero -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -fstrict-flex-arrays=3 -fwrapv -fno-delete-null-pointer-checks)
add_compile_options(-ftrapv)
add_link_options(-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,nodlopen -Wl,-z,noexecstack -Wl,--as-needed -Wl,--no-copy-dt-needed-entries) #add_link_options(-pie -z relro)
# linker (man ld) -rpath=dir # gdb info symbol <addr>

# -finstrument-functions-once for tests
#add_link_options(-rdynamic)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--wrap=FT_New_Face")

add_compile_options(-Wno-nullability-extension)
add_compile_options(-Wno-nullability-completeness)
add_compile_options(-Wno-language-extension-token)
add_compile_options(-Wno-gnu-folding-constant)
add_compile_options(-Wno-logical-op-parentheses)
add_compile_options(-Wno-gnu-pointer-arith)
add_compile_options(-Wno-zero-length-array)
add_compile_options(-Wno-gnu-empty-struct)
#add_compile_options(-Wno-gnu-flexible-array-initializer)
add_compile_options(-Wno-address-of-packed-member)
add_compile_options(-Wno-microsoft-flexible-array)
add_compile_options(-Wno-flexible-array-extensions)
add_compile_options(-Wno-gnu-variable-sized-type-not-at-end)
add_compile_options(-Wno-unknown-pragmas)
add_compile_options(-Wno-sign-conversion)

# -fcond-mismatch

add_compile_options(-fblocks)
add_compile_options(-fms-extensions -Wno-microsoft-anon-tag)
add_compile_options(-fcommon)

add_compile_definitions("-D_GNU_SOURCE -D_THREAD_SAFE -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3")

#file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS src/*.c src/*.h src/*/*.c src/*/*.h)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/*.c src/*.h)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

add_compile_definitions(-DLV_CONF_PATH="../lv_conf.h")
include_directories(lib/lvgl/include)
include_directories(lib/sdl3net/include)
include_directories(/usr/include/freetype2)

add_compile_options(-flto)
add_link_options(-flto)
add_link_options(-fuse-ld=lld)

add_link_options(-static-libasan)

target_link_libraries(${PROJECT_NAME}
#    Collections
    asan
    c
    m
    atomic
    SDL3
    ${PROJECT_SOURCE_DIR}/lib/lvgl/bin/liblvgl.so
    freetype
    sodium
    ${PROJECT_SOURCE_DIR}/lib/sdl3net/bin/libSDL3_net.so # SDL3_net TODO
    BlocksRuntime
    # sqlite3 # TODO
    # sqlcipher
    # dbus
    # xml
    # qrcode, barcode, aztec-code
    # svg
    # googleTest (gtest)
    # png
    # jpeg-turbo
    # thorvg
     lz4
    # ffmpeg
)

#file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

#add_compile_definitions(TESTING)

# mkdir build && (cd build; cmake .. && cd .. && cmake --build build -j 18)
# objdump -x Klenalo | grep NEEDED

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initial clangd flags for clion (remove) -Wno-unused-variable,-Wno-infinite-recursion,-Wno-array-bounds,-Wno-return-stack-address,-Werror=implicit-function-declaration,-Wshadow,-Wno-shadow-field-in-constructor-modified,-Wno-shadow-ivar,-Wuninitialized,-Wunused-label,-Wunused-lambda-capture
