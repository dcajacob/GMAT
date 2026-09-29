"""Run with: python3 LaunchTests.py /path/to/GmatQt /path/to/propagate.script."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    executable, fixture = (Path(value).absolute() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="gmat qt launch ") as directory:
        root = Path(directory)
        script = root / "mission with spaces.script"
        shutil.copyfile(fixture, script)
        environment = dict(os.environ, QT_QPA_PLATFORM="offscreen",
                           XDG_CONFIG_HOME=str(root / "settings"))

        def run(arguments, expected):
            result = subprocess.run([str(executable), "--settings-dir", str(root / "settings"), *map(str, arguments)],
                                    cwd=root, env=environment, capture_output=True,
                                    text=True, timeout=30)
            if result.returncode != expected:
                raise AssertionError(f"Exit {result.returncode}, expected {expected}: "
                                     f"{result.stdout}\n{result.stderr}")

        screenshot = root / "mission result.png"
        run(["--run", "--screenshot", screenshot, script], 0)
        assert screenshot.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"), "Screenshot missing"
        run(["--startup", root / "missing.txt", "--screenshot", screenshot], 2)
        run(["--screenshot", screenshot, root / "missing.script"], 2)
        invalid = root / "invalid.script"
        invalid.write_text("Not a valid GMAT command;\n")
        run(["--run", "--screenshot", screenshot, invalid], 1)
        assert not (root / "GmatLog.txt").exists(), "Launcher wrote a log in caller's directory"
    print("PASS: default startup, unrelated working directory, spaced paths, run/capture, and failure exit codes")


if __name__ == "__main__":
    main()
