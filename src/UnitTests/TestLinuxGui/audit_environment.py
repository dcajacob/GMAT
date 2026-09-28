#!/usr/bin/env python3
"""Record the source/build environment; this file does not claim test success."""
import hashlib
import json
import os
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def record_environment(build):
    build = build.resolve()
    cache = {}
    for line in (build/'CMakeCache.txt').read_text().splitlines():
        if '=' in line and not line.startswith(('#', '//')):
            key, value = line.split('=', 1)
            cache[key.split(':', 1)[0]] = value
    root = Path(cache['CMAKE_HOME_DIRECTORY'])
    def read(command):
        return subprocess.run(command, cwd=root, text=True, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=10).stdout.strip()
    manifest = {
        'recorded_at_utc': datetime.now(timezone.utc).isoformat(),
        'commit': read(['git', 'rev-parse', 'HEAD']),
        'working_tree': read(['git', 'status', '--short']),
        'kernel': platform.platform(),
        'wxwidgets': read(['wx-config', '--version']),
        'gtk_and_mesa': read(['dpkg-query', '-W', '-f=${Package} ${Version}\n',
                              'libgtk-3-0t64', 'libgl1-mesa-dri']),
        'session': {key: os.environ.get(key) for key in
                    ('XDG_SESSION_TYPE', 'GDK_BACKEND', 'LANG', 'LC_ALL')},
        'build': {key: cache.get(key) for key in
                  ('CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'GMAT_RELEASE_NAME',
                   'GMAT_INCLUDE_GUI', 'PLUGIN_OPENFRAMESINTERFACE', 'PLUGIN_OVTOOFI')},
        'test_sources_sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in sorted((root/'src/UnitTests/TestLinuxGui').glob('*'))
                                if p.suffix in ('.cpp', '.hpp', '.py', '.script')},
        'automated_display': 'Xvfb / X11 / software OpenGL; scale and theme specified by each suite',
        'result_policy': 'See suite logs and runner exit status; this manifest is environment evidence only.',
    }
    destination = build/'linux-gui-tests/environment.json'
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(manifest, indent=2)+'\n')
    return destination


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', type=Path)
    print(record_environment(parser.parse_args().build_dir))
