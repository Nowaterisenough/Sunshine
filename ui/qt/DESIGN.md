# Magpie AI visual reference

Reference: the macOS screenshots and the Windows side-by-side supplied on
2026-11-01. They show the
AI app **magpie**, not Blinue/Magpie. The target screenshot canvas is 1004 x 631.
This document replaces the earlier, incorrect WinUI design reference.

| Element | Qt implementation |
| --- | --- |
| Window | 1004 x 631 default; Windows 8 px / macOS 14 px outer corner; one-pixel outline |
| Main background | #1a1a1f |
| Group/card background | #232327 |
| Input/segment background | #2d2d32 |
| Group outline | #33333a |
| Row divider | #2d2d34 |
| Primary text | #ededf1 |
| Secondary text | #a3a3ac |
| Muted text | #919199 |
| Accent text / pill | #aab6fa / #2b2d47 |
| Main navigation | Windows: separate 32 px caption and centered tabs at y=40; macOS: compact 48 px single row |
| Content | 18 px side margins; starts 6 px below chrome, or below settings tabs |
| Grouped rows | Windows 54 px / macOS 66 px minimum; no gaps; 12 px group radius |
| Typography | Explicit Microsoft YaHei UI / PingFang SC; 14 px titles; 13 px descriptions and controls |
| Settings switches | 30 px segmented control with close/open choices |
| Device switches | 36 x 20 px; white on-state track and dark thumb |
| Application grid | Three columns at full width, 12 px gaps, 144 px cards |

Colors were sampled from the supplied files. Layout and component behavior are
implemented in Qt Quick; no original app source, brand assets or fonts are
copied. macOS gets traffic-light window controls, Windows gets appropriate
caption controls. Plain QML Text uses the same explicit sans-serif family as
Controls, through PanelText; font selection is logged by the screenshot runner.
The Windows layout is checked on Windows, since macOS rendering cannot verify
Windows CJK fallback or caption geometry.

The main pages expose actual Sunshine functionality: host, sessions,
applications, clients and logs. The gear opens settings with general,
audio/video, input, network, connection and about categories. Search filters
real configuration options; application filters preserve original app indices.

Validation uses screenshots of the real application with explicitly labelled
HTTPS fixture data. The fixture does not claim a real stream is running.
