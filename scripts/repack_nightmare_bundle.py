"""Repack a verified nightmare bundle into an ISO

Taking in bundle's prebuilt artifacts and command line
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

import gen_limine_conf

LIMINE_BOOT_ASSETS = ("limine-bios.sys", "limine-bios-cd.bin", "limine-uefi-cd.bin")
EFI_ASSETS = ("BOOTX64.EFI", "BOOTIA32.EFI")

XORRISO_FLAGS = (
    "-as", "mkisofs",
    "-R", "-r", "-J",
    "-b", "boot/limine/limine-bios-cd.bin",
    "-no-emul-boot",
    "-boot-load-size", "4",
    "-boot-info-table",
    "-hfsplus",
    "-apm-block-size", "2048",
    "--efi-boot", "boot/limine/limine-uefi-cd.bin",
    "-efi-boot-part", "--efi-boot-image",
    "--protective-msdos-label",
)  # fmt: skip


class RepackError(Exception):
    pass


def _run(command: list[str]) -> None:
    completed = subprocess.run(command, capture_output=True, text=True, check=False)
    if completed.returncode != 0:
        raise RepackError(
            f"{Path(command[0]).name} failed with exit code {completed.returncode}: "
            f"{completed.stdout}{completed.stderr}"
        )


def repack(bundle_dir: Path, cmdline: Path, output_iso: Path, work_dir: Path) -> None:
    artifacts = bundle_dir / "artifacts"
    iso_root = work_dir / "iso_root"

    shutil.rmtree(iso_root, ignore_errors=True)
    (iso_root / "boot" / "limine").mkdir(parents=True)
    (iso_root / "EFI" / "BOOT").mkdir(parents=True)

    shutil.copyfile(artifacts / "kernel", iso_root / "boot" / "kernel")

    try:
        line = gen_limine_conf.resolve_cmdline(
            {
                "CMDLINE": str(cmdline),
                "NIGHTMARE_TESTS": "",
                "TESTS": "",
                "EXTRA_CMDLINE": "",
            }
        )
    except gen_limine_conf.ConfError as error:
        raise RepackError(str(error)) from error

    conf = (artifacts / "limine.conf").read_text(encoding="utf-8")
    destination = iso_root / "boot" / "limine" / "limine.conf"
    destination.write_text(gen_limine_conf.render(conf, line), encoding="utf-8")

    for asset in LIMINE_BOOT_ASSETS:
        shutil.copyfile(artifacts / asset, iso_root / "boot" / "limine" / asset)
    for asset in EFI_ASSETS:
        shutil.copyfile(artifacts / asset, iso_root / "EFI" / "BOOT" / asset)

    output_iso.parent.mkdir(parents=True, exist_ok=True)
    _run(["xorriso", *XORRISO_FLAGS, str(iso_root), "-o", str(output_iso)])
    _run([str(artifacts / "limine"), "bios-install", str(output_iso)])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle-dir", type=Path, required=True)
    parser.add_argument("--cmdline", type=Path, required=True)
    parser.add_argument("--output-iso", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True)
    args = parser.parse_args(argv)

    try:
        repack(
            args.bundle_dir.resolve(),
            args.cmdline.resolve(),
            args.output_iso.resolve(),
            args.work_dir.resolve(),
        )
    except (RepackError, OSError) as error:
        print(f"repack_nightmare_bundle: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
