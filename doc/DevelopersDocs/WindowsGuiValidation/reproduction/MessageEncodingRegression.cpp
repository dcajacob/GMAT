#include "GuiTest.hpp"
#include "GuiMessageReceiver.hpp"
#include <wx/modalhook.h>
#include <wx/msgdlg.h>
#include <wx/textctrl.h>

class EncodingDialogs : public wxModalDialogHook
{
public:
   wxString message;
   int Enter(wxDialog *dialog) override
   {
      if (auto box = dynamic_cast<wxMessageDialog *>(dialog))
         message = box->GetMessage();
      return wxID_OK;
   }
};

class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto app = GmatAppData::Instance();
      auto frame = app->GetMainFrame();
      auto pane = app->GetMessageTextCtrl();
      auto receiver = GuiMessageReceiver::Instance();
      EncodingDialogs dialogs;
      dialogs.Register();
      const std::string messages[] = {
         "ASCII 100%", u8"café 100% 日本語", "", std::string("legacy caf\xe9")
      };
      for (int i = 0; i < 4; ++i)
      {
#ifndef __WXMSW__
         // ANSI fallback is a Windows-specific compatibility check.
         if (i == 3) continue;
#endif
         const auto &message = messages[i];
         const wxString expected = i == 3 ? wxString(message.c_str()) :
               wxString::FromUTF8(message.c_str());
         pane->Clear();
         receiver->ShowMessage(message);
         Check(pane->GetValue() == expected,
               "string message preserves decoded text");
         pane->Clear();
         receiver->ShowMessage("%s", message.c_str());
         Check(pane->GetValue() == expected,
               "formatted message preserves decoded text");
         for (auto type : {Gmat::ERROR_, Gmat::WARNING_, Gmat::INFO_})
         {
            dialogs.message.clear();
            pane->Clear();
            receiver->PopupMessage(type, message);
            Check(dialogs.message == expected, "dialog preserves decoded text");
            Check(pane->GetValue() == expected,
                  "dialog log pane preserves decoded text");
         }
      }
      dialogs.Unregister();
      frame->SetAutoExitAfterRun(true);
      frame->Close();
   }
};

extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc, argv); }
