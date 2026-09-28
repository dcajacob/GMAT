#include "GuiTest.hpp"
#include <wx/modalhook.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

class PythonDialogs : public wxModalDialogHook
{
public:
   wxString message;
   int Enter(wxDialog *dialog) override
   {
      if (auto box = dynamic_cast<wxMessageDialog *>(dialog))
      {
         message = box->GetMessage();
         std::printf("PYTHON_DIALOG: %s\n", message.utf8_str().data());
         if (box->GetMessageDialogStyle() & wxYES_NO) return wxID_NO;
      }
      return wxID_OK;
   }
};
class PythonGui : public wxTimer
{
   int step = 0;
   PythonDialogs dialogs;
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      if (step == 0) dialogs.Register();
      const char *names[] = {"missing-module", "valid", "missing-function", "valid",
                            "error", "valid", "unprintable", "valid", "empty", "valid",
                            "ragged", "valid", "nonnumeric", "valid", "multiple", "multiple"};
      const char *details[] = {"ModuleNotFoundError", "", "AttributeError", "",
                              "bad caf\xc3\xa9: 100% complete", "", "unavailable", "",
                              "empty", "", "same number", "", "floats or integers", "", "", ""};
      if (step == 16)
      {
         frame->SetAutoExitAfterRun(true);
         frame->Close();
         return;
      }
      const wxString directory = wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"));
      dialogs.message.clear();
      Check(frame->BuildScript(directory+"/"+names[step]+".script"), "Python GUI fixture builds");
      int result = frame->RunCurrentMission();
      bool valid = std::string(names[step]) == "valid" || std::string(names[step]) == "multiple";
      Check(valid ? result == 1 : result != 1, "Python GUI run has expected outcome");
      if (!valid)
         Check(dialogs.message.Contains(wxString::FromUTF8(details[step])), "Python error dialog includes the real cause");
      else
         Check(dialogs.message.empty(), "successful recovery has no error dialog");
      ++step;
      StartOnce(100);
   }
};
extern "C" int __wrap_main(int argc, char **argv)
{
   wxLog::SetActiveTarget(new wxLogStderr());
   return RunGuiTest<PythonGui>(argc, argv);
}
