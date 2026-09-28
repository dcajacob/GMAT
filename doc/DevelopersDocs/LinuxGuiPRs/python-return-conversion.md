# Validate Python numeric return arrays before inspecting their elements

Converting a vector called `PyList_Size` on its first scalar element, leaving a pending Python error that could break the next call. Check each type before using list APIs, accept mixed integers/floats in vectors and rectangular matrices, and reject empty, ragged or nonnumeric arrays with a recoverable GMAT error. Release the owned return object when conversion fails and avoid converting a single result twice.

Reproduction: call a Python function returning `[1.0, 2.0, 3.0]` twice consecutively. Both vectors must arrive intact with no pending Python exception. Repeat with mixed numeric vectors, matrices, scalar/string outputs, then malformed arrays and a successful command.

Validation: all eleven conversion cases pass with this patch alone, including numeric overflow. Integration GUI tests cover repeated multiple outputs and recovery after malformed arrays. The unchanged `Ex_IOD.script` also completes both calculations with the separate Linux Python-symbol patch; its two velocities agree with direct Python within 8.9e-16 km/s. NumPy import support is an independent prerequisite for that end-to-end example, not part of this patch. Windows/macOS have not been tested.

Supplemental reproductions: `PythonRegression.cpp`, `test_python.py`, `test_python_gui.py` and `test_python_iod.py` on the integration branch. See `LinuxGuiPublicFixes.md` for Linux runtime prerequisites.

Production-only branch: `pr/python-return-conversion`. No PR, issue or external comment has been submitted.
