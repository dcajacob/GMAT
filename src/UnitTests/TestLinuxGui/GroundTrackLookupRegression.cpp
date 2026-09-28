#include "GuiTest.hpp"
#include "GuiPlotReceiver.hpp"
#include "MdiTsPlotData.hpp"
#include "GroundTrackWindow.hpp"

static GroundTrackWindow *FindTrack(const std::string &name)
{
   for (auto node = MdiTsPlot::mdiChildren.GetFirst(); node; node=node->GetNext())
   {
      auto child = static_cast<GmatMdiChildFrame *>(node->GetData());
      if (auto track = dynamic_cast<GroundTrackWindow *>(child))
         if (track->GetPlotName() == name) return track;
   }
   return nullptr;
}
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      auto receiver=GuiPlotReceiver::Instance();
      Check(receiver->CreateXyPlotWindow("UnrelatedXY","",0,0,0.5,0.5,false,"XY","X","Y"),"XY window created");
      Check(receiver->CreateGroundTrackWindow("Track","UnrelatedXY","Track",0,0,0.5,0.5,false),"track window creation succeeds with an XY old-name collision");
      auto original=FindTrack("Track");
      Check(original!=nullptr,"old XY name is not mistaken for a GroundTrack window");
      Check(!receiver->TakeGroundTrackAction("UnrelatedXY","ClearData"),"ground-track action ignores XY windows");
      double point[]={0,0};
      Check(!receiver->UpdateGroundTrackData("UnrelatedXY",21545,point,1),"ground-track data ignores XY windows");
      receiver->SetGroundTrackOption("UnrelatedXY","TextureMap","");
      Check(receiver->CreateGroundTrackWindow("RenamedTrack","Track","Track",0,0,0.5,0.5,false),"track rename succeeds");
      Check(original && FindTrack("RenamedTrack")==original && !FindTrack("Track"),"renaming reuses the correct GroundTrack window");
      Check(receiver->UpdateGroundTrackData("RenamedTrack",21545,point,1),"renamed track receives data");
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
