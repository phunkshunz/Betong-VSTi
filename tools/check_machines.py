"""Kontrollerar maskinerna i CMakeLists.txt: att varje maskin har en egen
identitet i DAW:en och att bygget och installatören tar med alla.

Kör:  python3 tools/check_machines.py

Kontrollen är till för att två maskiner aldrig ska kunna få samma plugin-kod,
samma bundle-id eller samma namn. DAW:en visar tillverkaren (Phunkshunz) och
produktnamnet, och söker man på namnet ska maskinerna gå att skilja åt; samma
kod eller samma bundle-id kan få värden att blanda ihop dem.
"""
from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIELDS = ("mål", "produktnamn", "plugin-kod", "bundle-id", "om-rad", "tagline", "öppningsmönster")
MANUFACTURER = "Phunkshunz"


def read(path: str) -> str:
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        return f.read()


def parse_machines(cmake: str) -> list[dict]:
    m = re.search(r"set\(BETONG_MACHINES\s*(.*?)\)\s*\n", cmake, re.S)
    if not m:
        raise SystemExit("hittade inte BETONG_MACHINES i CMakeLists.txt")
    machines = []
    for line in m.group(1).splitlines():
        line = line.strip()
        if not line.startswith('"'):
            continue
        row = line.strip().strip('"').split("|")
        machines.append(dict(zip(FIELDS, row)))
    return machines


def pattern_names(presets: str) -> set[str]:
    body = presets[presets.index("kPatterns[]"):]
    return set(re.findall(r'\{\s*"([^"]+)"\s*,\s*[0-9.]+f\s*,', body))


def main() -> int:
    cmake = read("CMakeLists.txt")
    presets = read("src/Presets.h")
    identity = read("src/Identity.h")
    workflow = read(".github/workflows/build-macos.yml")
    installers = {p: read(p) for p in ("scripts/installera.command", "scripts/installera.bat")}

    fails: list[str] = []
    warns: list[str] = []

    def check(name: str, cond: bool, hint: str = "") -> None:
        print(f"  [{'ok' if cond else 'FEL'}] {name}")
        if not cond:
            fails.append(hint or name)

    machines = parse_machines(cmake)
    print(f"maskiner i CMakeLists.txt: {len(machines)}")
    check("minst en maskin är angiven", len(machines) > 0)
    check("varje rad har sju fält", all(len(m) == len(FIELDS) and all(m.values()) for m in machines),
          "en maskinrad i BETONG_MACHINES saknar ett fält")

    for key in ("mål", "produktnamn", "plugin-kod", "bundle-id"):
        values = [m[key] for m in machines]
        check(f"{key} är unikt per maskin: {', '.join(values)}", len(set(values)) == len(values),
              f"två maskiner har samma {key}")

    check(f"tillverkaren är {MANUFACTURER}", f'COMPANY_NAME "{MANUFACTURER}"' in cmake,
          f"COMPANY_NAME i CMakeLists.txt är inte {MANUFACTURER}")

    for m in machines:
        name = m["produktnamn"]
        check(f"{name}: plugin-koden är fyra tecken ({m['plugin-kod']})",
              bool(re.fullmatch(r"[A-Za-z0-9]{4}", m["plugin-kod"])), "plugin-koden måste vara fyra tecken")
        check(f"{name}: plugin-koden krockar inte med tillverkarkoden Phnk", m["plugin-kod"] != "Phnk")
        check(f"{name}: bundle-id ligger under com.phunkshunz.",
              m["bundle-id"].startswith("com.phunkshunz."), "bundle-id ska börja med com.phunkshunz.")
        check(f"{name}: namnet är ASCII", name.isascii(), "produktnamnet måste vara ASCII för sökbarheten")
        check(f"{name}: om-rad och tagline är ASCII",
              m["om-rad"].isascii() and m["tagline"].isascii())
        if len(name) > 8:
            warns.append(f"produktnamnet {name} är {len(name)} tecken och kan klippas i loggrutan")

    names = pattern_names(presets)
    for m in machines:
        check(f"{m['produktnamn']}: öppningsmönstret {m['öppningsmönster']} finns i mönstertabellen",
              m["öppningsmönster"] in names, "öppningsmönstret måste finnas i src/Presets.h")

    for macro in ("BETONG_PRODUCT_NAME", "BETONG_ABOUT_LINE", "BETONG_TAGLINE", "BETONG_DEFAULT_PATTERN"):
        check(f"Identity.h har standardvärde för {macro}", f"#ifndef {macro}" in identity)

    check("koden läser namnet ur Identity.h i stället för att hårdkoda det",
          'return BETONG_PRODUCT_NAME;' in read("src/PluginProcessor.h")
          and 'opts.folderName = BETONG_PRODUCT_NAME;' in read("src/PluginProcessor.cpp"),
          "getName() eller inställningsmappen hårdkodar fortfarande ett namn")

    targets = " ".join(m["mål"] for m in machines)
    check(f"bygget tar med alla maskiner ({targets})",
          all(t in workflow for t in [m["mål"] for m in machines]),
          "en maskin saknas i .github/workflows/build-macos.yml")

    for path, text in installers.items():
        check(f"{path} installerar alla maskiner, inte en hårdkodad",
              re.search(r"\*\.vst3", text) is not None, "installatören pekar på ett enstaka plugin")

    for w in warns:
        print(f"  [varning] {w}")

    if fails:
        print(f"\n{len(fails)} fel:")
        for f in fails:
            print(f"  - {f}")
        return 1
    print("\nallt ok: maskinerna har egna identiteter och hänger med genom bygget och installationen")
    return 0


if __name__ == "__main__":
    sys.exit(main())
