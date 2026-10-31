# Magpie AI visual reference

Reference: three screenshots supplied by the user on 2026-10-31. They show the
AI app **magpie**, not Blinue/Magpie. The target screenshot canvas is 1004 x 631.
This document replaces the earlier, incorrect WinUI design reference.

| Element | Qt implementation |
| --- | --- |
| Window | 1004 x 631 default; 18 px outer corner; one-pixel outline |
| Main background | #1a1a1f |
| Group/card background | #232327 |
| Input/segment background | #2d2d32 |
| Group outline | #33333a |
| Row divider | #2d2d34 |
| Primary text | #ededf1 |
| Secondary text | #b7b7c0 |
| Muted text | #919199 |
| Accent text / pill | #aab6fa / #2b2d47 |
| Main navigation | Horizontal; 50 px header; 28 px selected tabs |
| Content | 18 px side margins; starts at y=54, or y=92 below settings tabs |
| Grouped rows | 62-66 px; no gaps or individual outlines; 14 px group radius |
| Typography | 14 px row titles; 13 px descriptions and controls; 15 px brand |
| Settings switches | 30 px segmented control with close/open choices |
| Device switches | 36 x 20 px; white on-state track and dark thumb |
| Application grid | Three columns at full width, 12 px gaps, 144 px cards |

Colors were sampled from the supplied files. Layout and component behavior are
implemented in Qt Quick; no original app source, brand assets or fonts are
copied. macOS gets traffic-light window controls, Windows gets appropriate
caption controls. Native text rasterization can differ across operating systems.

The main pages expose actual Sunshine functionality: host, sessions,
applications, clients and logs. The gear opens settings with general,
audio/video, input, network, connection and about categories. Search filters
real configuration options; application filters preserve original app indices.

Validation uses screenshots of the real application with explicitly labelled
HTTPS fixture data. The fixture does not claim a real stream is running.
