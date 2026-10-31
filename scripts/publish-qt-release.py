#!/usr/bin/env python3
"""Publish validated host and panel artifacts as one complete Qt prerelease."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile


def gh(*args):
    return subprocess.check_output(["gh", *map(str, args)], text=True).strip()


def api(path, optional=False):
    result = subprocess.run(["gh", "api", path], capture_output=True, text=True)
    if result.returncode:
        if optional and "HTTP 404" in result.stderr:
            return None
        raise RuntimeError(result.stderr.strip())
    return json.loads(result.stdout)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def checked_file(folder, name, expected):
    if not isinstance(name, str) or Path(name).name != name or "\\" in name:
        raise ValueError("Invalid artifact filename")
    path = folder / name
    if not re.fullmatch(r"[a-fA-F0-9]{64}", expected) or not path.is_file() or digest(path) != expected.lower():
        raise ValueError(f"Artifact checksum mismatch: {name}")
    return path


def panel_asset(folder, platform, commit):
    info = json.loads((folder / "build-info.json").read_text(encoding="utf-8-sig"))
    if (info.get("commit") != commit or info.get("platform") != platform or
            info.get("includes_streaming_host") is not False or
            not re.fullmatch(r"6\.12\.\d+", info.get("qt_version", ""))):
        raise ValueError(f"Panel build metadata mismatch: {platform}")
    expected, name = (folder / "SHA256SUMS.txt").read_text().strip().split(maxsplit=1)
    if name != f"Sunshine-Qt-Panel-{info['qt_version']}-{platform}.zip":
        raise ValueError(f"Unexpected panel archive: {name}")
    return checked_file(folder, name, expected), info


def host_assets(folder):
    checks = json.loads((folder / "checksums.json").read_text(encoding="utf-8-sig"))
    if any(not (item["File"].endswith((".WindowsInstaller.exe", ".Portable-x64.zip")) or
                item["File"] == "Sunshine.Ds5Sidecar.x64.zip") for item in checks):
        raise ValueError("Unexpected file in host release checksums")
    files = [checked_file(folder, item["File"], item["Hash"]) for item in checks]
    installers = [path for path in files if path.name.endswith(".WindowsInstaller.exe")]
    portable = [path for path in files if path.name.endswith(".Portable-x64.zip")]
    if len(installers) != 1 or len(portable) != 1:
        raise ValueError("Expected one complete Windows installer and portable package")
    with zipfile.ZipFile(portable[0]) as archive:
        names = [name.replace("\\", "/").lower() for name in archive.namelist()]
        for suffix in ("/sunshine.exe", "/assets/gui/sunshine-gui.exe", "/assets/gui/qt6core.dll"):
            if not any(name.endswith(suffix) for name in names):
                raise ValueError(f"Incomplete host package: {suffix}")
        if any("/assets/web/" in name or name.endswith(("webview2loader.dll", "alkaidlab-plugin-stylus.dll")) for name in names):
            raise ValueError("Legacy UI found in Qt host package")
        rtx_hdr = any(name.endswith("/foundation_rtx_video_adapter.dll") for name in names)
    optional = folder / "Sunshine.Ds5Sidecar.manifest.json"
    if optional.is_file():
        files.append(optional)
    return files, rtx_hdr


def validate_run(run, repository, branch, paths, current_run):
    if (run["head_repository"]["full_name"] != repository or run["head_branch"] != branch or
            run["event"] not in ("push", "workflow_dispatch") or run["path"].split("@")[0] not in paths):
        raise ValueError("Only builds from this repository, branch and trusted workflows can be published")
    # A publish job runs before its own workflow can acquire a final conclusion.
    if run["conclusion"] != "success" and not (str(run["id"]) == current_run and run["status"] == "in_progress"):
        raise ValueError("Producer workflow did not succeed")


def download(repository, run_id, name, folder):
    records = api(f"repos/{repository}/actions/runs/{run_id}/artifacts?per_page=100")["artifacts"]
    matching = [item for item in records if item["name"] == name and not item["expired"]]
    if len(matching) != 1:
        raise ValueError(f"Missing or ambiguous artifact: {name}")
    folder.mkdir()
    gh("run", "download", run_id, "--repo", repository, "--name", name, "--dir", folder)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host-run", default=os.environ.get("GITHUB_RUN_ID", ""))
    parser.add_argument("--panel-run", default=os.environ.get("GITHUB_RUN_ID", ""))
    args = parser.parse_args()
    if not args.host_run.isdecimal() or not args.panel_run.isdecimal():
        parser.error("Both producer run IDs are required")
    repository, branch = os.environ["GITHUB_REPOSITORY"], os.environ["GITHUB_REF_NAME"]
    current_run = os.environ.get("GITHUB_RUN_ID", "")
    host = api(f"repos/{repository}/actions/runs/{args.host_run}")
    panel = api(f"repos/{repository}/actions/runs/{args.panel_run}")
    validate_run(host, repository, branch, {".github/workflows/main.yml"}, current_run)
    panel_paths = {".github/workflows/qt-panel.yml"}
    if args.panel_run == args.host_run:
        panel_paths.add(".github/workflows/main.yml")
    validate_run(panel, repository, branch, panel_paths, current_run)
    for commit in (host["head_sha"], panel["head_sha"]):
        subprocess.run(["git", "merge-base", "--is-ancestor", commit, "HEAD"], check=True)
    inputs = ["ui/qt", "CMakeLists.txt", "CMakePresets.json", "cmake/prep/ui_options.cmake",
              "scripts/package-qt-panel.py", "LICENSE"]
    changed = subprocess.check_output(["git", "diff", "--name-only", panel["head_sha"], host["head_sha"], "--", *inputs], text=True)
    if changed.strip():
        raise ValueError(f"Host and panel builds use different Qt inputs:\n{changed}")

    with tempfile.TemporaryDirectory(prefix="sunshine-qt-release-") as temporary:
        root = Path(temporary)
        download(repository, args.host_run, f"sunshine-windows-r{host['run_number']}", root / "host")
        files, rtx_hdr = host_assets(root / "host")
        panels = {}
        for platform in ("windows-x64", "macos-arm64"):
            folder = root / platform
            download(repository, args.panel_run, f"Sunshine-Qt-Panel-{platform}", folder)
            archive, info = panel_asset(folder, platform, panel["head_sha"])
            panels[platform] = info
            files.append(archive)
        if panels["windows-x64"]["qt_version"] != panels["macos-arm64"]["qt_version"]:
            raise ValueError("Panel Qt versions differ")

        output = root / "release"
        output.mkdir()
        for path in files:
            # Legacy updaters treat every asset containing 'Windows' as an installer.
            name = path.name.replace("-windows-x64.zip", "-win-x64.zip")
            if (output / name).exists():
                raise ValueError(f"Duplicate release asset: {name}")
            shutil.copy2(path, output / name)
        metadata = {"host_commit": host["head_sha"], "panel_commit": panel["head_sha"],
                    "host_run": host["html_url"], "panel_run": panel["html_url"],
                    "macos_arm64_includes_streaming_host": False, "rtx_video_hdr_adapter": rtx_hdr,
                    "panels": panels}
        (output / "release-build-info.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        sums = "".join(f"{digest(path)}  {path.name}\n" for path in sorted(output.iterdir()))
        (output / "SHA256SUMS.txt").write_text(sums, encoding="utf-8")
        assets = sorted(output.iterdir())
        tag = f"qt-{host['head_sha'][:12]}"
        notes = root / "release-notes.md"
        rtx = "包含" if rtx_hdr else "未包含"
        notes.write_text(f"""Qt 6.12 界面预览版，默认关闭旧 Web/Tauri 界面和 AI 功能。

| 下载文件 | 内容 |
| --- | --- |
| `*.WindowsInstaller.exe` | Windows x64 完整安装器，包含串流内核和 Qt 界面 |
| `*.Portable-x64.zip` | Windows x64 完整便携包 |
| `Sunshine-Qt-Panel-*-macos-arm64.zip` | **macOS ARM64 Qt 控制面板及运行库，需要已有的本机 Sunshine 服务，不包含串流内核** |
| `Sunshine-Qt-Panel-*-win-x64.zip` | Windows x64 独立 Qt 控制面板，需要已有的本机 Sunshine 服务 |
| `Sunshine.Ds5Sidecar.x64.zip` | 可选 DualSense 组件 |

macOS ARM64 完整串流主机版尚未完成；本包不能单独将 Mac 变成串流主机。
这些是开发预览构建，尚未正式签名或公证，也未进行实体显卡/Moonlight 串流验收。
完整 Windows 包{rtx}可选 RTX Video HDR 适配器。

主机源码：`{host['head_sha']}`；面板源码：`{panel['head_sha']}`，Qt 构建输入已核对一致。
[完整 Windows 构建]({host['html_url']}) · [Qt 面板构建]({panel['html_url']})。
下载后可用 `SHA256SUMS.txt` 校验，详细来源见 `release-build-info.json`。
""", encoding="utf-8")

        ref = api(f"repos/{repository}/git/ref/tags/{tag}", optional=True)
        if ref and (ref["object"]["type"] != "commit" or ref["object"]["sha"] != host["head_sha"]):
            raise ValueError("Release tag points at a different commit")
        release = api(f"repos/{repository}/releases/tags/{tag}", optional=True)
        if release and not release["draft"]:
            remote = {item["name"]: item.get("digest") for item in release["assets"]}
            if remote != {path.name: f"sha256:{digest(path)}" for path in assets}:
                raise ValueError("Published release differs; refusing to overwrite its assets")
        else:
            if not release:
                # Do not ask GITHUB_TOKEN to create a ref to a historical commit.
                target = ["--verify-tag"] if ref else ["--target", host["head_sha"]]
                gh("release", "create", tag, "--repo", repository, *target,
                   "--title", f"Sunshine Qt 6.12 Preview ({host['created_at'][:10]}, {host['head_sha'][:7]})",
                   "--notes-file", notes, "--draft", "--prerelease")
            gh("release", "upload", tag, *assets, "--repo", repository, "--clobber")
            uploaded = api(f"repos/{repository}/releases/tags/{tag}")
            remote = {item["name"]: item.get("digest") for item in uploaded["assets"]}
            if remote != {path.name: f"sha256:{digest(path)}" for path in assets}:
                raise ValueError("Release upload verification failed; leaving the draft unpublished")
            gh("release", "edit", tag, "--repo", repository, "--draft=false", "--prerelease", "--latest=false")
        url = f"https://github.com/{repository}/releases/tag/{tag}"
        print(f"Published verified Qt release: {url}")
        if os.environ.get("GITHUB_STEP_SUMMARY"):
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as summary:
                summary.write(f"[Download Windows and macOS ARM64 Qt packages]({url})\n")


if __name__ == "__main__":
    main()
