#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "OrbitView.hpp"
#include "GroundTrackPlot.hpp"
#include "PlotInterface.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static const PlotCurve &curve(const PlotModel &model,const QString &name)
{
   for (auto it=model.curves.cbegin();it!=model.curves.cend();++it) if (it->name==name) return it.value();
   throw std::runtime_error(("Missing curve "+name).toStdString());
}
static double radius(const PlotPoint &point) { return std::hypot(point.x,point.y,point.z); }
static void point(const PlotPoint &actual,double epoch,double x,double y,double z)
{
   require(std::abs(actual.epoch-epoch)<1e-12 &&
      std::hypot(actual.x-x,actual.y-y,actual.z-z)<1e-7,"Displayed sample differs from independent supplied/reported state");
}
static QString read(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Independent state report unavailable");
   return QString::fromUtf8(file.readAll());
}
static QVector<std::array<double,8>> reportRows(const QString &path)
{
   QVector<std::array<double,8>> result;
   for (const auto &line:read(path).trimmed().split('\n')) {
      const auto fields=line.simplified().split(' '); require(fields.size()==8,"Unexpected independent report columns");
      std::array<double,8> row{};
      for (int i=0;i<8;++i) { bool ok=false; row[i]=fields[i].toDouble(&ok); require(ok && std::isfinite(row[i]),"Nonfinite independent state report"); }
      result.append(row);
   }
   return result;
}
class CountingSpacecraft : public Spacecraft
{
public:
   explicit CountingSpacecraft(const std::string &name) : Spacecraft(name) {}
   int stateQueries=0;
   const Rvector6 GetMJ2000State(const A1Mjd &) override {
      ++stateQueries; return Rvector6(1000,0,0,0,0,0);
   }
};
static std::string availability(double epoch,bool a,bool b)
{
   std::ostringstream action;
   action<<"OrbitDataAvailability:"<<std::setprecision(17)<<epoch<<" 2 \"A\" "<<a<<" \"B\" "<<b;
   return action.str();
}

// Exercise each real overriding CURRENT replay path with independently supplied
// publications, without an unrelated solver convergence/numerical matrix.
template<class Plot> class CurrentReplay : public Plot
{
public:
   explicit CurrentReplay(const std::string &name) : Plot(name) {
      this->mScCount=2; this->mScNameArray={"A","B"}; this->mSolverIterOption=this->SI_CURRENT;
      this->mScXArray.assign(2,0); this->mScYArray.assign(2,0); this->mScZArray.assign(2,0);
      this->mScVxArray.assign(2,0); this->mScVyArray.assign(2,0); this->mScVzArray.assign(2,0);
      this->mScPrevDataPresent.assign(2,false); this->mScPrevEpoch.assign(2,0);
      this->mScPrevX.assign(2,0); this->mScPrevY.assign(2,0); this->mScPrevZ.assign(2,0);
      this->mScPrevVx.assign(2,0); this->mScPrevVy.assign(2,0); this->mScPrevVz.assign(2,0);
   }
   void sample(double epoch,const std::string &object,double x,double y,double z) {
      this->theDataLabels={{"Epoch",object+".X",object+".Y",object+".Z",object+".Vx",object+".Vy",object+".Vz"}};
      const Real data[]={epoch,x,y,z,0,0,0}; this->runstate=Gmat::SOLVING;
      require(this->BufferOrbitData(data,7)==2,"CURRENT sample bypassed its production buffer");
   }
   void replay() {
      // Avoid the separate trial-clear action so the replayed samples can be
      // inspected. Their actual solver flags remain true; no accepted claim.
      this->runstate=Gmat::SOLVEDPASS; require(this->UpdateSolverData(),"CURRENT override replay failed");
      require(this->mCurrDataAvailabilityArray.empty() && this->mDataAvailabilityContext.empty(),"CURRENT override left availability buffered after replay");
   }
};

template<class Plot> static void currentReplay(QtPlotReceiver &receiver,const std::string &name,bool ground)
{
   receiver.SetViewType(ground ? GmatPlot::GROUND_TRACK_PLOT : GmatPlot::TRAJECTORY_PLOT);
   require(receiver.CreateGlPlotWindow(name,"",0,0,.4,.4,false,0),"CURRENT replay display failed");
   receiver.SetGlObject(name,{"A","B"},{nullptr,nullptr});
   CurrentReplay<Plot> plot(name);
   plot.sample(31000,"A",100,0,0); plot.sample(31000.001,"B",0,200,0); plot.sample(31000.002,"A",300,0,0);
   auto model=receiver.model(QString::fromStdString(name)); require(model!=nullptr,"CURRENT replay model missing");
   require(curve(*model,"A").points.empty() && curve(*model,"B").points.empty(),"CURRENT samples appeared before replay");
   plot.replay();
   require(curve(*model,"A").points.size()==2 && curve(*model,"B").points.size()==1,"CURRENT replay appended absent spacecraft placeholders");
   const auto last=curve(*model,"A").points.back();
   if (ground) point(last,31000.002,0,0,0); else point(last,31000.002,300,0,0);
   require(!last.connect && last.solver,"CURRENT replay lost absence gap or changed trial identity");
   // The next replay must start with the newly buffered mask, not stale index0.
   plot.sample(31000.003,"B",0,400,0); plot.replay();
   require(curve(*model,"A").points.size()==2 && curve(*model,"B").points.size()==2,"Subsequent CURRENT replay reused stale availability");
   if (ground) point(curve(*model,"B").points.back(),31000.003,90,0,0);
   else point(curve(*model,"B").points.back(),31000.003,0,400,0);
   require(!curve(*model,"B").points.back().connect,"CURRENT B return joined across its missing publication");
}

static void receiverContract(QtPlotReceiver *restore)
{
   QMdiArea area; QtPlotReceiver receiver(&area); PlotInterface::SetPlotReceiver(&receiver);
   struct Reset { QtPlotReceiver *prior; ~Reset() { PlotInterface::SetPlotReceiver(prior); } } reset{restore};
   receiver.SetViewType(GmatPlot::TRAJECTORY_PLOT);
   require(receiver.CreateGlPlotWindow("Protocol","",0,0,.4,.4,false,0),"Protocol display failed");
   receiver.SetGlObject("Protocol",{"A","B"},{nullptr,nullptr});
   auto model=receiver.model("Protocol"); require(model!=nullptr,"Protocol model unavailable");
   const auto publish=[&](double epoch,double a,double b) {
      require(receiver.UpdateGlPlot("Protocol","",{"A","B"},epoch,{a,b},{0,0},{0,0},{},{},{},{},{},false,0,false,true,false),"Protocol publication failed");
   };
   require(receiver.TakeGlAction("Protocol",availability(31000,true,true)),"Valid availability rejected"); publish(31000,100,200);
   require(receiver.TakeGlAction("Protocol",availability(31000.001,false,true)),"Absent availability rejected"); publish(31000.001,0,210);
   require(curve(*model,"A").points.size()==1 && curve(*model,"A").points.back().x==100 && curve(*model,"A").breakNext,"Explicit absence moved the last object or failed to break the line");
   // No metadata: legacy direct callbacks accept an exact valid origin. This
   // also proves the previous missing flag was consumed by one publication.
   publish(31000.002,0,220);
   require(curve(*model,"A").points.size()==2 && radius(curve(*model,"A").points.back())==0 && !curve(*model,"A").points.back().connect,"One-shot absence leaked or valid zero was rejected");
   receiver.TakeGlAction("Protocol",availability(31000.003,false,true)); publish(31000.004,0,230);
   require(curve(*model,"A").points.size()==3,"Stale-epoch metadata hid a valid origin");
   receiver.TakeGlAction("Protocol",availability(31000.005,false,true));
   require(!receiver.TakeGlAction("Protocol","OrbitDataAvailability:31000.005 2 \"A\" 0 \"A\" 1"),"Malformed duplicate mask accepted"); publish(31000.005,0,240);
   require(curve(*model,"A").points.size()==4,"Malformed replacement left a prior mask armed");
   require(!receiver.TakeGlAction("Protocol","OrbitDataAvailability:31000.006 2 \"A\" 0 \"B\" 1 trailing"),"Trailing malformed metadata accepted");
   receiver.TakeGlAction("Protocol",availability(31000.006,false,true)); receiver.TakeGlAction("Protocol","ClearData"); publish(31000.006,0,250);
   require(curve(*model,"A").points.size()==1,"ClearData left availability armed");
   receiver.TakeGlAction("Protocol",availability(31000.007,false,true)); receiver.TakeGlAction("Protocol","ClearSolverData"); publish(31000.007,0,260);
   require(curve(*model,"A").points.size()==2,"ClearSolverData left availability armed");
   receiver.TakeGlAction("Protocol",availability(31000.008,false,true)); receiver.TakeGlAction("Protocol","ClearObjects");
   receiver.SetGlObject("Protocol",{"A","B"},{nullptr,nullptr}); publish(31000.008,0,270);
   require(curve(*model,"A").points.size()==1,"ClearObjects left availability armed");

   // Metadata-only preparation must preserve the real mask and cannot publish
   // a named camera using a different spacecraft absent from the pose subset.
   CountingSpacecraft a("A"); Spacecraft b("B");
   QtCameraSetting settings; settings.segmentFrame="B.Leg"; settings.lookAtRotation=true;
   receiver.cameraSettings["Protocol"]=settings;
   receiver.SetGl3dViewOption("Protocol",&b,nullptr,&a,1,{0,0,0},{0,-1000,0},{0,0,0},"Z",false,true,false);
   receiver.TakeGlAction("Protocol","SetOrbitProvider:99 0 1 Leg"); receiver.TakeGlAction("Protocol","BeginOrbitArcSample");
   receiver.TakeGlAction("Protocol",availability(31000.009,false,true));
   require(receiver.TakeGlAction("Protocol","OrbitArcPose:B 31000.009 0 280 0 1 0 0 0 1 0 0 0 1 1 0 0 0 1 0 0 0 1"),"Named pose rejected");
   require(receiver.TakeGlAction("Protocol","PrepareOrbitArcCameras"),"Named camera preparation failed");
   receiver.TakeGlAction("Protocol","FinalizeOrbitArc");
   require(model->cameras.empty() && a.stateQueries==0,"Named camera queried/finalized an absent spacecraft despite a finite available fallback");
   const auto before=curve(*model,"A").points.size(); publish(31000.009,0,280);
   require(curve(*model,"A").points.size()==before,"Arc-camera metadata consumed the real absence mask");

   // Same raw-buffer/replay contract applies to both production overrides.
   currentReplay<OrbitView>(receiver,"CurrentOrbit",false);
   currentReplay<GroundTrackPlot>(receiver,"CurrentGround",true);
   receiver.TakeGlAction("CurrentGround",availability(32000,false,true));
   receiver.TakeGroundTrackAction("CurrentGround","Reset");
   require(receiver.UpdateGlPlot("CurrentGround","",{"A","B"},32000,{100,0},{0,100},{0,0},{},{},{},{},{},false,0,false,true,false),"Ground reset publication failed");
   require(curve(*receiver.model("CurrentGround"),"A").points.size()==1,"Ground reset left availability armed");
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtOrbitDataAvailability");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Availability regression requires offscreen startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Availability fixture directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Availability runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Availability script editor missing");
      const auto report=files.filePath("sequential states.txt");
      const QString spacecraft="Create Spacecraft SatA SatB;\nSatA.CoordinateSystem = EarthMJ2000Eq;\nSatA.DisplayStateType = Cartesian;\nSatA.X = 7000;\nSatA.Y = 0;\nSatA.Z = 0;\nSatA.VX = 0;\nSatA.VY = 7.54605329010754;\nSatA.VZ = 0;\nSatB.CoordinateSystem = EarthMJ2000Eq;\nSatB.DisplayStateType = Cartesian;\nSatB.X = 8000;\nSatB.Y = 0;\nSatB.Z = 0;\nSatB.VX = 0;\nSatB.VY = 7.058686508480171;\nSatB.VZ = 0;\n";
      const QString resources="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\nCreate CoordinateSystem AFrame;\nAFrame.Origin = SatA;\nAFrame.Axes = MJ2000Eq;\nCreate OrbitView Shared Centered;\nShared.Add = {SatA, SatB, Earth};\nShared.CoordinateSystem = EarthMJ2000Eq;\nShared.UpdatePlotFrequency = 1;\nCentered.Add = {SatA, SatB};\nCentered.CoordinateSystem = AFrame;\nCentered.ViewPointReference = SatA;\nCentered.ViewPointVector = [0 -15000 5000];\nCentered.UpdatePlotFrequency = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\n";
      const QString state="Report Values SatA.A1ModJulian SatA.EarthMJ2000Eq.X SatA.EarthMJ2000Eq.Y SatA.EarthMJ2000Eq.Z SatB.A1ModJulian SatB.EarthMJ2000Eq.X SatB.EarthMJ2000Eq.Y SatB.EarthMJ2000Eq.Z;\n";
      const auto source=spacecraft+resources+"BeginMissionSequence;\nPropagate 'ALeg' P(SatA) {SatA.ElapsedSecs = 37};\n"+state+"Propagate 'BLeg' P(SatB) {SatB.ElapsedSecs = 19};\n"+state+"Propagate 'ALater' P(SatA) {SatA.ElapsedSecs = 61};\n"+state;
      auto *receiver=window.plotReceiver();
      editor->setPlainText(source); bool timeout=false; QTimer watchdog; watchdog.setSingleShot(true);
      QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { timeout=true; window.stopMission(); }); watchdog.start(10000);
      const auto result=window.runMission(); watchdog.stop();
      require(!timeout && result==MainWindow::RunResult::Completed,"Sequential availability mission failed or exceeded bound");
      require(editor->toPlainText()==source,"Availability metadata rewrote mission source");
      const auto rows=reportRows(report);
      std::cout<<"Sequential independent report rows="<<rows.size()<<'\n';
      require(rows.size()==3,"Sequential independent report does not contain exactly three rows");
      for (int i=0;i<4;++i) require(rows[0][i]==rows[1][i],"Independent report changed A during B-only propagation");
      auto model=receiver->model("Shared"); require(model!=nullptr,"Final shared model missing");
      const auto &a=curve(*model,"SatA"),&b=curve(*model,"SatB");
      require(!a.points.empty() && !b.points.empty(),"Sequential actual histories are empty");
      const auto providers=[](const PlotCurve &value) {
         QStringList result; for (const auto &sample:value.points) if (!result.contains(sample.provider)) result.append(sample.provider);
         return result.join(",").toStdString();
      };
      std::cout<<"SatA samples="<<a.points.size()<<" providers="<<providers(a)
               <<"; SatB samples="<<b.points.size()<<" providers="<<providers(b)<<'\n';
      // changed() is a structural window notification, not a publication hook.
      // The renderer/replay selects the last real object sample at/before each
      // B frame. Inspect that exact retained-history contract after the run.
      size_t bFrames=0;
      for (const auto &sample:b.points) if (sample.provider=="BLeg") {
         const PlotPoint *retained=nullptr;
         for (auto it=a.points.rbegin();it!=a.points.rend();++it) if (it->frame<=sample.frame) { retained=&*it; break; }
         require(retained,"A has no retained pose at a B publication frame");
         point(*retained,rows[0][0],rows[0][1],rows[0][2],rows[0][3]); ++bFrames;
      }
      require(bFrames>0,"BLeg provider publication frames were not retained");
      point(a.points.back(),rows[2][0],rows[2][1],rows[2][2],rows[2][3]);
      point(b.points.back(),rows[1][4],rows[1][5],rows[1][6],rows[1][7]);
      bool resumed=false;
      for (const auto &sample:a.points) {
         require(radius(sample)>6000 && sample.provider!="BLeg","A history contains an absent zero/sample");
         if (!resumed && sample.provider=="ALater") { require(!sample.connect,"A return joined across B-only publication"); resumed=true; }
      }
      require(resumed,"A resumed publication missing");
      for (const auto &sample:b.points) require(radius(sample)>7000 && sample.provider=="BLeg","B history contains an absent sample");
      auto centered=receiver->model("Centered"); require(centered && !curve(*centered,"SatA").points.empty(),"Spacecraft-centered origin samples were rejected");
      for (const auto &sample:curve(*centered,"SatA").points) require(radius(sample)<1e-7,"Spacecraft-centered origin did not remain valid");
      receiverContract(receiver);
      std::cout<<"PASS sequential A/B/A publication retains last real state, breaks missing histories and matches independent Reports; valid spacecraft-centered origins; both production CURRENT replay overrides preserve masks and clear subsequent replay state; one-shot/stale/malformed/data-object-solver-ground reset and named-camera preparation guards. No host/pixel/solver-convergence claim.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"Availability check failed: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"Availability check failed: "<<error.what()<<'\n'; return 1; }
}
