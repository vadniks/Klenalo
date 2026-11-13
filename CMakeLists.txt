
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
    c m pthread atomic SDL3 SDL3_net sodium # gtest or cmocka, ini-files-reader-writer, compression
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
#    -Wcast-qual
    -Wcast-align
    -Wshadow
    -Wstrict-aliasing=1
    -Wswitch-enum
    -Wundef
    -Wpointer-arith
    -Wformat-security
    -Winit-self
    -Wfloat-equal
    -Wwrite-strings
    -Wredundant-decls
    -Wstrict-overflow=2
    # Optimization and Architecture Flags
#    -s
    -O3
    -mabi=sysv
    -m64
    -march=x86-64
    -mno-red-zone
    -mpopcnt
    -mfma
    -mavx2
    -mbmi2
    -mrdrnd
    -msha
    -maes
    -ffast-math
    -fsemantic-interposition
#    -fno-omit-frame-pointer
    # Debugging, LTO, and Stack Protection
    -g3
    -fstack-protector-all
    -fstack-check
    -fstack-clash-protection
    -fcf-protection=full
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
    -fsanitize=shift
    -fsanitize=shift-exponent
    -fsanitize=shift-base
    -fsanitize=integer-divide-by-zero
    -fsanitize=float-divide-by-zero
    -fsanitize=float-cast-overflow
    -fsanitize=builtin
    #
    -fstrict-aliasing
    #
#    -fno-optimize-sibling-calls
    -fsanitize=address # ASAN_OPTIONS="abort_on_error=1:detect_leaks=1";LSAN_OPTIONS="print_suppressions=0:report_objects=1:use_globals=0:use_tls=0"
    -fsanitize-address-use-after-return=always # #include <sanitizer/lsan_interface.h>
    -fsanitize-address-use-after-scope
    -fsanitize=pointer-compare
    -fsanitize=pointer-subtract
    #
    -fsanitize=leak
#    -fsanitize=safe-stack
#    -fsanitize=type
#    -fsanitize=memory
#    -finstrument-functions
#    -fsanitize=thread
#    -fsanitize=concurrency
    #
#    -fsanitize-minimal-runtime
#    -fno-sanitize-merge #
    # Custom visibility and PIE/Hardening flags
    -fvisibility=hidden
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

set(EXE_COMPILER_FLAGS
    -fPIE
    -fpie
    -fsanitize=unsigned-integer-overflow
)

add_compile_definitions(
    "-D_GNU_SOURCE=
    -D_THREAD_SAFE=
    -U_FORTIFY_SOURCE=
    -D_FORTIFY_SOURCE=3"
)

include_directories(
    libx
)

set(EXE_LINKER_FLAGS
    -Wl,--wrap=FT_New_Face
    -pie -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,nodlopen
    -Wl,-z,noexecstack
    -Wl,--as-needed
    -Wl,--no-copy-dt-needed-entries
#    -Wl,-rpath,'$ORIGIN/libs'
)
add_link_options(-flto=full -fuse-ld=ld.lld -fsanitize=undefined -fsanitize=address)

set(LIB_THIRD_PARTY thirdParty)
file(GLOB_RECURSE THIRD_PARTY_SOURCES CONFIGURE_DEPENDS thirdParty/*)
add_library(${LIB_THIRD_PARTY} ${THIRD_PARTY_SOURCES})
target_compile_options(${LIB_THIRD_PARTY} PRIVATE -fPIC -fpic)
target_link_options(${LIB_THIRD_PARTY} PRIVATE -pic)

file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS server/* common/*)
set(SERVER_EXE KlenaloServer)
add_executable(${SERVER_EXE} ${SERVER_SOURCES})
target_link_libraries(${SERVER_EXE} ${LIB_THIRD_PARTY})
target_compile_options(${SERVER_EXE} PRIVATE ${EXE_COMPILER_FLAGS})
target_link_options(${SERVER_EXE} PRIVATE ${EXE_LINKER_FLAGS})

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS client/* common/*)
set(CLIENT_EXE KlenaloClient)
add_executable(${CLIENT_EXE} ${CLIENT_SOURCES})
target_link_libraries(${CLIENT_EXE} ${LIB_THIRD_PARTY})
target_compile_options(${CLIENT_EXE} PRIVATE ${EXE_COMPILER_FLAGS})
target_link_options(${CLIENT_EXE} PRIVATE ${EXE_LINKER_FLAGS})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ENABLE_TESTS true)
if(ENABLE_TESTS)
    file(GLOB_RECURSE SERVER_TEST_SOURCES CONFIGURE_DEPENDS server/* tests/* common/*)
    list(FILTER SERVER_TEST_SOURCES EXCLUDE REGEX server/main.c)

    set(SERVER_TESTS_EXE serverTests)
    add_executable(${SERVER_TESTS_EXE} ${SERVER_TEST_SOURCES})

    target_link_libraries(${SERVER_TESTS_EXE} ${LIB_THIRD_PARTY})

    enable_testing()
    target_compile_definitions(${SERVER_TESTS_EXE} PRIVATE TESTING)
    target_compile_options(${SERVER_TESTS_EXE} PRIVATE ${EXE_COMPILER_FLAGS})
    target_link_options(${SERVER_EXE} PRIVATE ${EXE_LINKER_FLAGS})

    add_test(NAME serverTests COMMAND $<TARGET_FILE:${SERVER_TESTS_EXE}>)
endif()
