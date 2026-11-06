
cmake_minimum_required(VERSION 4.3)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_LINKER_TYPE LLD)

project(Klenalo LANGUAGES C VERSION 0.1)

set(CMAKE_C_STANDARD 23) # gnu2y | 2Y | 26 - not yet supported
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(SEND_ERROR "Extensions are used and only Clang is supported.")
endif()

link_libraries(
    c m pthread atomic SDL3
    ${PROJECT_SOURCE_DIR}/lib/lvgl/bin/liblvgl.so.9
    freetype sodium
    ${PROJECT_SOURCE_DIR}/lib/sdl3net/bin/libSDL3_net.so.0
    lz4
)

add_compile_options(
    # Standard Compiler Warnings & Strictness
    -Wall
    -Wextra
    -Wpedantic
    -Wabi
    -Wpsabi
    -Wformat
    -Wformat=2
    -Wconversion
    -Wimplicit-fallthrough
    -Werror=format-security
    -Wstrict-prototypes
    -Wdeprecated-non-prototype
    -Wold-style-definition
    -Wthread-safety
    -Werror=return-type
    #
    -Wcast-qual
    -Wcast-align
    -Wshadow
    -Wstrict-aliasing=1
    -Wswitch-enum
    -Wdeclaration-after-statement
    -Wundef
    -Wpointer-arith
    -Wformat-security
    -Wvla
    -Winit-self
    -Wfloat-equal
    -Wwrite-strings
    -Wredundant-decls
    -Wstrict-overflow=2
    # Optimization and Architecture Flags
#    -s
    -O3
    -mavx2
    -mbmi2
    -mrdrnd
    -ffast-math
    -fsemantic-interposition
    -fno-omit-frame-pointer
    # Debugging, LTO, and Stack Protection
    -g3
    -fstack-protector-all
    -fstack-check
    -fstack-clash-protection
    -flto=full
    -fno-fat-lto-objects
    # Sanitizers
    -fsanitize=undefined
    -fsanitize-trap=all
    -fsanitize=alignment
    -fsanitize=bounds
    -fsanitize=vla-bound
    -fsanitize=null
    -fsanitize=object-size
    -fsanitize=nonnull-attribute
    -fsanitize=returns-nonnull-attribute
    -fsanitize=signed-integer-overflow
    -fsanitize=float-cast-overflow
    -fsanitize=pointer-overflow
    -fsanitize=enum
    -fsanitize=bool
    # Custom visibility and PIE/Hardening flags
    -fvisibility=hidden
    -fPIE
    -fpie
    # Strict C Language Extensions & Type Safety
    -fno-delete-null-pointer-checks
    -fno-strict-overflow
    -fno-strict-aliasing
    -ftrivial-auto-var-init=zero
    -Werror=implicit
    -Werror=incompatible-pointer-types
    -Werror=int-conversion
    -fstrict-flex-arrays=3
    -fwrapv
    -ftrapv
    # Suppressions and Extensions (Keep these for compatibility with specific libraries/codebases)
    -Wno-nullability-extension
    -Wno-nullability-completeness
    -Wno-language-extension-token
    -Wno-gnu-folding-constant
    -Wno-logical-op-parentheses
    -Wno-gnu-pointer-arith
    -Wno-zero-length-array
    -Wno-gnu-empty-struct
    -Wno-address-of-packed-member
    -Wno-microsoft-flexible-array
    -Wno-flexible-array-extensions
    -Wno-gnu-variable-sized-type-not-at-end
    -Wno-unknown-pragmas
    -Wno-sign-conversion
    # MSVC/Specific extensions for compatibility
    -fms-extensions
    -Wno-microsoft-anon-tag
    -Wno-microsoft-string-literal-from-predefined
    -fcommon
)

# Preprocessor Definitions
add_compile_definitions(
    "-D_GNU_SOURCE=
    -D_THREAD_SAFE=
    -U_FORTIFY_SOURCE=
    -D_FORTIFY_SOURCE=3"
    LV_CONF_PATH="../lv_conf.h"
)

include_directories(
    lib/lvgl/include
    lib/sdl3net/include
    libx
    /usr/include/freetype2
)

set(LINKER_FLAGS
    -Wl,--wrap=FT_New_Face
    -pie -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,nodlopen
    -Wl,-z,noexecstack
    -Wl,--as-needed
    -Wl,--no-copy-dt-needed-entries
#    -Wl,-rpath,'$ORIGIN/libs'
)
add_link_options(-flto=full ${LINKER_FLAGS} -fuse-ld=ld.lld)

add_executable(${PROJECT_NAME}
    src/collections/deque.c
    src/collections/deque.h
    src/collections/hashtable.c
    src/collections/hashtable.h
    src/collections/list.c
    src/collections/list.h
    src/collections/treeMap.c
    src/collections/treeMap.h

    src/core/lifecycle.c
    src/core/lifecycle.h
    src/core/main.c

    src/crypto/crypto.c
    src/crypto/crypto.h
    src/crypto/keyStore.c
    src/crypto/keyStore.h

    src/integration/input.c
    src/integration/input.h
    src/integration/resources.c
    src/integration/resources.h
    src/integration/video.c
    src/integration/video.h
    src/integration/xlvgl.c
    src/integration/xlvgl.h

    src/networking/net.c
    src/networking/net.h

    src/scenes/loginScene.c
    src/scenes/loginScene.h
    src/scenes/scenes.c
    src/scenes/scenes.h
    src/scenes/splashScene.c
    src/scenes/splashScene.h

    src/utils/barrier.c
    src/utils/barrier.h
    src/utils/conditionObserver.c
    src/utils/conditionObserver.h
    src/utils/rwMutex.c
    src/utils/rwMutex.h

    src/consts.c
    src/consts.h
    src/defs.c
    src/defs.h
)

set(LIB_THIRD_PARTY thirdParty)
add_library(${LIB_THIRD_PARTY}
    thirdParty/halfSipHash.c
    thirdParty/sipHash.c
    thirdParty/xxHash.c
)

target_link_libraries(${PROJECT_NAME} ${LIB_THIRD_PARTY})

target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=unsigned-integer-overflow)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ENABLE_TESTS true)
if(ENABLE_TESTS)
    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS src/* tests/*)
    list(FILTER TEST_SOURCES EXCLUDE REGEX src/core/main.c)

    set(TESTS_EXE tests)
    add_executable(${TESTS_EXE} ${TEST_SOURCES})

    target_link_libraries(${TESTS_EXE} ${LIB_THIRD_PARTY})

    enable_testing()
    target_compile_definitions(${TESTS_EXE} PRIVATE TESTING)
    target_compile_options(${TESTS_EXE} PRIVATE -fsanitize=unsigned-integer-overflow)

    foreach(INDEX RANGE 3)
        add_test(NAME test${INDEX} COMMAND $<TARGET_FILE:${TESTS_EXE}> ${INDEX})
    endforeach()
endif()
