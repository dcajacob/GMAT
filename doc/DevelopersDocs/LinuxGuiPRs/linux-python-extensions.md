# Make the linked Python runtime available to Linux extension modules

NumPy imported successfully in standalone Python but failed from GMAT's locally loaded PythonInterface plugin with an undefined Python symbol. During PythonInterface initialization, locate the library that already supplies `Py_Initialize` and promote it with `RTLD_NOLOAD | RTLD_GLOBAL | RTLD_NOW`, retaining one handle for the interpreter lifetime. Report loader failures as interface errors and link the Linux dynamic-loader dependency.

Reproduction: enable PythonInterface for a Python installation containing NumPy and run a function that imports NumPy and returns `float(numpy.sum([1, 2, 3]))`. It must work without `LD_PRELOAD`, including a second call. The generic GMAT plugin loader and startup settings remain unchanged; the implementation is Linux-guarded and has no hard-coded Python paths.

Validation: a separately built plugin containing only this patch passes two real GUI calls importing NumPy 2.3.5 under Python 3.14, without preloading or directly linking the GUI test executable to Python. On the integration branch, the unchanged `Ex_IOD.script` completes and matches direct Python numerically. That example also requires the separate return-conversion fix; it is not an isolated test of this patch. Windows/macOS and other Python builds have not been tested.

The promotion uses the documented Linux [dlopen mechanism](https://man7.org/linux/man-pages/man3/dlopen.3.html). See `LinuxGuiPublicFixes.md` for exact prerequisites and independent validation evidence.

Production-only branch: `pr/linux-python-extensions`. No PR, issue or external comment has been submitted.
