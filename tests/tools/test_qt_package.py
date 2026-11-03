"""Apple Silicon artifacts must retain their ARM64 executable and runtime."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "qt_package", Path(__file__).resolve().parents[2] / "scripts/package-qt-panel.py")
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class MacArchitectureTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.bundle = Path(temporary.name) / "Sunshine.app/Contents"
        self.executable = self.binary("MacOS/sunshine-gui")
        self.framework = self.binary("Frameworks/QtCore.framework/Versions/A/QtCore")
        self.plugin = self.binary("PlugIns/platforms/libqcocoa.dylib")
        resource = self.bundle / "Resources/qml/qmldir"
        resource.parent.mkdir(parents=True)
        resource.write_text("module QtQuick\n", encoding="utf-8")

    def binary(self, path):
        binary = self.bundle / path
        binary.parent.mkdir(parents=True, exist_ok=True)
        binary.write_bytes(bytes.fromhex("cffaedfe") + b"fixture")
        return binary

    def verify(self, architectures):
        def lipo(command, **kwargs):
            self.assertEqual(command[:2], ["lipo", "-archs"])
            return architectures[Path(command[2]).resolve()]

        with patch.object(package.subprocess, "check_output", side_effect=lipo) as probe:
            result = package.verify_macos_runtime(self.bundle, self.executable)
            return result, probe.call_count

    def architectures(self, executable="arm64", framework="arm64 x86_64", plugin="arm64 x86_64"):
        return {path.resolve(): value for path, value in (
            (self.executable, executable), (self.framework, framework), (self.plugin, plugin))}

    def test_arm64_bundle_checks_framework_and_plugin(self):
        architectures, calls = self.verify(self.architectures())
        self.assertEqual(architectures, {"arm64"})
        self.assertEqual(calls, 3)

    def test_thin_arm64_runtime_is_accepted(self):
        self.assertEqual(self.verify(self.architectures(framework="arm64", plugin="arm64"))[0], {"arm64"})

    def test_intel_executable_cannot_be_labelled_arm64(self):
        with self.assertRaisesRegex(RuntimeError, "Expected an arm64-only panel"):
            self.verify(self.architectures(executable="x86_64"))

    def test_intel_only_framework_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, r"QtCore.*missing architectures.*arm64"):
            self.verify(self.architectures(framework="x86_64"))

    def test_universal_executable_is_rejected_for_arm64_only_product(self):
        with self.assertRaisesRegex(RuntimeError, "Expected an arm64-only panel"):
            self.verify(self.architectures(executable="arm64 x86_64"))

    def test_intel_only_plugin_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, r"libqcocoa.*missing architectures.*arm64"):
            self.verify(self.architectures(plugin="x86_64"))

    def test_framework_symlink_is_checked_once(self):
        alias = self.bundle / "Frameworks/QtCore.framework/QtCore"
        try:
            alias.symlink_to("Versions/A/QtCore")
        except OSError:
            self.skipTest("Creating symlinks requires an enabled Windows privilege")
        self.assertEqual(self.verify(self.architectures())[1], 3)


if __name__ == "__main__":
    unittest.main()
