#[[

Emulation targets

Machines are declared in scripts/machines/, so we don't handle that here
]]

set(MACHINE_PROFILE
    "default"
    CACHE STRING "Machine profile in scripts/machines/")
option(MACHINE_KVM "Enable KVM acceleration" OFF)
option(MACHINE_GDB_WAIT "Halt at startup waiting for gdb (-S)" OFF)
set(MACHINE_MEMORY
    ""
    CACHE STRING "Override the profile's memory, e.g. 4G")
set(MACHINE_SMP
    ""
    CACHE STRING "Override the profile's SMP topology, e.g. sockets=1,cores=2,threads=1")

set(MACHINE_PROFILE_FILE "${CMAKE_SOURCE_DIR}/scripts/machines/${MACHINE_PROFILE}.toml")
if (NOT EXISTS "${MACHINE_PROFILE_FILE}")
    message(FATAL_ERROR "MACHINE_PROFILE=${MACHINE_PROFILE}: no ${MACHINE_PROFILE_FILE}")
endif ()
set_property(
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS "${MACHINE_PROFILE_FILE}")

set(MACHINE_ARGS_DIR "${CMAKE_BINARY_DIR}/machine")
set(NDJSON_LOG "${CMAKE_BINARY_DIR}/ndjson.log")
set(QMP_SOCKET "${CMAKE_BINARY_DIR}/qmp.sock")

set(MACHINE_RENDER_ARGS "")
if (MACHINE_KVM)
    list(APPEND MACHINE_RENDER_ARGS --kvm)
endif ()
if (MACHINE_GDB_WAIT)
    list(APPEND MACHINE_RENDER_ARGS --gdb-wait)
endif ()
if (NOT MACHINE_MEMORY STREQUAL "")
    list(APPEND MACHINE_RENDER_ARGS --memory ${MACHINE_MEMORY})
endif ()
if (NOT MACHINE_SMP STREQUAL "")
    list(APPEND MACHINE_RENDER_ARGS --smp ${MACHINE_SMP})
endif ()

# Paths are relative because every run target runs in CMAKE_BINARY_DIR
foreach (_mode run headless tests debug tests-debug)
    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_SOURCE_DIR}/scripts ${Python3_EXECUTABLE} -m charm machine
            render --profile ${MACHINE_PROFILE} --mode ${_mode} --iso ${IMAGE_NAME}.iso --disk disk.img --qmp-socket
            ${QMP_SOCKET} --machine-log ${NDJSON_LOG} --trace-log trace.log --acpi-dir ${CMAKE_BINARY_DIR}/acpi
            --check-version --out ${MACHINE_ARGS_DIR}/${_mode}.args ${MACHINE_RENDER_ARGS}
        RESULT_VARIABLE _machine_rc
        ERROR_VARIABLE _machine_err)
    if (NOT _machine_rc EQUAL 0)
        string(STRIP "${_machine_err}" _machine_err)
        message(FATAL_ERROR "machine profile ${MACHINE_PROFILE}, mode ${_mode}:\n  ${_machine_err}")
    endif ()
endforeach ()

add_custom_target(
    iso
    DEPENDS kernel
    COMMAND ${CMAKE_COMMAND} -E rm -rf iso_root
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/boot
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> iso_root/boot/
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/boot/limine
    COMMAND
        ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/gen_limine_conf.py ${CMAKE_SOURCE_DIR}/kernel/limine.conf
        ${CMAKE_BINARY_DIR}/iso_root/boot/limine/limine.conf
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/EFI/BOOT
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine/limine-bios.sys
        ${CMAKE_SOURCE_DIR}/limine/limine-bios-cd.bin ${CMAKE_SOURCE_DIR}/limine/limine-uefi-cd.bin
        iso_root/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine/BOOTX64.EFI iso_root/EFI/BOOT/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine/BOOTIA32.EFI iso_root/EFI/BOOT/
    COMMAND
        xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table
        -hfsplus -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image
        --protective-msdos-label iso_root -o ${IMAGE_NAME}.iso > /dev/null 2>&1
    COMMAND make -C ${CMAKE_SOURCE_DIR}/limine
    COMMAND ${CMAKE_SOURCE_DIR}/limine/limine bios-install ${IMAGE_NAME}.iso
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building bootable ISO: ${IMAGE_NAME}.iso")

set(DISK_PRISTINE "${CMAKE_BINARY_DIR}/d.img")
set(DISK_RUNTIME "${CMAKE_BINARY_DIR}/disk.img")
set(DISK_SIZE_MB
    "8"
    CACHE STRING "Disk image size in megabytes")

find_program(
    MKE2FS_BIN
    NAMES mke2fs
    PATHS /opt/homebrew/opt/e2fsprogs/sbin /usr/local/opt/e2fsprogs/sbin
    PATH_SUFFIXES sbin)
if (NOT MKE2FS_BIN)
    message(WARNING "mke2fs not found - disk image generation will fail. "
                    "Install e2fsprogs (brew install e2fsprogs).")
    set(MKE2FS_BIN "mke2fs")
endif ()

add_custom_command(
    OUTPUT ${DISK_PRISTINE}
    COMMAND dd if=/dev/zero of=${DISK_PRISTINE} bs=1M count=${DISK_SIZE_MB} status=none
    COMMAND ${MKE2FS_BIN} -t ext2 -q ${DISK_PRISTINE}
    COMMENT "Creating pristine ${DISK_SIZE_MB}MB ext2 image"
    VERBATIM)
add_custom_target(pristine-disk DEPENDS ${DISK_PRISTINE})

# map_exit: run_qemu.sh translates isa-debug-exit codes, for the modes that carry one
function (register_run_target tgt mode map_exit)
    add_custom_target(
        ${tgt}
        DEPENDS iso pristine-disk
        COMMAND ${CMAKE_COMMAND} -E copy ${DISK_PRISTINE} ${DISK_RUNTIME}
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run_qemu.sh ${CMAKE_BINARY_DIR}/output.log ${map_exit}
                ${MACHINE_ARGS_DIR}/${mode}.args
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL)
endfunction ()

register_run_target(run run 0)
register_run_target(headless headless 0)
register_run_target(tests tests 1)
register_run_target(debug debug 0)
register_run_target(tests-debug tests-debug 0)

function (machine_report_configuration)
    message(STATUS "  Machine      : ${MACHINE_PROFILE} (${MACHINE_ARGS_DIR}/<mode>.args)")
    message(STATUS "  QMP socket   : ${QMP_SOCKET}")
endfunction ()
