"""The machine profile"""

import shutil
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


class ProfileTests(unittest.TestCase):
    def test_the_declared_machine_type_is_what_gets_emitted(self) -> None:
        self.assertEqual(flag_values(render("tests"), "-M"), [load().type])

    # TODO: Update this! This is a HACK:
    def test_the_version_floor_is_not_above_what_ci_runs(self) -> None:
        self.assertLessEqual(
            M._version_tuple(load().min_qemu), (8, 2), "floor is above the CI image"
        )

    def test_an_alias_is_reported_as_what_it_resolves_to(self) -> None:
        machine = load()
        if shutil.which(machine.qemu) is None:
            self.skipTest(f"{machine.qemu} is not installed")

        resolved = M.resolve_machine_type(machine)
        if machine.type == "q35":
            self.assertRegex(resolved, r"^pc-q35-\d+\.\d+$")
        else:
            self.assertEqual(resolved, machine.type)

    def test_resolving_never_fails_when_qemu_is_absent(self) -> None:
        machine = M.Machine(
            name="x", arch="nonesuch", type="q35", min_qemu="0", nodefaults=True,
            memory_mib=1024, smp=M.Smp(1, 1, 1), numa_nodes=0, numa_distances=(),
            devices={}, acpi={}, trace={}, modes={},
        )  # fmt: skip
        self.assertEqual(M.resolve_machine_type(machine), "q35")

    def test_arch_drives_the_binary_name(self) -> None:
        self.assertEqual(load().qemu, "qemu-system-x86_64")

    def test_a_missing_profile_says_where_it_looked(self) -> None:
        with self.assertRaises(M.MachineError) as caught:
            M.load("nope", directory=PROFILES)
        self.assertIn("nope.toml", str(caught.exception))


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


if __name__ == "__main__":
    unittest.main()
