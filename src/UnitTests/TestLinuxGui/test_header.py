#!/usr/bin/env python3
"""Exercise DE binary header copying with fortified libc and assertions enabled."""
import argparse
from pathlib import Path
import shlex
import subprocess

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', type=Path)
    build = parser.parse_args().build_dir.resolve()
    work = build/'linux-gui-tests/header'
    work.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parent/'DeFileHeaderCopyTest.cpp'
    commands = subprocess.check_output(['ninja', '-t', 'commands', 'GmatGUI'], cwd=build, text=True).splitlines()
    flags = shlex.split(next(c for c in commands if ' -c ' in c and c.endswith('/app/GmatApp.cpp')))
    flags = flags[:flags.index('-MD')]
    binary = work/'header-test'
    subprocess.run(flags+['-O2', '-U_FORTIFY_SOURCE', '-D_FORTIFY_SOURCE=3', '-UNDEBUG',
                          str(source), '-o', str(binary)], cwd=build, check=True)
    with (work/'header.log').open('w') as log:
        subprocess.run([str(binary)], cwd=work, stdout=log, stderr=subprocess.STDOUT, check=True)
    print('PASS: fixed-width DE header copy, assignment and self-assignment', flush=True)
