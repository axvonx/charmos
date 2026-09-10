"""Copy limine.conf and append a kernel command line to it

    gen_limine_conf.py <in> <out>

Mutually exclusive, in precedence:

    CMDLINE=<path>          read <path>, append its contents
    NIGHTMARE_TESTS=<name>  shorthand, expands to nightmare=<name>
    TESTS=a,b,c             expands to test.filter=a,b,c

EXTRA_CMDLINE=<string> appends at the end
"""

import os
import re
import sys
from collections.abc import Mapping
from pathlib import Path

CMDLINE_LINE_RE = re.compile(r"\n[ \t]*cmdline:[^\n]*")
PATH_LINE_RE = re.compile(r"\n[ \t]*path:[^\n]*")


class ConfError(Exception):
    pass


def resolve_cmdline(env: Mapping[str, str] | None = None) -> str:
    source = os.environ if env is None else env

    def value(name: str) -> str:
        return source.get(name, "").strip()

    cmdline_file = value("CMDLINE")
    nightmare = value("NIGHTMARE_TESTS")
    tests = value("TESTS")

    chosen = [
        name
        for name, value in (
            ("CMDLINE", cmdline_file),
            ("NIGHTMARE_TESTS", nightmare),
            ("TESTS", tests),
        )
        if value
    ]
    if len(chosen) > 1:
        raise ConfError(f"{', '.join(chosen)} are set together")

    line = ""
    if cmdline_file:
        path = Path(cmdline_file)
        if not path.exists():
            raise ConfError(f"CMDLINE points at '{cmdline_file}', which does not exist")
        line = re.sub(r"[\r\n]+", " ", path.read_text(encoding="utf-8")).strip()
        if not line:
            raise ConfError(f"CMDLINE file '{cmdline_file}' is empty")

    elif nightmare:
        if re.search(r"[ \t,]", nightmare):
            raise ConfError(
                f"NIGHTMARE_TESTS takes exactly one test name (got '{nightmare}')"
            )
        line = f"nightmare={nightmare}"

    elif tests:
        normalized = re.sub(r"[ \t,]+", ",", tests)
        if normalized.startswith(",") or normalized.endswith(","):
            raise ConfError(
                f"TESTS list has a leading or trailing comma (got '{tests}')"
            )
        line = f"test.filter={normalized}"

    extra = value("EXTRA_CMDLINE")
    if extra:
        line = f"{line} {extra}".strip()
    return line


def render(conf: str, line: str) -> str:
    """Splice `line` into an existing cmdline"""
    if not line:
        return conf
    if CMDLINE_LINE_RE.search(conf):
        return CMDLINE_LINE_RE.sub(lambda m: f"{m.group(0)} {line}", conf)
    return PATH_LINE_RE.sub(lambda m: f"{m.group(0)}\n    cmdline: {line}", conf)


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(f"usage: {Path(argv[0]).name} <in> <out>", file=sys.stderr)
        return 2

    source, destination = Path(argv[1]), Path(argv[2])
    try:
        conf = source.read_text(encoding="utf-8")
        line = resolve_cmdline()
    except ConfError as error:
        print(f"gen_limine_conf: {error}", file=sys.stderr)
        return 1
    except OSError as error:
        print(f"gen_limine_conf: {error.strerror or error}", file=sys.stderr)
        return 1

    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(render(conf, line), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
