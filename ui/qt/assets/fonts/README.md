# Windows Chinese UI font

Noto Sans SC is distributed under the SIL Open Font License 1.1. The complete
license is in `OFL-NotoSansSC.txt` and is installed alongside the Windows panel.
The unmodified variable font is renamed locally for a shell-safe filename.

Source: https://github.com/google/fonts/tree/a85815a42757630ce188fdad368c2dfc444d4773/ofl/notosanssc

- Original filename: `NotoSansSC[wght].ttf`
- Font Git blob: `fb0637bafbcd804fe32152370a1225990745b4bc`
- License Git blob: `1c9f43281b8f216c5461fe9ac729afbade7724e4`

Windows loads this font from `fonts/` before creating the UI. Package validation
requires both the font and license, and startup checks Chinese glyph coverage.
macOS uses its system PingFang SC font and does not ship this font file.
