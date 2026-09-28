#include "GuiTest.hpp"
#include "GroundTrackWindow.hpp"
static GroundTrackWindow *Find(wxWindow *w)
{
   if(auto p=dynamic_cast<GroundTrackWindow *>(w))return p;
   for(auto c:w->GetChildren())if(auto p=Find(c))return p;
   return nullptr;
}
class Regression : public wxTimer
{
   int step=0;
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      if(step==0)
      {
         Check(frame->RunCurrentMission()==1,"default mission completes");
         auto plot=Find(frame);Check(plot!=nullptr,"GroundTrack window exists");
         if(plot){plot->Activate();plot->Close();}
         ++step;StartOnce(200);
      }
      else
      {
         Check(Find(frame)==nullptr,"direct close removes GroundTrack window safely");
         Check(frame->RunCurrentMission()==1 && Find(frame)!=nullptr,"mission recreates GroundTrack after direct close");
         frame->SetAutoExitAfterRun(true);frame->Close();
      }
   }
};
extern "C" int __wrap_main(int argc,char **argv){return RunGuiTest<Regression>(argc,argv);}
