#!/usr/bin/env python3
"""Check InitializeInterface diagnostics using an exact Git source-body slice.

Exit 0: diagnostic contracts passed; 1: contract failure; 2: test infrastructure
failure. This intentionally does not test or replace any unload/recovery method.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import traceback


SOURCE_PATH = "src/base/spice/SpiceInterface.cpp"
SIGNATURE = "void SpiceInterface::InitializeInterface()"
SENTINEL = "GMAT_DIAGNOSTIC_REVIEW_SENTINEL"
EXISTING_BYTES = b"PREEXISTING_DIAGNOSTICS_MUST_BE_PRESERVED\n"

SHIMS = r'''
// FileManager, MessageInterface, Integer and UtilityException are minimal test
// shims. No mission, SWIG or full GMAT lifecycle is represented by this harness.
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cmath>
extern "C" {
#include "SpiceUsr.h"
}
using Integer = int;
class UtilityException : public std::runtime_error {
public:
   using std::runtime_error::runtime_error;
};
static std::string reviewOutputPath;
class FileManager {
public:
   enum { OUTPUT_PATH };
   static FileManager* Instance() { static FileManager fm; return &fm; }
   std::string GetAbsPathname(int) { return reviewOutputPath; }
};
class MessageInterface {
public:
   static void ShowMessage(const char* format, ...) {
      va_list args;
      va_start(args, format);
      vprintf(format, args);
      va_end(args);
   }
};
class SpiceInterface {
   static Integer numInstances;
   static std::map<std::string, std::string> loadedKernels;
   static constexpr Integer MAX_LONG_MESSAGE = 1840;
public:
   static void InitializeInterface();
};
Integer SpiceInterface::numInstances = 0;
std::map<std::string, std::string> SpiceInterface::loadedKernels;
'''

DRIVER = r'''
int main(int argc, char** argv) {
   if (argc != 2) return 3;
   reviewOutputPath = argv[1];
   printf("REVIEW_TOOLKIT=%s\n", tkvrsn_c("TOOLKIT"));
   try {
      SpiceInterface::InitializeInterface();
   } catch (const std::exception& error) {
      printf("REVIEW_INIT_EXCEPTION=%s\n", error.what());
      return 4;
   }
   printf("REVIEW_INITIALIZED=1\n");
   SpiceChar device[1024];
   errdev_c("GET", sizeof(device), device);
   printf("REVIEW_ERROR_DEVICE=%s\n", device);
   setmsg_c("GMAT_DIAGNOSTIC_REVIEW_SENTINEL: invented test error");
   sigerr_c("SPICE(REVIEWERROR)");
   printf("REVIEW_RETURNED_AFTER_ERROR=1\n");
   printf("REVIEW_ERROR_WAS_SET=%d\n", failed_c() ? 1 : 0);
   SpiceChar original[1840];
   getmsg_c("SHORT", sizeof(original), original);
   printf("REVIEW_ORIGINAL_ERROR=%s\n", original);
   reset_c();
   SpiceInt code = 0;
   SpiceBoolean found = SPICEFALSE;
   bodn2c_c("EARTH", &code, &found);
   const SpiceDouble vector[3] = {3.0, 4.0, 0.0};
   const bool usable = found && code == 399 && vnorm_c(vector) == 5.0 && !failed_c();
   printf("REVIEW_USABLE_AFTER_RESET=%d\n", usable ? 1 : 0);
   return usable ? 0 : 5;
}
'''


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def exact_method(source: str) -> str:
    """Find the definition, matching braces while ignoring comments/literals."""
    start = source.index(SIGNATURE)
    brace = source.index("{", start + len(SIGNATURE))
    depth, i, state = 1, brace + 1, "code"
    while i < len(source):
        char = source[i]
        following = source[i:i + 2]
        if state == "line":
            if char == "\n":
                state = "code"
        elif state == "block":
            if following == "*/":
                state, i = "code", i + 1
        elif state in {"string", "char"}:
            if char == "\\":
                i += 1
            elif char == ('"' if state == "string" else "'"):
                state = "code"
        elif following == "//":
            state, i = "line", i + 1
        elif following == "/*":
            state, i = "block", i + 1
        elif char == '"':
            state = "string"
        elif char == "'":
            state = "char"
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
        i += 1
    raise ValueError("Unterminated InitializeInterface definition")


def run_command(command: list[str], *, cwd: Path, timeout: int,
                commands: list[dict]) -> subprocess.CompletedProcess:
    raw = subprocess.run(command, cwd=cwd, capture_output=True,
                         timeout=timeout, check=False)
    # Decode without universal-newline conversion so Git source bytes, including
    # any CRLFs, survive the extraction and recorded hashes unchanged.
    completed = subprocess.CompletedProcess(command, raw.returncode,
                                            raw.stdout.decode("utf-8"),
                                            raw.stderr.decode("utf-8"))
    commands.append({"argv": command, "cwd": str(cwd),
                     "returncode": completed.returncode})
    return completed


def header_inventory(include: Path) -> dict:
    files = {p.relative_to(include).as_posix(): sha256(p.read_bytes())
             for p in sorted(include.rglob("*")) if p.is_file()}
    encoded = json.dumps(files, sort_keys=True, separators=(",", ":")).encode()
    return {"files": files, "canonical_inventory_sha256": sha256(encoded)}


def markers(stdout: str) -> dict[str, str]:
    return dict(line.split("=", 1) for line in stdout.splitlines()
                if line.startswith("REVIEW_") and "=" in line)


def check_case(name: str, fixture: Path, binary: Path, out: Path,
               commands: list[dict]) -> dict:
    fixture.mkdir()
    output_path = fixture / "output"
    existing = None
    if name == "existing_log":
        if len(str(output_path / "GMATSpiceKernelError.1.txt")) > 255:
            raise RuntimeError("Evidence output path is too long for the existing-log fixture; choose a shorter --output")
        output_path.mkdir()
        existing = output_path / "GMATSpiceKernelError.txt"
        existing.write_bytes(EXISTING_BYTES)
    elif name == "missing_directory":
        # Deliberately leave the configured output directory absent.
        pass
    elif name == "long_path":
        # The existing directory is valid, while the SPICE destination exceeds
        # its filename bound. Keep each path component below common host limits.
        output_path.mkdir()
        while len(str(output_path / "GMATSpiceKernelError.txt")) <= 300:
            output_path /= "long_destination_" + "x" * 64
            output_path.mkdir()
    else:
        raise ValueError(name)
    invocation = [str(binary), str(output_path) + os.sep]
    completed = run_command(invocation, cwd=fixture, timeout=15, commands=commands)
    (out / f"{name}.stdout.txt").write_text(completed.stdout)
    (out / f"{name}.stderr.txt").write_text(completed.stderr)
    values = markers(completed.stdout)
    common = {
        "process_returned_successfully": completed.returncode == 0,
        "cspice_n0067": values.get("REVIEW_TOOLKIT") == "CSPICE_N0067",
        "initialization_returned": values.get("REVIEW_INITIALIZED") == "1",
        "synthetic_error_returned": values.get("REVIEW_RETURNED_AFTER_ERROR") == "1",
        "error_flag_was_set": values.get("REVIEW_ERROR_WAS_SET") == "1",
        "original_error_preserved": values.get("REVIEW_ORIGINAL_ERROR") == "SPICE(REVIEWERROR)",
        "usable_after_reset": values.get("REVIEW_USABLE_AFTER_RESET") == "1",
        "no_fileopenfailed_noise": "FILEOPENFAILED" not in completed.stdout + completed.stderr,
    }
    generated = sorted(output_path.glob("GMATSpiceKernelError*.txt")) if output_path.exists() else []
    detail = {"configured_output_path": str(output_path), "markers": values,
              "returncode": completed.returncode,
              "diagnostic_files": {p.name: sha256(p.read_bytes()) for p in generated}}
    if name == "existing_log":
        fresh = [p for p in generated if p != existing]
        common.update({
            "prior_log_bytes_preserved": existing.read_bytes() == EXISTING_BYTES,
            "one_fresh_diagnostic_selected": len(fresh) == 1,
            "fresh_sibling_selected": len(fresh) == 1 and fresh[0].name == "GMATSpiceKernelError.1.txt",
            "fresh_log_contains_original_diagnostic": len(fresh) == 1 and SENTINEL in fresh[0].read_text(errors="replace"),
            "device_matches_fresh_log": len(fresh) == 1 and values.get("REVIEW_ERROR_DEVICE") == str(fresh[0]),
        })
        detail["prior_log_expected_sha256"] = sha256(EXISTING_BYTES)
    else:
        common.update({
            "console_fallback_selected": values.get("REVIEW_ERROR_DEVICE") == "SCREEN",
            "console_has_original_diagnostic": SENTINEL in completed.stdout,
            "fallback_warning_present": "Raw SPICE diagnostics will use the console" in completed.stdout,
            "no_diagnostic_file_created": not generated,
        })
    return {"name": name, "passed": all(common.values()), "contracts": common,
            "evidence": detail}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cspice-dir", type=Path, required=True)
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    repo, out, cspice = args.repo.resolve(), args.output.resolve(), args.cspice_dir.resolve()
    # Require a fresh location so evidence cannot accidentally be overwritten.
    try:
        out.mkdir(parents=True, exist_ok=False)
    except OSError as error:
        print(f"infrastructure_failure: cannot create fresh --output: {error}", file=sys.stderr)
        return 2
    commands: list[dict] = []
    result = {
        "schema_version": 1, "test": "spice-initialize-diagnostics-only",
        "requested_revision": args.revision, "repo": str(repo), "commands": commands,
        "limitations": [
            "Verbatim InitializeInterface source-body slice, not complete linked GMAT, SWIG, or mission execution.",
            "FileManager, MessageInterface, Integer and UtilityException use explicit minimal shims.",
            "Only existing-log, missing-directory and overlong-path contracts are tested here.",
            "No unload, recovery, kernel ordering, global ownership, propagation or scientific-data behavior is tested.",
            "Usability means original SPICE error retrieval, reset, built-in EARTH name lookup and vector arithmetic succeed.",
            "Host compile/link flags use the supplied existing CSPICE static archive; this does not qualify other platforms.",
        ],
    }
    try:
        resolved = run_command(["git", "rev-parse", "--verify", args.revision + "^{commit}"],
                               cwd=repo, timeout=15, commands=commands)
        if resolved.returncode:
            raise RuntimeError(resolved.stderr)
        revision = resolved.stdout.strip()
        source_run = run_command(["git", "show", revision + ":" + SOURCE_PATH],
                                 cwd=repo, timeout=15, commands=commands)
        if source_run.returncode:
            raise RuntimeError(source_run.stderr)
        source = source_run.stdout
        method = exact_method(source)
        source_notice = source.partition("#include")[0]
        harness = "// Generated review harness; provenance is in results.json.\n" + source_notice + SHIMS + "\n" + method + "\n" + DRIVER
        harness_path, binary = out / "diagnostic_slice.cpp", out / "diagnostic_slice"
        harness_path.write_bytes(harness.encode("utf-8"))
        (out / "InitializeInterface.exact.txt").write_bytes(method.encode("utf-8"))
        # Independently verify the emitted method, without whitespace rewriting.
        if exact_method(harness_path.read_bytes().decode("utf-8")) != method:
            raise RuntimeError("Emitted method does not exactly match extracted Git method")
        library, include = cspice / "lib/cspice.a", cspice / "include"
        if not library.is_file() or not (include / "SpiceUsr.h").is_file():
            raise RuntimeError("--cspice-dir must contain lib/cspice.a and include/SpiceUsr.h")
        compiler = run_command([args.cxx, "--version"], cwd=out, timeout=15, commands=commands)
        if compiler.returncode:
            raise RuntimeError("Compiler version lookup failed: " + compiler.stderr)
        result.update({
            "revision": revision, "source_path": SOURCE_PATH,
            "source_file_sha256": sha256(source.encode()), "method_signature": SIGNATURE,
            "method_sha256": sha256(method.encode()), "method_bytes": len(method.encode()),
            "emitted_method_verbatim_match": True, "harness_sha256": sha256(harness.encode()),
            "compiler": compiler.stdout, "compile_flags": ["-std=c++11", "-O0"],
            "dependency": {"cspice_dir": str(cspice), "library": str(library),
                           "library_sha256": sha256(library.read_bytes()),
                           "headers": header_inventory(include)},
        })
        compile_command = [args.cxx, "-std=c++11", "-O0", "-I" + str(include),
                           str(harness_path), str(library), "-lm", "-o", str(binary)]
        compiled = run_command(compile_command, cwd=out, timeout=120, commands=commands)
        (out / "compile.stdout.txt").write_text(compiled.stdout)
        (out / "compile.stderr.txt").write_text(compiled.stderr)
        if compiled.returncode:
            raise RuntimeError("Harness compilation failed; see compile.stderr.txt")
        result["cases"] = [check_case(name, out / ("fixture-" + name), binary, out, commands)
                           for name in ("existing_log", "missing_directory", "long_path")]
        result["passed"] = all(case["passed"] for case in result["cases"])
        exit_code = 0 if result["passed"] else 1
        result["outcome"] = "passed" if exit_code == 0 else "contract_failure"
    except Exception as error:
        result.update({"passed": False, "outcome": "infrastructure_failure",
                       "error": str(error), "traceback": traceback.format_exc()})
        exit_code = 2
    result["exit_code"] = exit_code
    (out / "results.json").write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(f"{result['outcome']}: {out / 'results.json'}")
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
