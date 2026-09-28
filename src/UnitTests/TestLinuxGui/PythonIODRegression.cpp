#include "GuiTest.hpp"
#include <wx/textctrl.h>
#include <wx/log.h>
#include <wx/modalhook.h>
#include <wx/msgdlg.h>

class IODDialogs : public wxModalDialogHook
{
   int Enter(wxDialog *dialog) override
   {
      if (auto box = dynamic_cast<wxMessageDialog *>(dialog))
         Check(false, box->GetMessage().utf8_str().data());
      return wxID_OK;
   }
};
class IOD : public wxTimer
{
   IODDialogs dialogs;
public:
   void Notify() override
   {
      auto data = GmatAppData::Instance();
      auto frame = data->GetMainFrame();
      dialogs.Register();
      Check(frame->BuildScript(wxString::FromUTF8(std::getenv("GMAT_IOD_SCRIPT"))), "unmodified IOD builds");
      data->GetMessageTextCtrl()->Clear();
      Check(frame->RunCurrentMission() == 1, "both IOD calculations complete");
      std::printf("IOD_MESSAGES_BEGIN\n%s\nIOD_MESSAGES_END\n",
                  data->GetMessageTextCtrl()->GetValue().utf8_str().data());
      frame->SetAutoExitAfterRun(true);
      frame->Close();
   }
};
extern "C" int __wrap_main(int argc, char **argv)
{
   wxLog::SetActiveTarget(new wxLogStderr());
   return RunGuiTest<IOD>(argc, argv);
}
