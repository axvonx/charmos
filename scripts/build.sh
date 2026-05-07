#!/usr/bin/env bash
set -euo pipefail

if [[ -t 1 ]] && [[ "$(tput colors 2>/dev/null || echo 0)" -ge 8 ]]; then
    GREEN=$'\033[0;32m'
    YELLOW=$'\033[0;33m'
    RED=$'\033[0;31m'
    BLUE=$'\033[0;34m'
    BOLD=$'\033[1m'
    NC=$'\033[0m'
else
    GREEN=''; YELLOW=''; RED=''; BLUE=''; BOLD=''; NC=''
fi

QUIET=false
CLEAN=false
COMPDB=false
BUILD_TYPE="Debug"
BUILD_DIR="build"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
CMAKE_EXTRA_ARGS=()
TARGETS=()

print_help() {
    local self="$1"
    cat <<EOF
${BOLD}charmOS build script${NC}

${BOLD}Usage:${NC} ${self} [OPTIONS] [-- CMAKE_ARGS...] [TARGETS...]

${BOLD}Options:${NC}
  -h, --help              Show this help and exit
  -q, -s, --quiet         Suppress command output
  -c, --clean             Remove build directory before building
  -C, --compdb            Generate compile_commands.json in project root
  -t, --type TYPE         Build type: Debug, Release, RelWithDebInfo, MinSizeRel
                          (default: Debug)
  -j, --jobs N            Parallel jobs (default: detected CPU count = ${JOBS})
  -B, --build-dir DIR     Build directory (default: build)
  --                      End of script options; remaining args go to cmake

${BOLD}Targets:${NC} (passed to make)
  iso         Build bootable ISO (default if no targets given)
  run         Build and run in QEMU with graphics
  headless    Build and run in QEMU without graphics
  tests       Build and run test suite
  debug       Build and run QEMU paused for gdb
  regen-syms[118;1:3u  Regenerate symbol table from built kernel
  clean-full  Wipe ISO, limine, ovmf, syms, etc.

${BOLD}Examples:${NC}
  ${self}                                  # Debug build, produce ISO
  ${self} -t Release run                   # Release build, run in QEMU
  ${self} -- -DQEMU_KVM=ON -DQEMU_NUMA=OFF run
  ${self} --clean -t RelWithDebInfo tests
EOF
    exit 0
}

PASSTHROUGH=false
while [[ $# -gt 0 ]]; do
    if $PASSTHROUGH; then
        CMAKE_EXTRA_ARGS+=("$1"); shift; continue
    fi
    case "$1" in
        -h|--help|help)        print_help "$0" ;;
        -q|-s|--quiet)         QUIET=true; shift ;;
        -c|--clean)            CLEAN=true; shift ;;
        -C|--compdb)           COMPDB=true; shift ;;
        -t|--type)             BUILD_TYPE="$2"; shift 2 ;;
        -j|--jobs)             JOBS="$2"; shift 2 ;;
        -B|--build-dir)        BUILD_DIR="$2"; shift 2 ;;
        --)                    PASSTHROUGH=true; shift ;;
        -*)                    echo "${RED}Unknown option: $1${NC}" >&2; exit 1 ;;
        *)                     TARGETS+=("$1"); shift ;;
    esac
done

if [[ ${#TARGETS[@]} -eq 0 ]]; then
    TARGETS=("iso")
fi

case "$BUILD_TYPE" in
    Debug|Release|RelWithDebInfo|MinSizeRel) ;;
    *) echo "${RED}Invalid build type: $BUILD_TYPE${NC}" >&2; exit 1 ;;
esac

log()    { $QUIET || echo -e "${GREEN}==>${NC} $*"; }
warn()   { $QUIET || echo -e "${YELLOW}==>${NC} $*"; }
err()    { echo -e "${RED}error:${NC} $*" >&2; }
note()   { $QUIET || echo -e "${BLUE}    $*${NC}"; }

run_cmd() {
    if $QUIET; then
        "$@" >/dev/null 2>&1
    else
        "$@"
    fi
}

check_tool() {
    local tool="$1"
    local hint="${2:-}"
    if ! command -v "$tool" >/dev/null 2>&1; then
        err "missing required tool: ${BOLD}${tool}${NC}"
        [[ -n "$hint" ]] && echo "       ${hint}" >&2
        return 1
    fi
}

check_required_tools() {
    log "checking required tools"
    local failed=0
    check_tool cmake    "install via your package manager (apt/dnf/brew install cmake)"   || failed=1
    check_tool make     "install build-essential / xcode-select --install"                || failed=1
    check_tool nasm     "install via your package manager (apt/dnf/brew install nasm)"    || failed=1
    check_tool nm       "binutils package"                                                || failed=1
    check_tool awk      "install gawk or mawk"                                            || failed=1
    check_tool xorriso  "install xorriso (apt/dnf/brew install xorriso)"                  || failed=1
    check_tool git      "git is required for fetching submodules and limine"              || failed=1

    for t in "${TARGETS[@]}"; do
        case "$t" in
            run|tests|headless|debug)
                check_tool qemu-system-x86_64 "install QEMU (apt/dnf/brew install qemu)" || failed=1
                break
                ;;
        esac
    done

    if grep -RIlq --include='*.rs' "" kernel/ 2>/dev/null; then
        if ! command -v cargo >/dev/null 2>&1; then
            warn "Rust sources detected in kernel/ but cargo not found"
            warn "install via https://rustup.rs/"
            failed=1
        fi
    fi

    [[ $failed -eq 0 ]] || { err "fix missing tools and rerun"; exit 1; }
}

check_required_tools

log "syncing submodules"
run_cmd git submodule update --init --recursive
rm -rf kernel/uACPI/tests 2>/dev/null || true

if [[ ! -d "limine" ]]; then
    warn "fetching limine bootloader"
    run_cmd git clone https://github.com/limine-bootloader/limine \
        --branch=v9.x-binary --depth=1
fi

if $CLEAN && [[ -d "$BUILD_DIR" ]]; then
    warn "removing existing $BUILD_DIR/"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

TOOLCHAIN_ARGS=()
if [[ "$(uname -s)" == "Darwin" ]]; then
    note "detected macOS — using macos_toolchain.cmake"
    TOOLCHAIN_ARGS=(-DCMAKE_TOOLCHAIN_FILE=../scripts/macos_toolchain.cmake)
elif [[ "$(uname -m)" == "aarch64" ]]; then
    note "detected aarch64 host — using cross toolchain"
    TOOLCHAIN_ARGS=(-DCMAKE_TOOLCHAIN_FILE=../scripts/toolchain.cmake)
fi

log "configuring cmake (${BUILD_TYPE})"
run_cmd cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    "${TOOLCHAIN_ARGS[@]}" \
    "${CMAKE_EXTRA_ARGS[@]}" \
    ..

if $COMPDB; then
    log "generating compile_commands.json"
    run_cmd cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
    cp compile_commands.json ..
fi

SYMS_FILE="../syms/fullsyms.c"

if [[ ! -f "$SYMS_FILE" ]]; then
    warn "first build: bootstrapping symbol table"
    run_cmd make -j"$JOBS" kernel
    run_cmd make regen-syms
    log "reconfiguring with real symbols"
    run_cmd cmake \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        "${TOOLCHAIN_ARGS[@]}" \
        "${CMAKE_EXTRA_ARGS[@]}" \
        ..
fi

log "building targets: ${TARGETS[*]}"
run_cmd make -j"$JOBS" "${TARGETS[@]}"

log "${BOLD}done${NC}"
