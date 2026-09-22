import unittest
from dataclasses import dataclass, field
from pathlib import Path
from subprocess import CompletedProcess
from types import SimpleNamespace
from unittest.mock import patch

from charm.nightmare import campaign as C
from charm.nightmare import codec
from charm.nightmare import suite as S
from charm.paths import nightmare_dir

SUITES = nightmare_dir() / "suites"
FIXTURES = nightmare_dir() / "fixtures" / "suites"

GATE_SEED = 0xFEEDFACE


def tokens(line: str) -> dict[str, str]:
    return dict(tok.split("=", 1) for tok in line.split(" "))


def limine_would_accept(text: str) -> bool:
    return bool(text.replace("\r", " ").replace("\n", " ").strip())


@dataclass
class RecordingBootRunner:
    status: str = C.BootStatus.OK.value
    calls: list[dict] = field(default_factory=list)

    def run_boot(
        self,
        manifest: C.CampaignManifest,
        task: S.Task,
        boot_index: int,
        cmdline: str,
        timeout_ms: int,
        out_dir: Path,
    ) -> C.BootResult:
        self.calls.append(
            {
                "task": task,
                "boot_index": boot_index,
                "cmdline": cmdline,
                "timeout_ms": timeout_ms,
                "out_dir": out_dir,
            }
        )
        return C.BootResult(
            boot_index=boot_index,
            task_name=task.name,
            cmdline=cmdline,
            seed=None,
            duration_ms=task.boot.duration_ms,
            exit_code=0,
            status=self.status,
            reason="stub",
            progress=1,
            findings=[],
        )


class GateCmdlineTests(unittest.TestCase):
    def setUp(self) -> None:
        self.task = S.load(SUITES / "overnight_locks.toml").task("locks_storm")

    def render(self) -> str:
        return codec.render_gate(self.task, base_seed=GATE_SEED)

    def test_the_gate_cmdline_is_never_empty(self) -> None:
        self.assertTrue(limine_would_accept(self.render()))

    def test_the_written_gate_file_is_one_limine_would_accept(self) -> None:
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            path = codec.write(Path(tmp) / "gate" / "cmdline.txt", self.render())
            self.assertTrue(limine_would_accept(path.read_text()))

    def test_the_gate_runs_the_same_subject_as_the_campaign(self) -> None:
        self.assertEqual(tokens(self.render())["nightmare"], "locks_storm")

    def test_the_gate_runs_briefly(self) -> None:
        t = tokens(self.render())

        self.assertEqual(t["nightmare.duration_ms"], f"{codec.GATE_DURATION_MS}ms")
        self.assertLess(codec.GATE_DURATION_MS, self.task.boot.duration_ms)

    def test_the_gate_keeps_the_task_perturbers_and_options(self) -> None:
        t = tokens(self.render())

        self.assertEqual(t["nightmare.perturb"], "migrator,waker,stutter")
        self.assertEqual(t["nightmare.locks_storm.worker_stall_ms"], "10000ms")

    def test_the_gate_timeout_is_derived_from_the_gate_duration(self) -> None:
        gate = codec.gate_task(self.task)

        self.assertLess(gate.boot.host_timeout_ms, self.task.boot.host_timeout_ms)
        self.assertEqual(
            gate.boot.host_timeout_ms,
            codec.GATE_DURATION_MS + gate.boot.drain_grace_ms + S.FLUSH_MARGIN_MS,
        )

    def test_a_seeded_gate_carries_the_campaign_base_seed(self) -> None:
        self.assertEqual(tokens(self.render())["nightmare.seed"], "0x00000000feedface")

    def test_a_seedless_gate_renders_without_a_seed(self) -> None:
        seedless = S.load(FIXTURES / "valid" / "seedless.toml").tasks[0]
        line = codec.render_gate(seedless, base_seed=GATE_SEED)

        self.assertTrue(limine_would_accept(line))
        self.assertNotIn("nightmare.seed=", line)


class GateDefaultTests(unittest.TestCase):
    def test_gating_is_on_unless_a_suite_opts_out(self) -> None:
        self.assertTrue(S.Boot(duration_ms=1000).gate_first)


class GateExecutionTests(unittest.TestCase):
    def manifest(self, tmp: Path, **kw) -> C.CampaignManifest:
        return C.CampaignManifest(
            suite=S.load(SUITES / "overnight_locks.toml"),
            base_seed=GATE_SEED,
            campaign_id="test-campaign",
            out_dir=tmp,
            **{"budget_ms": 1, **kw},  # default: no room for a real boot
        )

    def test_the_gate_boot_runs_first_and_into_its_own_directory(self) -> None:
        import tempfile

        runner = RecordingBootRunner()
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            C.CampaignRunner(self.manifest(out), boot_runner=runner).execute()

        self.assertTrue(runner.calls)
        gate = runner.calls[0]
        self.assertEqual(gate["boot_index"], 0)
        self.assertEqual(gate["out_dir"], out / "gate")
        self.assertTrue(limine_would_accept(gate["cmdline"]))

    def test_the_gate_boot_is_given_the_gate_timeout_not_the_task_timeout(
        self,
    ) -> None:
        import tempfile

        runner = RecordingBootRunner()
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            manifest = self.manifest(out)
            C.CampaignRunner(manifest, boot_runner=runner).execute()

        real = manifest.suite.task("locks_storm").boot.host_timeout_ms
        self.assertEqual(
            runner.calls[0]["timeout_ms"],
            codec.gate_task(manifest.suite.task("locks_storm")).boot.host_timeout_ms,
        )
        self.assertLess(runner.calls[0]["timeout_ms"], real)

    def test_a_gate_the_rig_could_not_run_stops_the_campaign(self) -> None:
        import tempfile

        runner = RecordingBootRunner(status=C.BootStatus.TIMEOUT.value)
        with tempfile.TemporaryDirectory() as tmp:
            result = C.CampaignRunner(
                self.manifest(Path(tmp)), boot_runner=runner
            ).execute()

        self.assertEqual(len(runner.calls), 1)
        self.assertFalse(result.ok)
        self.assertEqual(result.status, C.CampaignStatus.INFRASTRUCTURE.value)
        self.assertTrue(result.gated_out)


class QemuInfrastructureRetryTests(unittest.TestCase):
    def test_a_signal_killed_qemu_is_infrastructure(self) -> None:
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            console = root / "console.log"
            machine = root / "machine.nd.log"
            console.write_text("", encoding="utf-8")
            machine.write_text("", encoding="utf-8")

            result = C._result_from_logs(
                boot_index=4,
                task_name="wake_storm",
                cmdline="nightmare=wake_storm",
                duration_ms=15,
                exit_code=-11,
                timed_out=False,
                console_log_path=console,
                machine_log_path=machine,
            )

        self.assertEqual(result.status, C.BootStatus.INFRA.value)
        self.assertEqual(result.reason, "qemu_sigsegv")
        self.assertTrue(C._is_retryable_qemu_crash(result))

    def test_guest_panic_evidence_is_not_retryable(self) -> None:
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            console = root / "console.log"
            machine = root / "machine.nd.log"
            console.write_text("", encoding="utf-8")
            machine.write_text('{"s":"panic","k":"at"}\n', encoding="utf-8")

            result = C._result_from_logs(
                boot_index=4,
                task_name="wake_storm",
                cmdline="nightmare=wake_storm",
                duration_ms=15,
                exit_code=-11,
                timed_out=False,
                console_log_path=console,
                machine_log_path=machine,
            )

        self.assertEqual(result.status, C.BootStatus.CRASH.value)
        self.assertFalse(C._is_retryable_qemu_crash(result))

    def test_bundle_runner_retries_once_and_preserves_attempts(self) -> None:
        import tempfile

        suite = S.load(SUITES / "overnight_wake.toml")
        task = suite.tasks[0]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            pristine = root / "pristine.img"
            pristine.write_text("disk", encoding="utf-8")
            iso = root / "image.iso"
            iso.write_text("iso", encoding="utf-8")
            bundle = SimpleNamespace(pristine_disk=pristine)
            runner = C.BundleBootRunner(bundle, root)
            manifest = C.CampaignManifest(
                suite=suite,
                campaign_id="test-campaign",
                out_dir=root / "results",
            )
            calls = 0

            def run(command: list[str], **_kwargs: object) -> CompletedProcess[str]:
                nonlocal calls
                calls += 1
                return CompletedProcess(
                    command,
                    -11 if calls == 1 else 1,
                    f"attempt {calls}\n",
                    "",
                )

            with (
                patch.object(C.subprocess, "run", side_effect=run),
                patch(
                    "charm.nightmare.build_bundle.repack",
                    return_value=SimpleNamespace(iso_path=iso),
                ) as repack,
                patch(
                    "charm.nightmare.build_bundle.qemu_command",
                    return_value=["qemu-system-x86_64"],
                ),
            ):
                result = runner.run_boot(
                    manifest=manifest,
                    task=task,
                    boot_index=59,
                    cmdline="nightmare=wake_storm",
                    timeout_ms=1000,
                    out_dir=manifest.out_dir,
                )

            first = manifest.out_dir / "boot-0059" / "attempts" / "attempt-01"
            second = manifest.out_dir / "boot-0059" / "attempts" / "attempt-02"
            self.assertEqual(calls, 2)
            self.assertTrue(result.ok)
            self.assertTrue(result.recovered_infrastructure)
            self.assertEqual(
                [attempt.status for attempt in result.attempts], ["infra", "ok"]
            )
            self.assertEqual(
                (first / "console.log").read_text(encoding="utf-8"),
                "attempt 1\n",
            )
            self.assertEqual(
                (second / "console.log").read_text(encoding="utf-8"),
                "attempt 2\n",
            )
            runtime_dir = repack.call_args.kwargs["out_dir"]
            self.assertFalse(runtime_dir.exists())

    def test_ordinary_boot_artifacts_are_pruned_after_the_result_is_parsed(
        self,
    ) -> None:
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            boot_dir = Path(tmp) / "boot-0001"
            boot_dir.mkdir()
            console = boot_dir / "console.log"
            machine = boot_dir / "machine.nd.log"
            console.write_text("console", encoding="utf-8")
            machine.write_text("machine", encoding="utf-8")
            result = C.BootResult(
                boot_index=1,
                task_name="locks_storm",
                cmdline="nightmare=locks_storm",
                seed=1,
                duration_ms=1,
                exit_code=0,
                status=C.BootStatus.OK.value,
                reason="ok",
                progress=1,
                findings=[],
                console_log=console,
                machine_log=machine,
            )

            C._prune_uninteresting_boot(result)

            self.assertFalse(boot_dir.exists())

    def test_discovery_boot_artifacts_are_retained(self) -> None:
        import tempfile

        for status in (
            C.BootStatus.FINDING.value,
            C.BootStatus.STALL.value,
            C.BootStatus.CRASH.value,
        ):
            with self.subTest(status=status), tempfile.TemporaryDirectory() as tmp:
                boot_dir = Path(tmp) / "boot-0001"
                boot_dir.mkdir()
                console = boot_dir / "console.log"
                console.write_text("evidence", encoding="utf-8")
                result = C.BootResult(
                    boot_index=1,
                    task_name="locks_storm",
                    cmdline="nightmare=locks_storm",
                    seed=1,
                    duration_ms=1,
                    exit_code=1,
                    status=status,
                    reason="evidence",
                    progress=1,
                    findings=[],
                    console_log=console,
                )

                C._prune_uninteresting_boot(result)

                self.assertEqual(console.read_text(encoding="utf-8"), "evidence")


if __name__ == "__main__":
    unittest.main()
