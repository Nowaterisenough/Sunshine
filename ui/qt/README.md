# Sunshine Qt control panel

Native C++ / Qt Quick control panel targeting **Qt 6.12.x**. The visual
reference is **Magpie, the AI desktop app**, based on the user's screenshots of
its Agent, Plugins and About/settings pages. It uses a dark neutral surface,
horizontal navigation, continuous settings rows, segmented controls, subtle
accent buttons and a three-column application grid. This is unrelated to the
WinUI window upscaler with the same name. See [design tokens](DESIGN.md).

The interface is Chinese; labels use `qsTr()` for future translations.

This is an independent local management process. Capture, encoding, audio,
GameStream pairing, transport, and the existing ABR algorithms remain in the
original Sunshine host. The panel uses its existing HTTPS API on
`127.0.0.1:47990`, with an editable management port. It does not embed a browser,
WebView2, Tauri, an AI assistant, analytics, advertisements, or a remote web UI.

## Build the panel

Install a matching Qt 6.12 C++ kit with Core, Gui, Network, Widgets, Quick,
QuickControls2, Svg and Test. On Windows, the Qt kit must match the compiler ABI;
do not mix MSVC and MinGW. The existing release job uses the MSYS2 UCRT64 Qt
packages. CMake rejects Qt versions outside the requested 6.12 series.

From the repository root:

```sh
cmake --preset qt-ui
cmake --build --preset qt-ui --parallel
ctest --preset qt-ui
cmake --install build/qt --prefix build/qt-install
```

If Qt is not discoverable, provide the installed kit using `CMAKE_PREFIX_PATH`
or use that kit's `qt-cmake`. No developer-specific Qt path is stored here.

The macOS CI job uses the native ARM64 `macos-26` runner and builds an
arm64-only executable. Packaging checks the executable architecture and ARM64
availability in every bundled Mach-O library/plugin; shared Qt libraries may
be universal. There is no Intel or universal application release variant.
See [runner allocation troubleshooting](../../docs/qt-actions.md#macos-runner-未分配时).

The panel can also be built without the host's submodules and dependencies:

```sh
cmake -S ui/qt -B build/panel -DCMAKE_BUILD_TYPE=Release
cmake --build build/panel --config Release --parallel
ctest --test-dir build/panel -C Release --output-on-failure
cmake --install build/panel --config Release --prefix build/panel-install
```

The executable is `sunshine-gui` (`sunshine-gui.exe` on Windows; a `.app` bundle
on macOS). Installation deploys the required Qt runtime and QML imports.
On Windows it installs under `assets/gui`, matching the host/service launcher.
The panel supports `--hidden`, `--url=<local-management-url>`, and the
installer's `--remove-autostart` argument. Multiple launches activate the same
per-user instance. Closing to the system tray leaves streaming running; the
tray's exit action exits only the panel. When no tray exists, closing exits.

## Build the default distribution

```sh
cmake --preset release
cmake --build --preset release --parallel
cpack --preset release
```

The default ZIP is written to `build/release/cpack_artifacts/Sunshine.zip`.
The Windows release workflow uses the same preset. With Inno Setup and the
host's sidecar package available, build the installer with
`cmake --build build/release --target innosetup`; that target builds both the
host and Qt panel before staging. `qt-host` remains an equivalent preset with
its own `build/qt-host` directory.

Release and development presets explicitly enable Qt, including when
reconfiguring an older build directory. While Qt is enabled, a cached or
command-line `BUILD_WEB_UI=ON` is normalized to `OFF`; Qt distributions cannot
accidentally include the legacy website. Qt discovery failure is fatal and
never falls back to downloading Tauri. Installer filters also exclude stale
legacy WebView2/stylus files and web assets from a reused staging directory.

The full host retains its original dependency/toolchain requirements. In this
fork, the VDD host sources currently include Windows headers unconditionally;
the independently built panel works on macOS, but a macOS host build is blocked
by that pre-existing platform dependency. Do not change capture/display code
merely to build the panel. The original crypto source also requires OpenSSL 3
rather than OpenSSL 4 with the tested toolchain.

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_QT_UI` | `ON` | Build/install the native panel; skip the old Tauri GUI download |
| `SUNSHINE_QT_ONLY` | `OFF` | Build only the panel, without configuring host dependencies |
| `SUNSHINE_QT_TESTS` | `ON` | Build Qt network contract tests |
| `SUNSHINE_ENABLE_AI` | `OFF` | Exclude LLM proxy, credential storage, provider and stream workers from the host |
| `BUILD_WEB_UI` | Forced `OFF` with Qt | Exclude the legacy Vue website even when an old cache enabled it |

With AI disabled, `src/ai/disabled.cpp` supplies the existing boundary symbols:
AI capability is false, the web AI routes are not registered, the legacy
GameStream AI endpoint returns 404, and ABR takes its existing non-LLM path.
It performs no credential reads, worker creation, or upstream network requests.
Core call sites remain unchanged. AI-only transport/provider regression tests
may still build their original implementation when `BUILD_TESTS=ON`; these test
executables are not linked into or shipped with the production host.

An already-installed legacy host is **not** modified when running just the Qt
panel. Disabling AI inside that host requires rebuilding it with the option
above. Legacy source remains available with
`-DBUILD_QT_UI=OFF -DBUILD_WEB_UI=ON -DSUNSHINE_ENABLE_AI=ON`.

## Implemented workflows

- Host status, active encoder and a dedicated live session page.
- Add, edit and remove applications; desktop streaming uses an empty command.
  Existing advanced application fields are preserved.
- PIN pairing, rename, enable/disable and unpair clients, with confirmation for
  operations that interrupt a session.
- Automatic tray-adjacent PIN window for incoming Moonlight pairing requests,
  including while the main panel is hidden. A tray menu action reopens a
  dismissed request; ordinary connections from paired clients do not prompt.
- General, audio/video, input and network settings; save/discard drafts and
  restart through the acknowledged tray API.
- Plain-text log filtering and export; bounded log reads, with no AI diagnosis.
- First-run account creation, local port selection and portable host launch.
- Dark appearance by default, optional light theme, vector icons, native tray and single instance.
- Application search/type filters and settings search across all categories.
- Native update checking, release notes, verified downloads and Windows installer handoff.

Saving configuration first reads the latest complete API snapshot and overlays
only edited fields. Changes to the same field in another editor are rejected.
Readonly response metadata is excluded. App mutations verify that the indexed
list has not changed and invalidate the editor when it has. The server has no
atomic revision/ETag facility, so an external write *between* the final read
and write cannot be eliminated without changing its API.

The host API client only connects to loopback, bypasses HTTP proxies, does not follow
redirects, and only permits the self-signed/hostname mismatch errors needed by
Sunshine's local certificate. Other TLS errors remain fatal. First-run
`/welcome` redirects open the native setup form. Passwords are not persisted
by the panel.

## Incoming pairing requests

While the Qt panel is running (including in the tray), it subscribes to the
host's existing `/api/tray/events` SSE endpoint. No streaming-host changes or
high-frequency polling are required. The current fork's tray protocol v1
provides the core instance, notification identity, revision and client name.
Repeated state events do not reopen a dismissed popup. A fresh request, even
from the same device, opens a new form with an empty PIN field. The popup is
placed inside the tray screen's available desktop area and does not navigate
the main window away from an unsaved form.

Enter the four digits shown by Moonlight and optionally edit the device name.
The panel rereads `/api/tray/state` before posting to `/api/pin`; a changed or
expired request is not knowingly submitted. This preflight is not an atomic
request binding: the existing PIN endpoint has no request-id argument. Changing
the host port aborts outstanding operations and discards old responses.
The Qt panel does not persist PINs or include them in its diagnostic output. A successful API reply
means **PIN submitted**, not completion of Moonlight's subsequent cryptographic
pairing handshake; the notification tells the user to confirm in Moonlight.

Expiry, cancellation and disconnection invalidate the form. The listener uses
the same bounded loopback-only TLS exception as the management client, accepts
no redirects, limits frame sizes, detects silent connections, and reconnects
with backoff. Exiting the panel ends this listener. A host without the tray SSE
API still supports the manual Clients page but cannot send automatic prompts.

## Updates

Settings > About and the tray menu provide **Check for updates**. A top-bar
button appears when a new version is available. The About page follows the
reference's grouped rows for version, release notes, update button, automatic
checks, interval, channel, and automatic download. Preferences persist locally.

The updater reads the latest 100 GitHub releases from
`Nowaterisenough/Sunshine`, accepting published `qt-<commit>` releases only.
The default channel includes prereleases (the fork's Qt releases currently use
that channel). Stable-only mode never falls back to legacy upstream builds.
Git ancestry comparison prevents downgrade or switching to an unrelated branch.
The panel embeds its own source commit, rather than comparing the connected
host's version string or a constant `1.0.0`.

| Running distribution | Selected update | After download |
| --- | --- | --- |
| Windows x64 installed host (`unins000.exe` beside `sunshine.exe`) | Full Windows installer | Explicit confirmation, UAC, then exit panel and run Inno upgrade |
| Windows x64 portable host | Portable x64 ZIP | Open download directory; exit and replace manually |
| Windows x64 standalone panel | Qt panel win-x64 ZIP | Open download directory; replace the panel manually |
| macOS arm64 panel | Qt panel macos-arm64 ZIP | Open download directory; exit and replace the App manually |

Automatic checks default to once at startup and every hour; intervals are
30 minutes, 1 hour, 6 hours, and 24 hours. Automatic downloading is **off** by
default. No mode installs automatically or interrupts streaming without an
explicit installation confirmation. ZIP packages do not perform installer
cleanup or in-place replacement, and the macOS package remains panel-only.

The updater has its own network manager with normal TLS verification and
system proxy support. It never inherits the loopback client's self-signed
certificate exception. HTTPS redirects are limited to GitHub release/API/CDN
hosts, package names are matched by platform and installation type, and a
GitHub SHA-256 digest is required. Downloads stream to an atomic cache file,
check size and hash, and are checked again before opening. Cancellation,
network failures, insufficient disk space, and failed elevation leave a
retryable state. No executable is run by the update tests.

## Verification

`panel_client_tests` exercises configuration preservation/conflicts, legacy
string and boolean status handling, setup redirects, malformed responses,
app index races, PIN validation, stale replies after port changes, log export,
and the acknowledged restart endpoint.

`panel_update_tests` covers package selection, channels, ancestry, unsafe
metadata/redirects, TLS and HTTP errors, cancellation/retry, download integrity,
post-download file changes, preferences, and explicit installer handoff through
an injected launcher. CI retains successful rendered screenshots on both
platforms; Windows also renders an 820 x 608 scene at 125% scale.

`panel_pairing_tests` exercises fragmented event streams, deduplication, core
restarts, disconnect/reconnect, stale requests and ports, PIN validation, failed
submissions and retry. HTTPS UI smoke also checks automatic opening with the
main window hidden, duplicate-event suppression, and expiry-driven dismissal.

The HTTPS smoke test runs the **actual application** against a temporary local
TLS fixture. It checks all eight pages, every settings tab, both themes,
compact layouts and first-run setup, then fails on QML runtime errors. Fixtures
are explicitly labelled as test data; they do not represent a real stream.

```sh
python3 ui/qt/tests/smoke.py \
  --binary build/qt/ui/qt/sunshine-gui.app/Contents/MacOS/sunshine-gui \
  --output build/qt-screenshots
```

Use the platform's actual executable path on Windows/Linux. Smoke mode uses
temporary settings and IPC, and never contacts or changes an installed host.
Hardware encoding, Moonlight end-to-end streaming, Windows service launch and
installer behavior still require validation on a Windows machine with the
host's drivers. The `Build Qt Packages` workflow produces downloadable Windows
x64 and macOS arm64 ZIPs with their Qt runtime and checksums. The main Windows
workflow builds the full host. See [Actions packaging](../../docs/qt-actions.md).

Visual reference: the user-provided Magpie AI screenshots (Agent list,
Plugins grid and About/settings). No Magpie source, logos or fonts are bundled.
The header keeps Sunshine's own identity and the navigation operates on real
Sunshine features; no AI functions or placeholder plugin actions are added.
