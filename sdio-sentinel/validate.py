#!/usr/bin/env python3
"""Local acceptance checks for SDIO Sentinel. Author: jayis1."""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parent
EXCLUDED_PARTS = {"node_modules", "dist"}
EXCLUDED_NAMES = {"sdio-sentinel"}


def source_files():
    for path in ROOT.rglob("*"):
        if not path.is_file() or EXCLUDED_PARTS.intersection(path.parts):
            continue
        if path.name.endswith(".o") or path.name in EXCLUDED_NAMES:
            continue
        yield path


def substantive_c_lines():
    count = 0
    for path in (ROOT / "firmware").rglob("*.c"):
        in_block = False
        for line in path.read_text().splitlines():
            stripped = line.strip()
            if in_block:
                if "*/" in stripped:
                    in_block = False
                continue
            if stripped.startswith("/*"):
                if "*/" not in stripped:
                    in_block = True
                continue
            if not stripped or stripped.startswith("//") or stripped.startswith("*"):
                continue
            count += 1
    return count


def check_sexpression(path, required):
    text = path.read_text()
    depth = 0
    quoted = False
    escape = False
    for character in text:
        if quoted:
            if escape:
                escape = False
            elif character == "\\":
                escape = True
            elif character == '"':
                quoted = False
        elif character == '"':
            quoted = True
        elif character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
        if depth < 0:
            raise ValueError(f"{path.name}: premature closing parenthesis")
    if depth or quoted:
        raise ValueError(f"{path.name}: unbalanced expression")
    missing = [token for token in required if token not in text]
    if missing:
        raise ValueError(f"{path.name}: missing structures {missing}")


def main():
    files = list(source_files())
    missing_author = [str(path.relative_to(ROOT)) for path in files
                      if "jayis1" not in path.read_text(errors="replace")]
    readme_words = len(re.findall(r"\b[\w’'-]+\b", (ROOT / "README.md").read_text()))
    c_lines = substantive_c_lines()
    required = [
        ROOT / "README.md", ROOT / "firmware/main.c", ROOT / "firmware/board.h",
        ROOT / "firmware/registers.h", ROOT / "firmware/Makefile",
        ROOT / "kicad/device.kicad_sch", ROOT / "kicad/device.kicad_pcb",
        ROOT / "kicad/device.kicad_pro", ROOT / "app/package.json",
        ROOT / "app/src/App.jsx",
    ]
    with (ROOT / "kicad/device.kicad_pro").open() as stream:
        json.load(stream)
    check_sexpression(ROOT / "kicad/device.kicad_sch", ["(symbol", "(wire", "(label"])
    check_sexpression(ROOT / "kicad/device.kicad_pcb", ["(footprint", "(net ", "(segment", "Edge.Cuts"])
    required_present = all(path.exists() and path.stat().st_size > 0 for path in required)
    print(f"files_checked={len(files)}")
    print(f"readme_words={readme_words}")
    print(f"substantive_c_lines={c_lines}")
    print(f"missing_author={missing_author}")
    print(f"required_present={required_present}")
    print("kicad_structure=PASS json=PASS")
    if readme_words < 2000 or c_lines < 500 or missing_author or not required_present:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
