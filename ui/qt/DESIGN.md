# Magpie AI visual reference

Reference: the macOS settings screenshot supplied on 2026-11-03, plus the
previous Windows side-by-side from 2026-11-01. They show the
AI app **magpie**, not Blinue/Magpie. The target screenshot canvas is 1004 x 631.
This document replaces the earlier, incorrect WinUI design reference.

| Element | Qt implementation |
| --- | --- |
| Window | 1004 x 631 default; Windows 8 px / macOS 14 px outer corner; one-pixel outline |
| Main background | #1a1a1e |
| Group/card background | #232327 |
| Input/segment background | #2d2d33 |
| Group outline | #333339 |
| Row divider | #2c2c32 |
| Primary text | #ededf1 |
| Secondary text | #b9b9c1 |
| Muted text | #8b8b94 |
| Accent text / pill | #a5b4fc / #2b2c47 |
| Main navigation | Windows: separate 32 px caption and centered tabs at y=40; macOS: compact 51 px single row |
| Content | 18 px side margins; starts 6 px below chrome, or below settings tabs |
| Grouped rows | Windows 54 px / macOS 57 px minimum; wrapped descriptions grow the row; no gaps; 12 px group radius |
| Typography | Bundled Noto Sans CJK SC on Windows / PingFang SC on macOS; 14 px titles; 13 px descriptions, segment labels and text fields |
| Settings switches | 28 px segmented control with close/open choices |
| Device switches | 36 x 20 px; white on-state track and dark thumb |
| Application grid | Three columns at full width, 12 px gaps, 144 px cards |

Colors were sampled from the supplied files. Layout and component behavior are
implemented in Qt Quick; no original app source, brand assets or fonts are
copied. macOS gets traffic-light window controls, Windows gets appropriate
caption controls. Plain QML Text uses the same explicit sans-serif family as
Controls, through PanelText; font selection is logged by the screenshot runner.
Windows ships Noto Sans CJK SC under the SIL Open Font License, avoiding serif
fallback on machines without the optional Chinese system fonts. The runner
forces stderr logging on GUI executables and requires the font diagnostic.
The Windows layout is checked on Windows, since macOS rendering cannot verify
Windows CJK fallback or caption geometry.

The main pages expose actual Sunshine functionality: host, sessions,
applications, clients and logs. The gear opens settings with general,
audio/video, input, network, connection and about categories. Search filters
real configuration options; application filters preserve original app indices.

Validation uses screenshots of the real application with explicitly labelled
HTTPS fixture data. The fixture does not claim a real stream is running.

The 2026-11-03 settings screenshot is the active layout reference: navigation
is across the top on every desktop platform. The reference selects 110% text size: body 13 CSS px maps to 14 logical px,
segment labels 12 CSS px to 13 px, and category tabs use 12 px semibold
text on a 24 px flat strip; search stays at its right edge. At 1004 x 631 on
macOS the settings group begins at y=91 with an 18 px side inset. The content
scrolls to the bottom window border; there is no reserved empty footer or
sidebar mode. Sunshine page names and real host settings remain its own.
