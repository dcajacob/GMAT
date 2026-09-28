#include "GuiTest.hpp"
#include "GroundTrackArea.hpp"
#include "Moderator.hpp"
#include "BodyFixedPoint.hpp"

class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      auto moderator=Moderator::Instance();
      auto area=new GroundTrackArea(frame,"StationRegression");
      Check(!area->TakeAction("AddStation=MissingStation"),"missing station is rejected without crashing");
      Check(!area->TakeAction("AddStation=DefaultSC"),"spacecraft is rejected as a station marker");
      auto station=dynamic_cast<BodyFixedPoint *>(moderator->CreateSpacePoint("GroundStation","MarkerStation"));
      Check(station!=nullptr,"public GroundStation factory creates the marker fixture");
      if(station)
      {
         station->SetSolarSystem(moderator->GetSolarSystemInUse());
         Check(station->Initialize(),"station fixture initializes");
         Check(area->TakeAction("AddStation=MarkerStation"),"valid station remains usable after rejected markers");
      }
      area->Destroy();
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
