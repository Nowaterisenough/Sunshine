# Sunshine: sunrise on a screen

The mark combines a rounded display, a rising half sun and a short stand.
Warm amber represents sunlight; the display identifies the streaming product.
Three short rays distinguish the sun without using a ring of fine strokes.
The flat application icon uses solid charcoal (`#242429`) and amber (`#f2c475`),
without gradients, highlights, shadows, or an outer rim.

- `sunshine.svg` is the vector master for the application and installer icon.
- `sunshine.png`, `sunshine.ico` and `sunshine.icns` are generated from that master.
- `ui/qt/assets/sunshine-mark.svg` is the transparent amber window mark.
- `sunshine-mark-light.svg` uses a darker ochre for light backgrounds.
- `sunshine-tray.svg` is the monochrome macOS template, tinted by the system.

The Qt executable embeds ICO on Windows and ICNS in the macOS bundle. Runtime
icons are rendered at multiple resolutions rather than scaling one 64 px image.
The existing host/installer packaging also consumes the shared brand assets.

Regenerate the native files with the same Qt 6.12 renderer used by the panel:

```sh
cmake -S tools/brand-icons -B build/brand-icons
cmake --build build/brand-icons
build/brand-icons/brand-icons assets/brand/sunshine.svg assets/brand
```

PNG and ICO are generated on all supported build hosts. ICNS generation runs
on macOS using `iconutil`. The generated icons are committed so release builds
do not require an extra image-conversion tool.
