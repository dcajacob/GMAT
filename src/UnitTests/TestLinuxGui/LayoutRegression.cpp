#include "GuiTest.hpp"

static void CheckPanes(GmatMainFrame *frame)
{
   const wxRect bounds(wxPoint(0,0), frame->GetClientSize());
   const wxRect mdi = frame->GetClientWindow()->GetRect();
   Check(mdi.width >= frame->FromDIP(160) && mdi.height >= frame->FromDIP(120),
         "editor/plot workspace remains usable after resizing");
   Check(bounds.Contains(mdi), "editor/plot workspace stays inside the client area");
   for (wxWindow *child : frame->GetChildren())
      if (auto sash = dynamic_cast<wxSashLayoutWindow *>(child))
         Check(bounds.Contains(sash->GetRect()), "sash pane stays inside the client area");
}

class Regression : public wxTimer
{
   int step = 0;
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      const char *mode = std::getenv("GMAT_GUI_TEST_MODE");
      if (step == 0)
      {
         CheckPanes(frame);
         if (mode && std::string(mode) == "bounds")
         {
            frame->SetAutoExitAfterRun(true);
            frame->Close();
            return;
         }
         GmatTreeItemData spacecraft("DefaultSC", GmatTree::SPACECRAFT);
         Check(frame->CreateChild(&spacecraft, false) != nullptr,
               "spacecraft properties panel opens before resizing");
         frame->SetSize(1100,800);
      }
      else if (step == 1)
      {
         for (wxWindow *child : frame->GetChildren())
            if (auto sash = dynamic_cast<wxSashLayoutWindow *>(child))
            {
               wxSashEvent drag(sash->GetId(), wxSASH_RIGHT);
               drag.SetDragRect(wxRect(0,0,700,500));
               if (sash->GetAlignment() == wxLAYOUT_LEFT) frame->OnSashDrag(drag);
               else frame->OnMsgSashDrag(drag);
            }
         frame->SetSize(640,480);
      }
      else if (step == 2)
      {
         CheckPanes(frame);
         Screenshot(frame,"small-window");
         // Also test invalid sash sizes independently of the initial config.
         for (wxWindow *child : frame->GetChildren())
            if (auto sash = dynamic_cast<wxSashLayoutWindow *>(child))
               sash->SetDefaultSize(wxSize(-500,-500));
         frame->SendSizeEvent();
      }
      else if (step == 3)
      {
         CheckPanes(frame);
         frame->SetSize(900,640);
         frame->Move(80,60);
      }
      else if (step == 4)
      {
         CheckPanes(frame);
         frame->SetAutoExitAfterRun(true);
         frame->Close();
         return;
      }
      ++step;
      StartOnce(300);
   }
};


extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
