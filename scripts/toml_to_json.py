"""Print a TOML file as JSON, for CMake to read"""

import json
import sys
import tomllib
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} <file.toml>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    try:
        with path.open("rb") as handle:
            document = tomllib.load(handle)
    except tomllib.TOMLDecodeError as exc:
        print(exc, file=sys.stderr)
        return 1
    except OSError as exc:
        print(exc.strerror or exc, file=sys.stderr)
        return 1

    json.dump(document, sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())
