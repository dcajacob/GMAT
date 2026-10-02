#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "OrbitCamera.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Unplotted-camera report missing"); return file.readAll(); }
static osg::Vec3d xyz(const std::array<double,3> &value) { return {value[0],value[1],value[2]}; }
static const PlotCurve *curve(const PlotModel &model,const QString &name) { for (const auto &item:model.curves) if (item.name==name) return &item; return nullptr; }
static double epoch(const PlotModel &model,quint64 frame) {
   const auto *sat=curve(model,"Sat"); require(sat,"Camera epoch has no spacecraft history");
   for (const auto &point:sat->points) if (point.frame==frame) return point.epoch;
   throw std::runtime_error("Camera frame has no sampled epoch");
}
static OrbitCameraBasis camera(const PlotModel &model,int selected,quint64 frame) {
   auto copy=model; copy.selectedCamera=selected;
   return orbitCamera(copy,frame,0,0,7000,1.5);
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtUnplottedBodyCameras");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Unplotted-body focused check requires offscreen and a startup path");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Unplotted-body temporary directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("unplotted camera state Δ.txt"),saved=files.filePath("unplotted body cameras Δ.script");
      const QString resources="% preserve camera-independent state α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 10;\nProp.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\n";
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60}; % retained numerical mission\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const QString views="Create OpenFramesView SunPrimary;\nSunPrimary.ViewFrame = Sun;\nSunPrimary.ViewTrajectory = Off;\nSunPrimary.InertialFrame = Off;\nSunPrimary.SetDefaultLocation = Off;\nSunPrimary.SetCurrentLocation = Off;\nSunPrimary.FOVy = 45;\nCreate OpenFramesView LunaSecondary;\nLunaSecondary.ViewFrame = Luna;\nLunaSecondary.ViewTrajectory = Off;\nLunaSecondary.InertialFrame = Off;\nLunaSecondary.SetDefaultLocation = Off;\nLunaSecondary.SetCurrentLocation = Off;\nLunaSecondary.FOVy = 45;\nCreate OpenFramesView SunSecondary;\nSunSecondary.ViewFrame = Sun;\nSunSecondary.ViewTrajectory = Off;\nSunSecondary.InertialFrame = Off;\nSunSecondary.SetDefaultLocation = Off;\nSunSecondary.SetCurrentLocation = Off;\nSunSecondary.FOVy = 45;\nCreate OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nDisplay.View = {SunPrimary, LunaSecondary, SunSecondary};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.EnableStars = Off;\n";
      MainWindow window; window.show(); require(window.initialize(startup),"Unplotted-body runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Unplotted-body script editor unavailable");
      editor->setPlainText(resources+mission); require(window.runMission()==MainWindow::RunResult::Completed,"Independent camera-free mission failed"); const auto expected=read(report);
      const auto converted=convertOpenFramesViews(resources+views+mission); require(converted.error.isEmpty() && converted.script.endsWith(mission),"Unplotted-body OF conversion changed/rejected mission");
      editor->setPlainText(converted.script); require(window.buildScript(),"Real SpacePoint outside Add rejected as an automatic body camera");
      const auto originalAdd=Moderator::Instance()->GetConfiguredObject("Display")->GetStringArrayParameter("Add");
      require(originalAdd==StringArray({"Sat","Earth"}),"Camera conversion changed plotted object membership");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed,"Unplotted-body save/reopen/execution failed");
      require(editor->toPlainText()==converted.script && read(report)==expected,"Unplotted body camera changed exact saved source or independent state report");
      auto retained=window.plotReceiver()->model("Display"); require(retained && retained->cameraViews.size()==3 && retained->cameras.size()>2,"Unplotted-body histories not retained");
      const auto *sun=curve(*retained,"Sun"); require(sun && sun->radius>600000 && !sun->drawsContent(),"Hidden Sun reference radius/history lost or became visible");
      require(!curve(*retained,"Luna"),"Secondary camera added Luna to drawn/retained plot objects");
      require(retained->cameraViews[1].automaticRadius>1700 && retained->cameraViews[1].automaticRadius<1800,"Unplotted secondary Luna fell back to unit radius");
      auto *solar=Moderator::Instance()->GetSolarSystemInUse();
      for (int selected:{0,1,2}) {
         const auto &history=selected ? retained->cameraViews[selected].cameras : retained->cameras;
         require(history.size()==retained->cameras.size(),"Unplotted secondary camera sample history incomplete");
         auto *body=solar->GetBody(selected==1 ? "Luna" : "Sun"); const auto radius=body->GetEquatorialRadius();
         for (const auto *sample:{&history.front(),&history.back()}) {
            const auto state=body->GetMJ2000State(epoch(*retained,sample->frame));
            const auto reference=osg::Vec3d(state[0],state[1],state[2]);
            require((xyz(sample->target)-reference).length()<1e-7,"Unplotted body camera target differs from epoch ephemeris");
            const auto view=camera(*retained,selected,sample->frame);
            require((view.target-reference).length()<1e-7 && std::abs(view.distance-radius/std::sin(45.0*3.14159265358979323846/360))<1e-7,"Unplotted body automatic framing ignored captured position/physical radius");
         }
         require((xyz(history.front().target)-xyz(history.back().target)).length()>10,"Unplotted replay fixture did not distinguish epochs");
      }
      const auto metadata=qtCameraSettings(converted.script); auto invalid=metadata;
      invalid["Display"].objectAxes["Sun"]=true; bool rejected=false;
      try { QtPlotReceiver::validateCameraReferences(invalid); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Relaxed camera membership also accepted an undrawn object flag");
      invalid=metadata; invalid["Display"].views[0].automaticBody="MissingBody"; rejected=false;
      try { QtPlotReceiver::validateCameraReferences(invalid); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Unknown automatic body camera accepted");
      QtPlotReceiver::validateCameraReferences(metadata);
      const auto first=camera(*retained,1,retained->cameraViews[1].cameras.front().frame),last=camera(*retained,1,retained->frame);
      editor->setPlainText("Create Spacecraft Replacement;\nBeginMissionSequence;\n"); require(window.buildScript(),"Replacement mission failed while retaining camera model");
      const auto replay=camera(*retained,1,retained->cameraViews[1].cameras.front().frame),latest=camera(*retained,1,retained->frame);
      require((replay.target-first.target).length()==0 && replay.distance==first.distance && (latest.target-last.target).length()==0 && latest.distance==last.distance,"Retained camera replay depended on discarded engine state");
      std::cout<<"PASS: real primary/secondary Sun and unplotted Luna cameras, unchanged Add/visibility/source and exact independent state report, epoch positions/physical radii, unknown body/object flag rejection and retained replay after engine replacement; offscreen only.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
