#!/usr/bin/env python3
"""Opt-in Linux GUI sweep of tracked GMAT scripts; preserves per-script evidence."""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import time
import uuid

from support import Context


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n')
    temporary.replace(path)


def classify(log, returncode, timed_out):
    match = re.search(r'AUDIT_RUN_END result=(-?\d+)', log)
    mission_result = int(match[1]) if match else None
    if timed_out:
        status = 'timeout'
    elif 'AUDIT_SOFT_TIMEOUT' in log:
        status = 'time_limit_stopped'
    elif 'AUDIT_BUILD_END success=0' in log:
        status = 'build_failed'
    elif mission_result is not None and mission_result != 1:
        status = 'mission_failed'
    elif mission_result == 1 and returncode == 0 and 'AUDIT_CLOSE_RETURN' in log:
        status = 'completed'
    else:
        status = 'crash_or_startup_failure'
    return status, mission_result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', type=Path)
    parser.add_argument('--output', type=Path, help='Evidence directory (defaults inside build)')
    parser.add_argument('--label', default='native')
    parser.add_argument('--mode', choices=('native', 'converted'), default='native')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--limit', type=int, default=180, help='Mission time limit in seconds')
    parser.add_argument('--scale', type=int, choices=(1, 2, 3), default=1)
    parser.add_argument('--only', default='', help='Regular expression selecting script paths')
    parser.add_argument('--legacy-only', action='store_true', help='Select Create OrbitView scripts')
    parser.add_argument('--python-version', help='Select one configured Python version, e.g. 3.14')
    parser.add_argument('--resume', action='store_true')
    args = parser.parse_args()
    if args.jobs < 1 or args.limit < 1 or not re.fullmatch(r'[A-Za-z0-9_-]+', args.label):
        parser.error('Use positive jobs/limit and an alphanumeric, dash or underscore label')
    for tool in ('bwrap', 'xvfb-run', 'ninja', 'import'):
        if not shutil.which(tool):
            parser.error('Required tool not found: ' + tool)

    # Each invocation owns its executable, including concurrent runs of the same lane.
    ctx = Context(args.build_dir, 'example-sweep-build-' + uuid.uuid4().hex[:12])
    ctx.runtime()
    binary = ctx.build_gui('ExampleSweep.cpp')
    root = ctx.root
    output = (args.output or ctx.build/'linux-gui-tests'/'example-sweep').resolve()
    lane = output/args.label
    lane.mkdir(parents=True, exist_ok=True)
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    files = subprocess.check_output(['git', 'ls-files', '*.script'], cwd=root, text=True).splitlines()
    files = sorted((f for f in files if not f.startswith('src/UnitTests/')),
                   key=lambda f: (not f.startswith('application/samples/'), f))
    inventory = [{'script': f, 'sha256': digest(root/f),
                  'category': 'bundled' if f.startswith('application/samples/') else 'supplemental',
                  'id': hashlib.sha256(f.encode()).hexdigest()[:12]} for f in files]
    manifest = inventory
    if args.legacy_only:
        manifest = [m for m in manifest if re.search(r'^\s*(?:GMAT\s+)?Create\s+OrbitView\b',
                    (root/m['script']).read_text(errors='replace'), re.M)]
    if args.only:
        manifest = [m for m in manifest if re.search(args.only, m['script'])]
    if not manifest:
        parser.error('Selection matches no tracked scripts')

    startup = ctx.startup.read_text()
    optional = ['libExtraPropagators', 'libPolyhedronGravity', 'libSaveCommand',
                'libOpenFramesInterface']
    versions = ctx.cache.get('GMAT_PYTHON3_VERSIONS', '').split(';')
    versions = [v for v in versions if (ctx.application/'plugins'/
                ('libPythonInterface_py'+v.replace('.', '')+'.so')).exists()]
    version = args.python_version
    if version and version not in versions:
        parser.error('Requested Python plugin is not installed in this build')
    if len(versions) > 1 and not version:
        parser.error('Multiple Python plugins available; select --python-version')
    if version or versions:
        suffix = (version or versions[0]).replace('.', '')
        optional += ['libPythonInterface_py'+suffix, 'libExternalForceModel_py'+suffix]
    if args.mode == 'converted':
        optional.append('libOVtoOFI')
        if not (ctx.application/'plugins/libOVtoOFI.so').exists():
            parser.error('Converted lane requires the OVtoOFI plugin')
    for name in optional:
        plugin = ctx.application/'plugins'/name
        if plugin.with_suffix('.so').exists() and str(plugin) not in startup:
            startup += '\nPLUGIN = '+str(plugin)+'\n'
    configuration = {'revision': revision, 'harness_sha256': digest(Path(__file__)),
                     'gui_harness_sha256': digest(ctx.source/'ExampleSweep.cpp'), 'mode': args.mode, 'scale': args.scale,
                     'limit_seconds': args.limit, 'startup_sha256':
                     hashlib.sha256(startup.replace(str(ctx.work), '<WORK>').encode()).hexdigest()}
    write_json(lane/'inventory.json', {'revision': revision, 'scripts': inventory,
                                      'selected': [m['script'] for m in manifest],
                                      'configuration': configuration})

    def execute(item):
        job = lane/item['id']
        resultfile = job/'result.json'
        if resultfile.exists():
            previous = json.loads(resultfile.read_text())
            if args.resume and previous.get('configuration') == configuration and previous['sha256'] == item['sha256']:
                return previous
            raise RuntimeError('Existing evidence differs or resume not requested: '+str(job)+
                               '. Choose a new label; evidence is never overwritten.')
        job.mkdir(parents=True, exist_ok=True)
        begin = time.monotonic()
        result = {**item, 'configuration': configuration, 'job': str(job)}
        try:
            app = job/'repo/application'
            cwd = app/'bin'
            cwd.mkdir(parents=True, exist_ok=True)
            for leaf in ('data', 'userfunctions', 'userincludes', 'matlab'):
                target = app/leaf
                if not target.exists() and (root/'application'/leaf).exists():
                    target.symlink_to(root/'application'/leaf, target_is_directory=True)
            shutil.copytree(root/'application/samples', app/'samples', dirs_exist_ok=True)
            source, copy = root/item['script'], job/'repo'/item['script']
            if not item['script'].startswith('application/samples/') and source.resolve() != copy.resolve():
                shutil.copytree(source.parent, copy.parent, dirs_exist_ok=True,
                                ignore=shutil.ignore_patterns('.git', '__pycache__'))
            # TLE scripts resolve ephemerides relative to their script location.
            # Copy fixture siblings without changing the script or orbital data.
            if item['script'].startswith('plugins/TLEPropagatorPlugin/'):
                for relative in ('TLE', 'test/TLE'):
                    fixtures = Path('plugins/TLEPropagatorPlugin')/relative
                    shutil.copytree(root/fixtures, job/'repo'/fixtures, dirs_exist_ok=True)
            out, images = job/'output', job/'images'
            out.mkdir(exist_ok=True)
            images.mkdir(exist_ok=True)
            preferences = job/'personalization.ini'
            preferences.write_text('[Main]\nShowWelcomeOnStart=false\n')
            local = startup.replace(str(ctx.work)+'/', str(out)+'/')
            local = re.sub(r'^PERSONALIZATION_FILE\s*=.*$', 'PERSONALIZATION_FILE = '+str(preferences),
                           local, flags=re.M)
            start = job/'startup.txt'
            start.write_text(local)
            env = {**ctx.of_environment, 'GDK_SCALE': str(args.scale), 'LP_NUM_THREADS': '2',
                   'OSG_NUM_PROCESSORS': '2', 'MESA_SHADER_CACHE_DIR': str(job/'mesa-cache'),
                   'GMAT_EXAMPLE_SCRIPT': str(copy), 'GMAT_EXAMPLE_LIMIT': str(args.limit),
                   'GMAT_GUI_TEST_IMAGES': str(images), 'GMAT_EXAMPLE_EXTERNAL_CAPTURE': '1'}
            env.pop('G_DEBUG', None)  # Retain known optional-plugin warnings without aborting.
            # Read-only source/host, writable evidence, isolated preferences and X display.
            command = ['bwrap', '--ro-bind', '/', '/', '--bind', str(job), str(job),
                       '--tmpfs', '/tmp', '--dev-bind', '/dev', '/dev', '--proc', '/proc',
                       '--unshare-pid', '--die-with-parent', '--chdir', str(cwd),
                       'xvfb-run', '-a', '-s', f'-screen 0 {1280*args.scale}x{900*args.scale}x24',
                       str(binary), '--startup_file', str(start), '--no_splash']
            timed_out = False
            with (job/'gui.log').open('w') as log:
                process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                           env=env, start_new_session=True)
                write_json(job/'live.json', {'pid': process.pid, 'started': time.time(), 'script': item['script']})
                try:
                    process.wait(timeout=args.limit+50)
                except subprocess.TimeoutExpired:
                    timed_out = True
                    os.killpg(process.pid, signal.SIGTERM)
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
            text = (job/'gui.log').read_text(errors='replace')
            status, mission_result = classify(text, process.returncode, timed_out)
            result.update(status=status, returncode=process.returncode, mission_result=mission_result,
                          screenshots=len(list(images.glob('*.png'))),
                          viewport_mismatches=text.count('AUDIT_VIEWPORT match=0'),
                          inactive_windows=text.count('AUDIT_ACTIVE match=0'),
                          screenshot_errors=len(re.findall(r'AUDIT_SCREENSHOT result=(?!0\b)', text)))
        except Exception as error:
            result.update(status='harness_error', error=str(error))
        result['seconds'] = round(time.monotonic()-begin, 2)
        write_json(resultfile, result)
        (job/'live.json').unlink(missing_ok=True)
        print(json.dumps({k: result[k] for k in ('script', 'status', 'seconds')}), flush=True)
        return result

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(execute, manifest))
    write_json(lane/'summary.json', results)
    print('SWEEP COMPLETE', len(results), '(see per-script status; this is not a blanket GUI pass)')
    return int(any(r['status'] in ('harness_error', 'crash_or_startup_failure', 'timeout') or
                   r.get('viewport_mismatches', 0) or r.get('inactive_windows', 0) or
                   r.get('screenshot_errors', 0) for r in results))


if __name__ == '__main__':
    raise SystemExit(main())
