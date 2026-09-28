#include "GuiTest.hpp"
#include "ResourceTree.hpp"
#include "MissionTree.hpp"
#include "OutputTree.hpp"
#include <wx/settings.h>
#include <cmath>
#include <algorithm>
static double Luminance(const wxColour &c)
{
   auto linear=[](unsigned char v){double s=v/255.0;return s<=0.04045?s/12.92:std::pow((s+0.055)/1.055,2.4);};
   return 0.2126*linear(c.Red())+0.7152*linear(c.Green())+0.0722*linear(c.Blue());
}
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto app=GmatAppData::Instance();auto frame=app->GetMainFrame();
      wxTreeCtrl *trees[]={app->GetResourceTree(),app->GetMissionTree(),app->GetOutputTree()};
      for(int mode:{1,2,1})
      {
         frame->UpdateAdvancedGuiMode(mode);
         for(auto tree:trees)
         {
            auto fg=tree->GetForegroundColour(),bg=tree->GetBackgroundColour();
            double a=Luminance(fg),b=Luminance(bg),contrast=(std::max(a,b)+0.05)/(std::min(a,b)+0.05);
            std::printf("TREE_CONTRAST: mode=%d contrast=%.2f foreground=%s background=%s\n",mode,contrast,fg.GetAsString().utf8_str().data(),bg.GetAsString().utf8_str().data());
            Check(contrast>=4.5,"tree text remains readable in normal and non-savable modes");
            Check(fg==wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),"tree uses the system text color");
            if(mode==1)Check(bg==wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW),"normal mode restores the system background");
            else Check(bg!=wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW),"non-savable mode retains a distinct tint");
         }
      }
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
