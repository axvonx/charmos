#!/usr/bin/env python3
"""
wrote this because awk was slow and i want fast
"""

import contextlib
import os
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


def uleb_extend(buf: bytearray, val: int) -> None:
    while True:
        b = val & 0x7F
        val >>= 7
        if val:
            buf.append(b | 0x80)
        else:
            buf.append(b)
            break


def zig(val: int) -> int:
    return (-val) * 2 - 1 if val < 0 else val * 2


def build_lines(decodedline_text: str, root: str) -> bytes:
    if not decodedline_text:
        return b""

    cur = ""
    full: dict[str, str] = {}
    unique_files: list[str] = []
    seen_file: set[str] = set()
    resolved: dict[tuple[str, str], str] = {}

    def resolve_file(col: str) -> str:
        key = (cur, col)
        if key in resolved:
            return resolved[key]
        if col in full:
            res = full[col]
            resolved[key] = res
            return res
        if cur and cur.endswith(col):
            resolved[key] = cur
            return cur

        hit = None
        nhit = 0
        for f in unique_files:
            if f.endswith(col):
                hit = f
                nhit += 1
        if nhit == 1 and hit is not None:
            resolved[key] = hit
            return hit

        resolved[key] = col
        return col

    rows: list[tuple[int, int, str]] = []
    for line in decodedline_text.splitlines():
        if not line:
            continue
        if line.endswith(":") and "0x" not in line:
            raw = line.rstrip(" \t:")
            if raw.startswith("CU:"):
                raw = raw[3:].lstrip(" \t")
            cur = raw
            if root and cur.startswith(root + "/"):
                cur = cur[len(root) + 1 :]
            full[raw] = cur
            full[cur] = cur
            if cur not in seen_file:
                seen_file.add(cur)
                unique_files.append(cur)
            continue

        parts = line.split()
        if len(parts) >= 3 and parts[2].startswith("0x"):
            ln_str = parts[1]
            addr_str = parts[2]
            if not ln_str.isdigit():
                continue
            ln = int(ln_str)
            if ln == 0:
                continue
            try:
                addr = int(addr_str, 16)
            except ValueError:
                continue
            if (addr >> 32) != 0xFFFFFFFF:
                continue
            rows.append((addr, ln, resolve_file(parts[0])))

    if not rows:
        return b""

    rows.sort(key=lambda r: r[0])

    base_addr = rows[0][0]
    base_hi = base_addr >> 32
    base_lo = base_addr & 0xFFFFFFFF

    filtered_addrs: list[int] = []
    filtered_fidxs: list[int] = []
    filtered_lines: list[int] = []
    file_idx: dict[str, int] = {}
    file_names: list[str] = []

    prev_a = -1
    prev_fi = -1
    prev_ln = -1

    for a, ln, f in rows:
        if (a >> 32) != base_hi:
            sys.stderr.write(
                f"stamp_syms: address {hex(a)} leaves the {hex(base_hi)} region\n"
            )
            sys.exit(1)

        lo = a & 0xFFFFFFFF
        if lo == prev_a:
            continue
        prev_a = lo

        if f not in file_idx:
            file_idx[f] = len(file_names)
            file_names.append(f)
        fi = file_idx[f]

        if filtered_addrs and fi == prev_fi and ln == prev_ln:
            continue

        filtered_addrs.append(lo)
        filtered_fidxs.append(fi)
        filtered_lines.append(ln)
        prev_fi = fi
        prev_ln = ln

    n = len(filtered_addrs)
    if n == 0:
        return b""

    stream = bytearray()
    pa = base_lo
    pf = 0
    pl = 0
    for i in range(n):
        uleb_extend(stream, filtered_addrs[i] - pa)
        uleb_extend(stream, zig(filtered_fidxs[i] - pf))
        uleb_extend(stream, zig(filtered_lines[i] - pl))
        pa = filtered_addrs[i]
        pf = filtered_fidxs[i]
        pl = filtered_lines[i]

    files_bytes = bytearray()
    for name in file_names:
        files_bytes.extend(name.encode("utf-8"))
        files_bytes.append(0)

    hdr_len = 32
    hdr = bytearray(b"LINE")
    hdr.extend(struct.pack("<I", n))
    hdr.extend(struct.pack("<Q", base_addr))
    hdr.extend(struct.pack("<I", hdr_len))
    hdr.extend(struct.pack("<I", len(stream)))
    hdr.extend(struct.pack("<I", hdr_len + len(stream)))
    hdr.extend(struct.pack("<I", len(files_bytes)))
    return bytes(hdr + stream + files_bytes)


def build_syms(nm_text: str, reserve: int, lines_len: int) -> bytes:
    symbols: list[tuple[int, str]] = []
    for line in nm_text.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[1] in ("t", "T"):
            try:
                addr = int(parts[0], 16)
            except ValueError:
                continue
            symbols.append((addr, parts[2]))

    nsyms = len(symbols)
    sym_hdr_len = 16
    sym_ent_len = 16
    strtab_off = sym_hdr_len + nsyms * sym_ent_len

    names_bytes = bytearray()
    name_offsets: list[int] = []
    curr_off = 0
    for _, name in symbols:
        name_offsets.append(curr_off)
        nb = name.encode("utf-8") + b"\0"
        names_bytes.extend(nb)
        curr_off += len(nb)

    total_sym = strtab_off + len(names_bytes)
    pad = (8 - total_sym % 8) % 8
    lines_off = (total_sym + pad) if lines_len > 0 else 0

    if total_sym + pad + lines_len > reserve:
        sys.stderr.write(
            f"stamp_syms: table needs {total_sym + pad + lines_len} bytes, "
            f".kernel_syms reserves {reserve}\n"
            "          raise KERNEL_SYMS_RESERVE in include/linker/symbol_table.h\n"
        )
        sys.exit(1)

    sym_hdr = bytearray(b"SYMS")
    sym_hdr.extend(struct.pack("<I", nsyms))
    sym_hdr.extend(struct.pack("<I", strtab_off))
    sym_hdr.extend(struct.pack("<I", lines_off))

    sym_table = bytearray()
    for (addr, _), noff in zip(symbols, name_offsets, strict=True):
        sym_table.extend(struct.pack("<Q", addr))
        sym_table.extend(struct.pack("<I", noff))
        sym_table.extend(struct.pack("<I", 0))

    return bytes(sym_hdr + sym_table + names_bytes + (b"\0" * pad))


def main() -> None:
    if len(sys.argv) != 6:
        sys.stderr.write(
            f"usage: {sys.argv[0]} <nm> <objcopy> <objdump> <reserve-bytes> <kernel>\n"
        )
        sys.exit(2)

    nm_bin = sys.argv[1]
    objcopy_bin = sys.argv[2]
    objdump_bin = sys.argv[3]
    reserve = int(sys.argv[4])
    kernel = sys.argv[5]

    script_dir = Path(__file__).resolve().parent
    root = str(script_dir.parent)

    def run_objdump() -> tuple[int, str]:
        p = subprocess.run(
            [objdump_bin, "--dwarf=decodedline", kernel],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        return p.returncode, p.stdout

    def run_nm() -> tuple[int, str]:
        p = subprocess.run(
            [nm_bin, "-n", kernel],
            capture_output=True,
            text=True,
            check=True,
        )
        return p.returncode, p.stdout

    with ThreadPoolExecutor() as executor:
        fut_objdump = executor.submit(run_objdump)
        fut_nm = executor.submit(run_nm)
        od_code, od_text = fut_objdump.result()
        _, nm_text = fut_nm.result()

    lines_blob = b""
    if od_code == 0 and od_text:
        try:
            lines_blob = build_lines(od_text, root)
        except Exception as e:
            sys.stderr.write(
                f"stamp_syms: line table generation failed ({e}), panics will have no file:line\n"
            )
            lines_blob = b""
    else:
        sys.stderr.write(
            f"stamp_syms: {objdump_bin} cannot dump DWARF line info, panics will have no file:line\n"
            "  (need GNU binutils objdump; llvm-objdump has no --dwarf)\n"
        )

    lines_len = len(lines_blob)
    syms_blob = build_syms(nm_text, reserve, lines_len)

    total_blob = syms_blob + lines_blob
    actual = len(total_blob)
    if actual > reserve:
        sys.stderr.write(
            f"stamp_syms: produced {actual} bytes, .kernel_syms reserves {reserve}\n"
        )
        sys.exit(1)

    pad_final = reserve - actual
    final_blob = total_blob + (b"\0" * pad_final)

    with tempfile.NamedTemporaryFile(delete=False) as tf:
        tf.write(final_blob)
        tmp_name = tf.name

    try:
        subprocess.run(
            [objcopy_bin, f"--update-section=.kernel_syms={tmp_name}", kernel],
            check=True,
        )
    finally:
        with contextlib.suppress(OSError):
            os.unlink(tmp_name)

    if lines_len > 0:
        print(
            f"stamp_syms: {len(syms_blob)} bytes of symbols + {lines_len} of line table, "
            f"{actual} of {reserve} used"
        )


if __name__ == "__main__":
    main()
