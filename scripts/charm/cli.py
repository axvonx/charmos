"""``charm`` the entry point for charmOS host tooling

Subcommand groups mirror the stage they serve:

    charm ci ...
    charm dev ...
    charm nightmare ...
"""

import argparse
import glob
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import Any

from . import ingest as I
from . import protocol as P
from . import record as R
from . import report as RP
from .nightmare import cli as nightmare_cli


def _expand(patterns: list[str]) -> tuple[list[Path], list[str]]:
    paths: list[Path] = []
    unmatched: list[str] = []
    for pattern in patterns:
        expanded = [Path(p) for p in sorted(glob.glob(pattern))]
        if expanded:
            paths.extend(p for p in expanded if p.is_file())
        else:
            unmatched.append(pattern)
    return paths, unmatched


def _write_report(rep: dict[str, Any], args: argparse.Namespace, run_count: int) -> int:
    markdown = RP.render_markdown(rep)
    if args.summary:
        with Path(args.summary).open("a", encoding="utf-8") as fh:
            fh.write(markdown)
    sys.stdout.write(markdown)

    if args.json:
        Path(args.json).write_text(RP.render_json(rep), encoding="utf-8")

    print(
        f"{run_count} shard(s), {len(rep['failed_runs'])} failed, "
        f"{len(rep['unique_crashes'])} unique crash(es)",
        file=sys.stderr,
    )
    return 1 if (args.fail_on_error and not rep["ok"]) else 0


def cmd_parse(args: argparse.Namespace) -> int:
    try:
        log_bytes = Path(args.log).stat().st_size
    except FileNotFoundError:
        log_bytes = 0

    try:
        with Path(args.ndjson).open(encoding="utf-8", errors="replace") as fh:
            result = R.parse_ndjson(fh)
    except FileNotFoundError:
        print(f"error: {args.ndjson} not found", file=sys.stderr)
        return 2

    stats = result.stats

    for err in stats.schema_errors:
        print(f"schema drift: {err}", file=sys.stderr)
        result.records.append({"type": "schema_error", "detail": err})

    if not stats.schema_seen:
        print(
            f"warning: {args.ndjson} contains no schema records (boot with ndjson.schema=true)",
            file=sys.stderr,
        )

    meta = R.RunMeta(
        shard=args.shard,
        scenario=args.scenario,
        seed=args.seed,
        sha=args.sha,
        compiler=args.compiler,
        config=args.config,
        exit_code=args.exit_code,
        duration_s=args.duration_s,
    )
    run = R.build_run_record(result, meta, log_bytes)
    text = R.render([run, *result.records])

    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)

    if not stats.ended:
        tail = f", last record was {stats.last_record}" if stats.last_record else ""
        print(
            f"guest stopped unexpectedly{tail}",
            file=sys.stderr,
        )

    print(
        f"parsed {args.ndjson}: {stats.records_seen} records, "
        f"{stats.tests_seen} tests, {stats.crashes} crashes, "
        f"outcome={run['outcome']}",
        file=sys.stderr,
    )

    if stats.schema_errors:
        print(
            f"{len(stats.schema_errors)} handler(s) disagree with the kernel's "
            "declared schema",
            file=sys.stderr,
        )
        if args.strict_schema:
            return 2

    return 0


def cmd_aggregate(args: argparse.Namespace) -> int:
    paths, unmatched = _expand(args.inputs)
    if unmatched:
        print(f"error: no files matched: {', '.join(unmatched)}", file=sys.stderr)
        return 2
    if not paths:
        print("error: no result files matched", file=sys.stderr)
        return 2

    runs, tests, crashes, stale = RP.load(paths)
    if not runs:
        print(f"error: no run records found in {len(paths)} file(s)", file=sys.stderr)
        return 2

    rep = RP.build_report(runs, tests, crashes, args.repro_template)
    for path in stale:
        rep["warnings"].insert(
            0, f"{RP.md_code(path)} was written by a different result version"
        )
    return _write_report(rep, args, len(runs))


def cmd_ingest(args: argparse.Namespace) -> int:
    root = Path(args.artifacts)
    if not root.is_dir():
        print(f"error: {root} is not a directory", file=sys.stderr)
        return 2

    written, problems = I.ingest(root, Path(args.ndjson_dir))
    I.report_problems(problems)

    if not written:
        print(f"error: no shard produced result records under {root}", file=sys.stderr)
        return 2

    runs, tests, crashes, stale = RP.load(written)
    if not runs:
        print(f"error: no run records in {len(written)} file(s)", file=sys.stderr)
        return 2

    rep = RP.build_report(runs, tests, crashes, args.repro_template)
    for path in stale:
        rep["warnings"].insert(
            0, f"{RP.md_code(path)} was written by a different result version"
        )
    for p in problems:
        if p.level == "error":
            rep["warnings"].insert(0, RP.md_escape(p.text))

    return _write_report(rep, args, len(runs))


def cmd_schema(args: argparse.Namespace) -> int:
    from . import schema as S

    log_path = Path(args.log)
    records = S.collect(log_path.read_text(encoding="utf-8", errors="replace"))
    if not records:
        print(
            f"error: {log_path} contains no schema records (boot with ndjson.schema=true)",
            file=sys.stderr,
        )
        return 1

    import json

    text = json.dumps(S.build(records), indent=2, sort_keys=True) + "\n"
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)

    print(f"described {len(records)} record types from {log_path}", file=sys.stderr)
    return 0


def cmd_protocol(_args: argparse.Namespace) -> int:
    P.dump()
    return 0


def cmd_workflow_policy(_args: argparse.Namespace) -> int:
    from . import workflow_policy as WP

    violations = WP.check()
    for violation in violations:
        print(violation, file=sys.stderr)
    if violations:
        print(
            f"error: {len(violations)} workflow policy violation(s)",
            file=sys.stderr,
        )
        return 1
    checked = len(WP.all_paths())
    print(
        f"ok: {checked} workflows checked "
        f"({len(WP.EXECUTION_WORKFLOWS)} held to the execution rules)"
    )
    return 0


def cmd_machine_list(args: argparse.Namespace) -> int:
    from . import machine as MC

    directory = MC.machines_dir()
    for path in sorted(directory.glob("*.toml")):
        m = MC.load(path.stem, directory=directory)
        modes = ", ".join(sorted(m.modes))
        print(f"{m.name:<12} {m.arch:<8} {m.type:<14} qemu>={m.min_qemu}  [{modes}]")
    return 0


def cmd_machine_resolve(args: argparse.Namespace) -> int:
    from . import machine as MC

    try:
        m = MC.load(args.profile)
    except MC.MachineError as error:
        print(f"charm machine: {error}", file=sys.stderr)
        return 1
    print(MC.resolve_machine_type(m))
    return 0


def cmd_machine_render(args: argparse.Namespace) -> int:
    from . import machine as MC

    try:
        m = MC.load(args.profile)
        if args.check_version:
            MC.check_qemu_version(m)
        smp = None
        if args.smp:
            fields = dict(
                part.split("=", 1) for part in args.smp.replace(" ", "").split(",")
            )
            smp = MC.Smp(**{k: int(v) for k, v in fields.items()})
        argv = MC.render(
            m,
            args.mode,
            iso=args.iso,
            disk=args.disk,
            qmp_socket=args.qmp_socket,
            machine_log=args.machine_log,
            trace_log=args.trace_log,
            acpi_dir=args.acpi_dir,
            memory_mib=MC.parse_memory(args.memory) if args.memory else None,
            smp=smp,
            kvm=args.kvm,
            gdb="wait" if args.gdb_wait else None,
        )
    except MC.MachineError as error:
        print(f"charm machine: {error}", file=sys.stderr)
        return 1

    text = "\n".join(argv) + "\n"
    if args.out:
        Path(args.out).parent.mkdir(parents=True, exist_ok=True)
        Path(args.out).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    return 0


def cmd_repeat(args: argparse.Namespace) -> int:
    from . import local as L

    build_dir = Path(args.build_dir)
    if not (build_dir / "CMakeCache.txt").is_file():
        print(
            f"error: {build_dir} is not a configured build directory; "
            "run scripts/build.sh first",
            file=sys.stderr,
        )
        return 2

    return L.repeat(
        build_dir=build_dir,
        results_root=Path(args.results_dir or build_dir / "repeated-test-runs"),
        runs=args.runs,
        timeout_s=args.timeout,
        target=args.target,
    )


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="charm", description=__doc__)
    groups = ap.add_subparsers(dest="group", required=True)

    ci = groups.add_parser("ci", help="CI result parsing and reporting")
    sub = ci.add_subparsers(dest="command", required=True)

    p = sub.add_parser("parse", help="one run's logs -> result records")
    p.add_argument("log", help="QEMU serial log, kept for its size only")
    p.add_argument("--ndjson", required=True, help="the kernel's machine channel")
    p.add_argument(
        "--strict-schema",
        action="store_true",
        help="exit non-zero when a handler disagrees with the emitted schema",
    )
    p.add_argument("--out", help="NDJSON output (default: stdout)")
    p.add_argument("--shard", default=None)
    p.add_argument("--scenario", default=None)
    p.add_argument("--seed", default=None)
    p.add_argument("--sha", default=None)
    p.add_argument("--compiler", default=None)
    p.add_argument("--config", default=None, help="free-form guest config label")
    p.add_argument("--duration-s", type=float, default=None)
    p.add_argument(
        "--exit-code",
        type=int,
        default=None,
        help="exit status of the QEMU wrapper (0 pass, 1 test failure, "
        "3 panic, 124 timeout by convention)",
    )
    p.set_defaults(fn=cmd_parse)

    a = sub.add_parser("aggregate", help="result records -> one report")
    a.add_argument("inputs", nargs="+", help="NDJSON files or globs")
    RP.add_report_args(a)
    a.set_defaults(fn=cmd_aggregate)

    g = sub.add_parser(
        "ingest",
        help="a downloaded artifact tree -> result records -> one report",
    )
    g.add_argument("artifacts", help="directory holding the downloaded artifacts")
    g.add_argument(
        "--ndjson-dir",
        default="ndjson",
        help="where per-shard result records are written (default: ndjson)",
    )
    RP.add_report_args(g)
    g.set_defaults(fn=cmd_ingest)

    s = sub.add_parser("schema", help="a schema-dump boot -> JSON Schema")
    s.add_argument("log", help="an ndjson log from a run with ndjson.schema=true")
    s.add_argument("--out", help="output path (default: stdout)")
    s.set_defaults(fn=cmd_schema)

    n = sub.add_parser(
        "protocol", help="print the wire names include/ndjson.h declares"
    )
    n.set_defaults(fn=cmd_protocol)

    wp = sub.add_parser(
        "workflow-policy",
        help="enforce read-only Git and immutable images in execution workflows",
    )
    wp.set_defaults(fn=cmd_workflow_policy)

    mc = groups.add_parser("machine", help="the guest QEMU boots")
    mcsub = mc.add_subparsers(dest="command", required=True)

    ml = mcsub.add_parser("list", help="every profile and the modes it defines")
    ml.set_defaults(fn=cmd_machine_list)

    mv = mcsub.add_parser(
        "resolve", help="what the profile's machine type means on this host"
    )
    mv.add_argument("--profile", default="default")
    mv.set_defaults(fn=cmd_machine_resolve)

    mr = mcsub.add_parser("render", help="one profile and mode -> a QEMU argv")
    mr.add_argument("--profile", default="default")
    mr.add_argument("--mode", required=True)
    mr.add_argument("--iso", required=True)
    mr.add_argument("--disk")
    mr.add_argument("--qmp-socket", required=True)
    mr.add_argument("--machine-log")
    mr.add_argument("--trace-log")
    mr.add_argument("--acpi-dir", type=Path)
    mr.add_argument("--memory", help="override, e.g. 4G")
    mr.add_argument("--smp", help="override, e.g. sockets=1,cores=2,threads=1")
    mr.add_argument("--kvm", action="store_true")
    mr.add_argument("--gdb-wait", action="store_true", help="halt at startup (-S)")
    mr.add_argument(
        "--check-version", action="store_true", help="refuse a QEMU below min_qemu"
    )
    mr.add_argument("--out", help="write here instead of stdout")
    mr.set_defaults(fn=cmd_machine_render)

    dev = groups.add_parser("dev", help="loops against a local build")
    devsub = dev.add_subparsers(dest="command", required=True)

    r = devsub.add_parser(
        "repeat", help="build the test target N times, stopping at the first failure"
    )
    r.add_argument(
        "runs", nargs="?", type=int, default=1, help="iterations (default 1)"
    )
    r.add_argument(
        "-t",
        "--timeout",
        type=int,
        default=60,
        help="per-iteration wall budget in seconds (default 60)",
    )
    r.add_argument("-B", "--build-dir", default="build", help="build directory")
    r.add_argument("--target", default="tests", help="build target (default: tests)")
    r.add_argument(
        "--results-dir",
        default=None,
        help="where per-run evidence is collected "
        "(default: <build-dir>/repeated-test-runs)",
    )
    r.set_defaults(fn=cmd_repeat)

    nm = groups.add_parser("nightmare", help="suite TOML and the cmdline codec")
    nightmare_cli.register_parsers(nm)

    return ap


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
