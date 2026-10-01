#include "ResourceProperties.hpp"
#include "TrackingConfigDialog.hpp"
#include "EpochIntervalDialog.hpp"
#include "ReportParameterDialog.hpp"
#include "GroundTrackDialog.hpp"
#include "OrbitViewDialog.hpp"
#include "ThrusterDialog.hpp"
#include "BurnDialog.hpp"
#include "OrbitPlot.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "FactoryManager.hpp"
#include "GmatType.hpp"
#include "FileUtil.hpp"
#include "FileManager.hpp"
#include "Rmatrix.hpp"
#include "Array.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "AxisSystem.hpp"
#include "CoordinateSystem.hpp"
#include "CalculatedPoint.hpp"
#include "StateConversionUtil.hpp"
#include "TimeSystemConverter.hpp"
#include <memory>
#include <array>
#include "Rvector.hpp"
#include "Attitude.hpp"
#include "Spacecraft.hpp"
#include "SpacecraftOrbit.hpp"
#include <optional>
#include <QRegularExpression>
#include <QSet>
#include <QFileInfo>
#include <QDir>
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
bool externalForceSetting(GmatBase &object,const QString &name)
{
   return object.IsOfType("ExternalModel") && QStringList{"ScriptFileName","ExcludeOtherForces","DerivativesFunction"}.contains(name);
}
GmatBase *forcePropertyOwner(GmatBase &object,const QString &name,QString &leaf)
{
   if (!object.IsOfType("ODEModel")) return nullptr;
   for (int i=0;i<object.GetOwnedObjectCount();++i) {
      auto *force=object.GetOwnedObject(i);
      if (!force) continue;
      const auto prefix=QString::fromStdString(object.BuildPropertyName(force))+".";
      if (name.startsWith(prefix)) { leaf=name.mid(prefix.size()); return force; }
      // CreateForceBody changes the canonical body prefix while other fields
      // in the same pending transaction still use the original force name.
      const auto original=QString::fromStdString(force->GetName())+".";
      if (force->IsOfType("PolyhedronGravityModel") && name.startsWith(original)) { leaf=name.mid(original.size()); return force; }
   }
   return nullptr;
}
}

QStringList pythonModuleNames()
{
   QStringList names;
   for (const auto &path:FileManager::Instance()->GetAllPythonModulePaths())
      for (const auto &file:QDir(QString::fromStdString(path)).entryInfoList({"*.py"},QDir::Files|QDir::Readable,QDir::Name)) {
         const auto module=file.completeBaseName(); if (!module.startsWith('_') && !names.contains(module)) names.append(module);
      }
   return names;
}
QVector<ResourceProperty> resourceProperties(GmatBase &object)
{
   QVector<ResourceProperty> fields;
   std::optional<Rvector6> orbit;
   for (Integer id = 0; id < object.GetParameterCount(); ++id) {
      try {
         // Array values are marked read-only for ordinary property syntax;
         // their specialized editor writes the array's indexed initial values.
         const bool arrayValues=object.GetTypeName()=="Array" && object.GetParameterText(id)=="RmatValue";
         // GroundTrack hides a default texture from serialization; wx still
         // permits replacing it through its dedicated texture-map control.
         const bool groundTexture=object.GetTypeName()=="GroundTrack" && object.GetParameterText(id)=="TextureMap";
         const auto fieldName=QString::fromStdString(object.GetParameterText(id));
         if (auto *point=dynamic_cast<CalculatedPoint *>(&object); point && point->IsBuiltIn() && fieldName!="OrbitColor" && fieldName!="TargetColor") continue;
         const bool thrusterSetting=(object.IsOfType("Thruster") && QStringList{"Origin","Axes","MixRatio"}.contains(fieldName)) ||
            (object.IsOfType("ImpulsiveBurn") && QStringList{"Origin","Axes"}.contains(fieldName));
         const bool ephemerisSetting=object.IsOfType("EphemerisFile") && QStringList{"DistanceUnit","IncludeEventBoundaries"}.contains(fieldName);
         // This input becomes writable for serialization only once populated.
         // The GUI must offer the first selection as well as later replacement.
         const bool ephemerisInput=object.IsOfType("Spacecraft") && fieldName=="EphemerisName";
         bool orbitElementId=false;
         if (object.IsOfType("Spacecraft")) for (int i=1;i<=6;++i) orbitElementId=orbitElementId || id==object.GetParameterID("Element"+std::to_string(i));
         if (object.IsParameterReadOnly(id) && !arrayValues && !orbitElementId && !groundTexture && !thrusterSetting && !ephemerisSetting && !ephemerisInput && !externalForceSetting(object,fieldName)) continue;
         ResourceProperty field;
         field.name = QString::fromStdString(object.GetParameterText(id));
         if (object.IsOfType("Spacecraft") && field.name=="StateType") continue; // Deprecated input-state alias; DisplayStateType is the GUI choice.
         field.unit = QString::fromStdString(object.GetParameterUnit(id));
         if (object.IsOfType("ErrorModel") && field.name=="NoiseSigma") field.unit=QString::fromStdString(object.GetParameterUnit(object.GetParameterID("Bias")));
         if (object.IsOfType(Gmat::AXIS_SYSTEM) && field.name=="Epoch") field.unit="A1ModJulian";
         switch (object.GetParameterType(id)) {
         case Gmat::RVECTOR_TYPE: {
            field.resizableVector=isResizableResourceVector(object,field.name);
            const auto &vector=object.GetRvectorParameter(id);
            field.rows=1; field.columns=vector.GetSize();
            QStringList values;
            for (int i=0;i<field.columns;++i) values.append(QString::number(vector[i],'g',17));
            field.value=values.join(" "); break;
         }
         case Gmat::RMATRIX_TYPE: {
            const auto &matrix=object.GetRmatrixParameter(id);
            field.rows=matrix.GetNumRows(); field.columns=matrix.GetNumColumns();
            QStringList rows;
            for (int r=0;r<field.rows;++r) {
               QStringList values;
               for (int c=0;c<field.columns;++c) values.append(QString::number(matrix(r,c),'g',17));
               rows.append(values.join(" "));
            }
            field.value=rows.join("; "); break;
         }
         case Gmat::REAL_TYPE: {
            bool orbitElement=false;
            if (auto *spacecraft=dynamic_cast<Spacecraft *>(&object)) for (int element=1;element<=6;++element) {
               if (id!=spacecraft->GetParameterID("Element"+std::to_string(element))) continue;
               // Read the complete displayed state. Individual anomaly getters
               // are not a substitute for the converted state's sixth element.
               if (!orbit) orbit=spacecraft->GetState(spacecraft->GetStringParameter("DisplayStateType"));
               field.name=spacecraftOrbitElementNames(object)[element-1];
               field.value=QString::number((*orbit)[element-1],'g',17); orbitElement=true; break;
            }
            if (!orbitElement) field.value=QString::number(object.GetRealParameter(id),'g',17);
            break;
         }
         case Gmat::INTEGER_TYPE: field.value = QString::number(object.GetIntegerParameter(id)); break;
         case Gmat::UNSIGNED_INT_TYPE: field.value = QString::number(object.GetUnsignedIntParameter(id)); break;
         case Gmat::BOOLEANARRAY_TYPE: {
            if (!object.IsOfType("OrbitView") || field.name!="DrawObject") continue;
            QStringList values;
            for (const auto &name:object.GetStringArrayParameter("Add")) values.append(static_cast<OrbitPlot &>(object).GetShowObject(name) ? "true" : "false");
            field.value=values.join(" "); break;
         }
         case Gmat::BOOLEAN_TYPE:
            field.choices = {"true", "false"}; field.value = object.GetBooleanParameter(id) ? "true" : "false"; break;
         case Gmat::ON_OFF_TYPE:
            field.choices = {"On", "Off"}; field.value = QString::fromStdString(object.GetOnOffParameter(id)); break;
         case Gmat::ENUMERATION_TYPE:
            for (const auto &choice : object.GetPropertyEnumStrings(id)) field.choices.append(QString::fromStdString(choice));
            field.value=QString::fromStdString(object.GetStringParameter(id)); break;
         case Gmat::COLOR_TYPE: field.color=true; [[fallthrough]];
         case Gmat::STRING_TYPE:
         case Gmat::FILENAME_TYPE:
         case Gmat::OBJECT_TYPE:
            field.value = QString::fromStdString(object.IsOfType("Imager") && field.name=="FieldOfView" ? object.GetRefObjectName(Gmat::FIELD_OF_VIEW) : object.GetStringParameter(id));
            break;
         case Gmat::STRINGARRAY_TYPE:
         case Gmat::OBJECTARRAY_TYPE:
            if ((object.IsOfType("Simulator") || object.IsOfType("Estimator")) && field.name=="Propagator") {
               if (object.IsOfType("Estimator")) { const auto names=object.GetStringArrayParameter(id); field.value=names.empty() ? QString() : QString::fromStdString(names.front()); }
               else field.value=QString::fromStdString(object.GetStringParameter(id));
               break;
            }
            if (!isResourceList(object, field.name)) continue;
            for (const auto &entry : object.GetStringArrayParameter(id)) field.choices.append(QString::fromStdString(entry));
            field.fileList=isResourceFileList(object,field.name);
            field.value = field.choices.join(field.fileList ? "\n" : ", ");
            field.choices.clear(); field.list = true; break;
         default: continue;
         }
         const bool atmosphereFile=object.IsOfType("DragForce") && QStringList{"CSSISpaceWeatherFile","SchattenFile","InputFile"}.contains(field.name);
         field.filename=field.fileList || object.GetParameterType(id)==Gmat::FILENAME_TYPE || atmosphereFile || ephemerisInput;
         if (field.filename) {
            const auto type=object.GetTypeName();
            field.fileOutput=object.IsOfType("ReportFile") || object.IsOfType("EphemerisFile") || object.IsOfType("EventLocator") ||
               (object.IsOfType("TrackingFileSet") && field.name=="FileName") ||
               (object.IsOfType("Solver") && field.name=="ReportFile") ||
               (object.IsOfType("Estimator") && (field.name=="MatlabFile" || field.name=="DataFile"));
            field.fileInput=atmosphereFile || object.IsOfType("GroundStation") || object.IsOfType("Function") || object.IsOfType("Spacecraft") ||
               type=="GroundTrack" || type=="GroundTrackPlot" || type=="FileInterface" || type=="ThrustHistoryFile" ||
               type=="CustomFOV" || type=="Code500" || type=="CCSDS-OEM" || type=="STK";
            if (object.IsOfType("TrackingFileSet") && field.name=="RampTable") field.fileInput=true;
         }
         if (object.IsOfType("PolyhedronGravityModel")) {
            if (field.name=="ShapeFileName") {
               field.filename=true; field.fileInput=true;
               field.help="Closed triangle mesh with outward face winding. Vertex coordinates are in kilometres.";
            }
            if (field.name=="BodyDensity") field.unit="kg/m^3";
            if (field.name=="CreateForceBody")
               for (const auto &body:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) field.references.append(QString::fromStdString(body));
         }
         if (object.IsOfType("ExternalModel") && field.name=="ScriptFileName") {
            // This engine field is typed as a filename but Python imports its
            // value as a module name. A file chooser would store an invalid
            // absolute path and lose portability when saving the mission.
            field.filename=false; field.fileInput=false;
            field.help="Python module name without .py, found in the configured Python search paths. Imported code is cached; restart GMAT after changing the Python module.";
            field.references=pythonModuleNames();
         }
         if (object.IsOfType("ODEModel") && field.name=="ErrorControl")
            field.choices={"RSSState","LargestState","None","LargestStep","RSSStep"};
         if (object.IsOfType("SeqEstimator")) {
            if (field.name=="InputWarmStartFile" || field.name=="OutputWarmStartFile") {
               field.filename=true; field.fileInput=field.name=="InputWarmStartFile"; field.fileOutput=!field.fileInput;
               field.help=field.fileInput ? "Filter state and covariance CSV. Leave empty for a cold start." : "Filter state and covariance CSV. Leave empty to disable warm-start output.";
            }
            if (field.name=="WarmStartEpochFormat") for (const auto &format:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) field.choices.append(QString::fromStdString(format));
            if (field.name=="WarmStartEpoch") {
               field.references={"FirstMeasurement","LastWarmStartRecord"};
               field.help="Enter a date in WarmStartEpochFormat, or select a boundary. FirstMeasurement requires an earlier seed record; LastWarmStartRecord requires later observations. Changing format converts an explicit date.";
            }
         }
         const bool processType=object.IsOfType("ProcessNoiseModel") && field.name=="Type";
         const bool parameterModel=object.IsOfType("EstimatedParameter") && field.name=="Model";
         if (processType || parameterModel) {
            for (const auto &type:Moderator::Instance()->GetListOfFactoryItems(GmatType::GetTypeId(processType ? "ProcessNoise" : "EstimatedParameterModel"))) field.choices.append(QString::fromStdString(type));
            field.help="Changing model installs its default settings. Apply, then reopen the resource to edit the new model's parameters.";
         }
         if (object.IsOfType("Smoother") && field.name=="Filter") {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SOLVER)) {
               auto *filter=Moderator::Instance()->GetConfiguredObject(name);
               if (filter && filter->IsOfType("SeqEstimator")) field.references.append(QString::fromStdString(name));
            }
         }
         if (object.GetParameterType(id)==Gmat::OBJECT_TYPE || object.GetParameterType(id)==Gmat::OBJECTARRAY_TYPE) {
            try {
               auto type=object.IsOfType("Formation") && field.name=="Add" ? Gmat::SPACECRAFT : object.GetPropertyObjectType(id);
               if (object.IsOfType(Gmat::AXIS_SYSTEM)) {
                  if (field.name=="Primary" || field.name=="Secondary" || field.name=="ReferenceObject") type=Gmat::SPACE_POINT;
                  if (field.name=="ConstraintCoordinateSystem") type=Gmat::COORDINATE_SYSTEM;
               }
               if (type!=Gmat::UNKNOWN_OBJECT) for (const auto &name:Moderator::Instance()->GetListOfObjects(type))
                  field.references.append(QString::fromStdString(name));
               field.references.removeDuplicates(); field.references.sort();
            } catch (BaseException &) {} // Keep editable text for plugin-defined reference types.
         }
         if (object.IsOfType("Thruster") || object.IsOfType("ImpulsiveBurn")) {
            if (field.name=="CoordinateSystem") {
               field.references.clear(); field.references.append("Local");
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::COORDINATE_SYSTEM)) field.references.append(QString::fromStdString(name));
            }
            if (field.name=="Tank") {
               field.references.clear();
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::FUEL_TANK)) {
                  auto *tank=Moderator::Instance()->GetConfiguredObject(name);
                  if (tank && (object.IsOfType("ImpulsiveBurn") || tank->IsOfType(object.IsOfType("ElectricThruster") ? "ElectricTank" : "ChemicalTank"))) field.references.append(QString::fromStdString(name));
               }
            }
         }
         if (object.IsOfType("OrbitView")) {
            const bool camera=QStringList{"ViewPointReference","ViewPointVector","ViewDirection"}.contains(field.name);
            const bool frame=field.name=="CoordinateSystem" || field.name=="ViewUpCoordinateSystem";
            if (camera || frame) {
               field.references.clear();
               for (const auto &name:Moderator::Instance()->GetListOfObjects(camera ? Gmat::SPACE_POINT : Gmat::COORDINATE_SYSTEM)) field.references.append(QString::fromStdString(name));
            }
            if (field.name=="ViewUpAxis" && field.choices.isEmpty()) field.choices={"X","Y","Z","-X","-Y","-Z"};
         }
         if (object.IsOfType("Spacecraft") && field.name=="DisplayStateType") {
            auto *frame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject(object.GetStringParameter("CoordinateSystem")));
            auto *origin=frame ? Moderator::Instance()->GetConfiguredObject(frame->GetStringParameter("Origin")) : nullptr;
            const auto *types=StateConversionUtil::GetStateTypeList();
            for (int i=0;i<StateConversionUtil::GetTypeCount();++i) {
               if (StateConversionUtil::RequiresCelestialBodyOrigin(types[i]) && (!origin || !origin->IsOfType(Gmat::CELESTIAL_BODY))) continue;
               if (StateConversionUtil::RequiresFixedCoordinateSystem(types[i]) && (!frame || !frame->AreAxesOfType("BodyFixedAxes"))) continue;
               field.choices.append(QString::fromStdString(types[i]));
            }
         }
         if (object.IsOfType("SolarPowerSystem") && field.name=="ShadowModel") field.choices={"None","DualCone"};
         if (object.IsOfType("LibrationPoint")) {
            if (field.name=="Point") field.choices={"L1","L2","L3","L4","L5"};
            if (field.name=="Primary" || field.name=="Secondary") {
               field.references.clear();
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
                  auto *point=Moderator::Instance()->GetConfiguredObject(name);
                  if (point && (point->IsOfType("CelestialBody") || point->IsOfType("Barycenter")) && name!="SolarSystemBarycenter") field.references.append(QString::fromStdString(name));
               }
               field.references.removeDuplicates(); field.references.sort();
            }
         }
         if (object.IsOfType("Barycenter") && field.name=="BodyNames") {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) field.references.append(QString::fromStdString(name));
            field.references.sort();
         }
         if (object.IsOfType("ErrorModel")) {
            if (field.name=="Type") field.choices=trackingMeasurementTypes();
            if (field.name=="SolveFors") field.references={"Bias","PassBiases"};
         }
         if (object.IsOfType("FirstOrderGaussMarkov") && field.name=="SolveFor") field.choices={"Cd","AtmosDensityScaleFactor"};
         if (object.IsOfType("ThrustHistoryFile") && field.name=="AddThrustSegment") {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::INTERFACE)) {
               auto *segment=Moderator::Instance()->GetConfiguredObject(name);
               if (segment && segment->IsOfType("ThrustSegment")) field.references.append(QString::fromStdString(name));
            }
         }
         if (object.IsOfType("ThrustSegment")) {
            if (field.name=="MassSource") {
               field.references.clear(); for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::FUEL_TANK)) field.references.append(QString::fromStdString(name));
            }
            if (field.name=="SolveFors") field.references={"ThrustScaleFactor","ThrustAngle1","ThrustAngle2"};
         }
         if (hasEpochInterval(object) && field.name=="EpochFormat") {
            field.choices.clear(); for (const auto &format:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) field.choices.append(QString::fromStdString(format));
         }
         if ((object.IsOfType("TrackingFileSet") || object.IsOfType("Estimator")) && field.name=="DataFilters") {
            field.references.clear(); for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::DATA_FILTER)) field.references.append(QString::fromStdString(name));
         }
         if (object.IsOfType(Gmat::DATA_FILTER)) {
            if (field.name=="ThinMode") field.choices={"Frequency","Time"};
            if (field.name=="DataTypes") { field.references=trackingMeasurementTypes(); field.references.prepend("All"); }
            if (field.name=="ObservedObjects" || field.name=="Trackers") {
               field.references={"All"};
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
                  auto *participant=Moderator::Instance()->GetConfiguredObject(name);
                  if (participant && (participant->IsOfType("Spacecraft") || (field.name=="Trackers" && participant->IsOfType("GroundStation")))) field.references.append(QString::fromStdString(name));
               }
            }
         }
         if (object.IsOfType("Imager") && field.name=="FieldOfView") {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::FIELD_OF_VIEW)) field.references.append(QString::fromStdString(name));
         }
         if (object.IsOfType("GroundStation") && (field.name=="ErrorModels" || field.name=="AddHardware")) {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(field.name=="ErrorModels" ? Gmat::ERROR_MODEL : Gmat::HARDWARE)) field.references.append(QString::fromStdString(name));
         }
         if (object.IsOfType("Spacecraft") && (field.name=="SolveFors" || field.name=="ProcessNoiseModel")) {
            if (field.name=="SolveFors") field.references={"CartesianState","KeplerianState","Cd","Cr","SPADDragScaleFactor","SPADSRPScaleFactor","AtmosDensityScaleFactor"};
            else field.references={""};
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT)) {
               auto *candidate=Moderator::Instance()->GetConfiguredObject(name);
               if (candidate && candidate->IsOfType(field.name=="SolveFors" ? "EstimatedParameter" : "ProcessNoiseModel")) field.references.append(QString::fromStdString(name));
            }
         }
         if ((object.IsOfType("Simulator") && field.name=="AddData") || (object.IsOfType("Estimator") && field.name=="Measurements")) {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::MEASUREMENT_MODEL)) {
               auto *measurement=Moderator::Instance()->GetConfiguredObject(name);
               if (measurement && measurement->IsOfType("TrackingFileSet")) field.references.append(QString::fromStdString(name));
            }
         }
         if (object.IsOfType("DragForce")) {
            if (field.name=="HistoricWeatherSource") field.choices={"ConstantFluxAndGeoMag","CSSISpaceWeatherFile"};
            if (field.name=="PredictedWeatherSource") field.choices={"ConstantFluxAndGeoMag","CSSISpaceWeatherFile","SchattenFile"};
            if (field.name=="SchattenErrorModel") field.choices={"Nominal","PlusTwoSigma","MinusTwoSigma"};
            if (field.name=="SchattenTimingModel") field.choices={"NominalCycle","EarlyCycle","LateCycle"};
            if (field.name=="DragModel") field.choices={"Spherical","SPADFile"};
         }
         if (object.IsOfType("EventLocator")) {
            if (field.name=="InputEpochFormat") for (const auto &format:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) field.choices.append(QString::fromStdString(format));
            if (field.name=="Spacecraft" || field.name=="Target") {
               field.references.clear();
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT)) field.references.append(QString::fromStdString(name));
               if (field.name=="Target") for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::REGION)) field.references.append(QString::fromStdString(name));
            }
            if (field.name=="Observers") {
               field.references.clear();
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
                  auto *observer=Moderator::Instance()->GetConfiguredObject(name);
                  if (observer && (observer->IsOfType("GroundStation") || observer->IsOfType("Spacecraft"))) field.references.append(QString::fromStdString(name));
               }
               field.references.removeAll(QString::fromStdString(object.GetStringParameter("Target")));
            }
            if (field.name=="EclipseTypes") for (const auto &choice:object.GetPropertyEnumStrings(id)) field.references.append(QString::fromStdString(choice));
            if (field.name=="Sensors") {
               field.references.clear();
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::HARDWARE)) {
                  auto *sensor=Moderator::Instance()->GetConfiguredObject(name);
                  if (sensor && sensor->IsOfType("Imager")) field.references.append(QString::fromStdString(name));
               }
            }
            if (field.name=="IntrudingBodies") field.references.removeAll(QString::fromStdString(object.GetStringParameter("Spacecraft")));
            if (field.name=="ReportCoordinates") for (const auto &choice:object.GetPropertyEnumStrings(id)) field.choices.append(QString::fromStdString(choice));
            if (field.name=="SpiceGridFrameFile") { field.filename=true; field.fileInput=true; field.fileOutput=false; }
            if (field.name=="ReportTimeFormat") field.choices={"UTCGregorian","UTCMJD","ISOYD"};
            if (field.name=="ReportFormat") field.choices={"Legacy","AzimuthElevationRangeReport","AzimuthElevationRangeRangeRateReport","ContactRangeReport","SiteViewMaxElevationRangeReport","SiteViewMaxElevationReport"};
         }
         // DifferentialCorrector marks these as enums but supplies no choice
         // metadata. Match its setters and the wx DC setup panel.
         if (object.IsOfType("DifferentialCorrector") && field.name=="Algorithm")
            field.choices={"NewtonRaphson","Broyden","ModifiedBroyden"};
         if (object.IsOfType("DifferentialCorrector") && field.name=="DerivativeMethod")
            field.choices={"CentralDifference","ForwardDifference","BackwardDifference"};
         if (object.IsOfType("SolarPowerSystem") && field.name=="ShadowBodies") {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) if (name!="Sun") field.references.append(QString::fromStdString(name));
            field.hiddenReferences={"Sun"};
         }
         if (object.IsOfType("ODEModel") && (field.name=="PrimaryBodies" || field.name=="PointMasses")) {
            field.references.clear();
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) field.references.append(QString::fromStdString(name));
         }
         if (object.IsOfType("CoordinateSystem") && field.name=="Axes")
            for (const auto &type:Moderator::Instance()->GetListOfFactoryItems(Gmat::AXIS_SYSTEM)) field.choices.append(QString::fromStdString(type));
         if (object.GetTypeName()=="ObjectReferenced" && (field.name=="XAxis" || field.name=="YAxis" || field.name=="ZAxis"))
            field.choices={"","R","-R","V","-V","N","-N"};
         fields.append(field);
      } catch (BaseException &) {
         // Some plugin and computed properties have no scalar editor.
      }
   }
   if (auto *setup=dynamic_cast<PropSetup *>(&object)) if (auto *propagator=setup->GetPropagator()) {
      for (const auto &field:resourceProperties(*propagator)) {
         const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
         if (!duplicate) fields.append(field);
      }
   }
   if (object.IsOfType("CoordinateSystem")) if (auto *axes=object.GetOwnedObject(0)) {
      for (const auto &field:resourceProperties(*axes)) {
         const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
         if (!duplicate) fields.append(field);
      }
   }
   if (object.IsOfType("ProcessNoiseModel") || object.IsOfType("EstimatedParameter")) if (auto *model=object.GetOwnedObject(0)) {
      for (auto field:resourceProperties(*model)) {
         if (field.name=="CoordinateSystem") {
            field.references.clear(); for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::COORDINATE_SYSTEM)) field.references.append(QString::fromStdString(name));
         }
         if (object.IsOfType("ProcessNoiseModel") && field.name=="AccelNoiseSigma") field.unit="km/s^(3/2)";
         if (object.IsOfType("EstimatedParameter") && field.name=="HalfLife") field.unit="s";
         const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
         if (!duplicate) fields.append(field);
      }
   }
   if (object.IsOfType("Spacecraft")) {
      // Attitude parameters are delegated by name at IDs above the spacecraft
      // parameter table. Enumerate the owned model rather than guessing IDs.
      if (auto *attitude=object.GetOwnedObject(0)) {
         for (const auto &field:resourceProperties(*attitude)) {
            const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
            if (!duplicate && !object.IsParameterReadOnly(object.GetParameterID(field.name.toStdString()))) fields.append(field);
         }
      }
   }
   if (object.IsOfType("ODEModel")) {
      // Engine-owned force components have their own parameter tables and
      // canonical prefixes (for example GravityField.Earth.Degree).
      for (int i=0;i<object.GetOwnedObjectCount();++i) {
         auto *force=object.GetOwnedObject(i); if (!force) continue;
         const auto prefix=QString::fromStdString(object.BuildPropertyName(force));
         if (prefix.isEmpty() || prefix=="UnknownForce") continue;
         for (auto field:resourceProperties(*force)) {
            if (field.list) continue;
            field.name=prefix+"."+field.name;
            const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
            if (!duplicate) fields.append(field);
         }
      }
   }
   return fields;
}

bool isResizableResourceVector(GmatBase &object,const QString &name)
{
   return object.IsOfType("ThrustSegment") && QStringList{"ThrustAngle1","ThrustAngle2","ThrustAngle1Sigma","ThrustAngle2Sigma"}.contains(name);
}
bool isResourceFileList(GmatBase &object,const QString &name)
{
   return (object.IsOfType("Spacecraft") && (name=="OrbitSpiceKernelName" || name=="AttitudeSpiceKernelName" ||
      name=="SCClockSpiceKernelName" || name=="FrameSpiceKernelName")) ||
      (object.IsOfType("TrackingFileSet") && (name=="FileName" || name=="RampTable")) ||
      (object.IsOfType(Gmat::DATA_FILTER) && name=="FileNames");
}

bool isResourceList(GmatBase &object, const QString &name)
{
   if (auto *setup=dynamic_cast<PropSetup *>(&object)) if (auto *propagator=setup->GetPropagator()) {
      try { if (propagator->GetParameterID(name.toStdString())>=0) return isResourceList(*propagator,name); }
      catch (BaseException &) {} // Parent-only fields such as FM and Type.
   }
   if (object.IsOfType("CoordinateSystem") && name!="Axes") if (auto *axes=object.GetOwnedObject(0)) {
      try { if (axes->GetParameterID(name.toStdString())>=0) return isResourceList(*axes,name); }
      catch (BaseException &) {}
   }
   // These lists use canonical {...} syntax. Other compound lists can carry
   // additional positional settings and need their own replacement handling.
   QString leaf;
   if (forcePropertyOwner(object,name,leaf)) return false;
   const auto id=object.GetParameterID(name.toStdString());
   const auto type=QString::fromStdString(object.GetTypeName());
   if (auto *point=dynamic_cast<CalculatedPoint *>(&object)) if (name=="BodyNames" && point->IsBuiltIn()) return false;
   const bool supported=isResourceFileList(object,name) || (name=="Add" && (type=="OrbitView" || type=="GroundTrack" || type=="GroundTrackPlot" || type=="ReportFile" || type=="Formation")) ||
      (type=="Barycenter" && name=="BodyNames") ||
      (object.IsOfType("GroundStation") && (name=="ErrorModels" || name=="AddHardware")) ||
      ((object.IsOfType("Spacecraft") || object.IsOfType("ErrorModel")) && name=="SolveFors") ||
      (object.IsOfType("Simulator") && name=="AddData") ||
      (object.IsOfType("Estimator") && name=="Measurements") ||
      (object.IsOfType("ThrustHistoryFile") && name=="AddThrustSegment") ||
      (object.IsOfType("ThrustSegment") && (name=="MassSource" || name=="SolveFors")) ||
      ((object.IsOfType("TrackingFileSet") || object.IsOfType("Estimator")) && name=="DataFilters") ||
      (object.IsOfType(Gmat::DATA_FILTER) && QStringList{"FileNames","ObservedObjects","Trackers","DataTypes","RecordNumbers"}.contains(name)) ||
      (name=="YVariables" && type=="XYPlot") ||
      (object.IsOfType("Spacecraft") && (name=="Tanks" || name=="Thrusters" || name=="AddHardware" || name=="AddPlates")) ||
      ((object.IsOfType("Thruster") || object.IsOfType("ImpulsiveBurn")) && name=="Tank") ||
      (type=="FiniteBurn" && name=="Thrusters") ||
      (type=="SolarPowerSystem" && name=="ShadowBodies") ||
      ((type=="ForceModel" || type=="ODEModel") && (name=="PrimaryBodies" || name=="PointMasses")) ||
      (object.IsOfType("EventLocator") && (name=="OccultingBodies" || name=="Observers" || name=="EclipseTypes" || name=="IntrudingBodies" || name=="Sensors"));
   const auto parameterType=object.GetParameterType(id);
   return supported && (parameterType==Gmat::OBJECTARRAY_TYPE || parameterType==Gmat::STRINGARRAY_TYPE) && !object.IsParameterReadOnly(id);
}

QStringList splitResourceReferences(const QString &value)
{
   QStringList entries;
   int start=0,depth=0;
   for (int i=0;i<value.size();++i) {
      if (value[i]=='(') ++depth;
      else if (value[i]==')') --depth;
      else if (depth==0 && (value[i]==',' || value[i]=='\n')) {
         entries.append(value.mid(start,i-start).trimmed()); start=i+1;
      }
   }
   if (!value.trimmed().isEmpty()) entries.append(value.mid(start).trimmed());
   return entries;
}

QString replaceResourceList(GmatBase &object, const QString &block, const QString &name, const QString &value, const QString *mixture)
{
   const auto id=object.GetParameterID(name.toStdString());
   if (!isResourceList(object,name) || object.IsParameterReadOnly(id)) throw std::runtime_error("This list cannot be edited here");
   // Candidate interpretation resolves resource references and validates kernel
   // files. File lists use newlines so commas within paths remain intact.
   static const QRegularExpression reference("^[A-Za-z_][A-Za-z0-9_]*(?:\\.[A-Za-z_][A-Za-z0-9_]*)*$");
   static const QRegularExpression arrayElement("^[A-Za-z_][A-Za-z0-9_]*\\(\\s*[1-9][0-9]*\\s*,\\s*[1-9][0-9]*\\s*\\)$");
   const bool reportParameters=(object.GetTypeName()=="ReportFile" && name=="Add") || (object.GetTypeName()=="XYPlot" && name=="YVariables");
   const bool files=isResourceFileList(object,name);
   QStringList entries;
   if (!value.trimmed().isEmpty()) {
      for (const auto &part : (files ? value.split('\n',Qt::SkipEmptyParts) : splitResourceReferences(value))) {
         const auto entry=part.trimmed();
         if (files && (entry.contains('\'') || entry.contains('\r') || entry.contains(';'))) throw std::runtime_error("File paths cannot contain quotes, semicolons or line breaks");
         const bool record=object.IsOfType(Gmat::DATA_FILTER) && name=="RecordNumbers";
         if (record) {
            if (!QRegularExpression("^(?:All|[1-9][0-9]*(?:-[1-9][0-9]*)?)$").match(entry).hasMatch()) throw std::runtime_error("Enter All, positive record numbers or ascending ranges such as 1-20.");
            if (entry.contains('-') && entry.section('-',0,0).toLongLong()>entry.section('-',1,1).toLongLong()) throw std::runtime_error("Record ranges must be ascending.");
         }
         if (!files && !record && !reference.match(entry).hasMatch() && !(reportParameters && arrayElement.match(entry).hasMatch())) throw std::runtime_error("Enter comma-separated resource or parameter names");
         if (object.IsOfType(Gmat::DATA_FILTER) && (name=="DataTypes" || name=="ObservedObjects" || name=="Trackers")) {
            const auto fields=resourceProperties(object); const auto field=std::find_if(fields.cbegin(),fields.cend(),[&](const auto &field) { return field.name==name; });
            if (field==fields.cend() || (!field->references.contains(entry) && !(name=="ObservedObjects" && entry.contains('.')))) throw std::runtime_error(("Select an available "+name+" entry.").toStdString());
         }
         if (object.GetTypeName()=="XYPlot" && name=="YVariables" && !ReportParameterDialog::isPlottableReference(entry))
            throw std::runtime_error(("Select a numeric plot parameter or a valid array element: "+entry).toStdString());
         if (name=="Add" && object.IsOfType("Formation")) {
            auto *member=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!member || !member->IsOfType(Gmat::SPACECRAFT)) throw std::runtime_error("Formation members must be existing spacecraft");
         }
         if (name=="Add" && (object.GetTypeName()=="OrbitView" || object.GetTypeName()=="GroundTrack" || object.GetTypeName()=="GroundTrackPlot")) {
            auto *target=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!target || !target->IsOfType(Gmat::SPACE_POINT))
               throw std::runtime_error(("Unknown space point: "+entry).toStdString());
            if (target->GetName()!=entry.toStdString())
               throw std::runtime_error("Use the resource name "+target->GetName()+" instead of "+entry.toStdString());
         }
         if (object.IsOfType("Thruster") && name=="Tank") {
            auto *tank=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!tank || !tank->IsOfType(object.IsOfType("ElectricThruster") ? "ElectricTank" : "ChemicalTank")) throw std::runtime_error("Select a tank matching the thruster type.");
         }
         if (object.IsOfType("FiniteBurn") && name=="Thrusters") {
            auto *thruster=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!thruster || !thruster->IsOfType(Gmat::THRUSTER)) throw std::runtime_error("Select an available thruster.");
         }
         if (object.IsOfType("SolarPowerSystem") && name=="ShadowBodies") {
            if (entry=="Sun") throw std::runtime_error("The Sun cannot shadow itself. Select other celestial bodies.");
            const auto &bodies=Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY);
            if (std::find(bodies.begin(),bodies.end(),entry.toStdString())==bodies.end()) throw std::runtime_error("Select an existing celestial body for solar shadows");
         }
         if (object.IsOfType("Barycenter") && name=="BodyNames") {
            auto *body=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!body || !body->IsOfType("CelestialBody") || body->GetName()!=entry.toStdString()) throw std::runtime_error("Select existing celestial bodies for the barycenter.");
         }
         const QString expectedType=(object.IsOfType("GroundStation") && name=="ErrorModels") ? "ErrorModel" :
            (object.IsOfType("ThrustHistoryFile") && name=="AddThrustSegment") ? "ThrustSegment" :
            (object.IsOfType("ThrustSegment") && name=="MassSource") ? "FuelTank" :
            (object.IsOfType("GroundStation") && name=="AddHardware") ? "Hardware" :
            ((object.IsOfType("Simulator") && name=="AddData") || (object.IsOfType("Estimator") && name=="Measurements")) ? "TrackingFileSet" :
            ((object.IsOfType("TrackingFileSet") || object.IsOfType("Estimator")) && name=="DataFilters") ? "DataFilter" : QString();
         if (!expectedType.isEmpty()) {
            auto *referenceObject=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!referenceObject || !referenceObject->IsOfType(expectedType.toStdString())) throw std::runtime_error(("Select existing "+expectedType+" resources for "+name+".").toStdString());
         }
         if (object.IsOfType("ThrustSegment") && name=="SolveFors" && !QStringList{"ThrustScaleFactor","ThrustAngle1","ThrustAngle2"}.contains(entry))
            throw std::runtime_error("Select a thrust scale factor or thrust angle solve-for.");
         if (entries.contains(entry)) throw std::runtime_error("Each list entry must be unique");
         entries.append(entry);
      }
   }
   if (object.GetTypeName()=="XYPlot" && name=="YVariables" && entries.isEmpty() && object.GetBooleanParameter("ShowPlot"))
      throw std::runtime_error("Select at least one Y parameter, or turn off Show plot.");
   if (object.IsOfType("Barycenter") && name=="BodyNames" && entries.isEmpty()) throw std::runtime_error("Select at least one celestial body for the barycenter.");
   const QString key=QString::fromStdString(object.GetName())+"."+name;
   const QRegularExpression assignment("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(key)+
      "[ \\t]*=[ \\t]*\\{[^;]*?\\}[ \\t]*;",QRegularExpression::MultilineOption);
   if (name=="Tank" && object.IsOfType("Thruster") && entries.isEmpty()) {
      if (mixture && !mixture->trimmed().isEmpty()) throw std::runtime_error("Enter one mixture ratio for each selected tank");
      // ClearTanks leaves both arrays empty. The interpreter cannot read an
      // explicit empty MixRatio vector, so serialize their default by omission.
      QString result=block;
      result.remove(assignment);
      const QString ratioKey=QString::fromStdString(object.GetName())+".MixRatio";
      result.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(ratioKey)+
         "[ \\t]*=[ \\t]*\\[[^;]*?\\][ \\t]*;",QRegularExpression::MultilineOption));
      return result;
   }
   auto matches=assignment.globalMatch(block);
   QStringList serialized=entries;
   if (object.GetParameterType(id)==Gmat::STRINGARRAY_TYPE)
      for (auto &entry:serialized) entry="'"+entry+"'";
   const QString replacement="GMAT "+key+" = {"+serialized.join(", ")+"};";
   QString result=block;
   // Empty spacecraft hardware lists are defaults, omitted by the engine's
   // serializer. Explicit {} is interpreted as a hardware name in these fields.
   const bool omitEmpty=object.GetTypeName()=="ReportFile" || object.GetTypeName()=="XYPlot" || (object.IsOfType("GroundStation") && (name=="AddHardware" || name=="ErrorModels")) || (object.IsOfType("FiniteBurn") && name=="Thrusters") || (object.IsOfType("Spacecraft") &&
      QSet<QString>{"Tanks","Thrusters","AddHardware","AddPlates"}.contains(name));
   if (!matches.hasNext()) {
      if (entries.isEmpty() && omitEmpty) return result;
      if (object.GetStringArrayParameter(id).empty())
         result+=(block.endsWith('\n') ? "" : "\n")+replacement+"\n";
      else throw std::runtime_error("Cannot locate the list safely in this resource's script");
   } else {
      const auto match=matches.next();
      if (matches.hasNext()) throw std::runtime_error("Multiple list assignments require the script editor");
      if (entries.isEmpty() && omitEmpty) {
         result.remove(match.capturedStart(),match.capturedLength());
         return result;
      }
      result.replace(match.capturedStart(),match.capturedLength(),replacement);
   }
   if (name=="Tank" && object.IsOfType("Thruster")) {
      // Mixture ratios belong to tanks, not list positions. New tanks start
      // with equal weighting and can be adjusted in the numeric cell editor.
      const auto oldNames=object.GetStringArrayParameter(id);
      const auto &ratios=object.GetRvectorParameter("MixRatio");
      QStringList values;
      for (const auto &entry:entries) {
         const auto found=std::find(oldNames.begin(),oldNames.end(),entry.toStdString());
         const auto position=std::distance(oldNames.begin(),found);
         values.append(QString::number(position<ratios.GetSize() ? ratios[position] : 1.0,'g',17));
      }
      // An explicit ratio edit belongs to the new ordered tank list. Otherwise
      // preserve existing associations by tank name as above.
      if (mixture) {
         values=mixture->trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         if (values.size()!=entries.size()) throw std::runtime_error("Enter one mixture ratio for each selected tank");
         for (auto &value:values) {
            bool valid=false; const double number=value.toDouble(&valid);
            if (!valid || !std::isfinite(number) || number<=0) throw std::runtime_error("Mixture ratios must be finite numbers greater than zero");
            value=QString::number(number,'g',17);
         }
      }
      const QString ratioKey=QString::fromStdString(object.GetName())+".MixRatio";
      const QRegularExpression ratioAssignment("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(ratioKey)+
         "[ \\t]*=[ \\t]*\\[[^;]*?\\][ \\t]*;",QRegularExpression::MultilineOption);
      const QString ratioValue="GMAT "+ratioKey+" = ["+values.join(" ")+"];";
      const auto ratio=ratioAssignment.match(result);
      if (ratio.hasMatch()) result.replace(ratio.capturedStart(),ratio.capturedLength(),ratioValue);
      else if (!entries.isEmpty()) result+=(result.endsWith('\n') ? "" : "\n")+ratioValue+"\n";
   }
   if (name=="Add" && (object.GetTypeName()=="OrbitView" || object.GetTypeName()=="GroundTrackPlot") && !object.IsParameterReadOnly(object.GetParameterID("DrawObject"))) {
      // DrawObject is positional in scripts. Preserve visibility by name when
      // entries move or disappear, and show newly added objects.
      const auto oldNames=object.GetStringArrayParameter(id);
      const auto flags=object.GetBooleanArrayParameter("DrawObject");
      QStringList visibility;
      for (const auto &entry:entries) {
         const auto found=std::find(oldNames.begin(),oldNames.end(),entry.toStdString());
         const auto position=static_cast<size_t>(std::distance(oldNames.begin(),found));
         visibility.append(position<flags.size() && !flags[position] ? "false" : "true");
      }
      const QString drawKey=QString::fromStdString(object.GetName())+".DrawObject";
      const QRegularExpression drawAssignment("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(drawKey)+
         "[ \\t]*=[ \\t]*\\[[^;]*?\\][ \\t]*;",QRegularExpression::MultilineOption);
      const QString drawValue="GMAT "+drawKey+" = ["+visibility.join(" ")+"];";
      const auto draw=drawAssignment.match(result);
      if (draw.hasMatch()) result.replace(draw.capturedStart(),draw.capturedLength(),drawValue);
      else result+=(result.endsWith('\n') ? "" : "\n")+drawValue+"\n";
   }
   return result;
}

void validateResourceProperties(GmatBase &object,bool replacingPolyhedron)
{
   if (object.IsOfType("ODEModel") && !replacingPolyhedron) for (int i=0;i<object.GetOwnedObjectCount();++i) {
      auto *force=object.GetOwnedObject(i);
      if (force && force->IsOfType("PolyhedronGravityModel")) {
         const auto body=force->GetStringParameter("CreateForceBody");
         auto *selected=Moderator::Instance()->GetConfiguredObject(body);
         if (!selected || !selected->IsOfType("CelestialBody")) throw std::runtime_error("Select an existing celestial body for polyhedron gravity.");
         force->Validate();
      }
   }
   validateBurnProperties(object);
   validateOrbitViewProperties(object);
   validateThrusterProperties(object);
   if (object.IsOfType("FileInterface") || object.IsOfType("ThrustHistoryFile")) {
      const bool history=object.IsOfType("ThrustHistoryFile");
      const auto filename=object.GetStringParameter(history ? "FileName" : "Filename");
      const QFileInfo file(QString::fromStdString(history ? GmatFileUtil::FindFile(filename) : filename));
      if (filename.empty() || !file.isFile() || !file.isReadable()) throw std::runtime_error("Choose an existing, readable input file.");
      return;
   }
   if (object.IsOfType("LibrationPoint")) {
      const auto primary=object.GetStringParameter("Primary"),secondary=object.GetStringParameter("Secondary");
      if (primary==secondary) throw std::runtime_error("Primary and secondary bodies must be different.");
      for (const auto &name:{primary,secondary}) {
         auto *point=Moderator::Instance()->GetConfiguredObject(name);
         if (!point || (!point->IsOfType("CelestialBody") && !point->IsOfType("Barycenter")) || name=="SolarSystemBarycenter" || point->GetName()!=name)
            throw std::runtime_error("Select celestial bodies or barycenters other than SolarSystemBarycenter for the libration point.");
      }
      return;
   }
   if (object.GetTypeName()=="GroundTrackPlot" || object.GetTypeName()=="GroundTrack") { validateGroundTrackTexture(object); return; }
   if (object.GetTypeName()=="XYPlot") {
      const auto x=QString::fromStdString(object.GetStringParameter("XVariable"));
      if ((object.GetBooleanParameter("ShowPlot") && x.isEmpty()) || (!x.isEmpty() && !ReportParameterDialog::isPlottableReference(x)))
         throw std::runtime_error("Select a numeric X plot parameter or a valid array element.");
      return;
   }
   if (!object.IsOfType("CoordinateSystem")) return;
   auto *axes=object.GetOwnedObject(0);
   if (!axes) return;
   if (axes->GetTypeName()=="LocalAlignedConstrained") {
      if (axes->GetStringParameter("ReferenceObject")==object.GetStringParameter("Origin"))
         throw std::runtime_error("Alignment reference must differ from the coordinate-system origin");
      if (axes->GetStringParameter("ConstraintCoordinateSystem")==object.GetName())
         throw std::runtime_error("A coordinate system cannot constrain itself");
      auto vector=[&](const std::string &prefix) {
         std::array<double,3> value{};
         for (int i=0;i<3;++i) value[i]=axes->GetRealParameter(prefix+"XYZ"[i]);
         const double length=std::hypot(value[0],value[1],value[2]);
         if (!std::isfinite(length) || length<1e-9) throw std::runtime_error(prefix+" must be finite and nonzero");
         return value;
      };
      const auto alignment=vector("AlignmentVector"),constraint=vector("ConstraintVector");
      vector("ConstraintReferenceVector");
      const double cross=std::hypot(alignment[1]*constraint[2]-alignment[2]*constraint[1],
         alignment[2]*constraint[0]-alignment[0]*constraint[2],alignment[0]*constraint[1]-alignment[1]*constraint[0]);
      if (!std::isfinite(cross) || cross<1e-9) throw std::runtime_error("Alignment and constraint vectors must be nonparallel and large enough to define axes");
      return;
   }
   if (axes->GetTypeName()!="ObjectReferenced") return;
   const auto primary=axes->GetStringParameter("Primary"),secondary=axes->GetStringParameter("Secondary");
   if (primary.empty() || secondary.empty() || primary==secondary)
      throw std::runtime_error("ObjectReferenced axes need distinct primary and secondary objects");
   QSet<QString> directions; int count=0;
   for (const auto *name:{"XAxis","YAxis","ZAxis"}) {
      auto value=QString::fromStdString(axes->GetStringParameter(name));
      if (value.isEmpty()) continue;
      ++count; value.remove('-'); directions.insert(value);
   }
   if (count!=2 || directions.size()!=2)
      throw std::runtime_error("Select exactly two different R, V or N directions; leave the third axis blank");
}

QStringList dataInterfaceFields(GmatBase &object)
{
   if (!object.IsOfType("DataInterface")) throw std::runtime_error("Select a data-interface resource.");
   // FileInterface creates its reader during initialization. Inspect a clone
   // so browsing its metadata does not alter the configured resource or read
   // data into a mission target.
   std::unique_ptr<GmatBase> copy(object.Clone());
   if (!copy) throw std::runtime_error("Cannot inspect this data interface.");
   validateResourceProperties(*copy);
   if (!copy->Initialize()) throw std::runtime_error("The data interface could not initialize its field list.");
   QStringList fields;
   const bool tvhf=copy->IsOfType("FileInterface") && copy->GetStringParameter("Format")=="TVHF_ASCII";
   for (const auto &field:copy->GetStringArrayParameter("SupportedFields")) {
      // TVHF also advertises vector components and frame/body metadata. Set
      // imports the whole CartesianState; selecting an individual component
      // does not assign it, while selecting frame/body metadata throws.
      if (!tvhf || field=="Epoch" || field=="CartesianState" || field=="Cr") fields.append(QString::fromStdString(field));
   }
   return fields;
}

QSet<QString> applyAttitudeProperties(GmatBase &spacecraft,const QMap<QString,QString> &values)
{
   QSet<QString> consumed;
   if (!spacecraft.IsOfType("Spacecraft")) return consumed;
   if (values.contains("Attitude")) {
      setResourceProperty(spacecraft,"Attitude",values.value("Attitude")); consumed.insert("Attitude");
   }
   auto *attitude=dynamic_cast<Attitude *>(spacecraft.GetOwnedObject(0));
   if (!attitude) return consumed;
   QSet<QString> names;
   for (int id=0;id<attitude->GetParameterCount();++id) {
      const auto name=QString::fromStdString(attitude->GetParameterText(id));
      if (name!="Epoch") names.insert(name);
   }
   // Selectors must precede dependent data. Submit state/rate as complete
   // vectors or matrices: scalar quaternion setters reject initialized clones.
   for (const auto &name:QStringList{"AttitudeDisplayStateType","AttitudeRateDisplayStateType","AttitudeCoordinateSystem","EulerAngleSequence"})
      if (values.contains(name)) { setResourceProperty(*attitude,name,values.value(name)); consumed.insert(name); }
   if (values.contains("AttitudeCoordinateSystem")) {
      const auto frameName=values.value("AttitudeCoordinateSystem").toStdString();
      auto *frame=Moderator::Instance()->GetConfiguredObject(frameName);
      if (!frame || !frame->IsOfType("CoordinateSystem")) throw std::runtime_error("Select an available attitude coordinate system.");
      attitude->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,frameName);
   }
   auto number=[&](const QString &name) {
      if (!values.contains(name)) return attitude->GetRealParameter(name.toStdString());
      if (attitude->IsParameterReadOnly(name.toStdString())) throw std::runtime_error((name+" is unavailable for this attitude model or representation.").toStdString());
      bool ok=false; const double value=values.value(name).toDouble(&ok);
      if (!ok || !std::isfinite(value)) throw std::runtime_error((name+" must be a finite number.").toStdString());
      consumed.insert(name); return value;
   };
   auto vector=[&](const QString &property,const QStringList &components) {
      bool changed=false; for (const auto &name:components) changed=changed || values.contains(name);
      if (!changed) return;
      Rvector vector(components.size());
      for (int i=0;i<components.size();++i) vector[i]=number(components[i]);
      attitude->SetRvectorParameter(property.toStdString(),vector);
   };
   vector("Quaternion",{"Q1","Q2","Q3","Q4"});
   vector("EulerAngles",{"EulerAngle1","EulerAngle2","EulerAngle3"});
   vector("MRPs",{"MRP1","MRP2","MRP3"});
   QStringList matrixNames; for (int r=1;r<=3;++r) for (int c=1;c<=3;++c) matrixNames.append(QString("DCM%1%2").arg(r).arg(c));
   bool matrixChanged=false; for (const auto &name:matrixNames) matrixChanged=matrixChanged || values.contains(name);
   if (matrixChanged) {
      Rmatrix matrix(3,3); for (int i=0;i<9;++i) matrix(i/3,i%3)=number(matrixNames[i]);
      attitude->SetRmatrixParameter("DirectionCosineMatrix",matrix);
   }
   vector("AngularVelocity",{"AngularVelocityX","AngularVelocityY","AngularVelocityZ"});
   vector("EulerAngleRates",{"EulerAngleRate1","EulerAngleRate2","EulerAngleRate3"});
   for (auto it=values.cbegin();it!=values.cend();++it) if (names.contains(it.key()) && !consumed.contains(it.key())) {
      setResourceProperty(*attitude,it.key(),it.value()); consumed.insert(it.key());
   }
   if (!consumed.isEmpty() && !attitude->Validate()) throw std::runtime_error("The attitude rejected these settings.");
   return consumed;
}

void setResourceProperty(GmatBase &object, const QString &name, const QString &value)
{
   if (object.IsOfType("Smoother") && name=="Filter") {
      auto *filter=Moderator::Instance()->GetConfiguredObject(value.trimmed().toStdString());
      if (!filter || !filter->IsOfType("SeqEstimator")) throw std::runtime_error("Select an existing sequential estimator for this smoother.");
      object.SetStringParameter("Filter",filter->GetName()); return;
   }
   const bool processType=object.IsOfType("ProcessNoiseModel") && name=="Type";
   const bool parameterModel=object.IsOfType("EstimatedParameter") && name=="Model";
   if (processType || parameterModel) {
      const auto type=GmatType::GetTypeId(processType ? "ProcessNoise" : "EstimatedParameterModel");
      const auto &choices=Moderator::Instance()->GetListOfFactoryItems(type);
      if (std::find(choices.begin(),choices.end(),value.toStdString())==choices.end()) throw std::runtime_error("Select a registered estimation model.");
      auto *old=object.GetOwnedObject(0);
      if (old && object.GetStringParameter(name.toStdString())==value.toStdString()) return;
      // The wrapper clones its model. Factory creation avoids installing a
      // temporary owned model in the mission's configured-object map.
      std::unique_ptr<GmatBase> model(FactoryManager::Instance()->CreateObject(type,value.toStdString(),""));
      if (!model) throw std::runtime_error("Cannot create the selected estimation model.");
      if (processType && old) model->SetStringParameter("CoordinateSystem",old->GetStringParameter("CoordinateSystem"));
      if (!object.SetRefObject(model.get(),type,"")) throw std::runtime_error("Cannot replace the estimation model.");
      return;
   }
   if (object.IsOfType("ProcessNoiseModel") || object.IsOfType("EstimatedParameter")) if (auto *model=object.GetOwnedObject(0)) {
      int id=-1; try { id=model->GetParameterID(name.toStdString()); } catch (BaseException &) {}
      if (id>=0 && !model->IsParameterReadOnly(id)) { setResourceProperty(*model,name,value); return; }
   }
   if (object.IsOfType("Imager") && name=="FieldOfView") {
      const auto selected=value.trimmed().toStdString();
      auto *fov=selected.empty() ? nullptr : Moderator::Instance()->GetConfiguredObject(selected);
      if (!selected.empty() && (!fov || !fov->IsOfType(Gmat::FIELD_OF_VIEW) || fov->GetName()!=selected)) throw std::runtime_error("Select an existing field of view, or leave it empty.");
      object.SetStringParameter("FieldOfView",selected); return;
   }
   if ((object.IsOfType("Simulator") || object.IsOfType("Estimator")) && name=="Propagator") {
      auto *propagator=Moderator::Instance()->GetConfiguredObject(value.trimmed().toStdString());
      if (!propagator || !propagator->IsOfType(Gmat::PROP_SETUP)) throw std::runtime_error("Select an existing default propagator.");
      object.SetStringParameter("Propagator",value.trimmed().toStdString()); return;
   }
   if (object.IsOfType("CoordinateSystem")) {
      // Even owned-axis edits must respect built-in coordinate-system protection.
      object.SetStringParameter("Axes",object.GetStringParameter("Axes"));
      if (name=="Axes") {
         if (object.GetStringParameter("Axes")==value.toStdString()) return;
         const auto &types=Moderator::Instance()->GetListOfFactoryItems(Gmat::AXIS_SYSTEM);
         if (std::find(types.begin(),types.end(),value.toStdString())==types.end()) throw std::runtime_error("Select a registered axis type");
         std::unique_ptr<AxisSystem> axes(Moderator::Instance()->CreateAxisSystem(value.toStdString(),"",0));
         if (!axes || !object.SetRefObject(axes.get(),Gmat::AXIS_SYSTEM,"")) throw std::runtime_error("Cannot replace axes");
         return;
      }
      if (auto *axes=object.GetOwnedObject(0)) {
         int id=-1; try { id=axes->GetParameterID(name.toStdString()); } catch (BaseException &) {}
         if (id>=0 && !axes->IsParameterReadOnly(id)) { setResourceProperty(*axes,name,value); return; }
      }
   }
   if (auto *setup=dynamic_cast<PropSetup *>(&object)) if (auto *propagator=setup->GetPropagator()) {
      int parameter=-1;
      try { parameter=propagator->GetParameterID(name.toStdString()); } catch (BaseException &) {}
      if (parameter>=0 && !propagator->IsParameterReadOnly(parameter)) { setResourceProperty(*propagator,name,value); return; }
   }
   QString leaf;
   if (auto *force=forcePropertyOwner(object,name,leaf)) { setResourceProperty(*force,leaf,value); return; }
   if (object.GetTypeName()=="ReportFile" && name=="Delimiter" &&
       (value.size()!=1 || value=="'" || value[0].unicode()>126 || (value[0].unicode()<32 && value!="\t")))
      throw std::runtime_error("Choose one delimiter character (space, tab or printable ASCII other than a quote)");
   auto id = object.GetParameterID(name.toStdString());
   // Spacecraft element names also have alias IDs outside the ordinary property
   // range. Use the writable displayed-element ID for validation and assignment.
   if (object.IsOfType("Spacecraft")) for (int element=1;element<=6;++element) {
      const auto elementId=object.GetParameterID("Element"+std::to_string(element));
      if (QString::fromStdString(object.GetParameterText(elementId))==name) { id=elementId; break; }
   }
   const bool arrayValues=object.GetTypeName()=="Array" && name=="RmatValue";
   const bool groundTexture=object.GetTypeName()=="GroundTrack" && name=="TextureMap";
   const bool ephemerisInput=object.IsOfType("Spacecraft") && name=="EphemerisName";
   if (object.IsParameterReadOnly(id) && !arrayValues && !groundTexture && !ephemerisInput && !externalForceSetting(object,name)) throw std::runtime_error("Property is read-only");
   bool valid = false;
   switch (object.GetParameterType(id)) {
   case Gmat::RVECTOR_TYPE:
   case Gmat::RMATRIX_TYPE: {
      const bool vector=object.GetParameterType(id)==Gmat::RVECTOR_TYPE;
      int rows=vector ? 1 : object.GetRmatrixParameter(id).GetNumRows();
      int columns=vector ? object.GetRvectorParameter(id).GetSize() : object.GetRmatrixParameter(id).GetNumColumns();
      const auto inputRows=value.trimmed().split(';');
      if (isResizableResourceVector(object,name)) {
         columns=inputRows.first().trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).size();
         if (columns<1 || columns>100000) throw std::runtime_error("Coefficient vectors need between 1 and 100000 finite values.");
      }
      if (arrayValues) {
         rows=inputRows.size();
         columns=inputRows.first().trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).size();
         if (rows<1 || columns<1 || rows>1000 || columns>1000 || rows*columns>1000000)
            throw std::runtime_error("Array dimensions must be positive and contain at most 1000000 cells");
      }
      if (inputRows.size()!=rows) throw std::runtime_error("Keep the existing number of rows; separate rows with semicolons");
      Rmatrix matrix(rows,columns);
      for (int r=0;r<rows;++r) {
         const auto cells=inputRows[r].trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         if (cells.size()!=columns) throw std::runtime_error("Keep the existing number of columns");
         for (int c=0;c<columns;++c) {
            const double number=cells[c].toDouble(&valid);
            if (!valid || !std::isfinite(number)) throw std::runtime_error("Every cell must contain a finite number");
            matrix(r,c)=number;
         }
      }
      if (vector) {
         Rvector values(columns);
         for (int c=0;c<columns;++c) values[c]=matrix(0,c);
         object.SetRvectorParameter(id,values);
      } else {
         if (arrayValues) static_cast<Array &>(object).SetSize(rows,columns);
         object.SetRmatrixParameter(id,matrix);
      }
      break;
   }
   case Gmat::REAL_TYPE: {
      const double number = value.toDouble(&valid);
      if (!valid || !std::isfinite(number)) throw std::runtime_error("Enter a finite number");
      object.SetRealParameter(id, number); break;
   }
   case Gmat::INTEGER_TYPE: {
      const auto number = value.toLongLong(&valid);
      if (!valid || number < std::numeric_limits<Integer>::min() || number > std::numeric_limits<Integer>::max())
         throw std::runtime_error("Enter an integer in range");
      object.SetIntegerParameter(id, static_cast<Integer>(number)); break;
   }
   case Gmat::UNSIGNED_INT_TYPE: {
      const auto number = value.toULongLong(&valid);
      if (!valid || value.trimmed().startsWith('-') || number > std::numeric_limits<UnsignedInt>::max())
         throw std::runtime_error("Enter a non-negative integer in range");
      object.SetUnsignedIntParameter(id, static_cast<UnsignedInt>(number)); break;
   }
   case Gmat::BOOLEAN_TYPE:
      if (value != "true" && value != "false") throw std::runtime_error("Select true or false");
      object.SetBooleanParameter(id, value == "true"); break;
   case Gmat::ON_OFF_TYPE:
      if (value != "On" && value != "Off") throw std::runtime_error("Select On or Off");
      if (!object.SetOnOffParameter(id, value.toStdString())) throw std::runtime_error("Property rejected value");
      break;
   case Gmat::COLOR_TYPE:
   case Gmat::STRING_TYPE:
   case Gmat::FILENAME_TYPE:
   case Gmat::OBJECT_TYPE:
   case Gmat::ENUMERATION_TYPE:
      if (!object.SetStringParameter(id, value.toStdString())) throw std::runtime_error("Property rejected value");
      break;
   default: throw std::runtime_error("This property requires a specialized editor");
   }
}
