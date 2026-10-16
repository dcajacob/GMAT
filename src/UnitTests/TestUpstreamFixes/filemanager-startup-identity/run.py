#!/usr/bin/env python3
"""Compile exact revision-pinned native source slices and test one behavioral contract.

Uses Python's standard library, Git and an existing C++17 compiler. Does not build
GMAT, import gmatpy, access the network or modify the source checkout.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def extract_function(text, signature):
    # Preserve offsets while hiding comments and quoted literals from brace counting.
    # The selected native methods contain no C++ raw string literals.
    pattern = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    masked = pattern.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m.group()), text)
    definition = re.search(r'(?m)^' + re.escape(signature), text)
    if definition is None:
        raise RuntimeError('Missing exact native definition: ' + signature)
    start = definition.start()
    opening = masked.index('{', definition.end())
    depth = 1
    end = opening + 1
    while depth:
        if end >= len(masked):
            raise RuntimeError('Unbalanced native definition: ' + signature)
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return text[start:end], text.count('\n', 0, start) + 1, text.count('\n', 0, end) + 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True, type=Path, help='Existing Git repository; read through git show only')
    parser.add_argument('--revision', required=True, help='Full immutable 40-character Git commit SHA')
    parser.add_argument('--output', required=True, type=Path, help='New output directory; must not exist')
    args = parser.parse_args()
    if not re.fullmatch(r'[0-9a-fA-F]{40}', args.revision):
        parser.error('--revision must be a full 40-character commit SHA')
    output = args.output.expanduser().resolve()
    output.mkdir(parents=True, exist_ok=False)
    repo = args.repo.expanduser().resolve()
    report = {
        'test': TEST_NAME, 'requested_revision': args.revision.lower(),
        'repository': str(repo), 'python': sys.version, 'status': 'infrastructure_error',
        'sources': [], 'extractions': [], 'commands': [], 'limitations': LIMITS,
        'exit_codes': {'0': 'all behavioral contracts passed', '1': 'compiled probe reports a behavioral contract failure', '2': 'infrastructure/extraction/compilation/probe-format error'},
    }
    def command(argv, *, cwd=output, timeout=60):
        argv = [str(x) for x in argv]
        report['commands'].append({'argv': argv, 'cwd': str(cwd)})
        result = subprocess.run(argv, cwd=cwd, capture_output=True, timeout=timeout)
        report['commands'][-1]['returncode'] = result.returncode
        return result
    exit_code = 2
    try:
        resolved = command(['git', '-C', repo, 'rev-parse', '--verify', args.revision + '^{commit}'])
        if resolved.returncode:
            raise RuntimeError(resolved.stderr.decode(errors='replace'))
        revision = resolved.stdout.decode().strip()
        report['revision'] = revision
        native = []
        source_output = output / 'source'
        source_output.mkdir()
        for path, signatures in SOURCE_DEFINITIONS:
            blob = command(['git', '-C', repo, 'show', revision + ':' + path])
            if blob.returncode:
                raise RuntimeError(blob.stderr.decode(errors='replace'))
            blob_id = command(['git', '-C', repo, 'rev-parse', revision + ':' + path])
            if blob_id.returncode:
                raise RuntimeError(blob_id.stderr.decode(errors='replace'))
            destination = source_output / Path(path).name
            destination.write_bytes(blob.stdout)
            report['sources'].append({'path': path, 'git_blob': blob_id.stdout.decode().strip(), 'sha256': sha256(blob.stdout), 'saved_as': str(destination.relative_to(output))})
            source_text = blob.stdout.decode('utf-8')
            for index, signature in enumerate(signatures):
                method, first, last = extract_function(source_text, signature)
                method_bytes = method.encode('utf-8')
                name = Path(path).stem + '-' + str(index) + '.inc'
                (source_output / name).write_bytes(method_bytes)
                report['extractions'].append({'path': path, 'signature': signature, 'first_line': first, 'last_line': last, 'sha256': sha256(method_bytes), 'saved_as': 'source/' + name})
                native.append(method)
        translation_unit = CPP_SUPPORT + '\n' + '\n'.join(native) + '\n' + CPP_MAIN
        source_file = output / 'probe.cpp'
        source_file.write_text(translation_unit, encoding='utf-8')
        report['generated_cpp_sha256'] = sha256(source_file.read_bytes())
        compiler = shlex.split(os.environ.get('CXX', 'c++'))
        if not compiler:
            raise RuntimeError('CXX is empty')
        compiler_path = shutil.which(compiler[0])
        if compiler_path is None:
            raise RuntimeError('Existing C++ compiler not found: ' + compiler[0])
        compiler[0] = compiler_path
        version = command(compiler + ['--version'])
        (output / 'compiler-version.txt').write_bytes(version.stdout + version.stderr)
        report['compiler'] = {'argv_prefix': compiler, 'version': (version.stdout + version.stderr).decode(errors='replace').strip()}
        binary = output / ('probe.exe' if os.name == 'nt' else 'probe')
        build = command(compiler + ['-std=c++17', '-O0', '-Wall', '-Wextra', str(source_file), '-o', str(binary)], timeout=120)
        (output / 'compile.stdout.log').write_bytes(build.stdout)
        (output / 'compile.stderr.log').write_bytes(build.stderr)
        if build.returncode:
            raise RuntimeError('C++ compilation failed; inspect compile.stderr.log')
        report['binary_sha256'] = sha256(binary.read_bytes())
        probe = command([binary], timeout=30)
        (output / 'probe.stdout.log').write_bytes(probe.stdout)
        (output / 'probe.stderr.log').write_bytes(probe.stderr)
        if probe.returncode not in (0, 1):
            raise RuntimeError('Native probe terminated unexpectedly: ' + str(probe.returncode))
        payload = json.loads(probe.stdout.decode('utf-8'))
        checks = payload['checks']
        if not checks or any(type(item.get('passed')) is not bool for item in checks):
            raise RuntimeError('Malformed native checks')
        expected_code = 0 if all(item['passed'] for item in checks) else 1
        if probe.returncode != expected_code:
            raise RuntimeError('Native status does not agree with reported checks')
        report['checks'] = checks
        report['passed'] = sum(item['passed'] for item in checks)
        report['failed'] = len(checks) - report['passed']
        report['status'] = 'pass' if expected_code == 0 else 'contract_failure'
        exit_code = expected_code
    except Exception as error:
        report['error'] = str(error)
    report['exit_code'] = exit_code
    (output / 'RESULTS.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'test': TEST_NAME, 'status': report['status'], 'exit_code': exit_code, 'results': str(output / 'RESULTS.json'), 'passed': report.get('passed'), 'failed': report.get('failed')}))
    return exit_code


TEST_NAME = 'filemanager-startup-identity'
SOURCE_DEFINITIONS = [('src/gmatutil/util/FileManager.cpp', ['std::string FileManager::GetFullStartupFilePath()'])]
LIMITS = ['Exact GetFullStartupFilePath definition is compiled against a small state shim; no FileManager singleton or startup parser is linked.', 'Initialized directory/name state is supplied directly, modeling the values saved by ReadStartupFile. GmatFileUtil cwd lookup uses the real standard-library process cwd.', 'Absolute, relative and empty stored directories are checked for return value and lack of mutation across actual cwd changes. Relative paths are preserved, not claimed to be made absolute.', 'No GUI, Windows-specific path spelling, encoding, concurrent cwd changes or complete Python initialization is validated.']
CPP_SUPPORT = r'''
#include <iostream>
#include <string>
#include <vector>
struct Check { std::string name, observed, expected; bool passed; };
std::vector<Check> checks;
std::string json_quote(const std::string &text) {
    std::string out="\"";
    const char* hex="0123456789abcdef";
    for (unsigned char c : text) {
        if (c=='"' || c=='\\') { out+='\\'; out+=static_cast<char>(c); }
        else if (c=='\n') out+="\\n";
        else if (c=='\r') out+="\\r";
        else if (c=='\t') out+="\\t";
        else if (c<32) { out+="\\u00"; out+=hex[c>>4]; out+=hex[c&15]; }
        else out+=static_cast<char>(c);
    }
    return out+'"';
}
void check(const std::string &name, const std::string &observed, const std::string &expected) {
    checks.push_back({name,observed,expected,observed==expected});
}
int finish() {
    bool passed=true;
    std::cout<<"{\"checks\":[";
    for (std::size_t i=0;i<checks.size();++i) {
        const auto &c=checks[i]; passed=passed&&c.passed;
        if(i) std::cout<<',';
        std::cout<<"{\"name\":"<<json_quote(c.name)<<",\"observed\":"<<json_quote(c.observed)
                 <<",\"expected\":"<<json_quote(c.expected)<<",\"passed\":"<<(c.passed?"true":"false")<<'}';
    }
    std::cout<<"]}\n";
    return passed?0:1;
}

#include <filesystem>
namespace GmatFileUtil {
std::string GetCurrentWorkingDirectory() { return std::filesystem::current_path().string(); }
}
struct FileManager {
    std::string mStartupFileDir,mStartupFileName,mPathSeparator="/";
    std::string GetFullStartupFilePath();
};
'''
CPP_MAIN = r'''
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::current_path();
    fs::create_directories(root/"configured");
    fs::create_directories(root/"caller-one");
    fs::create_directories(root/"caller-two");
    FileManager manager;
    const std::string configured=(root/"configured").string()+"/";
    manager.mStartupFileDir=configured;
    manager.mStartupFileName="custom_startup.txt";
    fs::current_path(root/"caller-one");
    check("absolute initialized path is returned",manager.GetFullStartupFilePath(),configured+"custom_startup.txt");
    check("getter preserves initialized directory",manager.mStartupFileDir,configured);
    fs::current_path(root/"caller-two");
    check("subsequent cwd change preserves startup identity",manager.GetFullStartupFilePath(),configured+"custom_startup.txt");
    check("second getter has no directory side effect",manager.mStartupFileDir,configured);
    manager.mStartupFileDir="relative/";
    check("relative stored directory is preserved",manager.GetFullStartupFilePath(),"relative/custom_startup.txt");
    check("relative getter has no directory side effect",manager.mStartupFileDir,"relative/");
    manager.mStartupFileDir="";
    check("empty directory returns filename",manager.GetFullStartupFilePath(),"custom_startup.txt");
    check("empty directory remains empty",manager.mStartupFileDir,"");
    fs::current_path(root);
    return finish();
}
'''

if __name__ == "__main__":
    raise SystemExit(main())
