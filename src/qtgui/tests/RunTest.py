"""CTest launcher: runtime directories followed by the test command."""
import os
import subprocess
import sys

environment = dict(os.environ, QT_QPA_PLATFORM="offscreen")
if os.name == "nt":
    # Avoid CMake list escaping of Windows PATH and keep the machine unchanged.
    environment["PATH"] = os.pathsep.join([*sys.argv[1:3], environment.get("PATH", "")])
sys.exit(subprocess.run(sys.argv[3:], env=environment).returncode)
