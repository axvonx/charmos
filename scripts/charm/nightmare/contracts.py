"""Versioned data contracts at the Nightmare execution seam."""

import hashlib
import json
import re
from collections.abc import Callable
from dataclasses import dataclass
from enum import StrEnum
from pathlib import Path
from typing import Any, NewType, TypeAlias

from . import grammar
from . import suite as suite_model

SCHEMA_VERSION = 1

ManifestId = NewType("ManifestId", str)
PlanId = NewType("PlanId", str)
BatchId = NewType("BatchId", str)
TaskId = NewType("TaskId", str)
BuildId = NewType("BuildId", str)
CommandId = NewType("CommandId", str)
RunnerId = NewType("RunnerId", str)
SnapshotVersion = NewType("SnapshotVersion", str)

ID_RE = re.compile(r"^[a-z][a-z0-9_:-]{0,127}$")
REPOSITORY_RE = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
IMAGE_RE = re.compile(
    r"^ghcr\.io/[A-Za-z0-9_.-]+/[A-Za-z0-9_./-]+@sha256:[0-9a-f]{64}$"
)
ARTIFACT_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$")


class DiscoveryKind(StrEnum):
    NONE = "none"
    FINDING = "finding"
    CRASH = "crash"
    STALL = "stall"
    MIXED = "mixed"


class ExecutionHealth(StrEnum):
    HEALTHY = "healthy"
    INFRASTRUCTURE = "infrastructure"
    PARTIAL = "partial"


class ExecutionLifecycle(StrEnum):
    COMPLETED = "completed"
    FAILED = "failed"
    CANCELLED = "cancelled"


@dataclass(frozen=True)
class Diagnostic:
    path: str
    message: str

    def __str__(self) -> str:
        return f"{self.path}: {self.message}"


class ContractError(ValueError):
    def __init__(self, source: str, diagnostics: list[Diagnostic]):
        self.source = source
        self.diagnostics = diagnostics
        body = "\n".join(f"  {diagnostic}" for diagnostic in diagnostics)
        super().__init__(f"{source}: {len(diagnostics)} problem(s)\n{body}")


@dataclass(frozen=True)
class SourceIdentity:
    repository: str
    commit: str


@dataclass(frozen=True)
class SuiteIdentity:
    id: str
    sha256: str
    resolved: dict[str, Any]
    model: suite_model.Suite


@dataclass(frozen=True)
class BuildIdentity:
    bundle_id: BuildId
    sha256: str
    runner_image: str


@dataclass(frozen=True)
class CampaignContract:
    campaign_id: str
    runner_index: int
    total_runners: int
    base_seed: int | None
    soft_budget_ms: int
    hard_budget_ms: int
    actions_job_budget_ms: int
    gate_first: bool
    dry_run: bool


@dataclass(frozen=True)
class ResultTarget:
    schema_version: int
    artifact_name: str


@dataclass(frozen=True)
class RunnerManifest:
    schema_version: int
    manifest_id: ManifestId
    plan_id: PlanId
    batch_id: BatchId
    task_id: TaskId
    attempt: int
    source: SourceIdentity
    suite: SuiteIdentity
    build: BuildIdentity
    campaign: CampaignContract
    result: ResultTarget
    document: dict[str, Any]


def canonical_json(value: Any) -> bytes:
    """The byte representation used by contract digests."""
    return json.dumps(
        value,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")


def sha256_json(value: Any) -> str:
    return hashlib.sha256(canonical_json(value)).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def suite_to_dict(suite: suite_model.Suite) -> dict[str, Any]:
    """Resolve defaults so a manifest is independent of loader evolution."""
    return {
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
            "smp": {
                "sockets": suite.build.smp.sockets,
                "cores": suite.build.smp.cores,
                "threads": suite.build.smp.threads,
            },
            "memory_mib": suite.build.memory_mib,
        },
        "tasks": [
            {
                "name": task.name,
                "mode": task.mode,
                "weight": task.weight,
                "priority": task.priority,
                **(
                    {"max_runners": task.max_runners}
                    if task.max_runners is not None
                    else {}
                ),
                "boot": {
                    "duration_ms": task.boot.duration_ms,
                    "drain_grace_ms": task.boot.drain_grace_ms,
                    "timeout_ms": task.boot.timeout_ms,
                    "gate_first": task.boot.gate_first,
                    "max_boots": task.boot.max_boots,
                    "min_interval_ms": task.boot.min_interval_ms,
                    "stat_interval_ms": task.boot.stat_interval_ms,
                    "stall_threshold_ms": task.boot.stall_threshold_ms,
                    "on_stall": task.boot.on_stall,
                },
                "nightmare": {
                    "intensity": task.nightmare.intensity,
                    "seed_mode": task.nightmare.seed_mode,
                    "perturb": list(task.nightmare.perturb),
                    "perturb_opts": task.nightmare.perturb_opts,
                    "opts": task.nightmare.opts,
                },
            }
            for task in suite.tasks
        ],
    }


# A shape maps each required key to a nested shape or to a rule that returns
# an error message (or None) for the value. Keys outside the shape are rejected.
Rule: TypeAlias = Callable[[Any], str | None]
Shape: TypeAlias = dict[str, "Shape | Rule"]


def string(pattern: re.Pattern[str] | None = None) -> Rule:
    def rule(value: Any) -> str | None:
        if not isinstance(value, str):
            return "expected a string"
        if pattern is not None and not pattern.fullmatch(value):
            return "has an invalid format"
        return None

    return rule


def integer(minimum: int = 0) -> Rule:
    def rule(value: Any) -> str | None:
        if type(value) is not int:
            return "expected an integer"
        if value < minimum:
            return f"must be at least {minimum}"
        return None

    return rule


def boolean(value: Any) -> str | None:
    return None if isinstance(value, bool) else "expected a boolean"


def constant(expected: Any) -> Rule:
    return lambda value: None if value == expected else f"must be {expected!r}"


def any_object(value: Any) -> str | None:
    return None if isinstance(value, dict) else "expected an object"


def _base_seed(value: Any) -> str | None:
    if value is None:
        return None
    if not isinstance(value, str):
        return "expected a string or null"
    try:
        grammar.parse_uint(value)
    except grammar.GrammarError as error:
        return str(error)
    return None


def check_shape(value: Any, shape: Shape, path: str) -> list[Diagnostic]:
    if not isinstance(value, dict):
        return [Diagnostic(path, "expected an object")]
    diagnostics = [
        Diagnostic(f"{path}.{key}", "is required")
        for key in sorted(set(shape) - set(value))
    ]
    diagnostics += [
        Diagnostic(f"{path}.{key}", "is not allowed")
        for key in sorted(set(value) - set(shape))
    ]
    for key, rule in shape.items():
        if key not in value:
            continue
        if isinstance(rule, dict):
            diagnostics += check_shape(value[key], rule, f"{path}.{key}")
        elif (message := rule(value[key])) is not None:
            diagnostics.append(Diagnostic(f"{path}.{key}", message))
    return diagnostics


MANIFEST_SHAPE: Shape = {
    "schema_version": constant(SCHEMA_VERSION),
    "manifest_id": string(ID_RE),
    "plan_id": string(ID_RE),
    "batch_id": string(ID_RE),
    "task_id": string(ID_RE),
    "attempt": integer(minimum=1),
    "source": {"repository": string(REPOSITORY_RE), "commit": string(COMMIT_RE)},
    "suite": {"id": string(ID_RE), "sha256": string(SHA256_RE), "resolved": any_object},
    "build": {
        "bundle_id": string(ID_RE),
        "sha256": string(SHA256_RE),
        "runner_image": string(IMAGE_RE),
    },
    "campaign": {
        "campaign_id": string(ID_RE),
        "runner_index": integer(),
        "total_runners": integer(minimum=1),
        "base_seed": _base_seed,
        "soft_budget_ms": integer(minimum=1),
        "hard_budget_ms": integer(minimum=1),
        "actions_job_budget_ms": integer(minimum=1),
        "gate_first": boolean,
        "dry_run": boolean,
    },
    "result": {
        "schema_version": constant(SCHEMA_VERSION),
        "artifact_name": string(ARTIFACT_RE),
    },
}


def validate_manifest(document: Any) -> list[Diagnostic]:
    diagnostics = check_shape(document, MANIFEST_SHAPE, "manifest")
    if diagnostics:
        return diagnostics

    suite = document["suite"]
    campaign = document["campaign"]
    if sha256_json(suite["resolved"]) != suite["sha256"]:
        diagnostics.append(
            Diagnostic(
                "manifest.suite.sha256", "does not match the canonical resolved suite"
            )
        )
    if campaign["runner_index"] >= campaign["total_runners"]:
        diagnostics.append(
            Diagnostic(
                "manifest.campaign.runner_index", "must be less than total_runners"
            )
        )
    if not campaign["soft_budget_ms"] < campaign["hard_budget_ms"]:
        diagnostics.append(
            Diagnostic(
                "manifest.campaign.hard_budget_ms",
                "must be greater than soft_budget_ms",
            )
        )
    if not campaign["hard_budget_ms"] < campaign["actions_job_budget_ms"]:
        diagnostics.append(
            Diagnostic(
                "manifest.campaign.actions_job_budget_ms",
                "must be greater than hard_budget_ms",
            )
        )

    try:
        model = suite_model.from_dict(
            suite["resolved"], source="manifest.suite.resolved"
        )
    except suite_model.SuiteError as error:
        diagnostics.extend(
            Diagnostic(f"manifest.suite.resolved.{item.path}", item.message)
            for item in error.diagnostics
        )
        return diagnostics
    if model.meta.name != suite["id"]:
        diagnostics.append(
            Diagnostic(
                "manifest.suite.id",
                f"does not match resolved suite name {model.meta.name!r}",
            )
        )
    required = max(task.boot.host_timeout_ms for task in model.tasks)
    if required > campaign["soft_budget_ms"]:
        diagnostics.append(
            Diagnostic(
                "manifest.campaign.soft_budget_ms",
                f"must fit one host boot timeout ({required}ms)",
            )
        )
    return diagnostics


def manifest_from_dict(document: Any, source: str = "<memory>") -> RunnerManifest:
    diagnostics = validate_manifest(document)
    if diagnostics:
        raise ContractError(source, diagnostics)
    source_doc = document["source"]
    suite_doc = document["suite"]
    build_doc = document["build"]
    campaign_doc = document["campaign"]
    result_doc = document["result"]
    resolved = suite_doc["resolved"]
    model = suite_model.from_dict(resolved, source=f"{source}:suite")
    base_seed = campaign_doc["base_seed"]
    return RunnerManifest(
        schema_version=SCHEMA_VERSION,
        manifest_id=ManifestId(document["manifest_id"]),
        plan_id=PlanId(document["plan_id"]),
        batch_id=BatchId(document["batch_id"]),
        task_id=TaskId(document["task_id"]),
        attempt=document["attempt"],
        source=SourceIdentity(source_doc["repository"], source_doc["commit"]),
        suite=SuiteIdentity(suite_doc["id"], suite_doc["sha256"], resolved, model),
        build=BuildIdentity(
            BuildId(build_doc["bundle_id"]),
            build_doc["sha256"],
            build_doc["runner_image"],
        ),
        campaign=CampaignContract(
            campaign_id=campaign_doc["campaign_id"],
            runner_index=campaign_doc["runner_index"],
            total_runners=campaign_doc["total_runners"],
            base_seed=(grammar.parse_uint(base_seed) if base_seed else None),
            soft_budget_ms=campaign_doc["soft_budget_ms"],
            hard_budget_ms=campaign_doc["hard_budget_ms"],
            actions_job_budget_ms=campaign_doc["actions_job_budget_ms"],
            gate_first=campaign_doc["gate_first"],
            dry_run=campaign_doc["dry_run"],
        ),
        result=ResultTarget(result_doc["schema_version"], result_doc["artifact_name"]),
        document=document,
    )


def load_manifest(path: Path) -> RunnerManifest:
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise ContractError(
            str(path), [Diagnostic("manifest", f"invalid JSON: {error}")]
        ) from None
    except OSError as error:
        raise ContractError(
            str(path), [Diagnostic("manifest", f"cannot read: {error}")]
        ) from None
    return manifest_from_dict(document, source=str(path))
