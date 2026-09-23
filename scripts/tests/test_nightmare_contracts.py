"""Runner-manifest validation against the checked-in contract fixtures."""

import copy
import json

import pytest

from charm.nightmare import contracts
from charm.paths import nightmare_dir

FIXTURES = nightmare_dir() / "fixtures" / "contracts"


def load(name: str) -> dict:
    return json.loads((FIXTURES / name).read_text(encoding="utf-8"))


def paths(document: dict) -> set[str]:
    return {item.path for item in contracts.validate_manifest(document)}


@pytest.mark.parametrize(
    "name", ["valid/runner_manifest.json", "real/p7_m3_short_manifest.json"]
)
def test_checked_in_manifests_are_accepted(name: str) -> None:
    assert contracts.validate_manifest(load(name)) == []


def test_invalid_fixture_names_missing_and_unknown_keys() -> None:
    found = paths(load("invalid/runner_manifest.json"))

    assert "manifest.campaign" in found
    assert "manifest.unexpected" in found


@pytest.mark.parametrize(
    ("section", "field", "value", "reported"),
    [
        ("suite", "sha256", "0" * 64, "manifest.suite.sha256"),
        ("campaign", "runner_index", 99, "manifest.campaign.runner_index"),
        ("campaign", "hard_budget_ms", 1, "manifest.campaign.hard_budget_ms"),
        ("build", "runner_image", "ubuntu:latest", "manifest.build.runner_image"),
    ],
)
def test_cross_field_and_format_rules_reject(
    section: str, field: str, value: object, reported: str
) -> None:
    document = copy.deepcopy(load("valid/runner_manifest.json"))
    document[section][field] = value

    assert reported in paths(document)
    with pytest.raises(contracts.ContractError):
        contracts.manifest_from_dict(document)
