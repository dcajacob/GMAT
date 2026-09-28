#include "GuiTest.hpp"
#include "GroundTrack.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SubscriberException.hpp"
#include <memory>

class Probe : public GroundTrack
{
public:
   explicit Probe(const std::string &name) : GroundTrack(name) {}
   bool HasResources() const { return longlat && viewCoordinates && inertialSystem; }
   bool IndependentOf(const Probe &other) const
   { return longlat != other.longlat && viewCoordinates != other.viewCoordinates && inertialSystem != other.inertialSystem; }
   size_t SatelliteCount() const { return theSats.size(); }
};
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      auto moderator = Moderator::Instance();
      auto sc = moderator->GetConfiguredObject("DefaultSC");
      Check(sc != nullptr, "default spacecraft exists");
      if (sc)
      {
         const size_t windowsBefore = frame->GetListOfMdiChildren()->GetCount();
         Probe source("OwnershipSource");
         source.SetSolarSystem(moderator->GetSolarSystemInUse());
         source.SetBooleanParameter("ShowPlot", false);
         source.SetStringParameter("Add", "DefaultSC");
         source.SetRefObject(sc, Gmat::SPACECRAFT, "DefaultSC");
         Check(source.Initialize() && source.HasResources(), "source initializes runtime coordinates and buffer");
         {
            Probe copy(source);
            Check(!copy.HasResources(), "copy does not share owned runtime resources");
            Check(copy.Initialize() && copy.IndependentOf(source), "copy initializes independent runtime resources");
            Probe assigned("Assigned");
            assigned = source;
            Check(assigned.Initialize() && assigned.IndependentOf(source), "assignment initializes independent runtime resources");
            assigned = assigned;
            Check(assigned.HasResources(), "self assignment preserves live resources");
            copy.SetRefObject(sc, Gmat::SPACECRAFT, "DefaultSC");
            Check(copy.SatelliteCount() == 1, "rebinding a copied spacecraft reference does not duplicate it");
         }
         Check(source.Initialize() && source.HasResources(), "source survives copy destruction and repeated initialization");
         source.SetRefObject(sc, Gmat::SPACECRAFT);
         Check(source.SatelliteCount() == 1, "default-name rebinding does not duplicate a spacecraft");
         Check(frame->GetListOfMdiChildren()->GetCount() == windowsBefore,
               "ShowPlot=false creates no plot windows");
      }
      frame->SetAutoExitAfterRun(true);
      frame->Close();
   }
};
extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
