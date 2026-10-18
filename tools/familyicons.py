#!/usr/bin/env python3
"""Renders every Novu application's mark to design/icons/<App>-256.png.

One drawing, one rule, seven products. The gem is the family's isometric cube
and the only thing that changes between products is the hue, exactly as
novu::ui::AppMark::paintGem() does it at run time:

    left  = mix(accent, black, 0.22)
    right = accent
    top   = mix(accent, white, 0.33)

So a PNG here and the gem on that application's own splash screen are the same
drawing in the same colours, and neither can drift without the other.

Where a product already keeps a mark of its own in its repository, that file
is the authority and is used instead of the rule - a committed asset is a
decision somebody made, and regenerating over it would quietly undo it.

    python tools/familyicons.py           # writes the PNGs
    python tools/familyicons.py --check   # reports, writes nothing

Needs Pillow. Each image is drawn at its own resolution rather than scaled
down from a larger one.
"""

from __future__ import annotations

import argparse
import os
import re
import sys

try:
    from PIL import Image, ImageDraw
except ImportError:  # pragma: no cover - a clear message beats a stack trace
    sys.exit("familyicons needs Pillow:  python -m pip install Pillow")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "design", "icons")
SIBLINGS = os.path.dirname(ROOT)

SIZE = 256

# Air left around the mark, per side, as a fraction of the box - the same
# value NovuExplorer's icongen.py uses, so a family sheet and a Windows icon
# sit at one visual weight.
MARGIN = 0.059

# Rendered this many times oversized and resampled down, which is what keeps
# the diagonals clean.
SUPERSAMPLE = 8

# The gem, on its own 48-unit grid, windowed to the 29-unit square around its
# centre. These are AppMark::paintGem()'s own numbers.
WINDOW = (9.5, 9.625, 29.0)
LEFT = [(23.58, 23.76), (23.58, 10.74), (12.30, 17.25), (12.30, 30.27)]
RIGHT = [(24.42, 23.76), (24.42, 10.74), (35.70, 17.25), (35.70, 30.27)]
TOP = [(24.00, 24.49), (35.27, 31.00), (24.00, 37.51), (12.73, 31.00)]

# Every application, with the accent it actually runs in and where that is
# written down. Listed rather than discovered: "is this a product" is a fact
# about the company, and novu::ui::isApplicationProfile() keeps the same list.
#
# NovuExplorer is the one worth reading twice. NovuUI's own table still carries
# the teal #58B09E it was given before the Steel decision; the application
# overrides Token::Accent to #78919C in installExplorerPalette() and that is
# what its splash is painted in. The running colour wins - the library's table
# is stale, not authoritative.
# A fourth column, where a product states its three faces outright instead of
# leaving them to be derived. NovuSource is the case: its ramp is hand-chosen
# and is not what the rule would produce from its accent, so deriving one
# would draw a mark its own application does not.
APPS = [
    ("NovuSpace",    "#D49335", "NovuUI ThemeProfile.cpp novuSpaceProfile()", None),
    ("NovuExplorer", "#78919C", "NovuExplorer installExplorerPalette() - Steel", None),
    ("NovuHub",      "#3CCB7F", "NovuUI AppProfiles.cpp novuHubProfile()", None),
    ("NovuAdmin",    "#3CCB7F", "NovuUI AppProfiles.cpp novuAdminProfile()", None),
    ("NovuAI",       "#4DA3FF", "NovuUI NovuAiProfile.cpp kAccent", None),
    ("NovuVoice",    "#C47B85", "NovuUI AppProfiles.cpp novuVoiceProfile()", None),
    ("NovuTrade",    "#10b981", "NovuUI ThemeProfile.cpp novuTradeProfile()", None),
    ("NovuSource",   "#7B6CF6", "NovuSource ui/Icons.cpp appMark()",
     ("#5B4BD6", "#7B6CF6", "#9C90FF")),
]


def hex_to_rgb(value: str) -> tuple[int, int, int]:
    value = value.lstrip("#")
    return tuple(int(value[i:i + 2], 16) for i in (0, 2, 4))


def mix(a: tuple[int, int, int], b: tuple[int, int, int],
        t: float) -> tuple[int, int, int]:
    """The same truncating mix AppMark::paintGem() uses, so the numbers match
    it exactly rather than nearly."""
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def faces_from_accent(accent: str):
    a = hex_to_rgb(accent)
    return [(LEFT, mix(a, (0, 0, 0), 0.22)),
            (RIGHT, a),
            (TOP, mix(a, (255, 255, 255), 0.33))]


def faces_from_ramp(ramp):
    left, right, top = (hex_to_rgb(c) for c in ramp)
    return [(LEFT, left), (RIGHT, right), (TOP, top)]


def faces_from_svg(path: str):
    """The polygons a product keeps for itself, read from its own file."""
    with open(path, "r", encoding="utf-8") as handle:
        text = handle.read()
    out = []
    for tag in re.findall(r"<polygon[^>]*>", text):
        points = re.search(r'points="([^"]+)"', tag)
        fill = re.search(r'fill="(#[0-9A-Fa-f]{6})"', tag)
        if not points or not fill:
            sys.exit(f"{path}: a polygon without points or fill")
        pairs = [tuple(float(n) for n in pair.split(","))
                 for pair in points.group(1).split()]
        out.append((pairs, hex_to_rgb(fill.group(1))))
    if len(out) != 3:
        sys.exit(f"{path}: expected three faces, found {len(out)}")
    return out


def own_mark(app: str) -> str | None:
    """That product's own mark file, if its repository is beside this one."""
    path = os.path.join(SIBLINGS, app, "design", f"{app}-mark.svg")
    return path if os.path.exists(path) else None


def render(faces, size: int) -> Image.Image:
    big = size * SUPERSAMPLE
    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    vx, vy, vw = WINDOW
    inset = big * MARGIN
    span = big - 2 * inset
    unit = span / vw

    for points, colour in faces:
        draw.polygon([((x - vx) * unit + inset, (y - vy) * unit + inset)
                      for x, y in points], fill=colour)

    return image.resize((size, size), Image.LANCZOS)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true",
                        help="report what would be written, and write nothing")
    args = parser.parse_args()

    if not args.check:
        os.makedirs(OUT_DIR, exist_ok=True)

    print("%-14s %-9s %-10s %s" % ("application", "accent", "source", "faces"))
    for app, accent, where, ramp in APPS:
        mark = own_mark(app)
        if mark:
            faces = faces_from_svg(mark)
            origin = "own mark"
        elif ramp:
            faces = faces_from_ramp(ramp)
            origin = "own ramp"
        else:
            faces = faces_from_accent(accent)
            origin = "accent"

        ramp = " ".join("#%02X%02X%02X" % colour for _points, colour in faces)
        print("%-14s %-9s %-10s %s" % (app, accent, origin, ramp))
        if origin == "own mark":
            print("%-14s %-9s %-10s %s" % ("", "", "", os.path.relpath(mark, SIBLINGS)))
        else:
            print("%-14s %-9s %-10s %s" % ("", "", "", where))

        if args.check:
            continue
        out = os.path.join(OUT_DIR, f"{app}-256.png")
        render(faces, SIZE).save(out, format="PNG", optimize=True)

    if args.check:
        print("\n--check: nothing written")
        return
    print("\nwritten to", os.path.relpath(OUT_DIR, ROOT))
    for name in sorted(os.listdir(OUT_DIR)):
        path = os.path.join(OUT_DIR, name)
        with Image.open(path) as image:
            print("   %-24s %d B  %dx%d"
                  % (name, os.path.getsize(path), image.size[0], image.size[1]))


if __name__ == "__main__":
    main()
