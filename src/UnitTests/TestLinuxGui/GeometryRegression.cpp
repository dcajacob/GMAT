#include "GuiTest.hpp"

class Regression : public wxTimer
{
   int step = 0;
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      const std::string mode = std::getenv("GMAT_GUI_TEST_MODE");
      Check(wxDisplay(0u).GetClientArea().Contains(frame->GetRect()),
            "window fits the available display");
      Check(!frame->IsIconized(), "application does not reopen minimized");
      if (mode == "save" && step++ == 0)
      {
         frame->SetSize(900,640);
         frame->Move(80,60);
         StartOnce(300);
         return;
      }
      if (mode == "restore")
      {
         Check(frame->GetSize() == wxSize(900,640), "saved Linux window size restored");
         Check(frame->GetPosition() == wxPoint(80,60), "saved X11 position restored");
      }
      frame->SetAutoExitAfterRun(true);
      frame->Close();
   }
};

extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
