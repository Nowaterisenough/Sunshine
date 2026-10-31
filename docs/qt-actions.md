# GitHub Actions 自动生成 Qt 版本

仓库包含两条构建路径。工作流和 Qt 源码推送到 GitHub 后，相关 push 或 pull request 会自动构建；工作流进入默认分支后也可以在 Actions 中手动运行。

## Qt 独立控制面板

工作流：`Build Qt Packages`（`.github/workflows/qt-panel.yml`）。

- Windows Server 2022 runner：MSVC 2022 x64，Qt 6.12.0 的 `win64_msvc2022_64` kit。
- macOS 15 ARM64 runner：AppleClang，Qt 6.12.0 的 `clang_64` kit，编译目标为 arm64。
- 自动执行配置、编译、接口测试、Qt 运行库部署、真实页面 HTTPS 测试、压缩、解压验证和程序启动检查。
- 不依赖主机的第三方子模块、驱动 SDK、发布令牌或上游仓库合并设置。
- `contents: read` 权限，构建产物保留 30 天；失败时上传 CMake/测试诊断、页面截图和进程日志。

在对应 workflow run 的 **Artifacts** 下载：

| Artifact | 内含 ZIP |
| --- | --- |
| `Sunshine-Qt-Panel-windows-x64` | `Sunshine-Qt-Panel-6.12.0-windows-x64.zip` |
| `Sunshine-Qt-Panel-macos-arm64` | `Sunshine-Qt-Panel-6.12.0-macos-arm64.zip` |

每个 artifact 还包含 `SHA256SUMS.txt` 和 `build-info.json`，记录源码提交、Qt 版本和构建地址。macOS 使用 `ditto` 保留 `.app` 的符号链接与权限。包内附有许可证和启动说明。macOS 使用系统 Secure Transport 后端，无需用户另装 OpenSSL；测试覆盖自签名本机服务连接及过期证书拒绝。

**这些 ZIP 是控制面板及其运行库，需要已有的本机 Sunshine 内核。** Windows 启动 `assets/gui/sunshine-gui.exe`，macOS 打开 `sunshine-gui.app`。它们是开发构建，未经过正式发布签名/公证。

## 完整 Windows 主机安装包

工作流：`Build and Release`（`.github/workflows/main.yml`）。

该流程使用默认 `release` 预设，编译原有 Sunshine 内核及 Qt 控制面板，输出安装器、便携 ZIP 和所需设备组件清单。打包阶段验证 Qt 运行库存在，并拒绝混入旧网页、WebView2 或旧 Tauri 插件。

fork 和手动构建使用运行编号生成版本信息，不需要上游 `setup-release-action` 的 Squash-only 合并设置或发布密钥。上游官方仓库仍保留原有发布流程。

完整构建依然需要主机的 Windows 编译依赖和设备 SDK；独立 Qt 任务可单独验证和下载界面。未经实际 Actions 执行，不能把本地工作流检查当作完整 Windows 打包成功。

## GitHub Release 发布

fork 的 `Build and Release` 会复用 `Build Qt Packages`，等待完整 Windows 包、Windows/macOS 面板以及主机测试全部通过后，自动发布一个 Qt **预发布 Release**。PR 只构建和测试，不发布。发布使用当前仓库的 `GITHUB_TOKEN`，无需配置 `GH_BOT_TOKEN`。

Release 同时包含完整 Windows 安装器、便携包、可选 DualSense 组件、Windows 独立面板和 **macOS ARM64 独立面板**，另附统一的 `SHA256SUMS.txt` 和 `release-build-info.json`。Windows 独立面板的 Release 文件名使用 `win-x64`，避免旧更新器把含 `Windows` 的面板 ZIP 误当成完整安装器。

**macOS ARM64 包目前只包含 Qt 界面，不包含串流内核。完整 macOS 串流主机版尚未完成。** Release 标题和说明会明确产品范围，开发预览包未签名/公证。

可以在 `Build and Release` 的手动运行选项中开启 `publish_existing`，填写成功的 `host_run_id` 和 `panel_run_id`，直接发布已经验证的产物，不必再次编译。运行工作流的分支必须与两份产物的来源分支一致。

发布前会验证工作流来源、状态、源码兼容性、平台元数据和 SHA-256；完整主机包也会检查内核及 Qt 文件是否齐全、是否混入旧界面。所有文件上传并核对远端摘要后才公开草稿。标签为 `qt-<主机构建提交的前12位>`，指向实际构建的主机源码；重复运行不会覆盖已发布且内容不同的 Release。

## 本地验证同一打包脚本

先完成 Qt 界面安装，再运行：

```sh
python scripts/package-qt-panel.py --staging staging --output artifacts --platform macos-arm64 --qt-version 6.12.0 --verify-run
```

Windows 将 `--platform` 改为 `windows-x64`。脚本检查必需的 Qt 库、平台插件和 QML 导入，生成 ZIP 后重新解压，启动解压后的程序验证运行时依赖。
