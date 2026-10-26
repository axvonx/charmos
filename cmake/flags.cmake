set(_min_GNU 14)
set(_min_Clang 23)
set(_min "${_min_${CMAKE_C_COMPILER_ID}}")
if (_min AND CMAKE_C_COMPILER_VERSION VERSION_LESS _min)
    message(FATAL_ERROR "charmOS needs ${CMAKE_C_COMPILER_ID} >= ${_min}, found ${CMAKE_C_COMPILER_VERSION}")
endif ()

string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)
if (_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    add_compile_definitions(BUILD_DEBUG)
elseif (_BUILD_TYPE_UPPER MATCHES "^(RELEASE|MINSIZEREL|RELWITHDEBINFO)$")
    add_compile_definitions(BUILD_RELEASE)
endif ()

set(KERNEL_WARNINGS
    -Wall
    -Wextra
    -Werror=pointer-sign
    -Werror=incompatible-pointer-types
    -Werror=int-conversion
    -Werror=enum-compare
    -Werror=enum-conversion
    -Werror=shift-count-overflow
    -Werror=shift-count-negative
    -Werror=shift-negative-value
    -Werror=format
    -Werror=format-extra-args
    -Werror=format-security
    -Wno-format-zero-length
    -Werror=return-type
    -Werror=implicit-function-declaration
    -Werror=implicit-int
    -Werror=uninitialized
    -Werror=array-bounds
    -Werror=array-parameter
    -Werror=nonnull
    -Werror=sizeof-pointer-memaccess
    -Werror=sizeof-array-argument
    -Werror=sizeof-pointer-div
    -Werror=cast-function-type
    -Werror=int-to-pointer-cast
    -Werror=pointer-to-int-cast
    -Werror=address-of-packed-member
    -Werror=varargs
    -Werror=infinite-recursion
    # logic slips
    -Werror=misleading-indentation
    -Werror=logical-not-parentheses
    -Werror=bool-operation
    -Werror=int-in-bool-context
    -Werror=implicit-fallthrough
    -Werror=undef
    -Werror=strict-prototypes)

if (DEBUG_ASAN)
    set(KERNEL_FRAME_LIMIT 16384)
else ()
    set(KERNEL_FRAME_LIMIT 4096)
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    list(APPEND KERNEL_WARNINGS -Wframe-larger-than=${KERNEL_FRAME_LIMIT} -Werror=frame-larger-than)
    list(
        APPEND
        KERNEL_WARNINGS
        -Werror=sometimes-uninitialized
        -Werror=array-bounds-pointer-arithmetic
        -Werror=fortify-source
        -Werror=tautological-pointer-compare
        -Werror=null-pointer-arithmetic
        -Werror=null-pointer-subtraction
        -Werror=return-stack-address
        -Werror=dangling
        -Werror=unsequenced
        -Werror=integer-overflow
        -Werror=shift-overflow
        -Werror=division-by-zero
        -Werror=incompatible-function-pointer-types
        -Werror=cast-function-type-strict
        -Werror=incompatible-library-redeclaration
        -Werror=deprecated-non-prototype
        -Werror=compare-distinct-pointer-types
        -Werror=self-assign
        -Werror=pointer-bool-conversion
        -Werror=string-compare
        -Werror=constant-conversion
        -Werror=bitfield-constant-conversion
        -Werror=visibility)
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND KERNEL_WARNINGS -Werror=frame-larger-than=${KERNEL_FRAME_LIMIT})
    # most of these are flow-based and only fire with optimization on
    list(
        APPEND
        KERNEL_WARNINGS
        -Werror=return-mismatch
        -Werror=maybe-uninitialized
        -Werror=stringop-overflow
        -Werror=stringop-overread
        -Werror=stringop-truncation
        -Werror=restrict
        -Werror=vla-parameter
        -Werror=alloc-zero
        -Werror=format-overflow
        -Werror=format-truncation
        -Werror=nonnull-compare
        -Werror=dangling-pointer
        -Werror=return-local-addr
        -Werror=free-nonheap-object
        -Werror=sequence-point
        -Werror=overflow
        -Werror=shift-overflow=2
        -Werror=div-by-zero
        -Werror=sizeof-array-div
        -Werror=zero-length-bounds
        -Werror=aggressive-loop-optimizations
        -Werror=builtin-declaration-mismatch
        -Werror=enum-int-mismatch
        -Werror=duplicated-cond
        -Werror=logical-op)
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    list(APPEND KERNEL_WARNINGS -Wthread-safety -Werror=thread-safety -Watomic-implicit-seq-cst)
    # -beta gives us TSA_ACQUIRED_AFTER/BEFORE ordering. -pointer catches
    # passing &guarded_member out of something unlocked. late-parse gives members' 
    # TSA_GUARDED_BY reference a lock declared later in the struct
    list(APPEND KERNEL_WARNINGS -Wthread-safety-beta -Wthread-safety-pointer -Werror=thread-safety-pointer)
    list(APPEND KERNEL_WARNINGS -fexperimental-late-parse-attributes)
    list(APPEND KERNEL_WARNINGS -Werror=excess-initializers)
    list(APPEND KERNEL_WARNINGS -Wno-nullability-completeness)
    # assume() is an unevaluated assumption without DEBUG_ASSUME, side effects
    # in its condition would silently vanish
    list(APPEND KERNEL_WARNINGS -Werror=assume)
endif ()

# for double free / use-after-free / wrong-free comptime detection
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND KERNEL_WARNINGS -Werror=use-after-free -Werror=mismatched-dealloc)
endif ()

option(CHARMOS_GCC_ANALYZER "Run GCC -fanalyzer on every compile (slow)" OFF)
if (CHARMOS_GCC_ANALYZER AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND KERNEL_WARNINGS -fanalyzer)
    list(
        APPEND
        KERNEL_WARNINGS
        -Werror=analyzer-double-free
        -Werror=analyzer-use-after-free
        -Werror=analyzer-free-of-non-heap
        -Werror=analyzer-use-of-pointer-in-stale-stack-frame
        -Werror=analyzer-stale-setjmp-buffer
        -Werror=analyzer-deref-before-check
        -Werror=analyzer-possible-null-dereference
        -Werror=analyzer-null-argument
        -Werror=analyzer-possible-null-argument
        -Werror=analyzer-jump-through-null
        -Werror=analyzer-overlapping-buffers
        -Werror=analyzer-exposure-through-uninit-copy
        -Werror=analyzer-infinite-recursion
        -Werror=analyzer-shift-count-negative
        -Werror=analyzer-shift-count-overflow
        -Werror=analyzer-write-to-string-literal
        -Werror=analyzer-va-arg-type-mismatch
        -Werror=analyzer-va-list-exhausted
        -Werror=analyzer-va-list-leak
        -Werror=analyzer-va-list-use-after-va-end)
    list(
        APPEND
        KERNEL_WARNINGS
        -Wno-analyzer-malloc-leak                    # 47 / 42
        -Wno-analyzer-null-dereference               # 22 / 24
        -Wno-analyzer-undefined-behavior-ptrdiff     #  - / 8
        -Wno-analyzer-out-of-bounds                  #  1 / 4
        -Wno-analyzer-write-to-const                 #  2 / 2
        -Wno-analyzer-use-of-uninitialized-value     #  1 / 2
        -Wno-analyzer-infinite-loop                  #  2 / 2
        -Wno-analyzer-allocation-size                #  2 / 2
        -Wno-analyzer-mismatching-deallocation)      #  1 / 1
endif ()

set(KERNEL_WARNINGS_RELEASE -Wunused -Wvla)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND KERNEL_WARNINGS_RELEASE -Wshadow=local -Wlogical-op -Wduplicated-cond -Wduplicated-branches)
endif ()

string(REPLACE ";" " " KERNEL_WARNINGS_RELEASE_STR "${KERNEL_WARNINGS_RELEASE}")

set(KERNEL_WARNINGS_DEBUG "")

string(REPLACE ";" " " KERNEL_WARNINGS_DEBUG_STR "${KERNEL_WARNINGS_DEBUG}")

set(KERNEL_FREESTANDING
    -ffreestanding
    -fno-strict-aliasing
    -fstack-protector-strong
    -mstack-protector-guard=global
    -fno-stack-check
    -fno-PIC
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -ffunction-sections
    -fdata-sections
    -fstrict-flex-arrays=3)

set(KERNEL_TARGET_ARCH
    -m64
    -mcmodel=kernel
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-sse3
    -mno-3dnow
    -mno-red-zone
    -mgeneral-regs-only
    -mcx16)

set(KERNEL_DEFINES -DLIMINE_API_REVISION=2 -DUACPI_DEFAULT_LOG_LEVEL=4)

set(KERNEL_RELFILE_OK FALSE)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 8)
    set(KERNEL_RELFILE_OK TRUE)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 10)
    set(KERNEL_RELFILE_OK TRUE)
endif ()

if (KERNEL_RELFILE_OK)
    list(APPEND KERNEL_DEFINES -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)
else ()
    message(WARNING "charmOS: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} "
                    "lacks -fmacro-prefix-map; __RELFILE__ will hold absolute paths")
endif ()

list(APPEND KERNEL_DEFINES -D__RELFILE__=__FILE__)

set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -g3 ${KERNEL_WARNINGS_DEBUG_STR}")
set(CMAKE_C_FLAGS_RELEASE "-O3 ${KERNEL_WARNINGS_RELEASE_STR}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -ggdb -g3")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")

option(KERNEL_STACK_USAGE "Emit .su files with -fstack-usage" ON)
if (KERNEL_STACK_USAGE)
    list(APPEND KERNEL_FREESTANDING -fstack-usage)
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND KERNEL_FREESTANDING -malign-data=abi)
endif ()

set(KERNEL_C_FLAGS_LIST ${KERNEL_WARNINGS} ${KERNEL_FREESTANDING} ${KERNEL_TARGET_ARCH} ${KERNEL_DEFINES} -pipe)

string(REPLACE ";" " " KERNEL_C_FLAGS_STR "${KERNEL_C_FLAGS_LIST}")
set(CMAKE_C_FLAGS "${KERNEL_C_FLAGS_STR}")

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_ASM_NASM_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(ENV{CCACHE_COLOR} "always")
endif ()
set(CMAKE_COLOR_DIAGNOSTICS ON CACHE BOOL "colored diagnostics")

option(CHARMOS_CLANG_TIDY "Run clang-tidy on every compile (incremental)" OFF)
if (CHARMOS_CLANG_TIDY AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    get_filename_component(_clang_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    string(REGEX MATCH "^[0-9]+" _clang_major "${CMAKE_C_COMPILER_VERSION}")
    find_program(
        CLANG_TIDY_PROGRAM
        NAMES clang-tidy clang-tidy-${_clang_major}
        HINTS ${_clang_dir})
    if (NOT CLANG_TIDY_PROGRAM)
        message(FATAL_ERROR "CHARMOS_CLANG_TIDY is ON but clang-tidy was not found")
    endif ()
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_PROGRAM}" --quiet)
    message(STATUS "clang-tidy    : ${CLANG_TIDY_PROGRAM}")
endif ()

if (APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target x86_64-unknown-unknown")
    set(CMAKE_LINKER "x86_64-elf-ld")
    set(CMAKE_EXE_LINKER_FLAGS
        "-nostdlib -static -Wl,--gc-sections -z max-page-size=0x1000 -T ${CMAKE_SOURCE_DIR}/kernel/linker-${ARCH}.ld")
else ()
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id=none -nostdlib -static -z max-page-size=0x1000 -Wl,--gc-sections -T ${CMAKE_SOURCE_DIR}/kernel/linker-${ARCH}.ld"
    )
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    find_program(LLD_LINKER ld.lld)
    if (LLD_LINKER)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    else ()
        find_program(ELF_LD x86_64-elf-ld)
        if (NOT ELF_LD)
            message(FATAL_ERROR "clang build needs an ELF linker: install lld (brew install lld) " "or x86_64-elf-ld")
        endif ()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=${ELF_LD}")
    endif ()
endif ()

# Exclude C compiler flags from the link rule
# to avoid passing compiler-only options to the linker driver
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# DEBUG_ASAN itself is declared in features.cmake (included before this file) so the implication pass there can see it;
# only its validation lives here.
if (DEBUG_ASAN)
    if (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "DEBUG_ASAN (KASAN) requires the clang toolchain. Reconfigure with:\n"
                            "    scripts/build.sh --compiler clang -- -DDEBUG_ASAN=ON")
    endif ()
    # KASAN with -O2 should work
    option(ASAN_ALLOW_O0 "Permit a KASAN build at -O0" OFF)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT ASAN_ALLOW_O0)
        message(
            FATAL_ERROR
                "DEBUG_ASAN at -O0 (CMAKE_BUILD_TYPE=Debug) is punishingly slow.\n" "Reconfigure with:\n"
                "    scripts/build.sh -t RelWithDebInfo --compiler clang -- -DDEBUG_ASAN=ON\n"
                "or pass -DASAN_ALLOW_O0=ON to build at -O0 anyway.")
    endif ()

    message(STATUS "charmOS: KASAN enabled (DEBUG_ASAN, outline instrumentation)")
    add_compile_definitions(DEBUG_ASAN)
    set(KERNEL_KASAN_FLAGS
        -fsanitize=kernel-address
        -mllvm
        -asan-instrumentation-with-call-threshold=0
        -mllvm
        -asan-globals=0
        -mllvm
        -asan-stack=0
        -fsanitize-recover=kernel-address
        -mllvm
        -asan-mapping-scale=3
        -mllvm
        -asan-mapping-offset=0xDFFFFE0000000000)
    string(REPLACE ";" " " KERNEL_KASAN_FLAGS_STR "${KERNEL_KASAN_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${KERNEL_KASAN_FLAGS_STR}")
endif ()

if (DEBUG_UBSAN)
    message(STATUS "charmOS: UBSan enabled (DEBUG_UBSAN)")
    add_compile_definitions(DEBUG_UBSAN)
    set(DEBUG_UBSAN_CHECKS
        "bounds;object-size;shift;signed-integer-overflow;integer-divide-by-zero;pointer-overflow;vla-bound;bool;enum;builtin;return;nonnull-attribute;returns-nonnull-attribute"
        CACHE STRING "UBSan checks for DEBUG_UBSAN, as a ;-list")
    string(REPLACE ";" "," KERNEL_UBSAN_CHECKS_STR "${DEBUG_UBSAN_CHECKS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${KERNEL_UBSAN_CHECKS_STR}")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-overflow")
    endif ()
endif ()

option(KERNEL_RANDCONFIG "Append a random subset of codegen flags (build fuzz)" OFF)
set(KERNEL_RANDCONFIG_SEED
    "0"
    CACHE STRING "Seed for KERNEL_RANDCONFIG selection")
if (KERNEL_RANDCONFIG)
    set(KERNEL_RANDCONFIG_POOL
        -funroll-loops
        -fno-strict-aliasing
        -fwrapv
        -fno-jump-tables
        -fno-common
        -fmerge-all-constants
        -fno-asynchronous-unwind-tables
        -fno-strict-overflow
        -finline-functions
        -fno-inline-functions)

    string(
        RANDOM
        LENGTH 1
        ALPHABET "01"
        RANDOM_SEED "${KERNEL_RANDCONFIG_SEED}" _rc_ignore)
    set(KERNEL_RANDCONFIG_CHOSEN "")
    foreach (_rc_flag ${KERNEL_RANDCONFIG_POOL})
        string(
            RANDOM
            LENGTH 1
            ALPHABET "01" _rc_bit)
        if (_rc_bit STREQUAL "1")
            list(APPEND KERNEL_RANDCONFIG_CHOSEN "${_rc_flag}")
        endif ()
    endforeach ()
    if (KERNEL_RANDCONFIG_CHOSEN)
        string(REPLACE ";" " " _rc_str "${KERNEL_RANDCONFIG_CHOSEN}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_rc_str}")
    endif ()
    message(STATUS "charmOS: RANDCONFIG seed=${KERNEL_RANDCONFIG_SEED} " "flags=[${KERNEL_RANDCONFIG_CHOSEN}]")
endif ()

enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_FLAGS "-F dwarf -g -Wall -f elf64 -Wno-reloc-rel-dword")
