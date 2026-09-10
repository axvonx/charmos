#!/usr/bin/env bash
#
# usage: run_qemu.sh <logfile> <map_debug_exit> <argv-file>

set -uo pipefail

LOGFILE="$1"
MAP_DEBUG_EXIT="$2"
ARGV_FILE="$3"

if [[ ! -r "$ARGV_FILE" ]]; then
    echo "run_qemu: cannot read $ARGV_FILE; reconfigure to render it" >&2
    exit 2
fi

QEMU_ARGV=()
while IFS= read -r line || [[ -n "$line" ]]; do
    [[ -n "$line" ]] && QEMU_ARGV+=("$line")
done < "$ARGV_FILE"

if [[ ${#QEMU_ARGV[@]} -eq 0 ]]; then
    echo "run_qemu: $ARGV_FILE is empty" >&2
    exit 2
fi

# QEMU's diagnostics end up in stderr
ERRLOG="${LOGFILE%.*}.stderr.log"

# Still shown live on the terminal,
# host and guest are never confused for one another
"${QEMU_ARGV[@]}" 2> >(tee "$ERRLOG" | sed 's/^/[qemu] /' >&2) | tee "$LOGFILE"
status="${PIPESTATUS[0]}"

if [[ "$MAP_DEBUG_EXIT" == "1" ]]; then
    case "$status" in
        1)  exit 0 ;;  # 0: nightmare/test OK
        3)  exit 1 ;;  # 1: gate test failure
        5)  exit 3 ;;  # 2: panic
        7)  exit 3 ;;  # 3: nightmare finding
        9)  exit 4 ;;  # 4: nightmare harness/subject failure
        11) exit 5 ;;  # 5: nightmare stall
        13) exit 6 ;;  # 6: nightmare refusal/skip
    esac
fi

exit "$status"
