#include "GuiTest.hpp"
#include "GuiInterpreter.hpp"
#include <wx/log.h>
#include <wx/modalhook.h>
#include <wx/msgdlg.h>

class AliasDialogs : public wxModalDialogHook
{
   int Enter(wxDialog *dialog) override
   {
      if (auto box = dynamic_cast<wxMessageDialog *>(dialog))
      {
         std::printf("ALIAS_DIALOG: %s\n", box->GetMessage().utf8_str().data());
         if (box->GetMessageDialogStyle() & wxYES_NO) return wxID_NO;
      }
      return wxID_OK;
   }
};
class Alias : public wxTimer
{
   AliasDialogs dialogs;
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      auto interpreter = GmatAppData::Instance()->GetGuiInterpreter();
      const wxString directory = wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"));
      dialogs.Register();
      try
      {
         for (const std::string value : {"On", "Off"})
         {
            Check(frame->BuildScript(directory+"/alias.script"), "alias fixture builds");
            auto plot = interpreter->GetConfiguredObject("BuildTestView");
            Check(plot->SetOnOffParameter("CelestialPlane", value), "deprecated alias accepts value");
            Check(plot->GetOnOffParameter("CelestialPlane") == value &&
                  plot->GetOnOffParameter("EclipticPlane") == value, "alias and canonical field agree");
            const wxString saved = directory+"/saved-"+value+".script";
            Check(interpreter->SaveScript(saved.ToStdString()), "save plot configuration");
            Check(frame->BuildScript(saved), "reload saved configuration");
            plot = interpreter->GetConfiguredObject("BuildTestView");
            Check(plot->GetOnOffParameter("EclipticPlane") == value, "saved canonical plane state round-trips");
         }
      }
      catch (BaseException &error)
      {
         Check(false, error.GetFullMessage().c_str());
      }
      frame->SetAutoExitAfterRun(true);
      frame->Close();
   }
};
extern "C" int __wrap_main(int argc, char **argv)
{
   wxLog::SetActiveTarget(new wxLogStderr());
   return RunGuiTest<Alias>(argc, argv);
}
