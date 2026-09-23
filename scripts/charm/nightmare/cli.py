"""CLI subcommands and argument parser registration for charm nightmare."""

import argparse
import sys
from pathlib import Path
from typing import Any

from .. import report as RP


def _load_suite(path_str: str) -> Any | None:
    from . import suite as NS

    try:
        return NS.load(Path(path_str))
    except NS.SuiteError as e:
        print(f"error: {e}", file=sys.stderr)
        return None


def _read_json_object(path: Path) -> dict[str, Any]:
    import json

    document = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(document, dict):
        raise ValueError(f"{path}: expected a JSON object")
    return document


def _execute_runner_manifest(
    manifest_path: Path,
    *,
    build_dir: Path,
    out_dir: Path,
    bundle_path: Path | None = None,
    allow_development_bundle: bool = False,
) -> int:
    import json

    from . import executor as NE

    execution = NE.execute_manifest(
        manifest_path,
        build_dir=build_dir,
        out_dir=out_dir,
        bundle_path=bundle_path,
        allow_development_bundle=allow_development_bundle,
    )
    print(json.dumps(execution.document, indent=2))
    print(
        f"runner result: {execution.result_path} "
        f"({execution.document['execution']['health']})",
        file=sys.stderr,
    )
    return execution.exit_code


def cmd_nm_validate(args: argparse.Namespace) -> int:
    from . import suite as NS

    failed = 0
    for path_str in args.suites:
        suite = _load_suite(path_str)
        if suite is None:
            failed += 1
            continue
        tasks = ", ".join(t.name for t in suite.tasks)
        print(f"ok {path_str}: {suite.meta.name} ({len(suite.tasks)} task(s): {tasks})")

    if not NS.SCHEMA_AVAILABLE:
        print(
            f"note: jsonschema is not installed; schema validation for {NS.SCHEMA_PATH.name} skipped",
            file=sys.stderr,
        )
        if args.require_schema:
            return 2

    return 1 if failed else 0


def cmd_nm_render(args: argparse.Namespace) -> int:
    from . import codec as NC

    suite = _load_suite(args.suite)
    if suite is None:
        return 2
    try:
        task = suite.task(args.task) if args.task else suite.tasks[0]
    except KeyError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    seed = None
    if args.seed is not None:
        seed = int(args.seed, 0)
        if args.runner is not None:
            seed = NC.seed_for(
                seed,
                runner=args.runner,
                boot_index=args.boot_index,
                boots_per_runner=task.boot.max_boots,
            )

    try:
        line = NC.render(
            task,
            NC.BootRequest(
                boot_index=args.boot_index,
                campaign_id=args.campaign_id,
                seed=seed,
            ),
        )
    except NC.CodecError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    if args.out:
        NC.write(Path(args.out), line)
        print(f"wrote {args.out} ({len(line)} bytes)", file=sys.stderr)
    else:
        print(line)
    return 0


def cmd_nm_build(args: argparse.Namespace) -> int:
    from . import codec as NC

    suite = _load_suite(args.suite)
    if suite is None:
        return 2
    print(" ".join(NC.build_command(suite, target=args.target)))
    return 0


def cmd_nm_show(args: argparse.Namespace) -> int:
    import json

    from . import codec as NC

    suite = _load_suite(args.suite)
    if suite is None:
        return 2

    doc = {
        "suite": {
            "name": suite.meta.name,
            "runners": suite.meta.runners,
            "budget_hours": suite.meta.budget_hours,
            "overlap_ratio": suite.meta.overlap_ratio,
        },
        "build": {
            "compiler": suite.build.compiler,
            "type": suite.build.type,
            "cmake_definitions": list(suite.build.cmake_definitions),
            "smp": suite.build.smp.topo(),
            "memory_mib": suite.build.memory_mib,
            "command": NC.build_command(suite),
        },
        "tasks": [
            {
                "name": t.name,
                "mode": t.mode,
                "weight": t.weight,
                "priority": t.priority,
                "max_runners": t.max_runners or suite.meta.runners,
                "budgets_ms": {
                    "soft": t.boot.duration_ms,
                    "guest_hard": t.boot.guest_hard_ms,
                    "host_timeout": t.boot.host_timeout_ms,
                },
                "max_boots": t.boot.max_boots,
                "seed_mode": t.nightmare.seed_mode,
                "perturb": list(t.nightmare.perturb),
            }
            for t in suite.tasks
        ],
    }
    print(json.dumps(doc, indent=2))
    return 0


def cmd_nm_run(args: argparse.Namespace) -> int:
    from . import campaign as NC
    from . import grammar as NG

    suite = _load_suite(args.suite)
    if suite is None:
        return 2

    base_seed = None
    if args.seed is not None:
        try:
            base_seed = NG.parse_uint(args.seed)
        except Exception as e:
            print(f"error: invalid seed {args.seed!r}: {e}", file=sys.stderr)
            return 2

    out_dir = (
        Path(args.out_dir)
        if args.out_dir
        else Path("campaign-results") / (args.campaign_id or suite.meta.name)
    )
    build_dir = Path(args.build_dir)

    manifest = NC.CampaignManifest(
        suite=suite,
        runner_index=args.runner,
        total_runners=args.total_runners or suite.meta.runners,
        base_seed=base_seed,
        campaign_id=args.campaign_id or "",
        budget_ms=int(args.budget_hours * 3600 * 1000) if args.budget_hours else 0,
        build_dir=build_dir,
        out_dir=out_dir,
        gate_first=args.gate_first,
        dry_run=args.dry_run,
    )

    result = NC.execute(manifest)
    json_path, md_path = NC.write_reports(result, out_dir)

    if args.summary:
        with Path(args.summary).open("a", encoding="utf-8") as fh:
            fh.write(NC.render_markdown(result))
    else:
        sys.stdout.write(NC.render_markdown(result))

    if args.json:
        Path(args.json).write_text(NC.render_json(result), encoding="utf-8")

    print(
        f"campaign {result.campaign_id}: {result.status} (ok={result.ok}), "
        f"{result.total_boots} boot(s), {len(result.findings)} unique finding(s). "
        f"Reports: {json_path}, {md_path}",
        file=sys.stderr,
    )

    return 0 if result.ok else 1


def cmd_nm_run_manifest(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest)
    out_dir = Path(args.out_dir or Path("runner-results") / manifest_path.stem)
    return _execute_runner_manifest(
        manifest_path,
        build_dir=Path(args.build_dir),
        out_dir=out_dir,
        bundle_path=Path(args.bundle) if args.bundle else None,
        allow_development_bundle=args.allow_development_bundle,
    )


def cmd_nm_replay(args: argparse.Namespace) -> int:
    from . import contracts as NCT
    from . import executor as NE

    try:
        manifest_path = NE.resolve_replay_manifest(Path(args.source))
    except NCT.ContractError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    out_dir = Path(args.out_dir or Path("replay-results") / manifest_path.stem)
    return _execute_runner_manifest(
        manifest_path,
        build_dir=Path(args.build_dir),
        out_dir=out_dir,
        bundle_path=Path(args.bundle) if args.bundle else None,
        allow_development_bundle=args.allow_development_bundle,
    )


def cmd_nm_plan(args: argparse.Namespace) -> int:
    import json
    from datetime import UTC, datetime

    from . import planner as OP

    try:
        command = _read_json_object(Path(args.command))
        snapshot = _read_json_object(Path(args.snapshot))
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    source = OP.Source(command.get("repository", {}).get("id", ""), args.source_commit)
    result = OP.Planner(Path(args.suite_dir)).plan(
        command,
        snapshot,
        source=source,
        runner_image=args.runner_image,
        now=datetime.now(UTC),
        ownership=args.ownership,
    )
    document = OP.render_result(result)
    text = json.dumps(document, indent=2) + "\n"
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    if isinstance(result, OP.Accepted):
        print(
            f"accepted {result.plan.id}: {len(result.plan.tasks)} manifest(s), "
            f"{len(result.plan.build_groups)} build group(s)",
            file=sys.stderr,
        )
        return 0
    if isinstance(result, OP.Stale):
        print(f"stale: current snapshot is {result.current_version}", file=sys.stderr)
    else:
        for diagnostic in result.diagnostics:
            print(f"{diagnostic['field']}: {diagnostic['message']}", file=sys.stderr)
    return 2


def cmd_nm_build_bundle(args: argparse.Namespace) -> int:
    import json

    from . import build_bundle as NB

    try:
        plan = _read_json_object(Path(args.plan))
        request = NB.request_from_plan(plan, args.group)
        bundle = NB.create_bundle(
            request,
            build_dir=Path(args.build_dir),
            out_dir=Path(args.out_dir),
            compile_kernel=not args.prebuilt,
        )
    except (OSError, ValueError, json.JSONDecodeError, NB.BundleError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    receipt = NB.receipt(bundle)
    if args.receipt:
        Path(args.receipt).write_text(
            json.dumps(receipt, indent=2) + "\n", encoding="utf-8"
        )
    print(json.dumps(receipt, indent=2))
    return 0


def cmd_nm_materialize(args: argparse.Namespace) -> int:
    from . import materialize as OM

    try:
        plan = OM.load_plan(Path(args.plan))
        receipts = tuple(OM.load_receipt(Path(path)) for path in args.receipts)
        bundle = OM.materialize(
            plan, receipts, attempt=args.attempt, dry_run=args.dry_run
        )
        index = OM.write_bundle(bundle, Path(args.out_dir))
    except OM.MaterializationError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(index)
    return 0


def cmd_nm_repack_bundle(args: argparse.Namespace) -> int:
    import json

    from . import build_bundle as NB

    try:
        bundle = NB.verify_bundle(Path(args.bundle))
        measurement = NB.repack(
            bundle,
            cmdline=Path(args.cmdline),
            out_dir=Path(args.out_dir),
        )
        transport = NB.measure_transport(bundle, Path(args.out_dir) / "measurement")
    except NB.BundleError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(
        json.dumps(
            {
                **transport,
                "repack_ms": round(measurement.repack_ms, 3),
                "iso_size_bytes": measurement.iso_size_bytes,
                "iso": str(measurement.iso_path),
            },
            indent=2,
        )
    )
    return 0


def cmd_nm_repository_command(args: argparse.Namespace) -> int:
    import json
    from datetime import UTC, datetime

    from . import workflow as OW

    try:
        at = (
            datetime.fromisoformat(args.at.replace("Z", "+00:00"))
            if args.at
            else datetime.now(UTC)
        )
        if at.tzinfo is None:
            raise ValueError("--at must include a UTC offset")
        command, snapshot = OW.repository_command(
            suite_id=args.suite,
            suite_dir=Path(args.suite_dir),
            repository=args.repository,
            ref=args.ref,
            now=at,
            runner_capacity=args.runner_capacity,
        )
        Path(args.command_out).write_text(
            json.dumps(command, indent=2) + "\n", encoding="utf-8"
        )
        Path(args.snapshot_out).write_text(
            json.dumps(snapshot, indent=2) + "\n", encoding="utf-8"
        )
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(f"{command['command_id']} {snapshot['version']}")
    return 0


def cmd_nm_inline_command(args: argparse.Namespace) -> int:
    import json
    from datetime import UTC, datetime

    from . import workflow as OW

    try:
        at = (
            datetime.fromisoformat(args.at.replace("Z", "+00:00"))
            if args.at
            else datetime.now(UTC)
        )
        if at.tzinfo is None:
            raise ValueError("--at must include a UTC offset")
        command, snapshot = OW.inline_command(
            toml_text=args.toml,
            repository=args.repository,
            ref=args.ref,
            now=at,
            runner_capacity=args.runner_capacity,
        )
        Path(args.command_out).write_text(
            json.dumps(command, indent=2) + "\n", encoding="utf-8"
        )
        Path(args.snapshot_out).write_text(
            json.dumps(snapshot, indent=2) + "\n", encoding="utf-8"
        )
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(f"{command['command_id']} {snapshot['version']}")
    return 0


def cmd_nm_matrix(args: argparse.Namespace) -> int:
    import json

    from . import workflow as OW

    try:
        document = _read_json_object(Path(args.document))
        rows = (
            OW.build_matrix(document, args.limit)
            if args.kind == "build"
            else OW.runner_matrix(document, args.limit)
        )
    except (OSError, ValueError, json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print(json.dumps({"include": rows}, separators=(",", ":")))
    return 0


def cmd_nm_aggregate(args: argparse.Namespace) -> int:
    import json

    from . import aggregate as NA

    try:
        accepted_plan = _read_json_object(Path(args.plan))
        report = NA.aggregate(
            accepted_plan,
            results_dir=Path(args.results_dir),
            plan_bundle_path=Path(args.plan_bundle) if args.plan_bundle else None,
            builds_dir=Path(args.builds_dir) if args.builds_dir else None,
        )
        NA.write(
            report,
            json_path=Path(args.json),
            markdown_path=Path(args.markdown),
        )
        markdown = NA.render_markdown(report)
        if args.summary:
            with Path(args.summary).open("a", encoding="utf-8") as handle:
                handle.write(markdown)
        sys.stdout.write(markdown)
    except (OSError, ValueError, json.JSONDecodeError, NA.AggregateError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 0 if report.ok else 1


def cmd_nm_wait_until(args: argparse.Namespace) -> int:
    from datetime import datetime

    from . import workflow as OW

    try:
        target = datetime.fromisoformat(args.at.replace("Z", "+00:00"))
        waited = OW.wait_until(target, max_wait_seconds=args.max_wait)
        if waited > 0:
            print(f"waited {waited:.2f}s until {target.isoformat()}")
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 0


def cmd_nm_queue_command(args: argparse.Namespace) -> int:
    import json
    from datetime import UTC, datetime

    from . import workflow as OW

    try:
        command = _read_json_object(Path(args.command))
        now = (
            datetime.fromisoformat(args.at.replace("Z", "+00:00"))
            if args.at
            else datetime.now(UTC)
        )
        metadata = OW.queue_metadata(
            command,
            batch_id=args.batch_id,
            source_sha=args.source_sha,
            now=now,
        )
        Path(args.out).write_text(
            json.dumps(metadata, indent=2) + "\n", encoding="utf-8"
        )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    print("true" if metadata["deferred"] else "false")
    return 0


def cmd_nm_wake_queue(args: argparse.Namespace) -> int:
    import os

    from . import waker as NW

    try:
        due = NW.wake_due(
            token=os.environ.get(args.token_env, ""),
            repository=args.repository,
            workflow=args.workflow,
            ref=args.ref,
            api_url=args.api_url,
        )
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    for queue in due:
        print(f"dispatched {queue.batch_id} from queued run {queue.run_id}")
    if not due:
        print("no deferred Nightmare plans are due")
    return 0


def _add_runner_args(sp: argparse.ArgumentParser) -> None:
    sp.add_argument(
        "--bundle",
        default=None,
        help="verified compile-once build bundle (required for real boots)",
    )
    sp.add_argument(
        "--allow-development-bundle",
        action="store_true",
        help="allow a prebuilt development bundle for local proof only",
    )
    sp.add_argument(
        "--build-dir",
        default="build",
        help="charmOS build directory (default: build)",
    )
    sp.add_argument("--out-dir", default=None)


def register_parsers(nm: argparse.ArgumentParser) -> None:
    """Register all nightmare subcommands on the given nightmare subparser."""
    nmsub = nm.add_subparsers(dest="command", required=True)

    v = nmsub.add_parser("validate", help="load a suite and report every problem")
    v.add_argument("suites", nargs="+", help="suite TOML files")
    v.add_argument(
        "--require-schema",
        action="store_true",
        help="fail if jsonschema is unavailable, so CI cannot silently skip the "
        "published contract",
    )
    v.set_defaults(fn=cmd_nm_validate)

    rd = nmsub.add_parser("render", help="one task and boot -> one kernel command line")
    rd.add_argument("suite", help="suite TOML file")
    rd.add_argument("--task", default=None, help="task name (default: the first)")
    rd.add_argument("--boot-index", type=int, default=0)
    rd.add_argument("--campaign-id", default=None)
    rd.add_argument(
        "--seed",
        default=None,
        help="base seed, decimal or 0x. Required unless the task is seedless",
    )
    rd.add_argument(
        "--runner",
        type=int,
        default=None,
        help="runner index; with --seed, partitions the seed space so two "
        "runners never explore the same seed",
    )
    rd.add_argument("--out", default=None, help="write here instead of stdout")
    rd.set_defaults(fn=cmd_nm_render)

    bl = nmsub.add_parser("build", help="the build.sh invocation for a suite")
    bl.add_argument("suite", help="suite TOML file")
    bl.add_argument("--target", default="iso")
    bl.set_defaults(fn=cmd_nm_build)

    sh = nmsub.add_parser("show", help="the resolved suite, defaults applied")
    sh.add_argument("suite", help="suite TOML file")
    sh.set_defaults(fn=cmd_nm_show)

    rn = nmsub.add_parser("run", help="execute a nightmare campaign from a suite TOML")
    rn.add_argument("suite", help="suite TOML file")
    rn.add_argument("--runner", type=int, default=0, help="runner index (0-based)")
    rn.add_argument(
        "--total-runners",
        type=int,
        default=None,
        help="total runners (default: suite.meta.runners)",
    )
    rn.add_argument(
        "--seed",
        default=None,
        help="base seed (hex or decimal)",
    )
    rn.add_argument(
        "--campaign-id",
        default=None,
        help="campaign identifier (default: <suite>-<timestamp>)",
    )
    rn.add_argument(
        "--budget-hours",
        type=float,
        default=None,
        help="override suite budget in hours",
    )
    rn.add_argument(
        "--out-dir",
        default=None,
        help="directory where per-boot logs and reports are saved",
    )
    rn.add_argument(
        "--build-dir",
        default="build",
        help="charmOS build directory (default: build)",
    )
    rn.add_argument(
        "--gate-first",
        action=argparse.BooleanOptionalAction,
        default=None,
        help="run gate test first (default: from suite)",
    )
    rn.add_argument(
        "--dry-run",
        action="store_true",
        help="plan and simulate without running QEMU",
    )
    RP.add_report_args(rn)
    rn.set_defaults(fn=cmd_nm_run)

    pl = nmsub.add_parser(
        "plan", help="validate and place one batch against a fleet snapshot"
    )
    pl.add_argument("command", help="normalized validate_batch command JSON")
    pl.add_argument("snapshot", help="authoritative fleet snapshot JSON")
    pl.add_argument("--source-commit", required=True, help="exact 40-hex source commit")
    pl.add_argument("--runner-image", required=True, help="immutable GHCR image digest")
    pl.add_argument("--suite-dir", default="nightmare/suites")
    pl.add_argument("--ownership", choices=("ad-hoc", "repository"), default="ad-hoc")
    pl.add_argument("--out", default=None, help="accepted/rejected result JSON")
    pl.set_defaults(fn=cmd_nm_plan)

    bb = nmsub.add_parser(
        "build-bundle", help="compile one accepted build group into a bundle"
    )
    bb.add_argument("plan", help="accepted plan JSON")
    bb.add_argument("--group", required=True, help="accepted build group ID")
    bb.add_argument("--build-dir", required=True)
    bb.add_argument("--out-dir", required=True)
    bb.add_argument(
        "--receipt", default=None, help="write the verified receipt JSON here"
    )
    bb.add_argument(
        "--prebuilt",
        action="store_true",
        help="package an existing build directory without compiling (development only)",
    )
    bb.set_defaults(fn=cmd_nm_build_bundle)

    mt = nmsub.add_parser(
        "materialize", help="accepted plan plus build receipts to runner manifests"
    )
    mt.add_argument("plan", help="accepted plan JSON")
    mt.add_argument("receipts", nargs="+", help="verified bundle.json receipts")
    mt.add_argument("--out-dir", required=True)
    mt.add_argument("--attempt", type=int, default=1)
    mt.add_argument("--dry-run", action="store_true")
    mt.set_defaults(fn=cmd_nm_materialize)

    rb = nmsub.add_parser(
        "repack-bundle", help="repack a verified bundle with one kernel command line"
    )
    rb.add_argument("bundle", help="build bundle directory")
    rb.add_argument("cmdline", help="kernel command-line file")
    rb.add_argument("--out-dir", required=True)
    rb.set_defaults(fn=cmd_nm_repack_bundle)

    rc = nmsub.add_parser(
        "repository-command",
        help="create trusted planner inputs for one committed repository suite",
    )
    rc.add_argument("--suite", required=True)
    rc.add_argument("--repository", required=True)
    rc.add_argument("--ref", default="main")
    rc.add_argument("--suite-dir", default="nightmare/suites")
    rc.add_argument("--runner-capacity", type=int, default=12)
    rc.add_argument("--at", default=None, help="injected ISO-8601 clock")
    rc.add_argument("--command-out", required=True)
    rc.add_argument("--snapshot-out", required=True)
    rc.set_defaults(fn=cmd_nm_repository_command)

    ic = nmsub.add_parser(
        "inline-command",
        help="create trusted planner inputs for one inline TOML batch definition",
    )
    ic.add_argument("--toml", required=True, help="raw batch TOML definition string")
    ic.add_argument("--repository", required=True)
    ic.add_argument("--ref", default="main")
    ic.add_argument("--runner-capacity", type=int, default=12)
    ic.add_argument("--at", default=None, help="injected ISO-8601 clock")
    ic.add_argument("--command-out", required=True)
    ic.add_argument("--snapshot-out", required=True)
    ic.set_defaults(fn=cmd_nm_inline_command)

    mx = nmsub.add_parser("matrix", help="emit a bounded trusted Actions matrix")
    mx.add_argument("kind", choices=("build", "runner"))
    mx.add_argument("document")
    mx.add_argument("--limit", type=int, default=64)
    mx.set_defaults(fn=cmd_nm_matrix)

    ag = nmsub.add_parser(
        "aggregate", help="verify completeness, identity, discovery, and infrastructure"
    )
    ag.add_argument("plan", help="accepted plan result JSON")
    ag.add_argument("results_dir")
    ag.add_argument("--plan-bundle", default=None)
    ag.add_argument("--builds-dir", default=None)
    ag.add_argument("--json", required=True)
    ag.add_argument("--markdown", required=True)
    ag.add_argument("--summary", default=None)
    ag.set_defaults(fn=cmd_nm_aggregate)

    wu = nmsub.add_parser(
        "wait-until",
        help="sleep until one future ISO-8601 timestamp before execution",
    )
    wu.add_argument("--at", required=True, help="future ISO-8601 target time")
    wu.add_argument(
        "--max-wait",
        type=int,
        default=21600,
        help="maximum wait window in seconds (default 6 hours)",
    )
    wu.set_defaults(fn=cmd_nm_wait_until)

    qc = nmsub.add_parser(
        "queue-command", help="classify one accepted command for deferred execution"
    )
    qc.add_argument("command")
    qc.add_argument("--batch-id", required=True)
    qc.add_argument("--source-sha", required=True)
    qc.add_argument("--out", required=True)
    qc.add_argument("--at", default=None, help="injected ISO-8601 clock")
    qc.set_defaults(fn=cmd_nm_queue_command)

    wq = nmsub.add_parser(
        "wake-queue", help="dispatch all due artifact-backed Nightmare plans"
    )
    wq.add_argument("--repository", required=True)
    wq.add_argument("--workflow", default="nightmare-orchestrator.yml")
    wq.add_argument("--ref", default="main")
    wq.add_argument("--api-url", default="https://api.github.com")
    wq.add_argument("--token-env", default="GITHUB_TOKEN")
    wq.set_defaults(fn=cmd_nm_wake_queue)

    rm = nmsub.add_parser(
        "run-manifest",
        help="execute one accepted runner manifest and always emit a result",
    )
    rm.add_argument("manifest", help="accepted runner manifest JSON")
    _add_runner_args(rm)
    rm.set_defaults(fn=cmd_nm_run_manifest)

    rp = nmsub.add_parser(
        "replay",
        help="re-execute a manifest, prior runner result, or result directory",
    )
    rp.add_argument("source", help="manifest, runner_result.json, or result directory")
    _add_runner_args(rp)
    rp.set_defaults(fn=cmd_nm_replay)
