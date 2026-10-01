#include "MainWindow.hpp"
#include "SolverBranchCommand.hpp"
#include "FileManager.hpp"
#include "Debugger.hpp"
#include "ResourceEditor.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QSlider>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <vector>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Solver report unavailable"); return file.readAll(); }
static const PlotCurve &curve(const PlotModel &model) {
   for (const auto &entry:model.curves) if (entry.name=="Sat" || model.kind==PlotModel::Kind::XY) return entry;
   throw std::runtime_error("Solver spacecraft curve missing");
}
static PlotWidget *viewer(MainWindow &window,const QString &name) {
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("plotName")==name) return dynamic_cast<PlotWidget *>(child->widget());
   throw std::runtime_error("Solver viewer missing");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtSolverPlots");
   const bool optimizer=argc>=3 && QString::fromLocal8Bit(argv[2])=="--optimizer";
   const bool cleanup=argc>=3 && QString::fromLocal8Bit(argv[2])=="--nested-cleanup";
   const bool nested=cleanup || (argc>=3 && QString::fromLocal8Bit(argv[2])=="--nested");
   if (argc!=2 && argc!=3 && !((optimizer || nested) && argc==4)) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : argc==3 && !optimizer && !nested ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Solver fixture directory unavailable");
      MainWindow window; window.show(); require(window.initialize(startup),"Solver plot runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("solver-state.txt"),saved=files.filePath("Solver ü.script");
      QString source="Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329;\nSat.VZ = 0;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\n"
         "Create Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 10;\nProp.MinStep = 10;\nProp.MaxStep = 10;\n"
         "Create DifferentialCorrector DC;\nDC.ReportStyle = Normal;\nDC.ReportFile = '"+files.filePath("dc.txt")+"';\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.SolverIterations = All;\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Sat;\n"
         "Create GroundTrack Ground;\nGround.Add = {Sat};\nGround.SolverIterations = All;\n"
         "Create XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Sat.EarthMJ2000Eq.X};\nXY.SolverIterations = All;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nValues.WriteHeaders = false;\nValues.SolverIterations = All;\n"
         "BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 20};\n"
         "Target 'retain solver label' DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
         "Vary DC(Sat.X = 7000, {Perturbation = 1, Lower = 6000, Upper = 8000, MaxStep = 100});\n"
         "Propagate Prop(Sat) {Sat.ElapsedSecs = 140};\n"
         "Achieve DC(Sat.X = 7100, {Tolerance = 0.000001});\n"
         "Report Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\n"
         "EndTarget; % retain solver comment\nPropagate Prop(Sat) {Sat.ElapsedSecs = 160};\n"
         "Report Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\n";
      if (optimizer) {
         // Keep the Earth and spacecraft visible in the short tiled native view.
         source.replace("Orb.ViewPointVector = [0 0 20000];", "Orb.ViewPointVector = [0 0 40000];");
         source.replace("Create DifferentialCorrector DC;\nDC.ReportStyle = Normal;\nDC.ReportFile = '"+files.filePath("dc.txt")+"';\n",
            "Create Yukon Opt;\nOpt.ReportStyle = Normal;\nOpt.ReportFile = '"+files.filePath("yukon.txt")+"';\nOpt.ShowProgress = false;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nCreate Variable Alpha Cost;\n");
         source.replace("Target 'retain solver label' DC", "Optimize 'retain solver label' Opt");
         source.replace("Vary DC(Sat.X = 7000, {Perturbation = 1, Lower = 6000, Upper = 8000, MaxStep = 100});",
            "Vary Opt(Alpha = 1, {Perturbation = 0.000001, Lower = 0, Upper = 3, MaxStep = 1});\nSat.X = 7000 + 100 * Alpha;");
         source.replace("Propagate Prop(Sat) {Sat.ElapsedSecs = 140};", "Propagate Prop(Sat) {Sat.ElapsedSecs = 60};");
         source.replace("Achieve DC(Sat.X = 7100, {Tolerance = 0.000001});", "Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);");
         source.replace("EndTarget;", "EndOptimize;").replace("Propagate Prop(Sat) {Sat.ElapsedSecs = 160};", "Propagate Prop(Sat) {Sat.ElapsedSecs = 20};");
      }
      if (nested) {
         source.replace("Orb.ViewPointVector = [0 0 20000];", "Orb.ViewPointVector = [0 0 40000];");
         source.replace("Create DifferentialCorrector DC;", "Create DifferentialCorrector DC Inner;\nInner.ReportFile = '"+files.filePath("inner.txt")+"';\nCreate Variable Alpha Beta;");
         source.replace("Vary DC(Sat.X = 7000, {Perturbation = 1, Lower = 6000, Upper = 8000, MaxStep = 100});",
            "Vary DC(Alpha = 1, {Perturbation = 0.01, Lower = 0, Upper = 4, MaxStep = 1});\nSat.X = 7000 + 100 * Alpha;");
         source.replace("Propagate Prop(Sat) {Sat.ElapsedSecs = 140};", "Propagate Prop(Sat) {Sat.ElapsedSecs = 20};");
         source.replace("Achieve DC(Sat.X = 7100, {Tolerance = 0.000001});",
            "Target 'retain inner label' Inner {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
            "Vary Inner(Beta = 1, {Perturbation = 0.01, Lower = 0, Upper = 5, MaxStep = 1});\n"
            "Sat.Y = 100 * Beta;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 20};\n"
            "Achieve Inner(Beta = 3, {Tolerance = 0.000001});\n"
            "Report Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\n"
            "EndTarget; % retain inner comment\nPropagate Prop(Sat) {Sat.ElapsedSecs = 20};\n"
            "Achieve DC(Alpha = 2, {Tolerance = 0.000001});");
         source.replace("Propagate Prop(Sat) {Sat.ElapsedSecs = 160};", "Propagate Prop(Sat) {Sat.ElapsedSecs = 20};");
      }
      const auto reference=QString(source).replace("Orb.SolverIterations = All;","Orb.SolverIterations = All;\nOrb.ShowPlot = false;")
         .replace("Ground.SolverIterations = All;","Ground.SolverIterations = All;\nGround.ShowPlot = false;")
         .replace("XY.SolverIterations = All;","XY.SolverIterations = All;\nXY.ShowPlot = false;");
      editor->setPlainText(reference); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Independent solver reference failed");
      const auto expected=read(report); require(!expected.isEmpty(),"Solver produced no report");
      if (nested && !capture.isEmpty()) {
         QFile evidence(capture+".state.txt");
         require(evidence.open(QIODevice::WriteOnly) && evidence.write(expected)==expected.size(),"Nested baseline evidence unavailable");
      }
      std::istringstream input(expected.toStdString()); std::vector<std::array<double,4>> rows; std::array<double,4> row;
      while (input>>row[0]>>row[1]>>row[2]>>row[3]) rows.push_back(row);
      require(rows.size()>3 && std::abs(rows.back()[0]-((optimizer || nested) ? 100 : 320))<1e-5,"Independent solve did not produce multiple iterations and the expected epoch");
      auto checkObjective=[&] {
         if (optimizer) {
            auto *alpha=Moderator::Instance()->GetInternalObject("Alpha"),*cost=Moderator::Instance()->GetInternalObject("Cost");
            require(alpha && cost && std::abs(alpha->GetRealParameter("Value")-2)<1e-5 && std::abs(cost->GetRealParameter("Value"))<1e-10,"Yukon did not retain the independently known quadratic optimum");
         } else if (nested) {
            auto *alpha=Moderator::Instance()->GetInternalObject("Alpha"),*beta=Moderator::Instance()->GetInternalObject("Beta");
            require(alpha && beta && std::abs(alpha->GetRealParameter("Value")-2)<1e-6 && std::abs(beta->GetRealParameter("Value")-3)<1e-6,"Nested targeters did not retain independently known goals 2 and 3");
         } else require(std::abs(rows[rows.size()-2][1]-7100)<1e-6,"Differential corrector did not reach the requested objective");
      };
      checkObjective();
      const auto final=rows.back();
      editor->setPlainText(source); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed && read(report)==expected,"Displayed solver baseline differs from independent report");
      QMap<QString,size_t> allCounts;
      for (const auto &name:QStringList{"Orb","Ground","XY"}) allCounts[name]=curve(*window.plotReceiver()->model(name)).points.size();
      QMap<QString,std::vector<std::array<double,4>>> acceptedPaths;
      for (const auto &mode:cleanup ? QStringList{} : QStringList{"Current","None","All"}) {
         for (const auto &name:QStringList{"Orb","Ground","XY"}) {
            const auto before=editor->toPlainText(); auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
            QWidget owner; QString error="Not applied";
            ResourceEditor panel(*object,[&](const auto &changes) { error=window.applyResourceChanges(name,changes,before); return error; },&owner,before);
            auto *table=panel.findChild<QTableWidget *>("resourceProperties"); require(table,"Solver property table missing");
            QComboBox *choice=nullptr;
            for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="SolverIterations") choice=qobject_cast<QComboBox *>(table->cellWidget(row,1));
            require(choice && choice->findText(mode)>=0,"Solver mode choices missing"); choice->setCurrentText(mode);
            require(panel.hasChanges() && editor->toPlainText()==before,"Solver selection changed source before Apply");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
            const auto changed=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==before,"Solver edit Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Solver edit Redo not exact");
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Solver mode save/reopen run failed");
         require(read(report)==expected,"Plot solver mode changed calculations or reports");
         checkObjective();
         for (const auto &name:QStringList{"Orb","Ground","XY"}) {
            const auto model=window.plotReceiver()->model(name); require(bool(model),"Solver plot missing");
            const auto &points=curve(*model).points;
            std::cerr<<name.toStdString()<<" "<<mode.toStdString()<<" points="<<points.size()<<" all="<<allCounts[name]<<'\n';
            require(points.size()>3 && (mode=="All" ? points.size()==allCounts[name] : points.size()<allCounts[name]),"Solver mode retained an incorrect iteration history");
            require(!points.back().solver,"Post-solver data retains solver classification");
            if (name=="Ground" || name=="XY") {
               std::vector<std::array<double,4>> path; for (const auto &point:points) path.push_back({point.x,point.y,point.z,point.epoch});
               if (mode=="Current") acceptedPaths[name]=path;
               if (mode=="None") {
                  const auto &accepted=acceptedPaths[name];
                  require(path.size()==accepted.size(),"Current mode has a different accepted sample count from None");
                  // Geodetic conversion reuses the engine's cached rotating
                  // frame; bound sub-nanoradian roundoff rather than demand
                  // bit equality from display coordinates. Reports stay exact.
                  for (size_t i=0;i<path.size();++i) for (int j=0;j<4;++j)
                     require(std::abs(path[i][j]-accepted[i][j])<(j==3 ? 1e-10 : 1e-8),"Current mode's accepted path differs from independently filtered None history");
               }
            }
            if (nested && mode=="None" && name=="XY") {
               for (const auto &point:points) if (point.x>20.00001)
                  require(point.y>7150,"Accepted history retained inner initialization/outer trial samples");
            }
            const auto &last=points.back();
            if (name=="Orb") {
               require(std::abs(last.x-final[1])<1e-8,"Orbit solver endpoint differs from report");
               require(!model->cameras.empty() && std::abs(model->cameras.back().target[0]-last.x)<1e-8,"Solver camera does not track the final reported state");
            } else if (name=="Ground") {
               require(std::abs(last.x-final[2])<1e-8 && std::abs(last.y-final[3])<1e-8,"Ground solver endpoint differs from geodetic report");
               bool trial=false; for (const auto &point:points) if (point.solver) { trial=true; require(point.color!=last.color,"Trial trajectory lost its target color"); }
               require(trial==(mode=="All"),"Ground solver history has incorrect trial classification");
            } else require(std::abs(last.x-final[0])<1e-5 && std::abs(last.y-final[1])<1e-8,"XY solver endpoint differs from report");
            require(window.plotReceiver()->show(name),"Solver output cannot activate"); QApplication::processEvents();
            auto *view=viewer(window,name); require(view && !view->canvas()->captureImage().isNull(),"Solver viewer cannot render");
            const auto count=points.size();
            if (model->kind!=PlotModel::Kind::XY) {
               auto *timeline=view->findChild<QSlider *>("plotTimeline"); require(timeline,"Solver replay control missing");
               const auto latest=view->canvas()->captureImage();
               timeline->setValue(0); QApplication::processEvents();
               const auto earlier=view->canvas()->captureImage();
               require(earlier!=latest,"Solver replay cannot render earlier data");
               timeline->setValue(timeline->maximum()); QApplication::processEvents();
               require(view->canvas()->captureImage()==latest,"Solver replay cannot restore latest data");
            }
            require(curve(*model).points.size()==count,"Solver replay changed retained data");
            for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("plotName")==name) child->close();
            require(window.plotReceiver()->show(name) && curve(*window.plotReceiver()->model(name)).points.size()==count,"Solver close/reopen lost history");
         }
         if (!capture.isEmpty()) {
            auto *area=window.findChild<QMdiArea *>("workspace"); area->tileSubWindows(); QApplication::processEvents();
            for (const auto &name:QStringList{"Orb","Ground","XY"}) {
               auto *view=viewer(window,name); require(view && view->canvas()->captureImage().save(capture+"."+mode+"."+name+".png"),"Solver scene capture failed");
            }
            require(window.grab().save(capture+"."+mode+".png"),"Solver viewer capture failed");
         }
      }
      if (cleanup) {
         auto broken=source; broken.replace("Achieve Inner(Beta", "Save Alpha; % force inner runtime write failure\nAchieve Inner(Beta");
         auto *fm=FileManager::Instance();
         struct OutputRestore { FileManager *manager; std::string path; ~OutputRestore() { manager->SetAbsPathname("OUTPUT_PATH",path); } };
         {
            OutputRestore restore{fm,fm->GetAbsPathname("OUTPUT_PATH")};
            fm->SetAbsPathname("OUTPUT_PATH",files.filePath("missing/nested").toStdString());
            editor->setPlainText(broken);
            require(window.runMission()==MainWindow::RunResult::Failed && !window.isRunning() && editor->toPlainText()==broken,"Inner write failure did not retain source/release mission");
            require(SolverBranchCommand::GetPlotRunState(Gmat::RUNNING)==Gmat::RUNNING,"Failed nested run retained a trial display context");
         }
         editor->setPlainText(source); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Nested write failure correction did not reopen");
         const auto snapshot=window.missionSnapshot(); int inner=-1;
         for (int i=0;i<snapshot.nodes.size();++i) {
            const auto &node=snapshot.nodes[i];
            if (node.type=="Propagate" && node.parent>=0 && snapshot.nodes[node.parent].label.contains("retain inner label")) inner=i;
         }
         require(inner>=0 && window.setBreakpoint(inner,true),"Inner solver breakpoint unavailable");
         Debugger *controller=nullptr; for (auto *child:window.children()) if (auto *candidate=dynamic_cast<Debugger *>(child)) controller=candidate;
         require(controller,"Nested cleanup debugger missing");
         bool stopped=false,timedOut=false; QTimer poll,timeout; poll.setInterval(10); timeout.setSingleShot(true); timeout.setInterval(5000);
         QObject::connect(&poll,&QTimer::timeout,&window,[&] { if (controller->isWaiting()) { stopped=true; window.stopMission(); } });
         QObject::connect(&timeout,&QTimer::timeout,&window,[&] { timedOut=true; window.stopMission(); });
         poll.start(); timeout.start(); const auto result=window.debugMission(); poll.stop(); timeout.stop();
         require(stopped && !timedOut && result==MainWindow::RunResult::Stopped && !window.isRunning() && !controller->isActive() && !controller->isWaiting() && editor->toPlainText()==source,"Stop at inner command did not release nested execution/source");
         require(SolverBranchCommand::GetPlotRunState(Gmat::RUNNING)==Gmat::RUNNING && window.setBreakpoint(inner,false),"Stopped nested run retained display context/breakpoint state");
         require(window.runMission()==MainWindow::RunResult::Completed && read(report)==expected,"Nested correction/rerun changed pre-failure numerical reports"); checkObjective();
         for (const auto &name:QStringList{"Orb","Ground","XY"}) {
            const auto model=window.plotReceiver()->model(name);
            require(model && curve(*model).points.size()==allCounts[name] && !curve(*model).points.back().solver,"Nested failure/Stop rerun lost plot history or retained trial classification");
         }
         auto *receiver=window.plotReceiver(); const std::string retained="RetainedScope";
         require(receiver->CreateXyPlotWindow(retained,"",0,0,0,0,false,"","","",true,false),"Retained-scope fixture unavailable");
         require(receiver->AddXyPlotCurve(retained,0,"Value",0xff0000),"Retained-scope curve unavailable");
         receiver->SetMaxGlDataPoints(retained,2); receiver->TakeXYAction(retained,"SolverScope=outer"); receiver->XyPlotMarkBreak(retained,-1,-1);
         const auto sample=[&](int value) { require(receiver->UpdateXyPlot(retained,"",value,Rvector(1,static_cast<double>(10*value)),"","","",false,true),"Retained-scope sample unavailable"); };
         sample(1); sample(2); receiver->TakeXYAction(retained,"SolverScope=inner"); receiver->XyPlotMarkBreak(retained,-1,-1); sample(3); sample(4); sample(5);
         require(curve(*receiver->model("RetainedScope")).points.size()==2,"Retained-scope limit not applied");
         receiver->TakeXYAction(retained,"SolverScope=outer"); receiver->XyPlotClearFromBreak(retained,-1,-1,-1);
         require(curve(*receiver->model("RetainedScope")).points.empty(),"Trimmed parent anchor failed to discard retained nested trial samples"); sample(6);
         const auto &retainedPoints=curve(*receiver->model("RetainedScope")).points;
         require(retainedPoints.size()==1 && retainedPoints.back().x==6 && retainedPoints.back().y==60 && !retainedPoints.back().connect,"Resumed retained history bridged discarded nested samples");
         receiver->DeleteXyPlot(retained);
         std::cout<<"PASS: runtime Save failure inside nested targeter, exact failed source retention/output-path restoration/Unicode correction, inner-command breakpoint Stop with full observer/context cleanup, corrected rerun with identical full reports/known goals/All histories; bounded retention discards scoped trials even after pruning a parent anchor.\n";
         return 0;
      }
      std::cout<<(optimizer ? "Yukon optimization: " : nested ? "Nested differential correctors: " : "Differential corrector: ")<<"PASS: GUI All/Current/None/All solver plot modes, exact Undo/Redo and Unicode save/reopen, independent solver/geodetic reports and objective, accepted histories/target colors/camera tracking, native/fallback rendering, replay and immediate close/reopen\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
