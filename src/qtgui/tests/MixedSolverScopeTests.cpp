#include "MainWindow.hpp"
#include "Debugger.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly),"Mixed solver report unavailable");
   return file.readAll();
}
void write(const QString &path,const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Mixed solver evidence cannot be preserved");
}
void run(MainWindow &window)
{
   bool timedOut=false;
   QTimer watchdog;
   watchdog.setSingleShot(true);
   QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { timedOut=true; window.stopMission(); });
   watchdog.start(12000);
   const auto result=window.runMission();
   watchdog.stop();
   require(!timedOut,"Mixed solver mission exceeded its 12 second execution bound");
   if (result!=MainWindow::RunResult::Completed)
      throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString());
}
const PlotCurve &curve(const PlotModel &model,const QString &name)
{
   for (const auto &item:model.curves) if (item.name==name) return item;
   throw std::runtime_error("Mixed solver curve missing: "+name.toStdString());
}
using Path=std::vector<std::array<double,4>>;
using Paths=std::map<QString,Path>;
Paths paths(MainWindow &window,const QString &mode,const QString &capture)
{
   Paths result;
   std::ostringstream diagnostic;
   for (const auto &name:QStringList{"Orb","Ground","XY"}) {
      const auto model=window.plotReceiver()->model(name);
      require(bool(model),"Mixed solver display missing");
      for (const auto &item:model->curves) {
         if (model->kind!=PlotModel::Kind::XY && item.name!="Sat") continue;
         auto &path=result[name+"/"+item.name];
         diagnostic<<mode.toStdString()<<' '<<name.toStdString()<<' '<<item.name.toStdString()<<" points="<<item.points.size()<<'\n';
         for (const auto &point:item.points) {
            path.push_back({point.x,point.y,point.z,point.epoch});
            diagnostic<<point.x<<' '<<point.y<<' '<<point.z<<' '<<point.epoch<<" solver="<<point.solver<<" connect="<<point.connect<<'\n';
         }
         std::cout<<mode.toStdString()<<' '<<name.toStdString()<<' '<<item.name.toStdString()<<" points="<<item.points.size()<<'\n';
      }
      require(window.plotReceiver()->show(name),"Mixed solver display cannot open");
      QApplication::processEvents();
      auto *area=window.findChild<QMdiArea *>("workspace");
      auto *view=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget());
      require(view && !view->canvas()->captureImage().isNull(),"Mixed solver display cannot render");
      if (!capture.isEmpty()) require(view->canvas()->captureImage().save(capture+"."+mode+"."+name+".png"),"Mixed solver scene evidence cannot be saved");
   }
   if (!capture.isEmpty()) write(capture+"."+mode+".history.txt",QByteArray::fromStdString(diagnostic.str()));
   return result;
}
void samePath(const Path &actual,const Path &expected,const QString &name)
{
   if (actual.size()!=expected.size())
      throw std::runtime_error(name.toStdString()+" Current count="+std::to_string(expected.size())+" None count="+std::to_string(actual.size()));
   for (size_t i=0;i<actual.size();++i) for (int j=0;j<4;++j) {
      // Subscriber geodetic conversion can reuse rotating-frame caches. Reports
      // remain byte exact; the display comparison permits the existing 1e-8
      // degree/km bound, and 1e-10 days for epochs.
      const auto tolerance=j==3 ? 1e-10 : 1e-8;
      if (std::abs(actual[i][j]-expected[i][j])>=tolerance)
         throw std::runtime_error(name.toStdString()+" accepted point="+std::to_string(i)+" component="+std::to_string(j)+" differs from independently filtered None history");
   }
}
std::array<double,10> reportObjective(const QByteArray &bytes)
{
   // Phase, Alpha, Beta, Cost, elapsed, Cartesian XYZ, longitude, latitude.
   std::istringstream input(bytes.toStdString());
   std::vector<std::array<double,10>> rows;
   std::array<double,10> row;
   while (input>>row[0]>>row[1]>>row[2]>>row[3]>>row[4]>>row[5]>>row[6]>>row[7]>>row[8]>>row[9]) rows.push_back(row);
   require(input.eof() && rows.size()>10,"Mixed mission did not independently report multiple inner and outer solves");
   bool outerTrial=false,innerAccepted=false;
   for (const auto &value:rows) {
      if (value[0]==1 && std::abs(value[2]-3)<1e-6) {
         innerAccepted=true;
         if (std::abs(value[1]-2)>0.1) outerTrial=true;
      }
   }
   require(innerAccepted && outerTrial,"Mixed mission did not exercise an accepted inner targeter inside an unaccepted outer optimization");
   const auto final=rows.back();
   require(final[0]==3 && std::abs(final[1]-2)<1e-5 && std::abs(final[2]-3)<1e-6 && std::abs(final[3])<1e-10 && std::abs(final[4]-100)<1e-5,"Mixed report missed known Alpha=2 quadratic optimum, Beta=3 target or accepted elapsed 100 s");
   auto *alpha=Moderator::Instance()->GetInternalObject("Alpha");
   auto *beta=Moderator::Instance()->GetInternalObject("Beta");
   auto *cost=Moderator::Instance()->GetInternalObject("Cost");
   require(alpha && beta && cost && std::abs(alpha->GetRealParameter("Value")-2)<1e-5 && std::abs(beta->GetRealParameter("Value")-3)<1e-6 && std::abs(cost->GetRealParameter("Value"))<1e-10,"Mixed sandbox objects missed independently known goals");
   return final;
}
void endpoints(MainWindow &window,const std::array<double,10> &final)
{
   const auto orbit=window.plotReceiver()->model("Orb"),ground=window.plotReceiver()->model("Ground"),xy=window.plotReceiver()->model("XY");
   require(orbit && ground && xy,"Mixed final displays unavailable");
   const auto &op=curve(*orbit,"Sat").points;
   const auto &gp=curve(*ground,"Sat").points;
   const auto &xp=curve(*xy,"Sat.EarthMJ2000Eq.X").points;
   require(op.size()>3 && gp.size()>3 && xp.size()>3,"Mixed accepted histories are unexpectedly empty");
   const auto &last=op.back();
   require(!last.solver && std::abs(last.x-final[5])<1e-8 && std::abs(last.y-final[6])<1e-8 && std::abs(last.z-final[7])<1e-8,"Mixed orbit endpoint differs from independent complete Cartesian Report");
   require(!orbit->cameras.empty() && orbit->cameras.back().frame==last.frame,"Mixed camera lost its final accepted frame");
   for (int i=0;i<3;++i) require(std::abs(orbit->cameras.back().target[i]-final[i+5])<1e-8,"Mixed camera target differs from independent complete Cartesian Report");
   require(!gp.back().solver && std::abs(gp.back().x-final[8])<1e-8 && std::abs(gp.back().y-final[9])<1e-8,"Mixed ground endpoint differs from independent geodetic Report");
   require(std::abs(xp.back().x-final[4])<1e-5 && std::abs(xp.back().y-final[5])<1e-8,"Mixed XY endpoint differs from independent Report");
}
void acceptedGoals(MainWindow &window)
{
   const auto model=window.plotReceiver()->model("XY");
   require(bool(model),"Mixed accepted XY reference missing");
   size_t checkedAlpha=0,checkedBeta=0;
   for (const auto &point:curve(*model,"Alpha").points) if (point.x>20.00001) {
      ++checkedAlpha;
      if (std::abs(point.y-2)>=1e-5)
         throw std::runtime_error("None leaked an outer Yukon trial through an accepted inner DC: elapsed="+std::to_string(point.x)+" Alpha="+std::to_string(point.y)+", known outer accepted Alpha=2");
   }
   for (const auto &point:curve(*model,"Beta").points) if (point.x>40.00001) {
      ++checkedBeta;
      require(std::abs(point.y-3)<1e-6,"None leaked an inner DC trial into accepted mixed history");
   }
   require(checkedAlpha>=3 && checkedBeta>=3,"Mixed None accepted path did not cover the solved nested branch and post-solver propagation");
}
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtMixedSolverScopes");
   try {
      const bool vf13ad=argc==4 && QString::fromLocal8Bit(argv[3])=="--vf13ad";
      require(argc==2 || argc==3 || vf13ad,"Mixed solver check expects startup, optional evidence prefix and --vf13ad");
      TestSettings settings;
      QTemporaryDir files; require(files.isValid(),"Mixed solver fixture directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto capture=argc>=3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Mixed solver runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(editor,"Mixed solver script editor unavailable");
      const auto report=files.filePath("mixed-states.txt"),saved=files.filePath("Mixed scopes Δ.script");
      const QString setup=
         "Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329;\nSat.VZ = 0;\n"
         "Earth.NutationUpdateInterval = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\n"
         "Create Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
         "Create Variable Alpha Beta Cost Phase;\nAlpha = 1;\nBeta = 1;\nCost = 0;\nPhase = 0;\n"
         "Create Yukon Opt;\nOpt.MaximumIterations = 12;\nOpt.ShowProgress = false;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nOpt.ReportFile = '"+files.filePath("outer-yukon.txt")+"';\n"
         "Create DifferentialCorrector Inner;\nInner.MaximumIterations = 8;\nInner.ShowProgress = false;\nInner.ReportFile = '"+files.filePath("inner-dc.txt")+"';\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.SolverIterations = Current;\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 40000];\nOrb.ViewDirection = Sat;\n"
         "Create GroundTrack Ground;\nGround.Add = {Sat};\nGround.SolverIterations = Current;\n"
         "Create XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Alpha, Beta, Sat.EarthMJ2000Eq.X};\nXY.SolverIterations = Current;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\nValues.SolverIterations = All;\n";
      const QString reportCommand="Report Values Phase Alpha Beta Cost Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.Earth.Longitude Sat.Earth.Latitude;\n";
      QString source=setup+
         "BeginMissionSequence;\nPropagate 'before mixed solvers' P(Sat) {Sat.ElapsedSecs = 20};\n"
         "Optimize 'outer quadratic optimum' Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
         "Vary Opt(Alpha = 1, {Perturbation = 0.000001, Lower = 0, Upper = 3, MaxStep = 1});\nSat.X = 7000 + 100 * Alpha;\n"
         "Propagate 'outer trial propagation' P(Sat) {Sat.ElapsedSecs = 20};\n"
         "Target 'inner independent goal' Inner {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
         "Vary Inner(Beta = 1, {Perturbation = 0.01, Lower = 0, Upper = 5, MaxStep = 1});\nSat.Y = 100 * Beta;\n"
         "Propagate 'inner trial propagation' P(Sat) {Sat.ElapsedSecs = 20};\nAchieve Inner(Beta = 3, {Tolerance = 0.000001});\nPhase = 1;\n"+reportCommand+
         "EndTarget; % preserve inner label and nesting\nPropagate 'after inner targeter' P(Sat) {Sat.ElapsedSecs = 20};\n"
         "Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);\nPhase = 2;\n"+reportCommand+
         "EndOptimize; % preserve outer label and nesting\nPropagate 'after mixed solvers' P(Sat) {Sat.ElapsedSecs = 20};\nPhase = 3;\n"+reportCommand;
      if (vf13ad) {
         source.replace("Create Yukon Opt;", "Create VF13ad Opt;");
         source.replace("Opt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;",
                        "Opt.Tolerance = 0.000000001;\nOpt.FeasibilityTolerance = 0.000000001;");
      }
      if (!capture.isEmpty()) write(capture+".authored.script",source.toUtf8());
      QElapsedTimer elapsed; elapsed.start();
      editor->setPlainText(source);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==source,"Mixed authored Unicode mission did not reopen byte exactly");
      run(window);
      const auto expectedReport=read(report);
      auto *optimizer=dynamic_cast<Solver *>(Moderator::Instance()->GetInternalObject("Opt"));
      require(optimizer && optimizer->IsSolverInternal(),"Mixed fixture did not exercise the expected internal optimizer path");
      const auto final=reportObjective(expectedReport);
      if (!capture.isEmpty()) write(capture+".Current.state.txt",expectedReport);
      endpoints(window,final);
      const auto current=paths(window,"Current",capture);
      const auto noneSource=QString(source).replace("SolverIterations = Current;","SolverIterations = None;");
      editor->setPlainText(noneSource); run(window);
      const auto noneReport=read(report);
      if (!capture.isEmpty()) write(capture+".None.state.txt",noneReport);
      const auto none=paths(window,"None",capture);
      require(noneReport==expectedReport,"Mixed Current and None display modes changed independent full iteration Reports");
      reportObjective(noneReport); endpoints(window,final);
      acceptedGoals(window);
      for (const auto &entry:none) if (entry.first.startsWith("Ground/") || entry.first.startsWith("XY/")) {
         require(current.count(entry.first)>0,"Current mixed accepted curve absent");
         samePath(entry.second,current.at(entry.first),entry.first);
      }

      // A Stop at the first inner publishing command tests this genuinely mixed
      // optimizer/targeter scope, rather than repeating the nested-DC Stop suite.
      editor->setPlainText(source); require(window.buildScript(),"Mixed Stop fixture did not build");
      const auto snapshot=window.missionSnapshot(); int inner=-1;
      for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'inner trial propagation'")) inner=i;
      require(inner>=0 && window.setBreakpoint(inner,true),"Mixed inner publishing breakpoint unavailable");
      Debugger *debugger=nullptr;
      for (auto *child:window.children()) if (auto *candidate=dynamic_cast<Debugger *>(child)) debugger=candidate;
      require(debugger,"Mixed Stop debugger unavailable");
      bool stoppedAtInner=false,timedOut=false; int observedIndex=-1;
      QString observedCommand; QTimer poll,watchdog;
      poll.setInterval(10); watchdog.setSingleShot(true);
      QObject::connect(&poll,&QTimer::timeout,&window,[&] {
         if (!debugger->isWaiting()) return;
         auto *dialog=window.findChild<QDialog *>("debuggerDialog");
         auto *command=dialog ? dialog->findChild<QPlainTextEdit *>("debuggerCommand") : nullptr;
         observedIndex=dialog ? dialog->property("commandIndex").toInt() : -1;
         observedCommand=command ? command->toPlainText() : QString();
         stoppedAtInner=observedIndex==inner && observedCommand.contains("'inner trial propagation'");
         window.stopMission();
      });
      QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { timedOut=true; window.stopMission(); });
      poll.start(); watchdog.start(5000);
      const auto stopped=window.debugMission(); poll.stop(); watchdog.stop();
      if (!capture.isEmpty()) write(capture+".Stopped.boundary.txt",
         QString("expected command index=%1\nobserved command index=%2\n%3\n").arg(inner).arg(observedIndex).arg(observedCommand).toUtf8());
      require(stoppedAtInner && !timedOut && stopped==MainWindow::RunResult::Stopped && !window.isRunning() && !debugger->isActive() && !debugger->isWaiting() && window.statusBar()->currentMessage()=="Mission stopped","Mixed inner Stop did not release observers/debugger or retain stopped classification");
      require(editor->toPlainText()==source && SolverBranchCommand::GetPlotRunState(Gmat::RUNNING)==Gmat::RUNNING && window.setBreakpoint(inner,false),"Mixed inner Stop leaked solver plot scope, breakpoint or changed source");
      // This breakpoint precedes the first Report command. ReportFile starts
      // a fresh run, so absence of a partial file is the expected observation.
      require(!QFileInfo::exists(report),"Mixed inner Stop executed a later Report command");
      if (!capture.isEmpty()) write(capture+".Stopped.partial-state.txt",QByteArray());
      run(window);
      const auto recovered=read(report);
      require(recovered==expectedReport && editor->toPlainText()==source,"Mixed Stop recovery changed independent full Report or authored source");
      reportObjective(recovered); endpoints(window,final);
      const auto recovery=paths(window,"RecoveredCurrent",capture);
      for (const auto &entry:current) {
         require(recovery.count(entry.first)>0,"Mixed recovered curve absent");
         samePath(recovery.at(entry.first),entry.second,entry.first+" recovery");
      }
      require(elapsed.elapsed()<45000,"Mixed scope check exceeded its 45 second aggregate bound");
      std::cout<<(vf13ad ? "VF13ad: " : "Yukon: ")<<"PASS: new mixed optimizer / inner DifferentialCorrector hierarchy; independently known Alpha=2 quadratic optimum and Beta=3 target; exact full Cartesian/geodetic Reports, Current cleanup versus None accepted filtering, all three final displays/camera, Unicode save/reopen, bounded inner breakpoint Stop and clean complete recovery. No previous solver mode matrices repeated. Elapsed="<<elapsed.elapsed()/1000.0<<" s\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
