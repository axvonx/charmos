"""`nightmare/schemas/suite-v1.schema.json`

charmos-ci generates its Zod schema from that file

two failure modes:

  drift      -- the schema's vocabulary and the Python model's disagree
  vacuity    -- a schema that accepts everything passes every positive test

Skipped when `jsonschema` is not installed
"""

import json
import unittest

from charm.nightmare import suite as S

SCHEMA = json.loads(S.SCHEMA_PATH.read_text())


def base(**overrides) -> dict:
    doc = {
        "suite": {"name": "x", "runners": 1, "budget_hours": 1.0},
        "build": {},
        "tasks": [{"name": "t", "boot": {"duration_ms": 10000}}],
    }
    doc.update(overrides)
    return doc


def enum(*path: str) -> tuple:
    node = SCHEMA["$defs"]
    for key in path:
        node = node[key]
    return tuple(node["enum"])


class VocabularyTests(unittest.TestCase):
    def test_every_enum_matches_the_model(self) -> None:
        for schema_path, model in (
            (("service",), S.SERVICES),
            (("build", "properties", "compiler"), S.COMPILERS),
            (("build", "properties", "type"), S.BUILD_TYPES),
            (("task", "properties", "mode"), S.MODES),
            (("nightmare", "properties", "seed_mode"), S.SEED_MODES),
            (("boot", "properties", "on_stall"), S.ON_STALL),
        ):
            with self.subTest(enum=".".join(schema_path)):
                self.assertEqual(enum(*schema_path), model)

    def test_schema_defaults_match_the_dataclass_defaults(self) -> None:
        for section, model in (
            ("boot", S.Boot(duration_ms=1)),
            ("nightmare", S.Nightmare()),
        ):
            for field, spec in SCHEMA["$defs"][section]["properties"].items():
                if "default" in spec:
                    with self.subTest(field=f"{section}.{field}"):
                        self.assertEqual(
                            json.loads(json.dumps(getattr(model, field))),
                            spec["default"],
                        )


def edit(path: str, value: object) -> dict:
    """base() with `value` set at a dotted path; DELETE removes the key"""
    doc = base()
    *parents, leaf = path.split(".")
    node: dict | list = doc
    for key in parents:
        node = node[int(key)] if isinstance(node, list) else node[key]
    if value is DELETE:
        del node[leaf]
    else:
        node[leaf] = value
    return doc


DELETE = object()

REJECTIONS = [
    ("unknown top-level key", base(surprise={}), "<root>"),
    ("unknown suite key", edit("suite.budget_days", 3), "suite"),
    ("unknown boot key", edit("tasks.0.boot.duration_sec", 3), "tasks[0].boot"),
    ("capitalised name", edit("suite.name", "Overnight_Locks"), "suite.name"),
    ("zero runners", edit("suite.runners", 0), "suite.runners"),
    (
        "intensity above one",
        edit("tasks.0.nightmare", {"intensity": 1.5}),
        "tasks[0].nightmare.intensity",
    ),
    (
        "unknown service",
        edit("tasks.0.nightmare", {"perturb": ["teleporter"]}),
        "tasks[0].nightmare.perturb[0]",
    ),
    (
        "duplicated service",
        edit("tasks.0.nightmare", {"perturb": ["migrator", "migrator"]}),
        "tasks[0].nightmare.perturb",
    ),
    ("empty task list", base(tasks=[]), "tasks"),
    (
        "cmake definition without a value",
        edit("build.cmake_definitions", ["DEBUG_ASAN"]),
        "build.cmake_definitions[0]",
    ),
    ("missing required field", edit("suite.runners", DELETE), "suite"),
    (
        "string where a number belongs",
        edit("tasks.0.boot.duration_ms", "10s"),
        "tasks[0].boot.duration_ms",
    ),
]


@unittest.skipUnless(S.SCHEMA_AVAILABLE, "jsonschema is not installed")
class RejectionTests(unittest.TestCase):
    def test_the_schema_rejects_each_bad_document_at_its_path(self) -> None:
        for why, doc, expect_path in REJECTIONS:
            with self.subTest(why):
                diags = S.schema_diagnostics(doc)
                self.assertIn(
                    expect_path, [d.path for d in diags], f"schema accepted {doc}"
                )


if __name__ == "__main__":
    unittest.main()
