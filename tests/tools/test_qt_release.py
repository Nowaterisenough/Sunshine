import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile


spec = importlib.util.spec_from_file_location("qt_release", Path(__file__).resolve().parents[2] / "scripts/publish-qt-release.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseValidationTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.folder = Path(self.temporary.name)

    def test_draft_lookup_when_tag_endpoint_returns_404(self):
        draft = {"id": 123, "tag_name": "qt-example", "draft": True}
        with patch.object(release, "api", side_effect=[None, [draft]]) as api:
            self.assertEqual(release.find_release("owner/repo", "qt-example"), draft)
            self.assertIn("/releases?", api.call_args.args[0])

    def test_published_release_lookup_needs_no_draft_listing(self):
        published = {"id": 123, "tag_name": "qt-example", "draft": False}
        with patch.object(release, "api", return_value=published) as api:
            self.assertEqual(release.find_release("owner/repo", "qt-example"), published)
            api.assert_called_once()

    def panel(self):
        name = "Sunshine-Qt-Panel-6.12.0-macos-arm64.zip"
        archive = self.folder / name
        archive.write_bytes(b"verified panel payload")
        (self.folder / "SHA256SUMS.txt").write_text(f"{release.digest(archive)}  {name}\n")
        info = {"commit": "panel-sha", "qt_version": "6.12.0", "platform": "macos-arm64", "includes_streaming_host": False}
        (self.folder / "build-info.json").write_text(json.dumps(info))
        return archive, info

    def host(self, extra=(), omit=()):
        portable = self.folder / "Sunshine.test.Portable-x64.zip"
        names = {"Sunshine/sunshine.exe", "Sunshine/assets/gui/sunshine-gui.exe", "Sunshine/assets/gui/Qt6Core.dll"}
        with zipfile.ZipFile(portable, "w") as archive:
            for name in names.difference(omit).union(extra):
                archive.writestr(name, b"fixture")
        installer = self.folder / "Sunshine.test.WindowsInstaller.exe"
        installer.write_bytes(b"fixture installer")
        checks = [{"File": path.name, "Hash": release.digest(path)} for path in (portable, installer)]
        (self.folder / "checksums.json").write_text(json.dumps(checks))
        return installer

    def test_verified_macos_panel_remains_labelled_panel_only(self):
        archive, info = self.panel()
        self.assertEqual(release.panel_asset(self.folder, "macos-arm64", "panel-sha"), (archive, info))
        self.assertIs(info["includes_streaming_host"], False)

    def test_wrong_panel_commit_and_architecture_rejected(self):
        self.panel()
        for platform, commit in (("windows-x64", "panel-sha"), ("macos-arm64", "wrong-sha")):
            with self.subTest(platform=platform, commit=commit), self.assertRaises(ValueError):
                release.panel_asset(self.folder, platform, commit)

    def test_changed_panel_payload_rejected(self):
        archive, _ = self.panel()
        archive.write_bytes(b"corrupt download")
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            release.panel_asset(self.folder, "macos-arm64", "panel-sha")

    def test_checksum_cannot_escape_artifact_directory(self):
        for name in ("../outside.exe", "..\\outside.exe", "/outside.exe"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                release.checked_file(self.folder, name, "0" * 64)

    def test_complete_host_and_optional_adapter_detected(self):
        self.host(extra=("Sunshine/tools/hdr/foundation_rtx_video_adapter.dll",))
        files, rtx = release.host_assets(self.folder)
        self.assertEqual(len(files), 2)
        self.assertTrue(rtx)

    def test_panel_cannot_be_mislabelled_complete_host(self):
        self.host(omit=("Sunshine/sunshine.exe",))
        with self.assertRaisesRegex(ValueError, "Incomplete host"):
            release.host_assets(self.folder)

    def test_legacy_ui_in_host_rejected(self):
        self.host(extra=("Sunshine/assets/web/index.html",))
        with self.assertRaisesRegex(ValueError, "Legacy UI"):
            release.host_assets(self.folder)

    def test_host_checksum_mismatch_rejected(self):
        installer = self.host()
        installer.write_bytes(b"corrupt installer")
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            release.host_assets(self.folder)

    def test_unrelated_executable_not_published(self):
        self.host()
        extra = self.folder / "unrelated.exe"
        extra.write_bytes(b"unrelated program")
        checks = json.loads((self.folder / "checksums.json").read_text())
        checks.append({"File": extra.name, "Hash": release.digest(extra)})
        (self.folder / "checksums.json").write_text(json.dumps(checks))
        with self.assertRaisesRegex(ValueError, "Unexpected file"):
            release.host_assets(self.folder)

    def test_only_trusted_successful_producers_accepted(self):
        run = {"id": 10, "head_repository": {"full_name": "owner/repo"}, "head_branch": "master",
               "event": "push", "path": ".github/workflows/main.yml", "status": "completed", "conclusion": "success"}
        paths = {".github/workflows/main.yml"}
        release.validate_run(run, "owner/repo", "master", paths, "20")
        for changes in ({"event": "pull_request"}, {"head_branch": "untrusted"}, {"conclusion": "failure"},
                        {"head_repository": {"full_name": "fork/repo"}}, {"path": "another.yml"}):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                release.validate_run({**run, **changes}, "owner/repo", "master", paths, "20")

    def test_running_producer_allowed_only_for_current_workflow(self):
        run = {"id": 10, "head_repository": {"full_name": "owner/repo"}, "head_branch": "master",
               "event": "workflow_dispatch", "path": ".github/workflows/main.yml", "status": "in_progress", "conclusion": None}
        paths = {".github/workflows/main.yml"}
        release.validate_run(run, "owner/repo", "master", paths, "10")
        with self.assertRaises(ValueError):
            release.validate_run(run, "owner/repo", "master", paths, "20")


if __name__ == "__main__":
    unittest.main()
