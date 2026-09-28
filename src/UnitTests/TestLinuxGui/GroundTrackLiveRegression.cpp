#include "GuiTest.hpp"
#include "GroundTrackArea.hpp"
#include "Publisher.hpp"
#include <wx/stopwatch.h>

template<class T> static T *Find(wxWindow *w)
{
   if(auto found=dynamic_cast<T *>(w))return found;
   for(auto child:w->GetChildren())if(auto found=Find<T>(child))return found;
   return nullptr;
}
class PaintWatch : public wxTimer
{
public:
   GroundTrackArea *area=nullptr;
   int runningPaints=0;
   void Notify() override
   {
      if(area)return;
      area=Find<GroundTrackArea>(GmatAppData::Instance()->GetMainFrame());
      if(area)area->Bind(wxEVT_PAINT,[this](wxPaintEvent &event){
         if(Publisher::Instance()->GetRunState()==Gmat::RUNNING)++runningPaints;
         event.Skip();
      });
   }
};
class Regression : public wxTimer
{
   PaintWatch watch;
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      wxString script=wxString::FromUTF8(std::getenv("GMAT_LIVE_SCRIPT"));
      Check(frame->BuildScript(script),"live-update mission builds");
      watch.Start(20);wxStopWatch elapsed;
      int result=frame->RunCurrentMission();watch.Stop();
      std::printf("LIVE_PAINTS: count=%d duration_ms=%ld\n",watch.runningPaints,elapsed.Time());
      Check(result==1,"long GroundTrack mission completes");
      Check(watch.area && watch.runningPaints>=3,"GroundTrack repaints repeatedly while the mission is running");
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
