#include "GuiTest.hpp"
#include "GroundTrackWindow.hpp"
#include "OrbitViewCanvas.hpp"
#include "GmatMenuBar.hpp"
#include <wx/toolbar.h>
#include <limits>
template<class T> static T *Find(wxWindow *w)
{
   if(auto p=dynamic_cast<T *>(w))return p;
   for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;
   return nullptr;
}
struct MapAccess : GroundTrackArea
{
   static size_t Count(GroundTrackArea *m,size_t c)
   {return (m->*(&MapAccess::GetDisplayedPointCount))(c);}
};
struct WindowAccess : GroundTrackWindow
{
   static double Rate(GroundTrackWindow *w)
   {return double(w->*(&WindowAccess::animationIncrement))/(w->*(&WindowAccess::animationInterval));}
};
class Regression : public wxTimer
{
   int step=0,paints=0;
   size_t frames=0,points=0;
   GroundTrackWindow *window=nullptr;
   GroundTrackArea *map=nullptr;
   void Paint(wxPaintEvent &e){++paints;e.Skip();}
   void Command(int id)
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      if(id==GmatMenu::TOOL_ANIMATION_PLAY)frame->GetToolBar()->ToggleTool(id,true);
      wxCommandEvent e(wxEVT_MENU,id);frame->OnAnimation(e);
   }
   void CheckRestored()
   {
      Check(!window->IsAnimationRunning() && !GmatAppData::Instance()->GetMainFrame()->IsAnimationRunning(),
            "playback stops and toolbar state returns to idle");
      Check(map->GetFrameCount()==frames && MapAccess::Count(map,0)==points,
            "playback preserves recorded publications and restores all points");
   }
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      if(step==0)
      {
         auto probe=new GroundTrackArea(frame,"HistoryProbe");
         double nan=std::numeric_limits<double>::quiet_NaN();
         double first[]={nan,nan,10,20},second[]={30,40,11,21},third[]={35,45,nan,nan};
         probe->AddData(21545,first,2);probe->AddData(21546,second,2);probe->AddData(21544,third,2);
         probe->SetAnimationFrame(1);
         Check(MapAccess::Count(probe,0)==0 && MapAccess::Count(probe,1)==1,"first publication shows only its available spacecraft");
         probe->SetAnimationFrame(2);
         Check(MapAccess::Count(probe,0)==1 && MapAccess::Count(probe,1)==2,"second publication retains distinct spacecraft counts");
         probe->SetAnimationFrame(3);
         Check(MapAccess::Count(probe,0)==2 && MapAccess::Count(probe,1)==2,"playback follows publication order even when time goes backwards");
         probe->SetAnimationFrame(0);
         Check(probe->GetFrameCount()==3 && MapAccess::Count(probe,0)==2,"restoring the full view retains recorded data");
         probe->TakeAction("ClearData");
         Check(probe->GetFrameCount()==0 && MapAccess::Count(probe,0)==0,"ClearData resets playback history");
         probe->AddData(21545,second,2);probe->Clear();
         Check(probe->GetFrameCount()==0,"reinitialization resets playback history");
         probe->Destroy();
         auto dir=wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"));
         Check(frame->BuildScript(dir+"/animation.script"),"build GroundTrack animation fixture");
         Check(frame->RunCurrentMission()==1,"GroundTrack animation mission completes");
         window=Find<GroundTrackWindow>(frame);Check(window!=nullptr,"GroundTrack output exists");
         if(!window){frame->SetAutoExitAfterRun(true);frame->Close();return;}
         window->Activate();map=Find<GroundTrackArea>(window);
         frames=map->GetFrameCount();points=MapAccess::Count(map,0);
         Check(frames>50 && points>50,"mission supplies a replayable recording");
         map->Bind(wxEVT_PAINT,&Regression::Paint,this);
      }
      else if(step==1)
      {
         paints=0;
         Check(frame->GetToolBar()->GetToolEnabled(GmatMenu::TOOL_ANIMATION_PLAY),"GroundTrack Play is enabled");
         Command(GmatMenu::TOOL_ANIMATION_SLOW);Command(GmatMenu::TOOL_ANIMATION_PLAY);
         Check(window->IsAnimationRunning(),"Play starts asynchronous GroundTrack playback");
         double slow=WindowAccess::Rate(window);Command(GmatMenu::TOOL_ANIMATION_FAST);
         Check(WindowAccess::Rate(window)>slow,"Faster increases playback rate");
         double fast=WindowAccess::Rate(window);Command(GmatMenu::TOOL_ANIMATION_SLOW);
         Check(WindowAccess::Rate(window)<fast,"Slower decreases playback rate");
         window->SetAnimationSpeed(20,1);
      }
      else if(step==2)
      {
         Check(window->IsAnimationRunning() && paints>=2,"GUI remains responsive while multiple playback frames paint");
         Check(MapAccess::Count(map,0)<points,"playback displays a prefix instead of the complete track");
         Command(GmatMenu::TOOL_ANIMATION_STOP);CheckRestored();
         paints=0;Command(GmatMenu::TOOL_ANIMATION_PLAY);window->SetAnimationSpeed(20,1);
      }
      else if(step==3)
      {
         Check(paints>=2,"GroundTrack replays again after Stop");
         auto other=frame->GetChild("BuildTestView");
         Check(other!=nullptr,"another plot tab is available");
         if(other)other->Activate();
         Check(frame->IsAnimationRunning(),"playback is still reported after switching tabs");
         double before=WindowAccess::Rate(window);Command(GmatMenu::TOOL_ANIMATION_FAST);
         Check(WindowAccess::Rate(window)>before,"speed control reaches the playing plot after switching tabs");
         Command(GmatMenu::TOOL_ANIMATION_STOP);CheckRestored();
         window->Activate();Command(GmatMenu::TOOL_ANIMATION_PLAY);window->SetAnimationSpeed(1,10000);
      }
      else if(step==4)
      {
         CheckRestored();
         map->Unbind(wxEVT_PAINT,&Regression::Paint,this);
         Command(GmatMenu::TOOL_ANIMATION_PLAY);window->SetAnimationSpeed(20,1);
         Check(window->IsAnimationRunning(),"playback restarts before the close test");
         frame->StopAnimation();CheckRestored();
         Command(GmatMenu::TOOL_ANIMATION_PLAY);window->SetAnimationSpeed(20,1);
         window->Close();window=nullptr;map=nullptr;
      }
      else
      {
         Check(!Find<GroundTrackWindow>(frame) && !frame->IsAnimationRunning(),"closing a playing plot stops its timer and clears toolbar state");
         Check(frame->RunCurrentMission()==1,"mission runs after closing a playing plot");
         window=Find<GroundTrackWindow>(frame);map=Find<GroundTrackArea>(window);
         Check(map->GetFrameCount()==frames,"recreated plot has fresh playback history");
         window->Activate();map->TakeAction("ClearData");Command(GmatMenu::TOOL_ANIMATION_PLAY);
         Check(!window->IsAnimationRunning() && !frame->IsAnimationRunning(),"empty plots do not leave Play stuck on");
         frame->SetAutoExitAfterRun(true);frame->Close();return;
      }
      ++step;StartOnce(200);
   }
};
extern "C" int __wrap_main(int argc,char **argv){return RunGuiTest<Regression>(argc,argv);}
