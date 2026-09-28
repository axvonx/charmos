from datetime import UTC, datetime, timedelta

import pytest

from charm.nightmare import workflow
from charm.paths import repo_root

WORKFLOWS = repo_root() / ".github" / "workflows"
NOW = datetime(2026, 9, 1, 12, 0, tzinfo=UTC)


def inline(start: str, tests: str = "harness_smoke", runners: int = 1, **kw):
    return workflow.inline_command(
        toml_text=f"""[batch]
name = "Batch"
start_utc = "{start}"
window_hours = 1
runners = {runners}
color = "#a7c080"
tests = ["{tests}"]
""",
        repository="axvonx/charmos",
        ref="main",
        now=NOW,
        **kw,
    )


def queue(command: dict, batch_id: str) -> dict:
    return workflow.queue_metadata(
        command, batch_id=batch_id, source_sha="abc123", now=NOW
    )


def test_inline_command_preserves_one_atomic_batch() -> None:
    command, snapshot = inline(
        "2026-09-01T12:05:00Z", "overnight_locks", runners=2, runner_capacity=12
    )

    assert command["operation"] == "validate_batch"
    assert command["payload"]["definition"]["tests"] == ["overnight_locks"]
    assert command["payload"]["definition"]["runners"] == 2
    assert snapshot["batches"] == []


def test_workflow_dispatch_is_batch_scoped_and_validates_before_queueing() -> None:
    text = (WORKFLOWS / "nightmare-orchestrator.yml").read_text()
    assert "batch_id:" in text
    assert "claim-{1}" in text
    assert text.index("- name: Validate and place") < text.index(
        "- name: Classify execution"
    )
    assert "ownership=ad-hoc" in text
    assert "wait-until" not in text
    assert "queued_run_id:" in text
    assert "name: nightmare-queue" in text
    assert text.count("needs.plan.outputs.deferred != 'true'") == 2


def test_materialize_finds_receipts_in_any_artifact_layout() -> None:
    text = (WORKFLOWS / "nightmare-orchestrator.yml").read_text()
    assert "builds/*/receipt.json" not in text
    assert "find builds -name receipt.json" in text


def test_future_inline_command_becomes_durable_queue_metadata() -> None:
    command, _ = inline((NOW + timedelta(minutes=20)).isoformat())

    metadata = queue(command, "batch-later")

    assert metadata["deferred"] is True
    assert metadata["batch_id"] == "batch-later"
    assert metadata["source_sha"] == "abc123"


def test_due_inline_command_executes_without_queueing() -> None:
    command, _ = inline(NOW.isoformat())

    metadata = queue(command, "batch-now")

    assert metadata["deferred"] is False


def test_deferred_command_cannot_outlive_its_queue_artifact() -> None:
    command, _ = inline((NOW + timedelta(days=30)).isoformat())

    with pytest.raises(ValueError, match="29-day queue horizon"):
        queue(command, "batch-too-far")
