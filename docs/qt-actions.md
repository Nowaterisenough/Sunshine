# GitHub Actions 自动生成 Qt 版本

仓库包含两条构建路径。工作流和 Qt 源码推送到 GitHub 后，相关 push 或 pull request 会自动构建；工作流进入默认分支后也可以在 Actions 中手动运行。

## Qt 独立控制面板

工作流：`Build Qt Packages`（`.github/workflows/qt-panel.yml`）。

- Windows Server 2022 runner：MSVC 2022 x64，Qt 6.12.0 的 `win64_msvc2022_64` kit。
- macOS 默认使用 `macos-15-intel` runner 交叉编译，Qt 6.12.0 的 `clang_64` kit，发行主程序仅编译 arm64。构建机器和产物架构分别记录；不会发布 Intel 或通用版主程序。
- 自动执行配置、编译、接口测试、Qt 运行库部署、真实页面 HTTPS 测试、压缩及解压验证。跨架构时，接口／页面测试运行在同一提交的临时本机构建上；发行包单独进行 ARM64 架构检查。原生 runner 还会启动解压后的发行程序。
- 不依赖主机的第三方子模块、驱动 SDK、发布令牌或上游仓库合并设置。
- `contents: read` 权限，构建产物保留 30 天；失败时上传 CMake/测试诊断、页面截图和进程日志。

在对应 workflow run 的 **Artifacts** 下载：

| Artifact | 内含 ZIP |
| --- | --- |
| `Sunshine-Qt-Panel-windows-x64` | `Sunshine-Qt-Panel-6.12.0-windows-x64.zip` |
| `Sunshine-Qt-Panel-macos-arm64` | `Sunshine-Qt-Panel-6.12.0-macos-arm64.zip` |

macOS 包校验会拒绝 x86_64 或通用版主程序，并检查包内每个 Mach-O 框架／插件均包含 arm64；Qt 自身的通用运行库可以保留。`build-info.json` 记录实际主程序架构和构建机器架构。

每个 artifact 还包含 `SHA256SUMS.txt` 和 `build-info.json`，记录源码提交、Qt 版本和构建地址。macOS 使用 `ditto` 保留 `.app` 的符号链接与权限。包内附有许可证和启动说明。macOS 使用系统 Secure Transport 后端，无需用户另装 OpenSSL；测试覆盖自签名本机服务连接及过期证书拒绝。

**这些 ZIP 是控制面板及其运行库，需要已有的本机 Sunshine 内核。** Windows 启动 `assets/gui/sunshine-gui.exe`，macOS 打开 `sunshine-gui.app`。它们是开发构建，未经过正式发布签名/公证。

### macOS runner 与 ARM64 验证

如果任务没有执行任何步骤，并报告 `The job was not acquired by Runner of type hosted`
或 ARM64 capacity constraints，这是 GitHub runner 分配失败；增加 job 的构建超时不会延长分配期限。
2026-11-03 的 `macos-15` 分配失败，切换到 `macos-26` 后仍排队，因此默认采用 Intel 池交叉编译 ARM64 产品。

- `build/panel` / `staging` 始终是 **arm64 产品**，只有它进入 `artifacts`。
- Intel runner 上额外使用 `build/panel-native` / `validation` 编译同一源码的 x86_64 测试程序，执行契约测试、实际页面与配对弹窗测试。这个临时测试程序不会打包、上传或发布。
- 包校验要求主程序架构严格为 arm64，并检查 Qt 框架／插件的 arm64 支持。它不会把在 Intel 上检查文件误报成已在 ARM64 上执行。
- ARM64 容量恢复后，可通过手动输入 `macos_runner=macos-26` 使用原生 ARM64 runner；此时契约测试、页面测试、解压启动验证直接作用于 ARM64 构建，无需临时测试程序。`workflow_call` 同样支持该参数。

```bash
gh workflow run qt-panel.yml --ref codex/qt-actions -f macos_runner=macos-26
```

默认交叉构建的原生 Apple Silicon 验收：下载 CI 的 `macos-arm64` ZIP 后，在 ARM64 Mac 上解压，运行
`ui/qt/tests/smoke.py --binary <解压后的 .app/Contents/MacOS/sunshine-gui> --output build/qt-arm64-validation`。
这会实际运行同一个发行目标文件，验证页面、HTTPS 与配对流程。正式发布前应完成这项验收或原生 ARM64 CI。
不要通过跳过 macOS、`continue-on-error` 或把 Intel 程序标记为 ARM64 来通过流水线。

## 完整 Windows 主机安装包

工作流：`Build and Release`（`.github/workflows/main.yml`）。

该流程使用默认 `release` 预设，编译原有 Sunshine 内核及 Qt 控制面板，输出安装器、便携 ZIP 和所需设备组件清单。打包阶段验证 Qt 运行库存在，并拒绝混入旧网页、WebView2 或旧 Tauri 插件。

Qt 安装器覆盖旧版时会删除安装目录内的旧 `assets/web`、`assets/gui/WebView2Loader.dll` 和 `assets/gui/alkaidlab-plugin-stylus.dll`，并用 Qt 程序覆盖同名的 `sunshine-gui.exe`。旧 GUI 文件的打包排除与升级删除共用同一份名单；显式关闭 Qt 的兼容构建不会删除它需要的 GUI DLL。

清理不会清空整个 GUI 或安装目录，也不会删除配置、应用列表、配对数据、封面、WebView 用户配置或系统共享的 WebView2 运行环境。Windows CI 会使用生产模板生成的清理规则，在临时安装目录执行真实 Inno Setup 覆盖安装，检查 Qt/兼容两种模式及重复升级。测试不安装真实服务或驱动。**便携 ZIP 直接覆盖解压不会执行安装器清理规则。**

fork 和手动构建使用运行编号生成版本信息，不需要上游 `setup-release-action` 的 Squash-only 合并设置或发布密钥。上游官方仓库仍保留原有发布流程。

完整构建依然需要主机的 Windows 编译依赖和设备 SDK；独立 Qt 任务可单独验证和下载界面。未经实际 Actions 执行，不能把本地工作流检查当作完整 Windows 打包成功。

## GitHub Release 发布

fork 的 `Build and Release` 会复用 `Build Qt Packages`，等待完整 Windows 包、Windows/macOS 面板以及主机测试全部通过后发布。**`master` 发布正式 Release 并标记 Latest；开发分支的手动构建发布预发布版，不占用 Latest。** PR 只构建和测试，不发布。发布使用当前仓库的 `GITHUB_TOKEN`，无需配置 `GH_BOT_TOKEN`。

Release 同时包含完整 Windows 安装器、便携包、可选 DualSense 组件、Windows 独立面板和 **macOS ARM64 独立面板**，另附统一的 `SHA256SUMS.txt` 和 `release-build-info.json`。Windows 独立面板的 Release 文件名使用 `win-x64`，避免旧更新器把含 `Windows` 的面板 ZIP 误当成完整安装器。

**macOS ARM64 包目前只包含 Qt 界面，不包含串流内核。完整 macOS 串流主机版尚未完成。** Release 标题和说明会明确产品范围及签名／公证状态。正式 Release 表示 GitHub 发布渠道；不会将未签名文件宣称为已签名。

可以在 `Build and Release` 的手动运行选项中开启 `publish_existing`，填写成功的 `host_run_id` 和 `panel_run_id`，直接发布已经验证的产物，不必再次编译。运行工作流的分支必须与两份产物的来源分支一致。

复用历史提交时，由维护者先推送指向该主机构建提交的 `qt-<提交前12位>` Git 标签。这样内置令牌只发布已验证的标签，无需为旧提交创建引用；正常构建会自行创建当前提交的标签。草稿按 Release ID 校验，重试会复用已经上传且摘要一致的文件。

发布前会验证工作流来源、状态、源码兼容性、平台元数据和 SHA-256；完整主机包也会检查内核及 Qt 文件是否齐全、是否混入旧界面。所有文件上传并核对远端摘要后才公开草稿。`--stable` 只允许从 `master` 使用，设置 `prerelease=false` 和 Latest；已有同内容预发布版可以升为正式版，已发布的正式版不会被预发布重跑降级。标签为 `qt-<主机构建提交的前12位>`，指向实际构建的主机源码；重复运行不会覆盖已发布且内容不同的 Release。

## 本地验证同一打包脚本

先完成 Qt 界面安装，再运行：

```sh
python scripts/package-qt-panel.py --staging staging --output artifacts --platform macos-arm64 --qt-version 6.12.0 --verify-run
```

Windows 将 `--platform` 改为 `windows-x64`。脚本检查必需的 Qt 库、平台插件和 QML 导入，生成 ZIP 后重新解压，启动解压后的程序验证运行时依赖。
