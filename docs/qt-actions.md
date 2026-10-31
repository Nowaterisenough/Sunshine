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

fork 和手动构建使用运行编号生成版本信息，仅上传 Artifacts，不需要上游 `setup-release-action` 的 Squash-only 合并设置或发布密钥。上游官方仓库仍保留原有发布流程。

完整构建依然需要主机的 Windows 编译依赖和设备 SDK；独立 Qt 任务可单独验证和下载界面。未经实际 Actions 执行，不能把本地工作流检查当作完整 Windows 打包成功。

## 本地验证同一打包脚本

先完成 Qt 界面安装，再运行：

```sh
python scripts/package-qt-panel.py --staging staging --output artifacts --platform macos-arm64 --qt-version 6.12.0 --verify-run
```

Windows 将 `--platform` 改为 `windows-x64`。脚本检查必需的 Qt 库、平台插件和 QML 导入，生成 ZIP 后重新解压，启动解压后的程序验证运行时依赖。
