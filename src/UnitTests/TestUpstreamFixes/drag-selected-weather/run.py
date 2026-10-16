#!/usr/bin/env python3
"""Compile immutable GMAT selected-weather code in a small explicit environment.

Exit 0: every selected-weather contract passed; 1: native contract failure;
2: extraction, compilation, invocation, or harness-protocol error.
No repository files, index, refs, or installed GMAT artifacts are modified.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

DRAG = "src/base/forcemodel/DragForce.cpp"
ATMOSPHERE = "src/base/solarsys/AtmosphereModel.cpp"
INITIALIZE_START = '            F107ID = atmos->GetParameterID("F107");'
INITIALIZE_END = "            if (cbFixed != NULL)"
ATTACH_SIGNATURE = "bool DragForce::SetInternalAtmosphereModel(AtmosphereModel* atm)"
SETTER_SIGNATURE = ("bool AtmosphereModel::SetStringParameter(const Integer id,\n"
                    "      const std::string &value)")


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def command(args: list[str], *, cwd: Path | None = None,
            timeout: int = 90, limits: bool = False) -> subprocess.CompletedProcess[str]:
    preexec = None
    if limits and os.name == "posix":
        import resource

        def set_limits() -> None:
            resource.setrlimit(resource.RLIMIT_CPU, (60, 60))
            resource.setrlimit(resource.RLIMIT_AS, (1024 ** 3, 1024 ** 3))
            resource.setrlimit(resource.RLIMIT_FSIZE, (64 * 1024 ** 2, 64 * 1024 ** 2))

        preexec = set_limits
    return subprocess.run(args, cwd=cwd, text=True, encoding="utf-8",
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=timeout, preexec_fn=preexec, check=False)


def git(repo: Path, *args: str) -> str:
    proc = command(["git", "-C", str(repo), *args], timeout=30)
    if proc.returncode:
        raise ValueError(f"Git read failed: {proc.stderr.strip()}")
    return proc.stdout


def closing_brace(source: str, opening: int) -> int:
    """Match a C++ function brace, excluding comments and string/char literals."""
    depth, pos, state = 0, opening, "code"
    while pos < len(source):
        ch, pair = source[pos], source[pos:pos + 2]
        if state == "line":
            if ch == "\n":
                state = "code"
        elif state == "block":
            if pair == "*/":
                state, pos = "code", pos + 1
        elif state in {"string", "char"}:
            if ch == "\\":
                pos += 1
            elif (state == "string" and ch == '"') or (state == "char" and ch == "'"):
                state = "code"
        elif pair == "//":
            state, pos = "line", pos + 1
        elif pair == "/*":
            state, pos = "block", pos + 1
        elif ch in {'"', "'"}:
            state = "string" if ch == '"' else "char"
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return pos
        pos += 1
    raise ValueError("Unmatched source-function brace")


def method(source: str, signature: str) -> tuple[str, int, int]:
    if source.count(signature) != 1:
        raise ValueError(f"Expected exactly one signature: {signature!r}")
    start = source.index(signature)
    opening = source.index("{", start + len(signature))
    end = closing_brace(source, opening) + 1
    return source[start:end], start, end


def extract(drag: str, atmosphere: str) -> dict[str, dict]:
    initialize, init_offset, _ = method(drag, "bool DragForce::Initialize()")
    if initialize.count(INITIALIZE_START) != 1 or initialize.count(INITIALIZE_END) != 1:
        raise ValueError("Initialize numeric/weather extraction anchors changed or are ambiguous")
    start, end = initialize.index(INITIALIZE_START), initialize.index(INITIALIZE_END)
    if end <= start:
        raise ValueError("Initialize extraction anchors are reversed")
    region = initialize[start:end]
    # Gate placement and scope on both NASA and candidate source. No density,
    # source-selection reimplementation, or hand-copied candidate logic is used.
    for expected in ["FileManager *fm = FileManager::Instance();",
                     "atmos->SetStringParameter(cssiWFileID, weatherfile);",
                     "atmos->SetStringParameter(schattenWFileID, weatherfile);"]:
        if region.count(expected) != 1:
            raise ValueError(f"Initialize region boundary/content gate failed: {expected}")
    if "atmos->Initialize()" in region or "Density(" in region:
        raise ValueError("Unexpected density or full-atmosphere initialization in selected region")
    attachment, attach_start, _ = method(drag, ATTACH_SIGNATURE)
    setter, setter_start, _ = method(atmosphere, SETTER_SIGNATURE)
    if attachment.count("return true;") != 1 or attachment.count("return false;") != 1:
        raise ValueError("Attachment whole-function boundary gate failed")
    if (setter.count("if (id == CSSI_WEATHER_FILE)") != 1 or
            setter.count("if (id == SCHATTEN_WEATHER_FILE)") != 1 or
            not setter.rstrip().endswith("return GmatBase::SetStringParameter(id, value);\n}")):
        raise ValueError("Atmosphere filename-setter whole-function boundary gate failed")
    snippets = {
        "initialize_region": (region, DRAG, drag, init_offset + start),
        "attachment_function": (attachment, DRAG, drag, attach_start),
        "atmosphere_filename_setter": (setter, ATMOSPHERE, atmosphere, setter_start),
    }
    result = {}
    for name, (code, path, full, offset) in snippets.items():
        result[name] = {"code": code, "path": path,
                        "start_line": full.count("\n", 0, offset) + 1,
                        "end_line": full.count("\n", 0, offset + len(code)) + 1,
                        "sha256": sha(code.encode("utf-8"))}
    return result


def fixtures(directory: Path) -> None:
    # Every numerical value here is invented; these are parser-tag fixtures,
    # not redistributed observed or predicted scientific data.
    (directory / "cssi-valid.txt").write_text(
        "DATATYPE CSSISPACEWEATHER\nBEGIN OBSERVED\n"
        "2020 7 20 0 0 30 30 30 30 30 30 30 30 240 15 15 15 15 15 15 15 15 15\n"
        "END OBSERVED\n", encoding="ascii")
    (directory / "schatten-valid.txt").write_text(
        "PREDICTED SOLAR DATA\nBEGIN_DATA\n"
        "7 2020 180 200 160 15 180 200 160 15 180 200 160 15\nEND_DATA\n",
        encoding="ascii")
    (directory / "malformed.txt").write_text("invalid synthetic fixture\n", encoding="ascii")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--prepare-only", action="store_true",
                        help="Extract and emit source/receipt without invoking a compiler")
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() and (not out.is_dir() or any(out.iterdir())):
        parser.error("--output must be a new directory or an existing empty directory; receipts are never overwritten")
    out.mkdir(parents=True, exist_ok=True)
    report = {"schema_version": 1, "test": "drag-selected-weather", "requested_revision": args.revision,
              "limits": {"cpu_seconds_per_child": 60, "address_space_bytes": 1024 ** 3,
                         "max_output_file_bytes": 64 * 1024 ** 2, "compiler_processes": 1,
                         "wall_timeout_seconds": 90, "posix_limits_applied": os.name == "posix"},
              "scope": "Exact selected Initialize numeric/weather region + entire attachment and actual atmosphere filename-setter functions in explicit environment stubs",
              "limitations": ["Not a full GMAT build or full DragForce Initialize execution",
                              "No density, weather numerical-record reader, force, integration, or physical-truth validation",
                              "Stubs implement only exercised FileManager, base-parameter, coordinate-pointer and text-line utilities",
                              "Synthetic tag fixtures do not represent observed or predicted scientific weather"]}
    try:
        repo = args.repo.resolve()
        revision = git(repo, "rev-parse", "--verify", args.revision + "^{commit}").strip()
        report["revision"] = revision
        raw_paths = {p: subprocess.check_output(["git", "-C", str(repo), "show", revision + ":" + p],
                                               timeout=30) for p in [DRAG, ATMOSPHERE]}
        paths = {p: contents.decode("utf-8") for p, contents in raw_paths.items()}
        report["source_files"] = [{"path": p, "sha256": sha(contents)} for p, contents in raw_paths.items()]
        snippets = extract(paths[DRAG], paths[ATMOSPHERE])
        report["extracted_regions"] = [{k: v for k, v in item.items() if k != "code"}
                                         | {"name": name} for name, item in snippets.items()]
        template_path = Path(__file__).with_name("harness.cpp.in")
        template = template_path.read_text(encoding="utf-8")
        report["harness_template_sha256"] = sha(template.encode())
        for name, item in snippets.items():
            token = "@@" + name.upper() + "@@"
            if template.count(token) != 1:
                raise ValueError(f"Harness template token is not unique: {token}")
            # #line provides original immutable source locations to diagnostics;
            # every character inside each extracted function/region is retained.
            code = (f'#line {item["start_line"]} "{item["path"]}@{revision}"\n' +
                    item["code"] + '\n#line 1 "harness_environment"\n')
            template = template.replace(token, code)
            (out / (name + ".cpp.fragment")).write_text(item["code"], encoding="utf-8")
        generated = out / "harness.cpp"
        generated.write_text(template, encoding="utf-8")
        report["generated_harness_sha256"] = sha(template.encode())
        if args.prepare_only:
            report.update(status="prepared_only", passed=None)
            (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
            print(json.dumps(report, indent=2))
            return 0
        compiler = shutil.which(args.cxx)
        if not compiler:
            raise ValueError(f"Compiler not found: {args.cxx!r}; pass a single executable with --cxx")
        version = command([compiler, "--version"], timeout=30)
        report["compiler"] = {"executable": compiler, "version": version.stdout.strip(),
                              "version_exit_code": version.returncode}
        executable = out / "drag-selected-weather"
        compile_args = [compiler, "-std=c++17", "-O0", "-Wall", "-Wextra", "-pedantic",
                        str(generated), "-o", str(executable)]
        report["compile_command"] = compile_args
        compiled = command(compile_args, cwd=out, limits=True)
        (out / "compiler.stdout.txt").write_text(compiled.stdout)
        (out / "compiler.stderr.txt").write_text(compiled.stderr)
        report["compile_exit_code"] = compiled.returncode
        if compiled.returncode:
            raise RuntimeError("Native harness compilation failed; see compiler.stderr.txt")
        report["executable_sha256"] = sha(executable.read_bytes())
        with tempfile.TemporaryDirectory(prefix="fixtures-", dir=out) as temp:
            fixture_path = Path(temp)
            fixtures(fixture_path)
            report["synthetic_fixtures"] = [{"name": p.name, "sha256": sha(p.read_bytes())}
                                             for p in sorted(fixture_path.iterdir())]
            run_args = [str(executable), str(fixture_path)]
            report["run_command"] = run_args
            ran = command(run_args, cwd=out, limits=True)
        (out / "native.stdout.json").write_text(ran.stdout)
        (out / "native.stderr.txt").write_text(ran.stderr)
        report["native_exit_code"] = ran.returncode
        payload = json.loads(ran.stdout)
        if not isinstance(payload.get("cases"), list) or ran.returncode not in {0, 1}:
            raise RuntimeError("Unexpected native harness report/exit code")
        failures = sum(not case["passed"] for case in payload["cases"])
        if (failures == 0) != (ran.returncode == 0):
            raise RuntimeError("Native exit code disagrees with its contract results")
        report.update(status="passed" if failures == 0 else "contract_failed", passed=failures == 0,
                      failed_cases=failures, cases=payload["cases"],
                      same_inputs_legacy_defined_success=payload["same_inputs_legacy_defined_success"])
        (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))
        return 0 if failures == 0 else 1
    except Exception as error:
        report.update(status="harness_error", passed=False, error=f"{type(error).__name__}: {error}")
        (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
