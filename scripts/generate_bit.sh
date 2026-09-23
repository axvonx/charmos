#!/usr/bin/env bash
# nice wrapper around bit flag comments

set -euo pipefail

if [[ $# -ne 1 || ! $1 =~ ^[1-9][0-9]*$ ]] || (( $1 % 4 != 0 )); then
    echo "Usage: $0 <positive bit width divisible by 4>" >&2
    exit 1
fi

bit_width=$1
nibbles=$((bit_width / 4))
bits=""
use=""

for ((nibble = nibbles - 1; nibble >= 0; nibble--)); do
    high_bit=$((nibble * 4 + 3))
    low_bit=$((nibble * 4))
    label="${high_bit}..${low_bit}"

    if ((nibble == 0)); then
        printf -v bit_cell '%-4s ' "$label"
        use_cell="AAAA "
    else
        printf -v bit_cell ' %-6s' "$label"

        if ((high_bit < 10)); then
            use_cell=" AAAA  "
        else
            use_cell="  AAAA "
        fi
    fi

    bits+=$bit_cell
    use+=$use_cell
done

horizontal_line=""
for ((column = 0; column < ${#bits}; column++)); do
    horizontal_line+="─"
done

printf '%s\n' \
    "/* NAME: ${bit_width} bit bitflags" \
    " *" \
    " *      ┌${horizontal_line}┐" \
    " * Bits │${bits}│" \
    " * Use  │${use}│" \
    " *      └${horizontal_line}┘" \
    " *" \
    " * A - Unused (available)" \
    " * * - Unused (unavailable)" \
    " *" \
    " */"
