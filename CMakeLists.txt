cmake_minimum_required(VERSION 3.16)
project(charmOS C ASM_NASM)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(ARCH "x86_64")
set(IMAGE_NAME "charmos-${ARCH}")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Build type: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

message(STATUS "charmOS build type: ${CMAKE_BUILD_TYPE}")

string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)
if(_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    add_compile_definitions(BUILD_DEBUG)
elseif(_BUILD_TYPE_UPPER STREQUAL "RELEASE" OR _BUILD_TYPE_UPPER STREQUAL "MINSIZEREL")
    add_compile_definitions(BUILD_RELEASE)
elseif(_BUILD_TYPE_UPPER STREQUAL "RELWITHDEBINFO")
    add_compile_definitions(BUILD_RELEASE)
endif()

set(PROFILING_FLAGS
    PROFILING_SCHED
    PROFILING_VFS
)

set(TEST_FLAGS
    TEST_SCHED TEST_TMPFS TEST_EXT2 TEST_MEM TEST_MINHEAP
    TEST_RCU TEST_RWLOCK TEST_MUTEX TEST_TIMER_DEFER TEST_FS
    TEST_APC TEST_BIO TEST_BIO_SCHED TEST_LOG TEST_MISC
)

set(TEST_NIGHTMARE_FLAGS
    TEST_NIGHTMARE_LOCKS
    TEST_NIGHTMARE_WAKE
)

function(define_flag_group GROUP_NAME ENABLE_ALL DEFAULT_ALL FLAGS)
    option(${ENABLE_ALL} "Enable all ${GROUP_NAME} flags" ${DEFAULT_ALL})
    foreach(flag ${FLAGS})
        option(${flag} "Enable ${flag}" OFF)
    endforeach()

    if(${ENABLE_ALL})
        foreach(flag ${FLAGS})
            set(${flag} ON CACHE BOOL "" FORCE)
        endforeach()
    endif()

    set(_LOCAL_GROUP_ENABLED OFF)
    foreach(flag ${FLAGS})
        if(${flag})
            set(_LOCAL_GROUP_ENABLED ON)
            add_compile_definitions(${flag})
        endif()
    endforeach()

    set(${GROUP_NAME}_ENABLED ${_LOCAL_GROUP_ENABLED} PARENT_SCOPE)

    if(${ENABLE_ALL})
        add_compile_definitions(${ENABLE_ALL})
    endif()
    if(_LOCAL_GROUP_ENABLED)
        add_compile_definitions(${GROUP_NAME}_ENABLED)
    endif()
endfunction()

define_flag_group(PROFILING       PROFILING_ALL       OFF "${PROFILING_FLAGS}")
define_flag_group(TEST            TEST_ALL            ON  "${TEST_FLAGS}")
define_flag_group(TEST_NIGHTMARE  TEST_NIGHTMARE_ALL  ON  "${TEST_NIGHTMARE_FLAGS}")

option(QEMU_KVM         "Enable KVM acceleration"                         OFF)
option(QEMU_IOMMU       "Attach intel-iommu device with intremap"         ON)
option(QEMU_NUMA        "Configure 4-node NUMA topology"                  ON)
option(QEMU_USB         "Attach xHCI controller with USB kbd/mouse"       ON)
option(QEMU_NVME        "Attach NVMe drive backed by disk.img"            ON)
option(QEMU_DEBUG_EXIT  "Attach isa-debug-exit for test harness"          ON)
option(QEMU_GDB_WAIT    "Halt at startup waiting for gdb (-S)"            OFF)
option(QEMU_TRACE_VTD   "Trace VT-d events into trace.log"                OFF)
option(QEMU_LOAD_ACPI   "Load custom ACPI tables from build/acpi/"        OFF)

set(QEMU_MEM_SIZE  "8G"     CACHE STRING "QEMU guest memory size")
set(QEMU_SMP_TOPO  "sockets=2,cores=2,threads=2"
    CACHE STRING "QEMU -smp topology string")

set(KERNEL_WARNINGS
    -Wall -Wextra
    -Wno-unused-function -Wno-unused-parameter
    -Wpointer-sign -Wenum-compare
    -Wno-address-of-packed-member
)

set(KERNEL_FREESTANDING
    -ffreestanding
    -fno-stack-protector -fno-stack-check
    -fno-PIC -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -ffunction-sections -fdata-sections
)

set(KERNEL_TARGET_ARCH
    -m64 -mcmodel=kernel
    -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
    -mno-red-zone
    -mgeneral-regs-only
)

set(KERNEL_DEFINES
    -DLIMINE_API_REVISION=2
    -DUACPI_DEFAULT_LOG_LEVEL=4
)

set(CMAKE_C_FLAGS_DEBUG          "-O0 -ggdb -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE        "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -ggdb -g3 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")

option(KERNEL_STACK_USAGE "Emit .su files with -fstack-usage" ON)
if(KERNEL_STACK_USAGE)
    list(APPEND KERNEL_FREESTANDING -fstack-usage)
endif()

set(KERNEL_C_FLAGS_LIST
    ${KERNEL_WARNINGS}
    ${KERNEL_FREESTANDING}
    ${KERNEL_TARGET_ARCH}
    ${KERNEL_DEFINES}
    -pipe
)

string(REPLACE ";" " " KERNEL_C_FLAGS_STR "${KERNEL_C_FLAGS_LIST}")
set(CMAKE_C_FLAGS "${KERNEL_C_FLAGS_STR} -std=gnu11")

if(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target x86_64-unknown-unknown")
    set(CMAKE_LINKER "x86_64-elf-ld")
    set(CMAKE_EXE_LINKER_FLAGS
        "-nostdlib -static -Wl,--gc-sections -z max-page-size=0x1000 -T ${CMAKE_SOURCE_DIR}/kernel/linker-${ARCH}.ld")
else()
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id=none -nostdlib -static -z max-page-size=0x1000 -Wl,--gc-sections -T ../../kernel/linker-${ARCH}.ld")
endif()

enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_FLAGS "-F dwarf -g -Wall -f elf64 -wno-reloc-rel-dword")

add_subdirectory(kernel)

target_include_directories(kernel PRIVATE include)

set(SYMS_OUTPUT  "${CMAKE_SOURCE_DIR}/kernel/syms/fullsyms.c")
set(SYMS_AWK     "${CMAKE_SOURCE_DIR}/scripts/syms.awk")
set(SYMS_STAMP   "${CMAKE_BINARY_DIR}/syms.stamp")

add_custom_command(
    OUTPUT ${SYMS_STAMP}
    COMMAND nm $<TARGET_FILE:kernel> | awk -f ${SYMS_AWK} > ${SYMS_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E touch ${SYMS_STAMP}
    DEPENDS kernel ${SYMS_AWK}
    COMMENT "Regenerating symbol table from built kernel"
    VERBATIM
)

add_custom_target(regen-syms DEPENDS ${SYMS_STAMP})

add_custom_target(iso
    DEPENDS kernel regen-syms
    COMMAND ${CMAKE_COMMAND} -E rm -rf iso_root
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/boot
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> iso_root/boot/
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/boot/limine
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/kernel/limine.conf iso_root/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/EFI/BOOT
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/limine/limine-bios.sys
        ${CMAKE_SOURCE_DIR}/limine/limine-bios-cd.bin
        ${CMAKE_SOURCE_DIR}/limine/limine-uefi-cd.bin
        iso_root/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine/BOOTX64.EFI iso_root/EFI/BOOT/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine/BOOTIA32.EFI iso_root/EFI/BOOT/
    COMMAND xorriso -as mkisofs -R -r -J
        -b boot/limine/limine-bios-cd.bin
        -no-emul-boot -boot-load-size 4 -boot-info-table
        -hfsplus -apm-block-size 2048
        --efi-boot boot/limine/limine-uefi-cd.bin
        -efi-boot-part --efi-boot-image --protective-msdos-label
        iso_root -o ${IMAGE_NAME}.iso > /dev/null 2>&1
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/limine
    COMMAND ${CMAKE_SOURCE_DIR}/limine/limine bios-install ${IMAGE_NAME}.iso > /dev/null 2>&1
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building bootable ISO: ${IMAGE_NAME}.iso"
)

set(QEMU_FLAGS
    -cdrom ${IMAGE_NAME}.iso
    -boot d
    -m ${QEMU_MEM_SIZE}
    -smp ${QEMU_SMP_TOPO}
    -M q35
    -qmp unix:/tmp/qmp.sock,server,nowait
    -s
    -monitor telnet:127.0.0.1:5555,server,nowait
)

if(QEMU_KVM)
    list(APPEND QEMU_FLAGS -enable-kvm -cpu host)
endif()

if(QEMU_GDB_WAIT)
    list(APPEND QEMU_FLAGS -S)
endif()

if(QEMU_NUMA)
    list(APPEND QEMU_FLAGS
        -object memory-backend-ram,size=2G,id=mem0
        -object memory-backend-ram,size=2G,id=mem1
        -object memory-backend-ram,size=2G,id=mem2
        -object memory-backend-ram,size=2G,id=mem3
        -numa node,cpus=0-1,nodeid=0,memdev=mem0
        -numa node,cpus=2-3,nodeid=1,memdev=mem1
        -numa node,cpus=4-5,nodeid=2,memdev=mem2
        -numa node,cpus=6-7,nodeid=3,memdev=mem3
        -numa dist,src=0,dst=1,val=15
        -numa dist,src=1,dst=0,val=15
        -numa dist,src=0,dst=2,val=20
        -numa dist,src=2,dst=0,val=20
        -numa dist,src=0,dst=3,val=30
        -numa dist,src=3,dst=0,val=30
        -numa dist,src=1,dst=2,val=25
        -numa dist,src=2,dst=1,val=25
        -numa dist,src=1,dst=3,val=35
        -numa dist,src=3,dst=1,val=35
        -numa dist,src=2,dst=3,val=15
        -numa dist,src=3,dst=2,val=15
    )
endif()

if(QEMU_USB)
    list(APPEND QEMU_FLAGS
        -device qemu-xhci,id=xhci
        -device usb-kbd,bus=xhci.0,port=1,id=usbkbd
        -device usb-mouse,bus=xhci.0,port=2,id=usbmouse
    )
endif()

if(QEMU_IOMMU)
    list(APPEND QEMU_FLAGS -device intel-iommu,intremap=on)
endif()

if(QEMU_NVME)
    list(APPEND QEMU_FLAGS
        -drive id=nvme0,file=disk.img,format=raw,if=none
        -device nvme,serial=boom,drive=nvme0
    )
endif()

if(QEMU_DEBUG_EXIT)
    list(APPEND QEMU_FLAGS -device isa-debug-exit,iobase=0xf4,iosize=0x04)
endif()

if(QEMU_TRACE_VTD)
    list(APPEND QEMU_FLAGS -d "trace:*vt*" -trace file=trace.log)
endif()

if(QEMU_LOAD_ACPI)
    set(ACPI_TABLES
        apic dmar dsdt ecdt facp facs hpet mcfg sbst
        ssdt1 ssdt2 ssdt3 ssdt5 ssdt6 ssdt7 ssdt8 ssdt9 ssdt10 ssdt11
    )
    foreach(tbl ${ACPI_TABLES})
        list(APPEND QEMU_FLAGS -acpitable file=acpi/${tbl}.dat)
    endforeach()
endif()

set(DISK_PRISTINE "${CMAKE_BINARY_DIR}/d.img")
set(DISK_RUNTIME  "${CMAKE_BINARY_DIR}/disk.img")
set(DISK_SIZE_MB  "8" CACHE STRING "Disk image size in megabytes")

find_program(MKE2FS_BIN
    NAMES mke2fs
    PATHS /opt/homebrew/opt/e2fsprogs/sbin /usr/local/opt/e2fsprogs/sbin
    PATH_SUFFIXES sbin
)
if(NOT MKE2FS_BIN)
    message(WARNING "mke2fs not found - disk image generation will fail. "
                    "Install e2fsprogs (brew install e2fsprogs).")
    set(MKE2FS_BIN "mke2fs") 
endif()

add_custom_command(
    OUTPUT ${DISK_PRISTINE}
    COMMAND dd if=/dev/zero of=${DISK_PRISTINE} bs=1M count=${DISK_SIZE_MB} status=none
    COMMAND ${MKE2FS_BIN} -t ext2 -q ${DISK_PRISTINE}
    COMMENT "Creating pristine ${DISK_SIZE_MB}MB ext2 image"
    VERBATIM
)
add_custom_target(pristine-disk DEPENDS ${DISK_PRISTINE})

function(register_run_target tgt)
    set(extra_args ${ARGN})
    add_custom_target(${tgt}
        DEPENDS iso pristine-disk
        COMMAND ${CMAKE_COMMAND} -E copy ${DISK_PRISTINE} ${DISK_RUNTIME}
        COMMAND qemu-system-${ARCH} ${QEMU_FLAGS} ${extra_args}
                2>&1 | tee ${CMAKE_BINARY_DIR}/output.log
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
    )
endfunction()

register_run_target(run      -serial stdio -no-shutdown -no-reboot)
register_run_target(headless -nographic    -no-shutdown -no-reboot)
register_run_target(tests    -nographic)
register_run_target(debug    -S -serial stdio -no-shutdown -no-reboot)

add_custom_target(clean-full
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        iso_root ${IMAGE_NAME}.iso ${IMAGE_NAME}.hdd
        kernel-deps limine ovmf ${SYMS_STAMP}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${SYMS_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "")
message(STATUS "charmOS configuration:")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Arch         : ${ARCH}")
message(STATUS "  Stack usage  : ${KERNEL_STACK_USAGE}")
message(STATUS "  Tests        : ${TEST_ENABLED} (TEST_ALL=${TEST_ALL})")
message(STATUS "  Profiling    : ${PROFILING_ENABLED} (PROFILING_ALL=${PROFILING_ALL})")
message(STATUS "  QEMU KVM     : ${QEMU_KVM}")
message(STATUS "  QEMU IOMMU   : ${QEMU_IOMMU}")
message(STATUS "  QEMU NUMA    : ${QEMU_NUMA}")
message(STATUS "  QEMU USB     : ${QEMU_USB}")
message(STATUS "  QEMU NVMe    : ${QEMU_NVME}")
message(STATUS "  QEMU memory  : ${QEMU_MEM_SIZE}")
message(STATUS "  QEMU SMP     : ${QEMU_SMP_TOPO}")
message(STATUS "")
