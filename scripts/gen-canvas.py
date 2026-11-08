#!/usr/bin/env python3
"""Generates libraries/canvas.xel (the bitmap font table is embedded from scripts/font8x14.json,
rendered from Bitstream Vera Sans Mono, which permits embedding)."""
import json, os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rows = json.load(open(os.path.join(root, "scripts/font8x14.json")))
font_calls = "\n".join('        load_glyph(font, %d, "%s")' % (c - 32, "".join("%02x" % r for r in rows[str(c)])) for c in range(32, 127))
template = open(os.path.join(root, "scripts/canvas.template")).read()
open(os.path.join(root, "libraries/canvas.xel"), "w").write(template.replace("@@FONT@@", font_calls))
print("wrote libraries/canvas.xel")
