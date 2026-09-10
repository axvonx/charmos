"""The machine profile"""

import unittest
from pathlib import Path

from charm import machine as M

PROFILES = Path(__file__).resolve().parents[1] / "machines"
PATHS = {
    "iso": "i.iso",
    "disk": "d.img",
    "qmp_socket": "q.sock",
    "machine_log": "m.log",
}


def load() -> M.Machine:
    return M.load("default", directory=PROFILES)


def render(mode: str, **kwargs) -> list[str]:
    return M.render(load(), mode, **{**PATHS, **kwargs})


def flag_values(argv: list[str], flag: str) -> list[str]:
    return [argv[i + 1] for i, a in enumerate(argv) if a == flag and i + 1 < len(argv)]


class MemoryTests(unittest.TestCase):
    def test_suffixes_become_mib(self) -> None:
        self.assertEqual(M.parse_memory("8G"), 8192)
        self.assertEqual(M.parse_memory("2048M"), 2048)
        self.assertEqual(M.parse_memory(512), 512)

    def test_a_bare_number_is_mib_not_bytes(self) -> None:
        self.assertEqual(M.parse_memory("2048"), 2048)

    def test_nonsense_is_refused(self) -> None:
        with self.assertRaises(M.MachineError):
            M.parse_memory("plenty")


class NumaTests(unittest.TestCase):
    def test_node_memory_and_cpus_are_derived_not_written_down(self) -> None:
        argv = render("tests")

        self.assertEqual(
            flag_values(argv, "-object"),
            [f"memory-backend-ram,size=2048M,id=mem{i}" for i in range(4)],
        )
        self.assertEqual(
            [v for v in flag_values(argv, "-numa") if v.startswith("node")],
            [
                f"node,cpus={i * 2}-{i * 2 + 1},nodeid={i},memdev=mem{i}"
                for i in range(4)
            ],
        )

    def test_memory_that_does_not_divide_is_refused(self) -> None:
        with self.assertRaises(M.MachineError) as caught:
            render("tests", memory_mib=999)
        self.assertIn("divide evenly", str(caught.exception))

    def test_cpus_that_do_not_divide_are_refused(self) -> None:
        with self.assertRaises(M.MachineError) as caught:
            render("tests", smp=M.Smp(1, 3, 2))
        self.assertIn("CPUs", str(caught.exception))

    def test_a_machine_smaller_than_its_node_count_gets_no_numa(self) -> None:
        argv = render("nightmare", smp=M.Smp(1, 2, 1), memory_mib=512)

        self.assertEqual(flag_values(argv, "-numa"), [])
        self.assertEqual(flag_values(argv, "-object"), [])

    def test_every_distance_is_emitted_in_both_directions(self) -> None:
        distances = [
            v for v in flag_values(render("tests"), "-numa") if v.startswith("dist")
        ]

        self.assertEqual(len(distances), 12)
        for src, dst, value in load().numa_distances:
            self.assertIn(f"dist,src={src},dst={dst},val={value}", distances)
            self.assertIn(f"dist,src={dst},dst={src},val={value}", distances)


class DeviceTests(unittest.TestCase):
    def test_every_mode_gets_a_display_adapter(self) -> None:
        for mode in load().modes:
            with self.subTest(mode=mode):
                self.assertIn("VGA", flag_values(render(mode), "-device"))

    def test_nodefaults_makes_nic_none_unnecessary(self) -> None:
        self.assertNotIn("-nic", render("tests"))

    def test_the_disk_is_attached_only_when_one_is_given(self) -> None:
        self.assertEqual(
            flag_values(render("tests"), "-drive"),
            ["id=nvme0,file=d.img,format=raw,if=none"],
        )
        self.assertEqual(flag_values(render("tests", disk=None), "-drive"), [])


class ModeTests(unittest.TestCase):
    def test_the_console_comes_before_the_machine_channel(self) -> None:
        for mode in load().modes:
            with self.subTest(mode=mode):
                serials = flag_values(render(mode), "-serial")
                self.assertEqual(len(serials), 2)
                self.assertTrue(serials[1].startswith("file:"))
                self.assertFalse(serials[0].startswith("file:"))

    def test_display_wiring_is_per_mode(self) -> None:
        self.assertIn("-nographic", render("tests"))
        self.assertNotIn("-nographic", render("run"))
        self.assertEqual(flag_values(render("nightmare"), "-display"), ["none"])

    def test_debug_exit_only_where_the_mode_declares_it(self) -> None:
        self.assertIn(M.DEBUG_EXIT_DEVICE, flag_values(render("tests"), "-device"))
        self.assertNotIn(M.DEBUG_EXIT_DEVICE, flag_values(render("run"), "-device"))

    def test_gdb_wait_halts_and_listen_does_not(self) -> None:
        self.assertIn("-S", render("debug"))
        self.assertIn("-s", render("debug"))
        self.assertIn("-s", render("tests-debug"))
        self.assertNotIn("-S", render("tests-debug"))

    def test_gdb_wait_can_be_forced_onto_any_mode(self) -> None:
        self.assertNotIn("-S", render("tests"))
        self.assertIn("-S", render("tests", gdb="wait"))

    def test_kvm_brings_the_host_cpu_with_it(self) -> None:
        argv = render("tests", kvm=True)
        self.assertIn("-enable-kvm", argv)
        self.assertEqual(flag_values(argv, "-cpu"), ["host"])

    def test_an_unknown_mode_names_the_ones_that_exist(self) -> None:
        with self.assertRaises(M.MachineError) as caught:
            render("nope")
        self.assertIn("tests", str(caught.exception))


class ProfileTests(unittest.TestCase):
    def test_the_machine_type_is_pinned_not_an_alias(self) -> None:
        self.assertNotEqual(load().type, "q35")
        self.assertRegex(load().type, r"^pc-q35-\d+\.\d+$")

    def test_arch_drives_the_binary_name(self) -> None:
        self.assertEqual(load().qemu, "qemu-system-x86_64")

    def test_a_missing_profile_says_where_it_looked(self) -> None:
        with self.assertRaises(M.MachineError) as caught:
            M.load("nope", directory=PROFILES)
        self.assertIn("nope.toml", str(caught.exception))


if __name__ == "__main__":
    unittest.main()


class CmakeVariableTests(unittest.TestCase):
    def cmake_text(self) -> str:
        root = Path(__file__).resolve().parents[2]
        sources = [root / "CMakeLists.txt", root / "kernel" / "CMakeLists.txt"]
        sources += sorted((root / "cmake").glob("*.cmake"))
        return "\n".join(p.read_text(encoding="utf-8") for p in sources)

    def test_every_definition_names_a_real_cmake_variable(self) -> None:
        from charm.nightmare import codec, suite

        cmake = self.cmake_text()
        fixtures = Path(__file__).resolve().parents[2] / "nightmare" / "suites"
        for path in sorted(fixtures.glob("*.toml")):
            for argument in codec.build_args(suite.load(path)):
                name = argument.removeprefix("-D").split("=", 1)[0]
                with self.subTest(suite=path.name, variable=name):
                    self.assertIn(name, cmake)
