#!/usr/bin/env python3
"""Build the repository's offline user guide and copy it to HELP_PATH's default."""
import pathlib
import shutil
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[2]
source = root / "doc" / "help"
destination = root / "application" / "docs" / "help"
result = subprocess.run(["make", "-C", str(source), "html"], check=False)
if result.returncode:
    sys.exit(result.returncode)
for page in ("index", "Spacecraft", "Propagate", "UsingGmat", "Tutorials"):
    if not (source / "html" / f"{page}.html").is_file():
        sys.exit(f"Help build did not produce {page}.html")
destination.mkdir(parents=True, exist_ok=True)
for directory in ("html", "files"):
    shutil.copytree(source / directory, destination / directory, dirs_exist_ok=True)
shutil.copy2(source / "help.html", destination / "help.html")
print(f"Offline GMAT help copied to {destination}")
