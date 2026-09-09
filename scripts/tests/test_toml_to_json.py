import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "toml_to_json.py"


def run(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        capture_output=True,
        text=True,
        check=False,
    )


class TomlToJsonTests(unittest.TestCase):
    def toml(self, text: str) -> str:
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name) / "tests.toml"
        path.write_text(text, encoding="utf-8")
        return str(path)

    def test_a_scalar_document_round_trips(self) -> None:
        path = self.toml('name = "slab"\ndefault = false\n')
        result = run(path)

        self.assertEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stdout), {"name": "slab", "default": False})

    def test_arrays_and_nested_tables_survive(self) -> None:
        path = self.toml('deps = ["iommu", "smp"]\n[limits]\nmax_tier = 2\n')
        result = run(path)

        self.assertEqual(result.returncode, 0)
        self.assertEqual(
            json.loads(result.stdout),
            {"deps": ["iommu", "smp"], "limits": {"max_tier": 2}},
        )

    def test_a_parse_error_reports_line_and_column_not_a_traceback(self) -> None:
        path = self.toml('name = "unterminated\n')
        result = run(path)

        self.assertEqual(result.returncode, 1)
        self.assertNotIn("Traceback", result.stderr)
        self.assertIn("line 1", result.stderr)

    def test_a_missing_file_fails_without_a_traceback(self) -> None:
        result = run("/nonexistent/tests.toml")

        self.assertEqual(result.returncode, 1)
        self.assertNotIn("Traceback", result.stderr)

    def test_wrong_arity_is_a_usage_error_distinct_from_a_parse_error(self) -> None:
        self.assertEqual(run().returncode, 2)


if __name__ == "__main__":
    unittest.main()
