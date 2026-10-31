# 开发文档

[项目首页](../../README.md) · [文档目录](README.md) · [English](../en/development.md)

默认发行版由 Sunshine 主机服务与 Qt 6.12 桌面控制面板组成。旧 WebUI 和 Tauri 源码保留为兼容入口，不进入默认 Qt 包；可选设备组件仍有独立的安装与会话生命周期。

## 代码结构

| 模块 | 入口 | 负责的内容 |
| --- | --- | --- |
| 主机服务 | `src/` | 捕获、编码、音频、输入、配对与会话 |
| 客户端扩展 | `src/nvhttp/`、`src/rtsp.cpp`、`src/stream.cpp` | 协商、控制通道、动态参数、显示控制与网络反馈 |
| 视频与 HDR | `src/video*`、`src/hdr/`、`src/platform/windows/display*` | 色彩、元数据、显示目标和捕获路径 |
| 网络自适应 | `src/streaming/` | 码率与丢包保护 |
| 旧浏览器 WebUI（默认关闭） | `src_assets/common/assets/web/` | 管理界面、配置、应用列表与网页诊断 |
| 旧 Tauri GUI（默认关闭） | `src_assets/common/sunshine-control-panel/` | 设备中心、本地扫描、组件与系统操作；独立仓库子模块 |
| Qt 桌面控制面板 | `ui/qt/` | 主机管理、配对、应用、串流设置和日志；默认打包界面 |
| 设备组件 | `src/platform/windows/ds5/`、`src/remote_usb/` 等 | 可选设备、传输与进程生命周期 |
| 自动化检查 | `tests/`、WebUI `tests/`、`.github/workflows/` | 单元、集成与构建检查 |

## 获取源码

```bash
git clone --recurse-submodules https://github.com/AlkaidLab/foundation-sunshine.git
cd foundation-sunshine
```

已有工作目录时，用 `git submodule update --init --recursive` 获取记录的子模块版本。主程序与 GUI 协同修改时，分别记录两个仓库的提交和使用的构建包。

## 默认 Qt 发行包

使用 Qt 6.12.x 及与它 ABI 匹配的编译器；完整 Windows 主机构建继续使用现有 MSYS2 UCRT64 工具链和设备依赖。在仓库根目录执行：

```bash
cmake --preset release
cmake --build --preset release --parallel
cpack --preset release
```

ZIP 输出为 `build/release/cpack_artifacts/Sunshine.zip`。配置好 Inno Setup 和原有 sidecar 包后，可用 `cmake --build build/release --target innosetup` 生成 Windows 安装器，打包目标会先构建内核和 Qt 程序。

默认不构建、下载或打包旧 Tauri/Vue 界面，AI 默认关闭。Qt 构建会把旧缓存中的 `BUILD_WEB_UI=ON` 归一为 `OFF`；缺少 Qt 时配置失败，不会回退到旧界面。对已有构建目录可使用 `cmake --preset release -B build` 重新选择 Qt 发行配置。

只开发界面时使用 `qt-ui` 预设，详见 [Qt 构建说明](../../ui/qt/README.md)。旧界面必须显式指定 `-DBUILD_QT_UI=OFF -DBUILD_WEB_UI=ON`。

## 主机服务构建

依赖和平台步骤见[构建说明](https://github.com/AlkaidLab/foundation-sunshine/blob/master/docs/building.md)。Windows 官方打包流程见 [Build and Release](https://github.com/AlkaidLab/foundation-sunshine/blob/master/.github/workflows/main.yml)；构建文档中的通用示例与官方安装包路径并不完全相同。

修改可选组件时同时检查包来源、版本、运行能力和回退路径：

- [DualSense 组件生命周期](https://github.com/AlkaidLab/foundation-sunshine/blob/master/docs/windows_dualsense_component_lifecycle.md)
- [NVIDIA HDR 构建](https://github.com/AlkaidLab/foundation-sunshine/blob/master/docs/rtx_hdr_build.md)
- [主程序与 GUI 配对构建](https://github.com/AlkaidLab/foundation-sunshine/blob/master/docs/paired-gui-build.md)

## 旧 WebUI 开发（显式启用）

使用主仓库 `.node-version` 指定的 Node.js 和 `package.json` 指定的 npm 版本，在仓库根目录执行：

```bash
npm ci
npm run dev-server
```

开发服务器使用 `https://localhost:3000`，接口需要可访问的 Sunshine 实例。修改后运行与该界面相关的检查：

```bash
npm run lint:webui
npm run test:webui
npm run build
```

语言变更还需检查 `npm run i18n:validate`。完整说明见 [WebUI 开发指南](https://github.com/AlkaidLab/foundation-sunshine/blob/master/docs/WEBUI_DEVELOPMENT.md)。

## 旧 Tauri GUI 开发（显式启用）

GUI 位于子模块，对应 [sunshine-control-panel](https://github.com/qiin2333/sunshine-control-panel)。在该目录按其依赖要求安装 Node.js、Rust 与 Windows 原生构建工具，再执行：

```bash
npm ci
npm run dev
```

`dev` 会构建原生插件并启动 Tauri；纯界面开发可用 `npm run dev:renderer`。主仓库 WebUI 与 GUI 联调时，先在主仓库启动 `npm run dev-server`，再在 GUI 目录运行 `npm run dev-webui`。

仅显式关闭 Qt 的旧界面构建路径会下载 GUI release 包。需要验证主程序和固定 GUI 提交的组合时，使用[配对构建说明](https://github.com/AlkaidLab/foundation-sunshine/blob/master/docs/paired-gui-build.md)，避免把工作目录子模块版本当成发行包内 GUI 的版本。

## 如何验证修改

| 修改类型 | 应确认的结果 |
| --- | --- |
| 捕获或编码 | 实际连接的捕获方法、编码器、格式和异常回退；必要时比较帧率与延迟 |
| HDR 与色彩 | 协商、主机信息注入、控制端解码和显示效果分别核对 |
| 音频或输入 | 真实设备录音、播放或按键反馈，确认断线后释放状态 |
| 设备组件 | 安装、不可用、修复、卸载与会话结束后的清理 |
| WebUI 或 GUI | 实际操作流程，浏览器与桌面环境差异，中英文显示 |
| 文档 | 相对链接、章节锚点、双语导航和用户照步骤能否完成任务 |

没有运行的检查写明原因。能通过单元测试不等于已经验证客户端、驱动或硬件。PR 内容要求见[贡献指南](../contributing.md)。
