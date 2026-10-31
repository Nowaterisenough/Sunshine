"""Exercise generated production cleanup rules without touching services or drivers."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[2]
LEGACY_FILES = {"WebView2Loader.dll", "alkaidlab-plugin-stylus.dll"}


class InstallerUpgradeTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="sunshine upgrade ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.app = self.root / "Sunshine old install"

    def render_cleanup(self, qt):
        driver = self.root / "render.cmake"
        generated = self.root / "sunshine_installer.iss"
        driver.write_text(
            f'set(BUILD_QT_UI {"ON" if qt else "OFF"})\n'
            f'include("{SOURCE.as_posix()}/cmake/packaging/ui_install_rules.cmake")\n'
            f'configure_file("{SOURCE.as_posix()}/cmake/packaging/sunshine.iss.in" '
            f'"{generated.as_posix()}" @ONLY)\n', encoding="utf-8")
        result = subprocess.run([shutil.which("cmake") or "cmake", "-P", str(driver)],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        text = generated.read_text(encoding="utf-8")
        block = re.search(r"(?ms)^\[InstallDelete\]\n(.*?)(?=^\[)", text).group(1)
        self.assertNotIn("@SUNSHINE_", block)
        return block, text

    def test_qt_filters_and_removes_only_known_legacy_gui_files(self):
        block, text = self.render_cleanup(True)
        cleanup = re.findall(r'Type: (\w+); Name: "\{app\}\\assets\\gui\\([^"\n]+)"', block)
        self.assertEqual(set(cleanup), {("files", name) for name in LEGACY_FILES})
        gui_line = next(line for line in text.splitlines() if line.startswith('Source: "{#MySourceDir}\\assets\\gui\\*"'))
        excludes = re.search(r'Excludes: "([^"]*)"', gui_line).group(1).split(",")
        self.assertEqual(set(excludes), LEGACY_FILES)
        self.assertNotRegex(block, r'Name: "\{app\}\\assets\\gui(?:\\\*)?"')
        self.assertNotIn("\\config", block)

    def test_legacy_mode_keeps_its_gui_libraries(self):
        block, text = self.render_cleanup(False)
        self.assertNotIn("\\assets\\gui", block)
        gui_line = next(line for line in text.splitlines() if line.startswith('Source: "{#MySourceDir}\\assets\\gui\\*"'))
        self.assertIn('Excludes: ""', gui_line)

    def write_installed(self, name, data):
        path = self.app / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def exercise_native_upgrade(self, qt):
        compiler = shutil.which("ISCC.exe") or shutil.which("iscc")
        if not compiler:
            if os.environ.get("SUNSHINE_REQUIRE_INNO_SMOKE") == "1":
                self.fail("Inno Setup is required for the Windows upgrade smoke test")
            self.skipTest("Inno Setup is not installed")
        block, _ = self.render_cleanup(qt)
        protected = {
            "config/sunshine.conf": b"user settings",
            "config/apps.json": b"user applications",
            "config/sunshine_state.json": b"paired clients",
            "config/credentials/server.pem": b"fixture certificate, not a real key",
            "config/covers/game.png": b"user cover",
            "assets/gui/custom-extension.dll": b"unknown user file",
            "assets/gui/Qt6Core.dll": b"current Qt runtime",
            "assets/gui/msvcp140.dll": b"current Qt compiler runtime",
            "assets/gui/plugins/platforms/qwindows.dll": b"current Qt platform plugin",
            "assets/gui/sunshine-gui.exe.WebView2/EBWebView/Preferences": b"user browser profile",
            "sunshine.exe": b"streaming host",
        }
        for name, data in protected.items():
            self.write_installed(name, data)
        for name in LEGACY_FILES:
            self.write_installed("assets/gui/" + name, b"legacy dependency")
        self.write_installed("assets/gui/sunshine-gui.exe", b"old Tauri executable")
        self.write_installed("assets/web/assets/old-hash.js", b"old web bundle")
        neighbor = self.root / "unrelated app" / "WebView2Loader.dll"
        neighbor.parent.mkdir()
        neighbor.write_bytes(b"another application")

        payload = self.root / "new-gui.bin"
        payload.write_bytes(b"new Qt executable" if qt else b"new legacy executable")
        setup = self.root / "upgrade.iss"
        setup.write_text(f'''[Setup]
AppId=SunshineUpgradeTestFixture
AppName=Sunshine upgrade test fixture
AppVersion=1.0
DefaultDirName={self.app}
PrivilegesRequired=lowest
Uninstallable=no
UsePreviousAppDir=no
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir={self.root}
OutputBaseFilename=upgrade-fixture
[InstallDelete]
{block}
[Files]
Source: "{payload}"; DestDir: "{{app}}\\assets\\gui"; DestName: "sunshine-gui.exe"; Flags: ignoreversion
''', encoding="utf-8-sig")
        compile_result = subprocess.run([compiler, str(setup)], capture_output=True, text=True, timeout=60)
        self.assertEqual(compile_result.returncode, 0, compile_result.stdout + compile_result.stderr)
        for attempt in (1, 2):
            log = self.root / f"install-{attempt}.log"
            result = subprocess.run([str(self.root / "upgrade-fixture.exe"), "/VERYSILENT", "/SUPPRESSMSGBOXES",
                "/NORESTART", "/SP-", f"/DIR={self.app}", f"/LOG={log}"], capture_output=True, timeout=60)
            detail = log.read_text(encoding="utf-8-sig", errors="replace") if log.exists() else repr(result.stderr)
            self.assertEqual(result.returncode, 0, detail)
            for name in LEGACY_FILES:
                path = self.app / "assets/gui" / name
                if qt:
                    self.assertFalse(path.exists(), str(path))
                else:
                    self.assertEqual(path.read_bytes(), b"legacy dependency")
            self.assertFalse((self.app / "assets/web").exists())
            self.assertEqual((self.app / "assets/gui/sunshine-gui.exe").read_bytes(), payload.read_bytes())
            for name, data in protected.items():
                self.assertEqual((self.app / name).read_bytes(), data, name)
            self.assertEqual(neighbor.read_bytes(), b"another application")

    @unittest.skipUnless(os.name == "nt", "Native Inno Setup execution requires Windows")
    def test_windows_qt_upgrade_preserves_data_and_is_repeatable(self):
        self.exercise_native_upgrade(True)

    @unittest.skipUnless(os.name == "nt", "Native Inno Setup execution requires Windows")
    def test_windows_legacy_upgrade_preserves_legacy_libraries(self):
        self.exercise_native_upgrade(False)


if __name__ == "__main__":
    unittest.main()
