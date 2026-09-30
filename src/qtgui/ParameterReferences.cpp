#include "ParameterReferences.hpp"
#include "Moderator.hpp"
#include "ParameterInfo.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"

ParameterDependency parameterDependency(const QString &owner,const QString &property)
{
   ParameterDependency result;
   auto *info=ParameterInfo::Instance(); auto *moderator=Moderator::Instance();
   const auto type=property.toStdString();
   const auto dep=info->GetDepObjectType(type);
   UnsignedInt objectType=Gmat::UNKNOWN_OBJECT;
   if (dep==GmatParam::COORD_SYS) { objectType=Gmat::COORDINATE_SYSTEM; result.label="Coordinate system"; }
   else if (dep==GmatParam::ORIGIN) { objectType=Gmat::CELESTIAL_BODY; result.label="Central body"; }
   else if (dep==GmatParam::ODE_MODEL) { objectType=Gmat::ODE_MODEL; result.label="Force model"; }
   const bool attached=dep==GmatParam::ATTACHED_OBJ;
   QStringList attachedNames;
   if (attached) {
      objectType=info->GetOwnedObjectType(type); result.label="Attached hardware";
      if (auto *object=moderator->GetConfiguredObject(owner.toStdString())) {
         // Only direct references belong to this owner. A thruster's tank does
         // not make the tank attached to an unrelated spacecraft.
         for (int id=0;id<object->GetParameterCount();++id) {
            try {
               const auto kind=object->GetParameterType(id);
               if (kind==Gmat::OBJECT_TYPE) attachedNames.append(QString::fromStdString(object->GetStringParameter(id)));
               else if (kind==Gmat::OBJECTARRAY_TYPE)
                  for (const auto &name:object->GetStringArrayParameter(id)) attachedNames.append(QString::fromStdString(name));
            } catch (BaseException &) {} // Plugin properties may lack a string getter.
         }
      }
   }
   result.needed=attached || objectType!=Gmat::UNKNOWN_OBJECT;
   if (result.needed && objectType!=Gmat::UNKNOWN_OBJECT) for (const auto &name:moderator->GetListOfObjects(objectType)) {
      auto *object=moderator->GetConfiguredObject(name);
      if (!object || (attached && !attachedNames.contains(QString::fromStdString(name)))) continue;
      if (dep==GmatParam::COORD_SYS) {
         try {
            if (info->RequiresBodyFixedCS(type) && object->GetStringParameter("Axes")!="BodyFixed") continue;
            if (info->RequiresCelestialBodyCSOrigin(type)) {
               auto *origin=moderator->GetConfiguredObject(object->GetStringParameter("Origin"));
               if (!origin || !origin->IsOfType(Gmat::CELESTIAL_BODY)) continue;
            }
         } catch (BaseException &) { continue; }
      }
      result.choices.append(QString::fromStdString(name));
   }
   result.choices.removeDuplicates(); result.choices.sort();
   result.preferred=dep==GmatParam::COORD_SYS ? "EarthMJ2000Eq" : "Earth";
   return result;
}

QString parameterReference(const QString &owner,const QString &property,const QString &dependency)
{
   QStringList parts{owner};
   if (!dependency.isEmpty()) parts.append(dependency);
   parts.append(property); return parts.join('.');
}
