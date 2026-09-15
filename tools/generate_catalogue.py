"""Regenerate docs/COMPONENTS.md from the gallery's own component specs.

The catalogue has one source of truth and it is not this file: every component
already declares its category, description, variants, states, properties, which
applications it is designed for and which consume it today, because the gallery
needs all of that to render. Writing it out a second time by hand is how a
catalogue starts disagreeing with the library it describes.

    python tools/generate_catalogue.py

Run it after adding or changing a component. The test
`theCatalogueMatchesTheRegistry()` fails if it was not.
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAGES = os.path.join(ROOT, "playground", "src", "pages")
OUT = os.path.join(ROOT, "docs", "COMPONENTS.md")

CATEGORY_ORDER = ["Foundation", "Input", "Navigation", "Data display", "Feedback"]

CATEGORY_ENUM = {
    "Category::Foundation": "Foundation",
    "Category::Input": "Input",
    "Category::Navigation": "Navigation",
    "Category::DataDisplay": "Data display",
    "Category::Feedback": "Feedback",
}


def literals(block, field):
    """Every QStringLiteral inside `spec.<field> = { ... };`."""
    m = re.search(r"spec\.%s\s*=\s*\{(.*?)\};" % field, block, re.S)
    return re.findall(r'QStringLiteral\("((?:[^"\\]|\\.)*)"\)', m.group(1)) if m else []


def one(block, field):
    m = re.search(r'spec\.%s\s*=\s*QStringLiteral\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)'
                  % field, block, re.S)
    if not m:
        return ""
    return "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))).replace('\\"', '"')


def entries(text):
    """Top-level `{...}` groups, by brace depth.

    A property entry contains a nested `{}` for its enum values, so splitting on
    braces or matching non-greedily loses everything after the first nested one -
    which is the description, the only part worth printing.
    """
    out = []
    depth = 0
    began = None
    for i, ch in enumerate(text):
        if ch == "{":
            if depth == 0:
                began = i + 1
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0 and began is not None:
                out.append(text[began:i])
                began = None
    return out


def properties(block):
    m = re.search(r"spec\.properties\s*=\s*\{(.*?)\n    \};", block, re.S)
    if not m:
        return []
    out = []
    for entry in entries(m.group(1)):
        # The nested enum-value group is dropped first, so what is left is
        # name, type, [default], description - in that order.
        flat = re.sub(r"\{[^{}]*\}", "", entry)
        found = re.findall(r'QStringLiteral\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)', flat)
        joined = ["".join(re.findall(r'"((?:[^"\\]|\\.)*)"', v)) for v in found]
        if len(joined) >= 2:
            note = joined[-1].replace('\\"', '"') if len(joined) > 2 else ""
            out.append((joined[0], joined[1], note))
    return out


def collect():
    specs = []
    for name in sorted(os.listdir(PAGES)):
        if not name.endswith(".cpp"):
            continue
        src = io.open(os.path.join(PAGES, name), encoding="utf-8", errors="replace").read()
        for m in re.finditer(r"ComponentSpec\s+\w+\(\)\s*\{(.*?)\n\}", src, re.S):
            b = m.group(1)
            if "spec.id" not in b:
                continue
            cat = re.search(r"spec\.category\s*=\s*(Category::\w+)", b)
            specs.append({
                "id": one(b, "id"),
                "name": one(b, "name"),
                "category": CATEGORY_ENUM.get(cat.group(1), "Foundation") if cat else "Foundation",
                "description": one(b, "description"),
                "header": one(b, "headerPath"),
                "source": one(b, "sourcePath"),
                "variants": literals(b, "variants"),
                "states": literals(b, "states"),
                "usedBy": literals(b, "usedBy"),
                "designedFor": literals(b, "designedFor"),
                "properties": properties(b),
                "page": name,
            })
    return specs


def render(specs):
    lines = []
    w = lines.append

    w("# NovuUI component catalogue")
    w("")
    w("**Generated — do not edit.** `python tools/generate_catalogue.py` rebuilds it")
    w("from the gallery's own component specs, which are what the Playground renders.")
    w("Editing this file by hand makes it disagree with the library.")
    w("")
    w("Every component here is real and previewable: open the Playground and pick it")
    w("from the navigation, or launch straight into it with `run.bat <id>`.")
    w("")
    w("- **Currently used by** is measured from each application's own sources.")
    w("  Empty means built and not yet adopted, which during extraction is most of them.")
    w("- **Designed for** is stated applicability — what the component was extracted for.")
    w("")
    w("See [README](../README.md) for the library's rules, [THEMING](THEMING.md) for how")
    w("styling resolves, and [EXTRACTION_STATUS](EXTRACTION_STATUS.md) for what is")
    w("deliberately missing.")
    w("")
    w("| | |")
    w("|---|---|")
    w("| components | %d |" % len(specs))
    for cat in CATEGORY_ORDER:
        n = len([s for s in specs if s["category"] == cat])
        if n:
            w("| %s | %d |" % (cat, n))
    w("")

    for cat in CATEGORY_ORDER:
        group = sorted([s for s in specs if s["category"] == cat], key=lambda s: s["name"])
        if not group:
            continue
        w("")
        w("## %s" % cat)
        w("")
        for spec in group:
            w("### %s" % spec["name"])
            w("")
            if spec["description"]:
                w(spec["description"])
                w("")
            w("```cpp")
            w('#include "%s"' % spec["header"].replace("include/", ""))
            w("```")
            w("")
            rows = []
            if spec["variants"]:
                rows.append(("Variants", ", ".join("`%s`" % v for v in spec["variants"])))
            if spec["states"]:
                rows.append(("States", ", ".join("`%s`" % v for v in spec["states"])))
            if spec["properties"]:
                rows.append(("API", ", ".join("`%s`" % p[0] for p in spec["properties"])))
            rows.append(("Currently used by",
                         ", ".join(spec["usedBy"]) if spec["usedBy"]
                         else "*nothing yet — built, not adopted*"))
            rows.append(("Designed for",
                         ", ".join(spec["designedFor"]) if spec["designedFor"] else "—"))
            rows.append(("Playground", "`run.bat %s`" % spec["id"]))
            rows.append(("Source", "`%s`" % spec["source"]))
            w("| | |")
            w("|---|---|")
            for k, v in rows:
                w("| %s | %s |" % (k, v))
            w("")
            if spec["properties"]:
                w("| property | type | notes |")
                w("|---|---|---|")
                for name, kind, note in spec["properties"]:
                    w("| `%s` | `%s` | %s |" % (name, kind, note))
                w("")
    return "\n".join(lines) + "\n"


def main():
    specs = collect()
    text = render(specs)
    check = "--check" in sys.argv
    if check:
        current = io.open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
        if current.replace("\r\n", "\n") != text:
            print("docs/COMPONENTS.md is stale - run tools/generate_catalogue.py")
            return 1
        print("catalogue is current (%d components)" % len(specs))
        return 0
    io.open(OUT, "w", encoding="utf-8", newline="\n").write(text)
    print("wrote %s (%d components)" % (OUT, len(specs)))
    return 0


sys.exit(main())
