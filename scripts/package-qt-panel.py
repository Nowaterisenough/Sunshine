#!/usr/bin/env python3
"""Package a deployed Qt panel, preserving macOS bundle links and permissions."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile


def verify_layout(staging: Path, target: str) -> Path:
    if target == "windows-x64":
        gui = staging / "assets/gui"
        executable = gui / "sunshine-gui.exe"
        required = [executable, gui / "qt.conf", gui / "plugins/platforms/qwindows.dll",
                    gui / "qml/QtQuick/qmldir"]
        required += [gui / f"Qt6{name}.dll" for name in
                     ("Core", "Gui", "Network", "Widgets", "Qml", "Quick", "QuickControls2")]
    else:
        bundle = staging / "sunshine-gui.app/Contents"
        executable = bundle / "MacOS/sunshine-gui"
        required = [executable, bundle / "Info.plist", bundle / "PlugIns/platforms/libqcocoa.dylib",
                    bundle / "PlugIns/tls/libqsecuretransportbackend.dylib",
                    bundle / "Resources/qml/QtQuick/qmldir"]
        required += [bundle / f"Frameworks/Qt{name}.framework/Versions/A/Qt{name}"
                     for name in ("Core", "Gui", "Network", "Qml", "Quick")]
        if executable.is_file():
            architectures = subprocess.check_output(["lipo", "-archs", str(executable)], text=True).split()
            if "arm64" not in architectures:
                raise RuntimeError(f"Expected an arm64 panel, found {architectures}")

    missing = [str(path.relative_to(staging)) for path in required if not path.is_file()]
    if missing:
        raise RuntimeError("Incomplete Qt deployment: " + ", ".join(missing))
    forbidden_names = {"webview2loader.dll", "alkaidlab-plugin-stylus.dll"}
    for path in staging.rglob("*"):
        if path.name.lower() in forbidden_names or path.relative_to(staging).as_posix().startswith("assets/web/"):
            raise RuntimeError(f"Legacy UI leaked into Qt panel package: {path}")
    return executable


def verify_launch(executable: Path, qt_root: str):
    environment = os.environ.copy()
    # A deployed package must find its own imports instead of the build machine's kit.
    for name in ("QML_IMPORT_PATH", "QML2_IMPORT_PATH", "QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH",
                 "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH", "LD_LIBRARY_PATH"):
        environment.pop(name, None)
    if qt_root:
        root = os.path.normcase(os.path.abspath(qt_root))
        entries = environment.get("PATH", "").split(os.pathsep)
        environment["PATH"] = os.pathsep.join(
            entry for entry in entries
            if not os.path.normcase(os.path.abspath(entry)).startswith(root + os.sep)
        )
    result = subprocess.run([str(executable), "--version"], env=environment, cwd=executable.parent,
                            capture_output=True, text=True, timeout=30)
    if result.returncode or "SunshinePanel" not in result.stdout:
        raise RuntimeError(f"Deployed panel failed to launch (exit {result.returncode}): {result.stdout}\n{result.stderr}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--staging", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--platform", choices=("windows-x64", "macos-arm64"), required=True)
    parser.add_argument("--qt-version", default="6.12.0")
    parser.add_argument("--verify-run", action="store_true")
    parser.add_argument("--verify-only", action="store_true", help="Check a deployed runtime without creating an archive")
    args = parser.parse_args()
    parts = args.qt_version.split(".")
    if len(parts) != 3 or parts[:2] != ["6", "12"] or not parts[2].isdecimal():
        parser.error("This package targets Qt 6.12.x")

    staging = args.staging.resolve()
    executable = verify_layout(staging, args.platform)
    if args.verify_only:
        if args.verify_run:
            verify_launch(executable, os.environ.get("QT_ROOT_DIR", ""))
        print(f"Verified deployed Qt runtime: {staging}")
        return
    if args.output is None:
        parser.error("--output is required when creating an archive")
    output = args.output.resolve()
    if staging == output or staging in output.parents:
        parser.error("The artifact directory must be outside staging")
    output.mkdir(parents=True, exist_ok=True)
    repo = Path(__file__).resolve().parents[1]
    name = f"Sunshine-Qt-Panel-{args.qt_version}-{args.platform}"
    archive = output / f"{name}.zip"
    metadata = {
        "product": "Sunshine Qt control panel",
        "qt_version": args.qt_version,
        "platform": args.platform,
        "commit": os.environ.get("GITHUB_SHA", "local"),
        "run_url": (f"https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}"
                    if os.environ.get("GITHUB_REPOSITORY") and os.environ.get("GITHUB_RUN_ID") else None),
        "includes_streaming_host": False,
    }

    with tempfile.TemporaryDirectory(prefix="qt-package-", dir=output.parent) as temporary:
        temporary = Path(temporary)
        package = temporary / name
        shutil.copytree(staging, package, symlinks=True)
        shutil.copy2(repo / "LICENSE", package / "LICENSE")
        executable_path = "assets/gui/sunshine-gui.exe" if args.platform == "windows-x64" else "sunshine-gui.app"
        (package / "README.txt").write_text(
            f"Sunshine Qt {args.qt_version} control panel\n\n"
            f"Start: {executable_path}\n"
            "This ZIP contains the native control panel and Qt runtime.\n"
            "It requires an existing Sunshine host on the same computer.\n"
            "Default management endpoint: https://127.0.0.1:47990\n"
            "The Build and Release workflow provides the complete Windows host installer.\n"
            "Development artifacts are not notarized release installers.\n",
            encoding="utf-8",
        )
        info = json.dumps(metadata, indent=2, ensure_ascii=False) + "\n"
        (package / "build-info.json").write_text(info, encoding="utf-8")
        if args.platform == "macos-arm64":
            subprocess.run(["ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", str(package), str(archive)], check=True)
        else:
            with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
                for path in sorted(package.rglob("*")):
                    if path.is_file():
                        bundle.write(path, path.relative_to(temporary))

        restored = temporary / "restored"
        if args.platform == "macos-arm64":
            subprocess.run(["ditto", "-x", "-k", str(archive), str(restored)], check=True)
        else:
            with zipfile.ZipFile(archive) as bundle:
                bundle.extractall(restored)
        executable = verify_layout(restored / name, args.platform)
        if args.verify_run:
            verify_launch(executable, os.environ.get("QT_ROOT_DIR", ""))

    (output / "build-info.json").write_text(info, encoding="utf-8")
    checksum = hashlib.sha256()
    with archive.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    digest = checksum.hexdigest()
    (output / "SHA256SUMS.txt").write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"Verified Qt package: {archive}")
    print(f"SHA256: {digest}")


if __name__ == "__main__":
    main()
