#include "GuiTest.hpp"
#include "GroundTrack.hpp"
#include "Moderator.hpp"
#include "PlotInterface.hpp"
#include "SubscriberException.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include <vector>
#include <cmath>

// Interpose only the data-delivery boundary in this test executable. The real
// subscriber still performs initialization, label lookup and coordinate conversion.
static std::vector<double> epochs;
bool PlotInterface::UpdateGroundTrackData(const std::string &, const double epoch,
      const double *positions, const int count)
{
   Check(count == 1 && std::isfinite(positions[0]) && std::isfinite(positions[1]),
         "collected position contains finite longitude and latitude");
   epochs.push_back(epoch);
   return true;
}
class Probe : public GroundTrack
{
public:
   Probe() : GroundTrack("SamplingTest") {}
   size_t BodyCount() const { return theBodies.size(); }
};
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto moderator = Moderator::Instance();
      auto frame = GmatAppData::Instance()->GetMainFrame();
      Probe track;
      track.SetSolarSystem(moderator->GetSolarSystemInUse());
      track.SetStringParameter("Add", "DefaultSC");
      track.SetRefObject(moderator->GetConfiguredObject("DefaultSC"), Gmat::SPACECRAFT, "DefaultSC");
      Check(track.Initialize(), "sampling subscriber initializes");
      track.SetDataLabels({"Epoch", "DefaultSC.X", "DefaultSC.Y", "DefaultSC.Z", "DefaultSC.VX", "DefaultSC.VY", "DefaultSC.VZ"});
      double values[] = {21545,7100,0,1300,0,7.35,1};
      track.SetIntegerParameter("DataCollectFrequency", 1);
      epochs.clear(); track.Distribute(values,7);
      Check(epochs.size() == 1, "frequency one delivers each point through the observation boundary");
      track.SetIntegerParameter("DataCollectFrequency", 5);
      track.Initialize(); epochs.clear();
      for (int i=0;i<12;++i) { values[0]=21545+i/86400.0; track.Distribute(values,7); }
      Check(epochs.size() == 3, "frequency five collects three of twelve points");
      Check(!epochs.empty() && epochs.front() == 21545, "first point survives decimation");
      track.Initialize(); epochs.clear(); track.Distribute(values,7);
      Check(epochs.size() == 1, "reinitialization resets sampling");
      for (const char *name : {"DataCollectFrequency", "UpdatePlotFrequency"})
         for (int invalid : {0,-1})
         {
            bool rejected=false;
            try { track.SetIntegerParameter(name,invalid); }
            catch (const SubscriberException &) { rejected=true; }
            Check(rejected,"nonpositive sampling/update frequency is rejected");
         }
      track.SetIntegerParameter("NumPointsToRedraw",10);
      Check(track.SetIntegerParameter("NumPointsToRedraw",0)==0,"redraw count can return to zero");
      auto body = moderator->GetSolarSystemInUse()->GetBody("Luna");
      track.SetRefObject(body,Gmat::CELESTIAL_BODY,"Luna");
      track.SetRefObject(body,Gmat::CELESTIAL_BODY,"Luna");
      Check(track.BodyCount()==1,"rebinding a celestial body preserves a single reference");
      track.SetBooleanParameter("ShowPlot",false); epochs.clear();track.Distribute(values,7);
      Check(epochs.empty(),"hidden subscriber sends no plot data");
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
