import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "gen_limine_conf.py"

CONF = "timeout: 0\n/charmOS\n    protocol: limine\n    path: boot():/boot/kernel\n    cmdline: root=nvme1p1\n"
CONF_NO_CMDLINE = (
    "timeout: 0\n/charmOS\n    protocol: limine\n    path: boot():/boot/kernel\n"
)


class GenLimineConfTests(unittest.TestCase):
    def setUp(self) -> None:
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.dir = Path(directory.name)

    def run_it(self, conf: str = CONF, **env: str) -> subprocess.CompletedProcess[str]:
        source = self.dir / "limine.conf"
        source.write_text(conf, encoding="utf-8")
        self.out = self.dir / "out" / "limine.conf"
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(source), str(self.out)],
            capture_output=True,
            text=True,
            check=False,
            env={"PATH": "/usr/bin:/bin", **env},
        )

    def written(self) -> str:
        return self.out.read_text(encoding="utf-8")

    def test_no_environment_copies_the_conf_verbatim(self) -> None:
        self.assertEqual(self.run_it().returncode, 0)
        self.assertEqual(self.written(), CONF)

    def test_tests_becomes_a_filter_appended_to_the_existing_cmdline(self) -> None:
        self.assertEqual(self.run_it(TESTS="avl,sort").returncode, 0)
        self.assertIn("cmdline: root=nvme1p1 test.filter=avl,sort", self.written())

    def test_a_ragged_tests_list_is_normalised(self) -> None:
        self.assertEqual(self.run_it(TESTS="avl,  sort  ,hash").returncode, 0)
        self.assertIn("test.filter=avl,sort,hash", self.written())

    def test_a_trailing_comma_in_tests_is_refused(self) -> None:
        result = self.run_it(TESTS="avl,")
        self.assertEqual(result.returncode, 1)
        self.assertIn("trailing comma", result.stderr)

    def test_nightmare_tests_takes_exactly_one_name(self) -> None:
        self.assertEqual(self.run_it(NIGHTMARE_TESTS="locks").returncode, 0)
        self.assertIn("nightmare=locks", self.written())

        result = self.run_it(NIGHTMARE_TESTS="locks,wake")
        self.assertEqual(result.returncode, 1)
        self.assertIn("exactly one test name", result.stderr)

    def test_a_cmdline_file_is_flattened_onto_one_line(self) -> None:
        cmdline = self.dir / "cmdline.txt"
        cmdline.write_text("nightmare=locks\nseed=0x1\n", encoding="utf-8")

        self.assertEqual(self.run_it(CMDLINE=str(cmdline)).returncode, 0)
        self.assertIn(
            "cmdline: root=nvme1p1 nightmare=locks seed=0x1\n", self.written()
        )

    def test_a_blank_cmdline_file_is_refused(self) -> None:
        cmdline = self.dir / "cmdline.txt"
        cmdline.write_text("\n  \n", encoding="utf-8")

        result = self.run_it(CMDLINE=str(cmdline))
        self.assertEqual(result.returncode, 1)
        self.assertIn("is empty", result.stderr)

    def test_a_missing_cmdline_file_is_refused(self) -> None:
        result = self.run_it(CMDLINE=str(self.dir / "nope.txt"))
        self.assertEqual(result.returncode, 1)
        self.assertIn("does not exist", result.stderr)

    def test_two_sources_together_are_refused(self) -> None:
        result = self.run_it(TESTS="avl", NIGHTMARE_TESTS="locks")
        self.assertEqual(result.returncode, 1)
        self.assertIn("set together", result.stderr)

    def test_extra_cmdline_composes_with_a_source(self) -> None:
        self.assertEqual(
            self.run_it(TESTS="avl", EXTRA_CMDLINE="debug=1").returncode, 0
        )
        self.assertIn("test.filter=avl debug=1", self.written())

    def test_extra_cmdline_alone_still_lands(self) -> None:
        self.assertEqual(self.run_it(EXTRA_CMDLINE="debug=1").returncode, 0)
        self.assertIn("cmdline: root=nvme1p1 debug=1", self.written())

    def test_a_conf_without_a_cmdline_line_gets_one_after_path(self) -> None:
        self.assertEqual(self.run_it(CONF_NO_CMDLINE, TESTS="avl").returncode, 0)
        self.assertIn(
            "    path: boot():/boot/kernel\n    cmdline: test.filter=avl",
            self.written(),
        )

    def test_wrong_arity_is_a_usage_error(self) -> None:
        result = subprocess.run(
            [sys.executable, str(SCRIPT)], capture_output=True, text=True, check=False
        )
        self.assertEqual(result.returncode, 2)


if __name__ == "__main__":
    unittest.main()
