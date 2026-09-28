import unittest

from charm.nightmare import codec as C
from charm.nightmare import suite as S
from charm.paths import nightmare_dir

SUITES = nightmare_dir() / "suites"
FIXTURES = nightmare_dir() / "fixtures" / "suites"


def tokens(line: str) -> dict[str, str]:
    return dict(tok.split("=", 1) for tok in line.split(" "))


class RenderTests(unittest.TestCase):
    def setUp(self) -> None:
        self.suite = S.load(SUITES / "overnight_locks.toml")
        self.task = self.suite.task("locks_storm")

    def render(self, **kw) -> dict[str, str]:
        kw.setdefault("seed", 0xDEADBEEF)
        return tokens(C.render(self.task, C.BootRequest(**kw)))

    def test_selection_comes_first_and_is_the_bare_root(self) -> None:
        line = C.render(self.task, C.BootRequest(seed=1))

        self.assertTrue(line.startswith("nightmare=locks_storm "))

    def test_durations_carry_their_unit(self) -> None:
        t = self.render()

        self.assertEqual(t["nightmare.duration_ms"], "300000ms")
        self.assertEqual(t["nightmare.drain_grace_ms"], "20000ms")
        self.assertEqual(t["nightmare.stall_threshold_ms"], "3000ms")
        self.assertEqual(t["nightmare.perturb.migrator.interval_us"], "500us")
        self.assertEqual(t["nightmare.locks_storm.worker_stall_ms"], "10000ms")

    def test_the_perturb_list_is_comma_separated(self) -> None:
        t = self.render()

        self.assertEqual(t["nightmare.perturb"], "migrator,waker,stutter")

    def test_identity_is_passed_through_untouched(self) -> None:
        t = self.render(boot_index=12, campaign_id="run-9:2")

        self.assertEqual(t["nightmare.boot_index"], "12")
        self.assertEqual(t["nightmare.campaign_id"], "run-9:2")

    def test_campaign_id_is_absent_rather_than_empty_when_unset(self) -> None:
        t = self.render()

        self.assertNotIn("nightmare.campaign_id", t)

    def test_rendering_is_deterministic(self) -> None:
        req = C.BootRequest(boot_index=1, seed=5)

        self.assertEqual(C.render(self.task, req), C.render(self.task, req))

    def test_nightmare_never_nests_under_test(self) -> None:
        line = C.render(self.task, C.BootRequest(seed=1))

        self.assertNotIn("test.", line)

    def test_a_campaign_id_the_cmdline_cannot_carry_is_refused(self) -> None:
        with self.assertRaises(C.CodecError):
            C.render(self.task, C.BootRequest(seed=1, campaign_id="two words"))


class SeedTests(unittest.TestCase):
    def setUp(self) -> None:
        self.split = S.load(SUITES / "overnight_locks.toml").task("locks_storm")
        self.seedless = S.load(FIXTURES / "valid" / "seedless.toml").tasks[0]

    def test_a_seeded_mode_refuses_to_render_without_a_seed(self) -> None:
        with self.assertRaises(C.CodecError) as cm:
            C.render(self.split, C.BootRequest())

        self.assertIn("requires a seed", str(cm.exception))

    def test_a_seedless_task_refuses_a_seed(self) -> None:
        with self.assertRaises(C.CodecError) as cm:
            C.render(self.seedless, C.BootRequest(seed=1))

        self.assertIn("seedless", str(cm.exception))

    def test_a_seedless_task_emits_no_seed(self) -> None:
        t = tokens(C.render(self.seedless, C.BootRequest()))

        self.assertNotIn("nightmare.seed", t)
        self.assertEqual(t["nightmare.seed_mode"], "seedless")

    def test_a_seed_is_hex_so_strtoull_cannot_read_it_as_octal(self) -> None:
        t = tokens(C.render(self.split, C.BootRequest(seed=0x1F)))

        self.assertEqual(t["nightmare.seed"], "0x000000000000001f")
