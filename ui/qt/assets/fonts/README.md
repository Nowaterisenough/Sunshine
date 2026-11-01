# Windows Chinese UI font

Noto Sans CJK SC is distributed under the SIL Open Font License 1.1. The complete
license is in `OFL-NotoSansSC.txt` and is installed alongside the Windows panel.
The unmodified static Regular face avoids variable-font selection differences
between Windows font engines.

Source: https://github.com/notofonts/noto-cjk/tree/f8d157532fbfaeda587e826d4cd5b21a49186f7c/Sans

- Original filename: `OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf`
- Font Git blob: `dc15562470b4f842321894787a0d066879ccff8b`
- License Git blob: `d952d62c065f3f35fb83a173496e90b21525aef3`

Windows loads this font from `fonts/` before creating the UI. Package validation
requires both the font and license. Startup checks Chinese glyph coverage, and
smoke mode checks the font actually resolved for the QML application window.
macOS uses its system PingFang SC font and does not ship this font file.
