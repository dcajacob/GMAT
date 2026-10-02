#!/usr/bin/env python3
"""Resumable, isolated qualification of shipped GMAT mission scripts.

Inventory is read-only. Build/run start offscreen GmatQt children with unchanged
source files, isolated startup/output/preferences, retained logs and hard timeout.
Successful completed stages are never repeated by default. Explicitly select
failed entries with --only and --retry-failed after fixing a failure.
"""
import argparse
import collections
import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import signal
import subprocess
import time

DEFAULT_REPO = Path(__file__).resolve().parents[3]
DEPENDENCIES = {
    'MATLAB': {'status': 'outside_selected_runtime', 'reason': 'MATLAB interface disabled; proprietary MATLAB runtime required', 'provenance': ['application/docs/help/html/SampleMissions.html', 'application/docs/help/html/ConfiguringGmat.html']},
    'SNOPT': {'status': 'missing_proprietary_dependency', 'reason': 'SNOPT is not distributed with GMAT and must be obtained from its vendor', 'provenance': ['application/docs/help/html/SNOPTOptimizer.html', 'application/docs/help/html/SampleMissions.html']},
    'VF13ad': {'status': 'missing_optional_dependency', 'reason': 'VF13ad plugin absent from selected startup; documented separate installation required; proprietary status not assumed', 'provenance': ['application/docs/help/html/VF13ad.html', 'application/docs/help/html/SampleMissions.html']},
    'OptimalControl': {'status': 'missing_optional_dependency', 'reason': 'Trajectory/Phase/OptimalControl plugin absent; GMAT_INCLUDE_CSALT=OFF, CSALT configuration requires SNOPT libraries', 'provenance': ['CMakeLists.txt:127', 'CMakeLists.txt:411-430', 'build/linux-gui/CMakeCache.txt:296']},
    'MarsGRAM2005': {'status': 'missing_optional_dependency', 'reason': 'libMarsGRAM is absent from selected startup; separate Mars-GRAM plugin/data distribution required', 'provenance': ['application/docs/help/html/ForceModel.html#ForceModel_Remarks_ConfiguringDragModels_MarsGRAM2005']},
    'GMATPythonAPI': {'status': 'required_public_build_configuration', 'reason': 'Requires public GMAT_INCLUDE_API=ON and API_GENERATE_PYTHON=ON with the Python version used by ExternalForceModel. Missing or incompatible API artifacts are a build/configuration issue, not a proprietary dependency exception', 'provenance': ['CMakeLists.txt:128-130', 'application/api/API_README.txt', 'application/samples/Ex_ExternalForceModel.script:10-13', 'application/userfunctions/python/SimpleExternalForceModel.py']},
}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def file_provenance(path):
    # Hash/stat the same open inode even if an incremental build replaces it.
    with path.open('rb') as stream:
        details = os.fstat(stream.fileno())
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return {'path': str(path), 'resolved_path': str(path.resolve()), 'sha256': digest, 'size': details.st_size, 'mtime_ns': details.st_mtime_ns, 'inode': details.st_ino, 'device': details.st_dev}

def runtime_inputs(repo):
    """Snapshot public Python helper and API modules without importing them."""
    helper = repo / 'application/userfunctions/python/SimpleExternalForceModel.py'
    package = repo / 'application/bin/gmatpy'
    paths = [helper] if helper.is_file() else []
    if package.is_dir():
        paths.extend(path for path in package.rglob('*')
                     if path.is_file() and '__pycache__' not in path.parts
                     and (path.suffix == '.py' or '.so' in path.name))
    return {'scope': 'SimpleExternalForceModel.py and installed gmatpy Python initializers/wrappers and versioned shared objects; presence/hashes do not establish API compatibility',
            'gmatpy_package_present': package.is_dir(),
            'files': [file_provenance(path) for path in sorted(paths)]}

def linked_core_libraries(repo):
    """Hash the installed core files named by this Qt executable's ELF linkage."""
    binary = repo / 'application/bin/GmatQt-R2026a'
    dynamic = subprocess.check_output(['readelf', '-d', str(binary)], text=True)
    names = re.findall(r'\(NEEDED\).*Shared library: \[(libGmat(?:Base|Util)\.so[^\]]*)\]', dynamic)
    return {'scope': 'Installed bin-directory files named by GmatQt ELF DT_NEEDED; named inode snapshots, not a claim about loader overrides or the build source state',
            'elf_needed_names': names,
            'files': [file_provenance(binary.parent / name) for name in names]}

def configured_plugins(repo):
    """Snapshot Linux plugin files named by startup, without importing them."""
    startup = repo / 'application/bin/gmat_startup_qt.txt'
    files, missing = [], []
    for name in re.findall(r'^\s*PLUGIN\s*=\s*(.*?)\s*$', startup.read_text(), re.M):
        candidate = Path(name)
        if not candidate.is_absolute():
            candidate = startup.parent / candidate
        if not candidate.is_file():
            candidate = Path(str(candidate) + '.so')
        if candidate.is_file():
            files.append(file_provenance(candidate))
        else:
            missing.append(name)
    return {'scope': 'Linux plugin files explicitly named by selected startup; named snapshots do not prove successful loading or ABI compatibility',
            'files': files, 'missing': missing}

def script_statements(source):
    """Read line statements, ignoring comments/terminators inside string literals."""
    for number, line in enumerate(source.splitlines(), 1):
        start, position, quote = 0, 0, None
        while position < len(line):
            character = line[position]
            if quote:
                if character == quote:
                    if position + 1 < len(line) and line[position + 1] == quote:
                        position += 2
                        continue
                    quote = None
            elif character in "'\"":
                quote = character
            elif character in ';%':
                statement = line[start:position].strip()
                if statement:
                    yield number, statement
                if character == '%':
                    start = len(line)
                    break
                start = position + 1
            position += 1
        statement = line[start:].strip()
        if statement:
            yield number, statement

def script_path_literal(value):
    """Accept a complete string or bare path, never an expression/variable."""
    if len(value) >= 2 and value[0] in "'\"" and value[-1] == value[0]:
        quote = value[0]
        interior = value[1:-1]
        if re.fullmatch(r'(?:[^' + re.escape(quote) + r']|' + re.escape(quote * 2) + r')*', interior):
            return interior.replace(quote * 2, quote)
        return None
    if re.fullmatch(r"[^\s'\";{}()+*=,$]+", value) and any(character in value for character in './\\'):
        return value
    return None

def prepare_output_directories(repo, script, output):
    """Prepare only contained output paths declared by known writable resources."""
    sources, statements, visited, skipped = [], [], set(), []

    def visit(path):
        path = path.resolve()
        if path in visited:
            return
        visited.add(path)
        sources.append(file_provenance(path))
        for number, statement in script_statements(path.read_text(encoding='utf-8-sig')):
            include = re.fullmatch(r'#Include\s+(.+)', statement, re.I)
            if include:
                literal = script_path_literal(include.group(1).strip())
                target = path.parent / literal.replace('\\', '/') if literal is not None else None
                if target is not None and target.is_file():
                    visit(target)
                else:
                    skipped.append({'source': str(path), 'line': number, 'reason': 'Include is not an existing explicit literal path relative to its containing source'})
            else:
                statements.append((path, number, statement))

    visit(script)
    resources = {}
    for path, number, statement in statements:
        declaration = re.fullmatch(r'(?:GMAT\s+)?Create\s+(ReportFile|EphemerisFile)\s+(.+)', statement)
        if declaration:
            for name in re.findall(r'[A-Za-z]\w*', declaration.group(2)):
                resources[name] = declaration.group(1)
    root = output.resolve()
    prepared, outputs = set(), []
    for path, number, statement in statements:
        assignment = re.fullmatch(r'(?:GMAT\s+)?([A-Za-z]\w*)\.Filename\s*=\s*(.+)', statement)
        if not assignment or assignment.group(1) not in resources:
            continue
        name, value = assignment.groups()
        record = {'source': str(path), 'line': number, 'resource': name,
                  'resource_type': resources[name], 'assignment': value}
        literal = script_path_literal(value)
        if literal is None:
            skipped.append({**record, 'reason': 'Filename is not a complete literal path'})
            continue
        normalized = literal.replace('\\', '/')
        filename = PurePosixPath(normalized)
        if (not normalized or '\x00' in normalized or normalized.endswith('/')
                or filename.is_absolute() or re.match(r'^[A-Za-z]:', normalized)
                or '..' in filename.parts or filename.name in ['', '.']):
            skipped.append({**record, 'reason': 'Filename is not a contained relative file path'})
            continue
        directory = (root / filename.parent).resolve()
        if not directory.is_relative_to(root):
            skipped.append({**record, 'reason': 'Output directory resolves outside isolated OUTPUT_PATH'})
            continue
        missing, current = [], directory
        while current != root and not current.exists():
            missing.append(current)
            current = current.parent
        try:
            for current in reversed(missing):
                current.mkdir()
                prepared.add(str(current))
            if not directory.is_dir():
                raise NotADirectoryError(str(directory))
        except OSError as error:
            skipped.append({**record, 'reason': 'Output directory could not be prepared: ' + str(error)})
            continue
        outputs.append({**record, 'literal_filename': literal, 'directory': str(directory)})
    return {'scope': 'Literal Filename assignments for declared ReportFile/EphemerisFile resources, including recursively read explicit includes; original filenames and sources unchanged',
            'output_path': str(root), 'prepared_directories': sorted(prepared),
            'declared_outputs': outputs, 'skipped': skipped, 'sources': sources,
            'limits': 'No input/asset directories, expression evaluation, parent/absolute/drive paths or EventLocator nested paths are prepared. GMAT may prefer an already existing source-relative directory before its OUTPUT_PATH fallback.'}

def stage_provenance(repo, entry):
    difference = subprocess.check_output(['git', 'diff', 'HEAD', '--no-ext-diff', '--binary'], cwd=repo)
    return {'repo_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip(), 'git_diff_sha256': hashlib.sha256(difference).hexdigest(), 'git_diff_byte_count': len(difference), 'source_context_limit': 'HEAD/diff describe current source context, not proof that the running binary was built from this worktree state', 'launcher': file_provenance(repo / 'application/bin/GmatQt'), 'actual_qt_binary': file_provenance(repo / 'application/bin/GmatQt-R2026a'), 'startup_sha256': sha(repo / 'application/bin/gmat_startup_qt.txt'), 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'platform': 'offscreen', 'runner_sha256': sha(Path(__file__)),
            'referenced_sources': [file_provenance(repo / path) for path in entry['input_paths']],
            'python_runtime_inputs': runtime_inputs(repo),
            'configured_plugin_files': configured_plugins(repo),
            'linked_core_libraries': linked_core_libraries(repo)}

def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.new')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')
    temporary.replace(path)

def source_without_comments(source):
    return '\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('%'))

def inventory(repo):
    primary = repo / 'application/samples'
    tutorials = repo / 'application/docs/help/files/scripts'
    userincludes = repo / 'application/userincludes'
    extras = [repo / 'application/api', repo / 'plugins/TLEPropagatorPlugin/samples']
    paths = sorted(primary.rglob('*.script'))
    paths += sorted(path for root in extras for path in root.rglob('*.script'))
    paths += sorted(tutorials.rglob('*.script')) + sorted(userincludes.rglob('*.script'))
    helpers = sorted((repo / 'application/userfunctions/gmat').rglob('*.gmf'))
    helpers += sorted((primary / 'Navigation').rglob('*.gmf'))
    helper_names = collections.defaultdict(list)
    for path in helpers:
        helper_names[path.stem].append(path)
    entries, support = [], []
    parents = collections.defaultdict(list)

    def relative(path):
        return path.relative_to(repo).as_posix() if path.is_relative_to(repo) else str(path)

    def references(path, text):
        referenced, functions = [], []
        # Preserve the original include record representation for resumable
        # manifests. Additional function/input metadata stays in separate keys.
        for match in re.finditer(r'^\s*#Include\s+(?:[\'\"]([^\'\"]+)[\'\"]|([^\s;%]+))', text, re.M | re.I):
            include = (path.parent / (match.group(1) or match.group(2))).resolve()
            reference = relative(include)
            referenced.append({'path': reference, 'exists': include.is_file(), 'sha256': sha(include) if include.is_file() else None})
            parents[reference].append(relative(path))
        function_paths = dict(re.findall(r'^\s*(?:GMAT\s+)?([A-Za-z]\w*)\.FunctionPath\s*=\s*[\'\"]([^\'\"]*)[\'\"]', text, re.M))
        for declaration in re.finditer(r'^\s*(?:GMAT\s+)?Create\s+GmatFunction\s+([^\n;%]+)', text, re.M):
            for name in re.findall(r'[A-Za-z]\w*', declaration.group(1)):
                requested = function_paths.get(name)
                candidates = []
                if requested:
                    requested_path = Path(requested)
                    candidates = [requested_path] if requested_path.is_absolute() else [path.parent / requested_path]
                    # Filename lookup can fall back to configured GMAT function
                    # roots (notably a Navigation function used by SupportFiles).
                    candidates += [helper for helper in helpers if helper.name == requested_path.name]
                else:
                    candidates = helper_names[name]
                resolved = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
                reference = relative(resolved) if resolved else None
                functions.append({'name': name, 'requested_path': requested, 'path': reference, 'exists': resolved is not None, 'sha256': sha(resolved) if resolved else None})
                if reference:
                    parents[reference].append(relative(path))
        return referenced, functions

    for path in paths:
        source = path.read_text(encoding='utf-8-sig')
        text = source_without_comments(source)
        dependencies = []
        for token, pattern in [('MATLAB', r'\b(?:MatlabFunction|FminconOptimizer)\b'), ('SNOPT', r'\bSNOPT\b'), ('VF13ad', r'\bVF13ad\b'), ('OptimalControl', r'\b(?:OptimalControlGuess|OptimalControlFunction|Trajectory|Phase|CustomLinkageConstraint)\b'), ('MarsGRAM2005', r'\bMarsGRAM2005\b')]:
            if re.search(pattern, text):
                dependencies.append(token)
        if path.name == 'Ex_ExternalForceModel.script':
            dependencies.append('GMATPythonAPI')
        referenced, functions = references(path, text)
        role = 'include_fragment' if path.parent.name == 'SupportFiles' or path.is_relative_to(userincludes) else 'mission'
        dependency_records = [{'name': token, **DEPENDENCIES[token]} for token in dependencies]
        if 'VF13ad' in dependencies:
            configured = configured_plugins(repo)
            if any('VF13adOptimizer' in item['path'] for item in configured['files']):
                for dependency in dependency_records:
                    if dependency['name'] == 'VF13ad':
                        dependency.update(status='configured_external_binary', reason='Separately distributed VF13ad binary is present in selected Qt startup; individual build/run stages still determine compatibility', provenance=dependency['provenance'] + ['https://www.thinksysinc.com/downloads.html'])
        scope = 'distribution_samples' if path.is_relative_to(primary) else 'documentation_tutorials' if path.is_relative_to(tutorials) else 'user_include_fragments' if path.is_relative_to(userincludes) else 'supplemental_api_or_plugin_examples'
        entries.append({'path': relative(path), 'scope': scope, 'role': role, 'sha256': sha(path), 'byte_count': path.stat().st_size, 'requires_view_conversion': bool(re.search(r'Create\s+(?:OpenFrames|OF[A-Z])', text)), 'dependencies': dependency_records, 'includes': referenced, 'functions': functions, 'coverage_parents': [], 'build': None, 'run': None})
    for path in helpers:
        referenced, functions = references(path, source_without_comments(path.read_text(encoding='utf-8-sig')))
        support.append({'path': relative(path), 'role': 'function_helper', 'sha256': sha(path), 'byte_count': path.stat().st_size, 'includes': referenced, 'functions': functions, 'coverage_parents': []})
    lookup = {entry['path']: entry for entry in entries + support}
    for entry in entries + support:
        entry['coverage_parents'] = sorted(set(parents[entry['path']]))

    def mission_parents(paths, visited=None):
        visited = set() if visited is None else visited
        result = []
        for parent in paths:
            if parent in visited or parent not in lookup:
                continue
            visited.add(parent)
            item = lookup[parent]
            result.extend([parent] if item['role'] == 'mission' else mission_parents(item['coverage_parents'], visited))
        return sorted(set(result))

    def input_paths(entry, visited=None):
        visited = set() if visited is None else visited
        if entry['path'] in visited:
            return visited
        visited.add(entry['path'])
        for reference in entry['includes'] + entry['functions']:
            name = reference.get('path')
            if not name or not reference['exists']:
                continue
            if name in lookup:
                input_paths(lookup[name], visited)
            else:
                visited.add(name)
        return visited

    for entry in entries + support:
        entry['coverage_mission_parents'] = mission_parents(entry['coverage_parents'])
        entry['input_paths'] = sorted(input_paths(entry))
    excluded = [{'path': relative(path), 'reason': 'CInterface MATLAB API configuration, not a GUI mission' if path.as_posix().endswith('/CInterfacePlugin/matlab/GmatConfig.script') else 'Plugin developer test script, not a distributed sample'}
                for path in sorted((repo / 'plugins').rglob('*.script'))
                if not path.is_relative_to(repo / 'plugins/TLEPropagatorPlugin/samples')]
    counts = {'script_files': len(entries),
              'primary_script_files': sum(entry['scope'] == 'distribution_samples' for entry in entries),
              'primary_missions': sum(entry['scope'] == 'distribution_samples' and entry['role'] == 'mission' for entry in entries),
              'supplemental_missions': sum(entry['scope'] == 'supplemental_api_or_plugin_examples' and entry['role'] == 'mission' for entry in entries),
              'tutorial_missions': sum(entry['scope'] == 'documentation_tutorials' for entry in entries),
              'include_fragments': sum(entry['role'] == 'include_fragment' for entry in entries),
              'user_include_fragments': sum(entry['scope'] == 'user_include_fragments' for entry in entries),
              'function_helpers': len(support),
              'primary_folders': dict(collections.Counter(str(Path(entry['path']).parent.relative_to('application/samples')) for entry in entries if entry['scope'] == 'distribution_samples'))}
    return {'version': 2, 'repo': str(repo), 'inventory_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'scope': 'All application/samples .script files, all application/docs/help/files/scripts .script tutorials, both application/userincludes .script fragments, application/api and TLE plugin sample .script files, and all application/userfunctions/gmat plus Navigation .gmf helpers. Plugin developer tests and CInterface MATLAB API GmatConfig.script are excluded. Python/MATLAB API clients, notebooks and script generators are outside this Qt .script build/run scope.',
            'coverage_scope': 'Static explicit #Include/Create GmatFunction/FunctionPath references, recursively traced to inventoried parent missions; declarations and parent stage results do not prove every helper branch executed. Functions without a parent remain uncovered rather than being run as standalone missions.',
            'counts': counts, 'entries': entries, 'support': support, 'excluded_scripts': excluded,
            'uncovered_function_helpers': [entry['path'] for entry in support if not entry['coverage_mission_parents']],
            'uncovered_include_fragments': [entry['path'] for entry in entries if entry['role'] == 'include_fragment' and not entry['coverage_mission_parents']]}

def merge_previous(fresh, previous):
    previous_by_path = {entry['path']: entry for entry in previous.get('entries', [])}
    for entry in fresh['entries']:
        old = previous_by_path.get(entry['path'])
        if old and old['sha256'] == entry['sha256'] and old.get('includes') == entry['includes']:
            for phase in ['build', 'run']:
                entry[phase] = old.get(phase)
            entry['history'] = old.get('history', [])
            if old.get('qualification_assessment'):
                entry['qualification_assessment'] = old['qualification_assessment']
        elif old:
            entry['history'] = old.get('history', []) + [{phase: old.get(phase) for phase in ['build', 'run']}]
    return fresh

def summary(manifest):
    output = {'counts': manifest['counts'],
              'uncovered_function_helpers': manifest.get('uncovered_function_helpers', []),
              'uncovered_include_fragments': manifest.get('uncovered_include_fragments', [])}
    for phase in ['build', 'run']:
        output[phase] = dict(collections.Counter((entry.get(phase) or {}).get('status', 'not_attempted') for entry in manifest['entries'] if entry['role'] == 'mission'))
    output['fragment_coverage'] = []
    lookup = {entry['path']: entry for entry in manifest['entries']}
    for entry in manifest['entries'] + manifest['support']:
        if entry['role'] == 'mission':
            continue
        output['fragment_coverage'].append({'path': entry['path'], 'parents': [{'path': parent, 'build': (lookup[parent].get('build') or {}).get('status', 'not_attempted'), 'run': (lookup[parent].get('run') or {}).get('status', 'not_attempted')} for parent in entry['coverage_mission_parents']]})
    return output

def child_stage(repo, evidence, entry, phase, timeout):
    identifier = entry['path'].replace('/', '__')
    stage_root = evidence / 'scripts' / identifier / phase
    attempt = 1
    while (stage_root / f'attempt-{attempt:02}').exists():
        attempt += 1
    root = stage_root / f'attempt-{attempt:02}'
    output = root / 'output'
    output.mkdir(parents=True)
    setup = prepare_output_directories(repo, repo / entry['path'], output)
    startup = (repo / 'application/bin/gmat_startup_qt.txt').read_text()
    replacements = {'ROOT_PATH': str(repo / 'application') + '/', 'OUTPUT_PATH': str(output) + '/', 'LOG_FILE': str(output / 'GmatLog.txt'), 'PERSONALIZATION_FILE': str(root / 'MyGmat.ini')}
    for name, value in replacements.items():
        startup = re.sub(r'^' + re.escape(name) + r'\s*=.*$', name + ' = ' + value, startup, flags=re.M)
    startup_path = root / 'startup.txt'
    startup_path.write_text(startup)
    (root / 'MyGmat.ini').write_text('[Main]\nShowWelcomeOnStart=false\n')
    command = [str(repo / 'application/bin/GmatQt'), '--startup', str(startup_path), '--settings-dir', str(root / 'settings'), '--convert-views', '--screenshot', str(root / 'result.png')]
    if phase == 'run':
        command.append('--run')
    command.append(str(repo / entry['path']))
    environment = dict(os.environ, QT_QPA_PLATFORM='offscreen', LIBGL_ALWAYS_SOFTWARE='1', PYTHONDONTWRITEBYTECODE='1')
    provenance = stage_provenance(repo, entry)
    provenance['setup'] = setup
    start = time.monotonic()
    timed_out = False
    with (root / 'stdout.txt').open('wb') as stdout, (root / 'stderr.txt').open('wb') as stderr:
        process = subprocess.Popen(command, cwd=repo / 'application/bin', env=environment, stdout=stdout, stderr=stderr, start_new_session=True)
        try:
            process_executable = Path(f'/proc/{process.pid}/exe')
            if 'GmatQt' in str(process_executable.resolve()):
                provenance['running_process_binary'] = file_provenance(process_executable)
        except (FileNotFoundError, PermissionError):
            provenance['running_process_binary_note'] = 'Process inode snapshot unavailable; named actual_qt_binary snapshot above retained'
        try:
            code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                code = process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                code = process.wait(timeout=3)
    status = 'timeout' if timed_out else 'passed' if code == 0 else 'crashed' if code < 0 else 'failed'
    # The final runtime exception may only reach GmatLog, after queued message
    # window text stops. Retain every raw log path/hash and per-log diagnostics.
    diagnostic_logs = [path for path in [root / 'stdout.txt', root / 'stderr.txt', output / 'GmatLog.txt'] if path.is_file()]
    diagnostic_pattern = re.compile(r'error|exception|not found|not exist|cannot|failed|unknown|unavailable|unsupported|invalid|rejected|not supported|not (?:yet )?implemented|SPICE', re.I)
    diagnostics_by_log = [{'path': str(path), 'diagnostics': [line for line in path.read_text(errors='replace').splitlines() if diagnostic_pattern.search(line)]}
                          for path in diagnostic_logs]
    diagnostics = [line for log in diagnostics_by_log for line in log['diagnostics']]
    # Declared dependencies are hypotheses until matching actual diagnostics.
    confirmed = []
    if status in ['failed', 'crashed']:
        for dependency in entry['dependencies']:
            token = dependency['name']
            pattern = {'MATLAB': r'MatlabFunction|Matlab|Fmincon', 'OptimalControl': r'OptimalControl|Trajectory|\bPhase\b', 'GMATPythonAPI': r'gmatpy|No module named|Python.*(?:import|module)', 'MarsGRAM2005': r'MarsGRAM', 'VF13ad': r'VF13ad', 'SNOPT': r'SNOPT'}[token]
            if re.search(pattern, '\n'.join(diagnostics), re.I):
                confirmed.append(dependency)
    result = {'status': status, 'phase': phase, 'exit_code': code, 'timed_out': timed_out, 'duration_seconds': round(time.monotonic() - start, 3), 'timeout_seconds': timeout, 'command': command, 'evidence': str(root), 'source_sha256': entry['sha256'], 'source_unchanged': sha(repo / entry['path']) == entry['sha256'],
              'referenced_sources_unchanged': all(sha(Path(source['path'])) == source['sha256'] for source in provenance['referenced_sources']),
              'setup_sources_unchanged': all(sha(Path(source['path'])) == source['sha256'] for source in setup['sources']),
              'confirmed_dependency_diagnostics': confirmed, 'diagnostics': diagnostics[-60:],
              'diagnostic_log_files': [file_provenance(path) for path in diagnostic_logs],
              'diagnostics_by_log': diagnostics_by_log, 'provenance': provenance, 'limits': 'Offscreen build/execution only; native viewer/lifecycle and numerical scientific qualification not established'}
    save(root / 'result.json', result)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=DEFAULT_REPO)
    parser.add_argument('--evidence', type=Path, default=Path('/tmp/gmat-shipped-examples'))
    parser.add_argument('--phase', choices=['inventory', 'build', 'run', 'all'], default='inventory')
    parser.add_argument('--build-timeout', type=float, default=35)
    parser.add_argument('--run-timeout', type=float, default=180)
    parser.add_argument('--only', action='append', default=[], help='Regex matching repository-relative paths; repeatable')
    parser.add_argument('--supplemental', action='store_true', help='Also execute API/TLE plugin sample scripts; always inventoried. Documentation tutorials are selected by default')
    parser.add_argument('--retry-failed', action='store_true', help='Explicitly retry previously failed/crashed/timed-out stages; passed stages still skipped')
    args = parser.parse_args()
    repo = args.repo.resolve()
    if not (repo / 'application/samples').is_dir():
        parser.error('Repository must contain application/samples')
    args.evidence = args.evidence.absolute()
    path = args.evidence / 'manifest.json'
    current = inventory(repo)
    if path.exists():
        current = merge_previous(current, json.loads(path.read_text()))
    save(path, current)
    if args.phase == 'inventory':
        print(json.dumps(summary(current), indent=2), flush=True)
        return
    selected = [entry for entry in current['entries'] if entry['role'] == 'mission' and (args.supplemental or entry['scope'] in ['distribution_samples', 'documentation_tutorials']) and (not args.only or any(re.search(pattern, entry['path']) for pattern in args.only))]
    phases = ['build', 'run'] if args.phase == 'all' else [args.phase]
    for phase in phases:
        for number, entry in enumerate(selected, 1):
            previous = entry.get(phase)
            if previous and (previous['status'] == 'passed' or not args.retry_failed):
                continue
            if phase == 'run' and (entry.get('build') or {}).get('status') != 'passed':
                continue
            print(f'{phase} {number}/{len(selected)} {entry["path"]}', flush=True)
            if previous:
                entry.setdefault('history', []).append(previous)
            result = child_stage(repo, args.evidence, entry, phase, args.build_timeout if phase == 'build' else args.run_timeout)
            entry[phase] = result
            save(path, current)
            save(args.evidence / 'summary.json', summary(current))
            print(f'  {result["status"]} exit={result["exit_code"]} {result["duration_seconds"]}s {result["evidence"]}', flush=True)
    print(json.dumps(summary(current), indent=2), flush=True)

if __name__ == '__main__':
    main()
