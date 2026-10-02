#include "ResourceDraft.hpp"
#include "FactoryManager.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "SolarSystem.hpp"
#include "Array.hpp"
#include "CelestialBody.hpp"
#include <stdexcept>

std::unique_ptr<GmatBase> resourceDraft(const QString &type,const QString &name)
{
   auto *factory=FactoryManager::Instance();
   std::unique_ptr<GmatBase> draft(factory->CreateObject(factory->GetBaseTypeOf(type.toStdString()),type.toStdString(),name.toStdString()));
   if (!draft) throw std::runtime_error("The runtime cannot prepare this resource type.");
   auto *moderator=Moderator::Instance();
   if (auto *body=dynamic_cast<CelestialBody *>(draft.get())) {
      body->SetUserDefined(true); body->SetSolarSystem(moderator->GetSolarSystemInUse()); body->SetUpBody();
   }
   if (auto *spacecraft=dynamic_cast<Spacecraft *>(draft.get())) {
      spacecraft->SetSolarSystem(moderator->GetSolarSystemInUse());
      auto *frame=dynamic_cast<CoordinateSystem *>(moderator->GetConfiguredObject("EarthMJ2000Eq"));
      if (frame) { spacecraft->SetInternalCoordSystem(frame); spacecraft->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,"EarthMJ2000Eq"); }
   }
   if (auto *array=dynamic_cast<Array *>(draft.get())) array->SetSize(1,1);
   const auto spacecraft=moderator->GetListOfObjects(Gmat::SPACECRAFT);
   if (type=="EphemerisFile" && !spacecraft.empty()) draft->SetStringParameter("Spacecraft",spacecraft.front());
   if ((type=="OrbitView" || type=="GroundTrack" || type=="GroundTrackPlot") && !spacecraft.empty()) {
      draft->SetStringParameter("Add",spacecraft.front()); draft->SetStringParameter("Add","Earth");
   }
   return draft;
}
