#!/usr/bin/env python3
"""Compile exact EphemerisFile source slices and exercise temporary output paths.

Standard-library-only driver. Requires git, a POSIX C++17 compiler and libc.
Exit 0: all regression assertions pass; 1: a regression assertion fails;
2: source extraction, compilation or execution could not complete.
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
import tempfile


EPHEMERIS_PATH = "src/base/subscriber/EphemerisFile.cpp"
FILE_UTIL_PATH = "src/gmatutil/util/FileUtil.cpp"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def brace_end(text, opening):
    """Find an actual C++ brace pair, ignoring quoted strings and comments."""
    if text[opening] != "{":
        raise ValueError("Extraction did not start at an opening brace")
    depth = 0
    position = opening
    state = "code"
    while position < len(text):
        char = text[position]
        following = text[position:position + 2]
        if state == "line_comment":
            if char == "\n":
                state = "code"
        elif state == "block_comment":
            if following == "*/":
                state = "code"
                position += 1
        elif state in ("string", "character"):
            if char == "\\":
                position += 1
            elif char == ('"' if state == "string" else "'"):
                state = "code"
        elif following == "//":
            state = "line_comment"
            position += 1
        elif following == "/*":
            state = "block_comment"
            position += 1
        elif char == '"':
            state = "string"
        elif char == "'":
            state = "character"
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return position + 1
        position += 1
    raise ValueError("Unbalanced C++ source braces")


def function_span(text, signature):
    matches = list(re.finditer(signature, text))
    if len(matches) != 1:
        raise ValueError("Expected exactly one function signature: " + signature)
    start = matches[0].start()
    opening = text.index("{", matches[0].end())
    return start, brace_end(text, opening)


def select_slices(ephemeris, file_util):
    initialize_start, initialize_end = function_span(
        ephemeris, r"bool EphemerisFile::Initialize\(\)")
    initialize = ephemeris[initialize_start:initialize_end]
    marker = "   // Delete old file to avoid showing old file contents in the GUI output tree"
    if initialize.count(marker) != 1:
        raise ValueError("Initialize deletion-region marker changed")
    deletion_start = initialize_start + initialize.index(marker)
    check = "if (GmatFileUtil::DoesFileExist(fullPathFileName))"
    if initialize.count(check) != 1:
        raise ValueError("Expected exactly one original DoesFileExist deletion block")
    check_start = initialize_start + initialize.index(check)
    deletion_end = brace_end(ephemeris, ephemeris.index("{", check_start))
    deletion = ephemeris[deletion_start:deletion_end]
    # This is a structural extraction check, not the expected behavior assertion.
    # In particular, never omit the baseline remove() call from the slice.
    if "remove(fullPathFileName.c_str());" not in deletion:
        raise ValueError("The exact original remove() call was not retained")

    setter_start, setter_end = function_span(
        ephemeris,
        r"bool EphemerisFile::SetStringParameter\(const Integer id, const std::string &value\)",
    )
    setter = ephemeris[setter_start:setter_end]
    case_marker = "case FILENAME:"
    if setter.count(case_marker) != 1:
        raise ValueError("Expected exactly one Filename switch branch")
    case_start = setter_start + setter.index(case_marker)
    case_open = ephemeris.index("{", case_start)
    case_end = brace_end(ephemeris, case_open)
    branch = ephemeris[case_open:case_end]
    assignments = list(re.finditer(r"(?m)^      prevFileName = fileName;$", branch))
    if len(assignments) != 1:
        raise ValueError("Filename resolution-region marker changed")
    resolution_start = case_open + assignments[0].start()
    returns = list(re.finditer(r"\breturn true;", ephemeris[resolution_start:case_end]))
    if len(returns) != 1:
        raise ValueError("Expected exactly one final Filename success return")
    resolution_end = resolution_start + returns[0].end()
    resolution = ephemeris[resolution_start:resolution_end]
    if "GmatBase::GetFullPathFileName" not in resolution or "SetFileName();" not in resolution:
        raise ValueError("Filename resolver or writer-name update was lost in extraction")

    result = {
        "initialize_deletion_region": (EPHEMERIS_PATH, deletion_start, deletion_end, deletion),
        "filename_resolution_validation_region": (EPHEMERIS_PATH, resolution_start, resolution_end, resolution),
    }
    signatures = {
        "parse_path_name": r"std::string GmatFileUtil::ParsePathName\(const std::string &fullPath,\s*bool appendSep\)",
        "does_directory_exist": r"bool GmatFileUtil::DoesDirectoryExist\(const std::string &fullPath, bool isBlankOk\)",
        "does_file_exist": r"bool GmatFileUtil::DoesFileExist\(const std::string &filename\)",
    }
    for name, signature in signatures.items():
        start, end = function_span(file_util, signature)
        result[name] = (FILE_UTIL_PATH, start, end, file_util[start:end])
    return result


HARNESS = r'''// Generated from exact source slices; see source-receipt.json.
#include <cstdio>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace GmatFileUtil {
std::string ParsePathName(const std::string &, bool appendSep = true);
bool DoesDirectoryExist(const std::string &, bool isBlankOk = true);
bool DoesFileExist(const std::string &);
}

// Exact revision-selected FileUtil dependencies, with debug macros undefined.
@@FILE_UTIL@@

class SubscriberException : public std::exception {
   std::string message;
public:
   SubscriberException() = default;
   explicit SubscriberException(const std::string &value) : message(value) {}
   void SetDetails(const char *format, const char *value) {
      char buffer[2048];
      std::snprintf(buffer, sizeof(buffer), format, value);
      message = buffer;
   }
   const char *what() const noexcept override { return message.c_str(); }
};

// Deliberate seam: no FileManager singleton/startup configuration is loaded.
// Each test injects the resolver result; the source's handling of it is exact.
struct GmatBase {
   static std::string resolverResult;
   static std::string GetFullPathFileName(const std::string &, const std::string &,
       std::string &, const std::string &, bool, const std::string &, bool, bool) {
      return resolverResult;
   }
};
std::string GmatBase::resolverResult;

struct EphemerisFileSlice {
   std::string fullPathFileName, fileName, prevFileName;
   bool isInitialized = false;
   bool usingDefaultFileName = true;
   unsigned writerNameUpdates = 0;
   std::string GetName() const { return "OutputPathRegression"; }
   void SetFileName() { ++writerNameUpdates; }
   void InitializeDeletionRegion() {
@@INITIALIZE@@
   }
   bool SetFilenameResolutionRegion(const std::string &value) {
@@SETTER@@
   }
};

struct Result { std::string name; bool passed; std::string observation; };
static std::vector<Result> results;
static void record(const std::string &name, bool passed, const std::string &observation) {
   results.push_back({name, passed, observation});
}
static void writeFile(const fs::path &path, const std::string &value) {
   std::ofstream stream(path);
   stream << value;
   if (!stream) throw std::runtime_error("Test fixture write failed");
}
static std::string readFile(const fs::path &path) {
   std::ifstream stream(path);
   return std::string(std::istreambuf_iterator<char>(stream), {});
}
static bool initializeRejected(EphemerisFileSlice &object) {
   try { object.InitializeDeletionRegion(); return false; }
   catch (const SubscriberException &) { return true; }
}
static bool setterRejected(EphemerisFileSlice &object, const std::string &value) {
   try { object.SetFilenameResolutionRegion(value); return false; }
   catch (const SubscriberException &) { return true; }
}
static bool validSetterAndWrite(EphemerisFileSlice &object, const std::string &path) {
   GmatBase::resolverResult = path;
   unsigned before = object.writerNameUpdates;
   if (!object.SetFilenameResolutionRegion(path) || object.fullPathFileName != path ||
       object.fileName != path || object.usingDefaultFileName ||
       object.writerNameUpdates != before + 1) return false;
   object.InitializeDeletionRegion();
   writeFile(path, "new output\n");
   return readFile(path) == "new output\n";
}

int main() {
   try {
      fs::create_directory("empty-output-directory");
      const bool ifstreamRecognizesDirectory =
         GmatFileUtil::DoesFileExist("empty-output-directory");
      EphemerisFileSlice empty;
      empty.fullPathFileName = "empty-output-directory";
      const bool rejectedEmpty = initializeRejected(empty);
      const bool preservedEmpty = fs::is_directory("empty-output-directory");
      record("initialize_preserves_and_rejects_empty_directory",
         rejectedEmpty && preservedEmpty,
         std::string("exception=") + (rejectedEmpty ? "true" : "false") +
         ";directory_preserved=" + (preservedEmpty ? "true" : "false") +
         ";actual_DoesFileExist_on_directory=" +
         (ifstreamRecognizesDirectory ? "true" : "false"));

      fs::create_directory("nonempty-output-directory");
      writeFile("nonempty-output-directory/marker", "preserve contents\n");
      EphemerisFileSlice nonempty;
      nonempty.fullPathFileName = "nonempty-output-directory";
      const bool rejectedNonempty = initializeRejected(nonempty);
      record("initialize_preserves_and_rejects_nonempty_directory",
         rejectedNonempty && fs::is_directory("nonempty-output-directory") &&
         readFile("nonempty-output-directory/marker") == "preserve contents\n",
         std::string("exception=") + (rejectedNonempty ? "true" : "false") +
         ";marker_preserved=" +
         (readFile("nonempty-output-directory/marker") == "preserve contents\n" ? "true" : "false"));

      writeFile("existing.eph", "old output\n");
      EphemerisFileSlice existing;
      existing.fullPathFileName = "existing.eph";
      const bool rejectedFile = initializeRejected(existing);
      const bool oldFileRemoved = !fs::exists("existing.eph");
      writeFile("existing.eph", "replacement output\n");
      record("initialize_regular_output_still_removes_old_file_and_allows_write",
         !rejectedFile && oldFileRemoved && readFile("existing.eph") == "replacement output\n",
         std::string("exception=") + (rejectedFile ? "true" : "false") +
         ";old_file_removed=" + (oldFileRemoved ? "true" : "false"));

      fs::create_directory("valid-parent");
      EphemerisFileSlice missing;
      missing.fullPathFileName = "valid-parent/missing.eph";
      const bool rejectedMissing = initializeRejected(missing);
      writeFile(missing.fullPathFileName, "new output\n");
      record("initialize_missing_output_allows_write",
         !rejectedMissing && readFile(missing.fullPathFileName) == "new output\n",
         std::string("exception=") + (rejectedMissing ? "true" : "false"));

      EphemerisFileSlice unresolved;
      GmatBase::resolverResult.clear();
      const bool rejectedUnresolved = setterRejected(unresolved, "unresolvable.eph");
      const bool untouchedWriter = unresolved.writerNameUpdates == 0;
      record("filename_rejects_empty_resolver_result_before_writer_update",
         rejectedUnresolved && untouchedWriter,
         std::string("exception=") + (rejectedUnresolved ? "true" : "false") +
         ";writer_name_updates=" + std::to_string(unresolved.writerNameUpdates));
      record("filename_valid_request_succeeds_after_empty_resolver_rejection",
         validSetterAndWrite(unresolved, "valid-parent/after-empty.eph"),
         "same_object_reused_for_valid_filename_and_file_write");

      EphemerisFileSlice missingParent;
      GmatBase::resolverResult = "absent-parent/output.eph";
      const bool rejectedParent = setterRejected(missingParent, "absent-parent/output.eph");
      record("filename_rejects_missing_parent",
         rejectedParent && missingParent.writerNameUpdates == 0 && !fs::exists("absent-parent"),
         std::string("exception=") + (rejectedParent ? "true" : "false"));
      record("filename_valid_request_succeeds_after_missing_parent_rejection",
         validSetterAndWrite(missingParent, "valid-parent/after-missing-parent.eph"),
         "same_object_reused_for_valid_filename_and_file_write");

      EphemerisFileSlice plain;
      record("filename_current_directory_output_still_succeeds",
         validSetterAndWrite(plain, "current-directory.eph"),
         "valid_resolution_and_file_write");

      bool allPassed = true;
      std::cout << "{\"tests\":[";
      for (std::size_t index = 0; index < results.size(); ++index) {
         const Result &result = results[index];
         if (index) std::cout << ',';
         std::cout << "{\"name\":\"" << result.name << "\",\"passed\":"
                   << (result.passed ? "true" : "false")
                   << ",\"observation\":\"" << result.observation << "\"}";
         allPassed = allPassed && result.passed;
      }
      std::cout << "],\"all_passed\":" << (allPassed ? "true" : "false") << "}\n";
      return allPassed ? 0 : 1;
   } catch (const std::exception &error) {
      // Fixed message keeps temporary paths/host details out of published logs.
      std::cerr << "Native harness could not complete a fixture operation\n";
      return 2;
   }
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, type=Path)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    commands = []
    report = {
        "test": "ephemeris-output-paths",
        "driver_sha256": sha256(Path(__file__).read_bytes()),
        "scope": "Exact contiguous EphemerisFile slices plus exact FileUtil dependencies; not a whole-class or ephemeris-writer integration test.",
        "limits": {
            "compiler_processes": 1,
            "compile_timeout_seconds": 60,
            "native_run_timeout_seconds": 20,
            "host": "POSIX C++17; Linux directory-ifstream behavior is observed, not emulated",
            "resolver": "Injected return value; actual FileManager path-resolution behavior is outside this test",
            "writer": "SetFileName call counted; valid file writes use std::ofstream rather than ephemeris serialization",
            "initialize": "Runs the deletion region only; full Initialize validation and already-initialized short circuit are outside this test",
            "setter": "Runs Filename assignment/resolution/validation/success region; preceding filename-character validation and initialized-writer shutdown are outside this test",
        },
        "commands": commands,
    }
    try:
        def git(*git_args):
            command = ["git", *git_args]
            commands.append({"cwd": "<repo>", "argv": command})
            return subprocess.check_output(command, cwd=args.repo, timeout=30)

        revision = git("rev-parse", "--verify", args.revision + "^{commit}").decode().strip()
        report["revision"] = revision
        sources = {path: git("show", revision + ":" + path) for path in (EPHEMERIS_PATH, FILE_UTIL_PATH)}
        decoded = {path: data.decode("utf-8") for path, data in sources.items()}
        slices = select_slices(decoded[EPHEMERIS_PATH], decoded[FILE_UTIL_PATH])
        report["source_files"] = {path: {"sha256": sha256(data), "bytes": len(data)} for path, data in sources.items()}
        receipt = []
        for name, (path, start, end, value) in slices.items():
            data = value.encode("utf-8")
            (output / (name + ".inc")).write_bytes(data)
            receipt.append({"name": name, "path": path, "start_line": decoded[path].count("\n", 0, start) + 1,
                            "end_line": decoded[path].count("\n", 0, end) + 1,
                            "sha256": sha256(data), "bytes": len(data), "selection": "Exact contiguous source bytes; no source substitutions or filtering"})
        report["slices"] = receipt
        file_util = "\n\n".join(slices[name][3] for name in ("parse_path_name", "does_directory_exist", "does_file_exist"))
        generated = (HARNESS.replace("@@FILE_UTIL@@", file_util)
                    .replace("@@INITIALIZE@@", slices["initialize_deletion_region"][3])
                    .replace("@@SETTER@@", slices["filename_resolution_validation_region"][3]))
        (output / "native-probe.cpp").write_text(generated, encoding="utf-8")
        report["generated_harness_sha256"] = sha256(generated.encode("utf-8"))
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        if not compiler or not shutil.which(compiler[0]):
            raise RuntimeError("No C++ compiler available; set CXX")
        compiler_version = subprocess.check_output([*compiler, "--version"], stderr=subprocess.STDOUT, timeout=10).decode()
        report["compiler"] = compiler_version.splitlines()[0]
        (output / "compiler-version.txt").write_text(compiler_version)
        compile_command = [*compiler, "-std=c++17", "-O0", "-Wall", "-Wextra", "native-probe.cpp", "-o", "native-probe"]
        commands.append({"cwd": "<output>", "argv": compile_command})
        compiled = subprocess.run(compile_command, cwd=output, capture_output=True, text=True, timeout=60)
        (output / "compile.stdout").write_text(compiled.stdout)
        (output / "compile.stderr").write_text(compiled.stderr)
        report["compile_returncode"] = compiled.returncode
        if compiled.returncode:
            raise RuntimeError("Native compilation failed; see compile.stderr")
        report["native_binary_sha256"] = sha256((output / "native-probe").read_bytes())
        with tempfile.TemporaryDirectory(prefix="ephemeris-output-paths-") as fixtures:
            commands.append({"cwd": "<temporary-fixtures>", "argv": ["<output>/native-probe"]})
            executed = subprocess.run([str(output / "native-probe")], cwd=fixtures, capture_output=True, text=True, timeout=20)
        (output / "native.stdout").write_text(executed.stdout)
        (output / "native.stderr").write_text(executed.stderr)
        report["native_returncode"] = executed.returncode
        if executed.returncode not in (0, 1):
            raise RuntimeError("Native harness could not complete; see native.stderr")
        report["results"] = json.loads(executed.stdout)
        report["status"] = "passed" if executed.returncode == 0 else "assertion_failed"
        exit_code = executed.returncode
    except Exception as error:
        report["status"] = "infrastructure_error"
        # Do not copy arbitrary subprocess argv/private host paths into reports.
        report["error"] = type(error).__name__
        (output / "failure.txt").write_text(type(error).__name__ + ": " + str(error) + "\n")
        exit_code = 2
    (output / "source-receipt.json").write_text(json.dumps(report, indent=2) + "\n")
    summary = {"status": report["status"], "revision": report.get("revision"), "report": "source-receipt.json"}
    if "results" in report:
        summary["failed_tests"] = [test["name"] for test in report["results"]["tests"] if not test["passed"]]
    print(json.dumps(summary, indent=2))
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
