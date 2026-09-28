#include "PythonInterface.hpp"
#include "CallPythonFunction.hpp"
#include "BaseException.hpp"
#include <cstdio>
#include <string>

class Converter : public CallPythonFunction
{
public:
   using CallPythonFunction::ConvertFromPyObject;
   using CallPythonFunction::PyIfVariant;
};
static int failures = 0;
static void Check(bool ok, const std::string &message)
{
   std::printf("%s: %s\n", ok ? "PASS" : "FAIL", message.c_str());
   if (!ok) ++failures;
}
static PyObject *Evaluate(const char *expression)
{
   PyObject *globals = PyModule_GetDict(PyImport_AddModule("__main__"));
   return PyRun_String(expression, Py_eval_input, globals, globals);
}
int main(int argc, char **argv)
{
   if (argc != 2) return 2;
   std::setvbuf(stdout, NULL, _IOLBF, 0);
   PythonInterface *python = PythonInterface::PyInstance();
   python->PyInitialize();
   const std::string mode(argv[1]);
   if (mode == "diagnostics")
   {
      PyRun_SimpleString(
         "import types, sys\n"
         "audit = types.ModuleType('gmat_python_audit')\n"
         "exec(\"class Unprintable(Exception):\\n def __str__(self): raise RuntimeError('formatting failed')\\n"
         "def ok(): return 7.0\\n"
         "def fail(): raise ValueError('bad caf\\u00e9: 100% complete')\\n"
         "def broken(): raise Unprintable()\\n\", audit.__dict__)\n"
         "sys.modules[audit.__name__] = audit\n");
      struct Case { const char *module; const char *function; const char *type; const char *detail; };
      const Case cases[] = {
         {"gmat_module_that_does_not_exist", "ok", "ModuleNotFoundError", "gmat_module_that_does_not_exist"},
         {"gmat_python_audit", "missing", "AttributeError", "missing"},
         {"gmat_python_audit", "fail", "ValueError", "bad caf\xc3\xa9: 100% complete"},
         {"gmat_python_audit", "broken", "Unprintable", "unavailable"}
      };
      for (const auto &test : cases)
      {
         bool caught = false;
         try { Py_XDECREF(python->PyFunctionWrapper(test.module, test.function, {})); }
         catch (BaseException &error)
         {
            caught = true;
            const std::string message = error.GetFullMessage();
            Check(message.find(test.type) != std::string::npos && message.find(test.detail) != std::string::npos,
                  std::string("exception type and detail: ") + test.function + " [" + message + "]");
         }
         Check(caught, "invalid Python call reports a GMAT exception");
         Check(!PyErr_Occurred(), "translated exception leaves no pending Python error");
         // Recover even on the baseline so all diagnostic assertions can run.
         PyErr_Clear();
         PyObject *value = python->PyFunctionWrapper("gmat_python_audit", "ok", {});
         Check(value && PyFloat_AsDouble(value) == 7.0 && !PyErr_Occurred(), "successful call after failure");
         Py_XDECREF(value);
      }
      std::string message;
      python->PyErrorMsg(PyExc_ValueError, NULL, NULL, message);
      Check(message.find("ValueError") != std::string::npos && !message.empty(), "missing exception value is safe");
      Check(!PyErr_Occurred(), "missing-value formatting leaves no Python error");
      PyErr_Clear();
   }
   else
   {
      Converter converter;
      struct Case { const char *mode; const char *expression; int rows; int columns; };
      const Case cases[] = {
         {"vector", "[1.0, 2.0, 3.0]", 1, 3},
         {"mixed", "[1, 2.5, 3]", 1, 3},
         {"matrix", "[[1, 2.5], [3.5, 4]]", 2, 2},
         {"scalar", "2.5", 0, 0},
         {"integer", "42", 0, 0},
         {"string", "'caf\\u00e9 100%'", 0, 0},
         {"empty", "[]", -1, 0},
         {"empty-row", "[[]]", -1, 0},
         {"ragged", "[[1.0, 2.0], [3.0]]", -1, 0},
         {"nonnumeric", "[1, 'bad']", -1, 0},
         {"overflow", "10**10000", -1, 0}
      };
      bool found = false;
      for (const auto &test : cases)
      {
         if (mode != test.mode) continue;
         found = true;
         // Consecutive conversions detect errors accidentally left by successful calls.
         for (int repeat = 0; repeat < 2; ++repeat)
         {
            PyObject *value = Evaluate(test.expression);
            Check(value != NULL, "fixture evaluation succeeded");
            if (!value) { PyErr_Clear(); break; }
            bool caught = false;
            try
            {
               auto converted = converter.ConvertFromPyObject(value);
               if (test.rows > 0)
               {
                  auto matrix = std::get<Rmatrix>(converted);
                  Check(matrix.GetNumRows() == test.rows && matrix.GetNumColumns() == test.columns,
                        "numeric result has expected shape");
                  const Real expected = mode == "vector" ? 2.0 : 2.5;
                  Check(matrix(0, 1) == expected, "numeric result preserves values");
               }
               else if (mode == "scalar" || mode == "integer")
                  Check(std::get<Real>(converted) == (mode == "scalar" ? 2.5 : 42), "scalar value preserved");
               else if (mode == "string")
                  Check(std::get<std::string>(converted) == "caf\xc3\xa9 100%", "Unicode string preserved");
            }
            catch (BaseException &error)
            {
               caught = true;
               Check(!error.GetFullMessage().empty(), "invalid result has an explanatory GMAT exception");
            }
            Check(caught == (test.rows == -1), "valid/invalid result classified correctly");
            Check(!PyErr_Occurred(), "conversion leaves no pending Python exception");
            PyErr_Clear();
            Py_DECREF(value);
         }
      }
      if (!found) return 2;
   }
   return failures ? 1 : 0;
}
