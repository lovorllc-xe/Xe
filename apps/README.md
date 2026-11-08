# Xe apps

Real programs written in Xe, built with the standard libraries of this repository.

## sorter - SentryAI image sorter (port of `sorter.py`)

A GUI tool for building image datasets, written entirely in Xe (`canvas`, `gui`, `window`, `image`, `dir`).

```sh
xe apps/sorter/sorter.xe -o sorter
./sorter                                   # start at the menu
./sorter SOURCE_DIR [CLASS_ROOT [LABELS_DIR]]   # preload folders
```

* **Class Sorter** - choose a source folder; step through its images (`<` `>` buttons or Left/Right keys) and copy the
  current one into a numbered file (`1.jpg`, `2.jpg`, ...) of any destination folder (button, or keys `1`-`9`).
* **Advanced Object** - same, plus bounding boxes: drag with the left mouse button on the image, pick the active class,
  then *Sort & Save* copies the image into the class folder and writes a YOLO label file
  (`class x_center y_center width height`, normalised, six decimals) to the labels folder. *Undo Box* and
  *Clear All Boxes* edit the boxes of the current image. *+ Add New Class Folder* creates `0`, `1`, `2`, ... in the class root.
* There is no native file dialog in Xe, so the app has its own **folder browser** (Browse... buttons): click folders to
  enter them, type a path and press Enter, `Up`, hidden-folder toggle, mouse wheel scrolling.
* Previews: PNG, baseline JPEG, BMP, PPM. WebP, GIF, TIFF and progressive JPEG files are listed and can be sorted
  (they are copied byte for byte) but show "cannot preview this format", and boxes cannot be drawn on them.
* Esc goes back to the menu; Esc in the menu quits. Text uses an ASCII bitmap font (non-ASCII folder names show as gaps).

Differences from the Python version: no native dialogs or message boxes (errors appear in the status line), the
two modes live in one window instead of two, and the glyphs `◀ ▶ ✕ ✓ ➔` became `< > x * ->`.

Developer options (environment): `XE_SORTER_SCREEN=1|2`, `XE_SORTER_FRAMES=n`, `XE_SORTER_SHOT=file.png`,
`XE_SORTER_SELFTEST=1` (non-interactive: draws one box on the first image, sorts it, writes the label, exits).
