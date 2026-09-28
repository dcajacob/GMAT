# Preserve Python exception details and allow subsequent commands

When a Python module/function is missing or a function raises, GMAT's dialog omitted the cause and the translated exception remained pending in Python. Return the exception type and UTF-8 message, release fetched and temporary references, and leave the interpreter error state clear. If exception formatting itself fails, retain the original type with fallback text.

Reproduction: call a nonexistent module/function, then a function that raises `ValueError("bad café: 100% complete")`, then a successful function in the same GUI session. The error must identify its cause and the following command must succeed. Also test an exception whose `__str__` raises and an absent exception value.

Validation: direct exception/error-state tests pass on this patch alone with Python 3.14. Integration GUI tests verify the actual dialog, log and recovery. The Linux validation checkout includes only the five existing prerequisites listed in `LinuxGuiPublicFixes.md`; no sibling Python fix is required for these diagnostic tests. Windows/macOS and other Python versions have not been tested. Public method signatures are unchanged.

The change follows Python's [exception ownership and error-state rules](https://docs.python.org/3/c-api/exceptions.html). Supplemental reproductions: `PythonRegression.cpp`, `test_python.py --mode diagnostics` and `test_python_gui.py` on the integration branch.

Production-only branch: `pr/python-diagnostics`. No PR, issue or external comment has been submitted.
