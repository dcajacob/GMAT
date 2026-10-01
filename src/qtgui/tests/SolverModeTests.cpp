#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "FileManager.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Solver mode report missing"); return file.readAll(); }
static const PlotCurve &curve(const PlotModel &model) { if (model.kind==PlotModel::Kind::XY) return model.curves.first(); for (const auto &entry:model.curves) if (entry.name=="Sat") return entry; throw std::runtime_error("Solver mode curve missing"); }
static void closePanels(MainWindow &window) {
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<CommandEditor *>(child->widget())) { panel->discardChanges(); child->close(); }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static QMdiSubWindow *open(MainWindow &window) {
   int index=-1; const auto snapshot=window.missionSnapshot();
   for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].label.endsWith(" — retain solver modes")) index=i;
   auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree);
   while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;
   require(index>=0 && snapshot.nodes[index].editable && *item,"Solver mode command source missing"); tree->itemDoubleClicked(*item,0);
   auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Solver mode editor missing"); return child;
}
struct Row { double pass,alpha,elapsed,x,longitude,latitude; };
static std::vector<Row> rows(const QByteArray &bytes) {
   std::istringstream input(bytes.toStdString()); std::vector<Row> result; Row row;
   while (input>>row.pass>>row.alpha>>row.elapsed>>row.x>>row.longitude>>row.latitude) result.push_back(row);
   require(input.eof() && !result.empty(),"Solver mode report invalid/empty"); return result;
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtSolverModes");
   try {
      const bool failure=argc>=3 && QString::fromLocal8Bit(argv[2])=="--disabled-failure";
      const bool disabled=failure || (argc>=3 && QString::fromLocal8Bit(argv[2])=="--disabled-stop");
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3 || (disabled && argc==4)) && files.isValid(),"Solver mode setup failed");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : argc==3 && !disabled ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Solver mode runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto run=[&] {
         auto *manager=FileManager::Instance(); const auto original=manager->GetFullPathname("OUTPUT_PATH");
         struct RestoreOutput { FileManager *manager; std::string path; ~RestoreOutput() { manager->SetAbsPathname("OUTPUT_PATH",path); } } restore{manager,original};
         if (failure) manager->SetAbsPathname("OUTPUT_PATH",(files.filePath("missing-output")+"/").toStdString());
         return window.runMission();
      };
      auto *area=window.findChild<QMdiArea *>("workspace"); const auto report=files.filePath("solver modes.txt"),saved=files.filePath("Solver modes Ω.script");
      const QString common="% preserve mode source α\nCreate Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329;\nSat.VZ = 0;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate Variable Alpha Cost Pass;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.SolverIterations = All;\nOrb.NumPointsToRedraw = 1;\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 40000];\nOrb.ViewDirection = Sat;\n"
         "Create GroundTrack Ground;\nGround.Add = {Sat};\nGround.SolverIterations = All;\nGround.NumPointsToRedraw = 1;\n"
         "Create XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Sat.EarthMJ2000Eq.X};\nXY.SolverIterations = All;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nValues.SolverIterations = All;\n";
      const QString reportCommand="Report Values Pass Alpha Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\n";
      for (bool optimizer:{false,true}) for (const auto &solve:QStringList{"Solve","RunInitialGuess"}) for (const auto &exit:QStringList{"SaveAndContinue","DiscardAndContinue","Stop"}) {
         if (disabled && (solve!="Solve" || exit!="DiscardAndContinue")) continue;
         const bool stopping=disabled || exit=="Stop";
         const auto wanted=failure ? MainWindow::RunResult::Failed : stopping ? MainWindow::RunResult::Stopped : MainWindow::RunResult::Completed;
         closePanels(window); const auto name=optimizer ? QString("Opt") : QString("DC"),type=optimizer ? QString("Optimize") : QString("Target"),end=optimizer ? QString("EndOptimize") : QString("EndTarget");
         const auto resources=optimizer ? QString("Create Yukon Opt;\nOpt.ShowProgress = false;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nOpt.ReportFile = '"+files.filePath("yukon.txt")+"';\n") : QString("Create DifferentialCorrector DC;\nDC.ReportFile = '"+files.filePath("dc.txt")+"';\n");
         const auto initial=common+resources+"BeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nFor 'repeat solver' Pass = 1:2;\n"+type+" 'retain solver modes' "+name+" {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false}; % keep modes\n"
            "Vary 'retain guess' "+name+"(Alpha = 1, {Perturbation = 0.000001, Lower = 0, Upper = 3, MaxStep = 1}); % keep options\nSat.X = 7000 + 100 * Alpha;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\n"+
            (optimizer ? "Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);\n" : "Achieve DC(Alpha = 2, {Tolerance = 0.000001});\n")+reportCommand+end+"; % keep end\n"+
            (disabled ? QString("Toggle 'disable completed solver plots' Orb Ground XY Off;\n")+(failure ? "Save Alpha; % retain write failure\n" : "Stop; % retain intentional stop\n") : QString())+reportCommand+"EndFor;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\n"+reportCommand;
         const auto candidate=QString(initial).replace("SolveMode = Solve, ExitMode = SaveAndContinue", "SolveMode = "+solve+", ExitMode = "+exit);
         const auto reference=QString(candidate).replace("Orb.SolverIterations = All;","Orb.SolverIterations = All;\nOrb.ShowPlot = false;").replace("Ground.SolverIterations = All;","Ground.SolverIterations = All;\nGround.ShowPlot = false;").replace("XY.SolverIterations = All;","XY.SolverIterations = All;\nXY.ShowPlot = false;");
         editor->setPlainText(reference); const auto baselineResult=run(); require(baselineResult==wanted,"Independent mode reference failed"); const auto expected=read(report); const auto independent=rows(expected);
         const auto alpha=solve=="Solve" ? 2. : 1.; require(std::abs(independent.back().alpha-alpha)<1e-5 && std::abs(independent.back().elapsed-(stopping ? 40 : 80))<1e-5,"Independent mode objective/final epoch incorrect");
         if (!stopping) {
            const Row *firstSecond=nullptr; for (const auto &row:independent) if (row.pass==2) { firstSecond=&row; break; }
            require(firstSecond && std::abs(firstSecond->alpha-(solve=="Solve" && exit=="SaveAndContinue" ? 2 : 1))<1e-5,"Save/Discard did not use the documented guess on the next solver invocation");
         } else require(independent.back().pass==1,"Stop executed a subsequent solver invocation");
         editor->setPlainText(initial); require(window.buildScript(),"Mode controls source build failed"); auto *child=open(window); auto *panel=dynamic_cast<CommandEditor *>(child->widget());
         auto *solveChoice=panel->findChild<QComboBox *>("commandChoice_SolveMode"),*exitChoice=panel->findChild<QComboBox *>("commandChoice_ExitMode"); require(solveChoice && exitChoice,"Mode dropdowns missing"); solveChoice->setCurrentText(solve); exitChoice->setCurrentText(exit); require(editor->toPlainText()==initial,"Mode controls changed source before Apply");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(editor->toPlainText()==candidate && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Mode Apply changed other source or failed to retain its panel");
         if (candidate!=initial) { editor->undo(); require(editor->toPlainText()==initial,"Mode Undo not exact"); editor->redo(); require(editor->toPlainText()==candidate,"Mode Redo not exact"); }
         closePanels(window); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode mode save/reopen failed"); const auto result=run(); require(read(report)==expected && editor->toPlainText()==candidate,"GUI mode changed independent report or source");
         QStringList issues; if (result!=wanted) issues.append("Incorrect terminal run result");
         if (window.isRunning() || editor->isReadOnly()) issues.append("Terminal run retained the mission/editor lock");
         if (stopping && !failure && window.statusBar()->currentMessage()!="Mission stopped") issues.append("Intentional Stop shown as failure");
         if (failure && !window.statusBar()->currentMessage().startsWith("Mission failed")) issues.append("Write failure no longer shown as failure");
         try { const auto summary=window.summaryText(-1,"EarthMJ2000Eq"); if (!summary.contains("retain solver modes") || !summary.contains("Sat")) issues.append("Partial mission summary missing solver/spacecraft"); }
         catch (const std::exception &e) { issues.append("Mission summary: "+QString::fromUtf8(e.what())); }
         catch (BaseException &e) { issues.append("Mission summary: "+QString::fromStdString(e.GetFullMessage())); }
         const auto final=independent.back(); const auto epoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"))->GetEpoch();
         for (const auto &plot:QStringList{"Orb","Ground","XY"}) {
            const auto model=window.plotReceiver()->model(plot); require(bool(model),"Mode viewer model missing"); const auto &points=curve(*model).points; require(points.size()>3,"Mode viewer retained insufficient history"); const auto &point=points.back();
            const auto elapsed=model->kind==PlotModel::Kind::XY ? point.x : (point.epoch-epoch)*86400.; require(std::abs(elapsed-final.elapsed)<1e-5,"Mode viewer final epoch differs from independent report");
            if (plot=="Ground") require(std::abs(point.x-final.longitude)<1e-8 && std::abs(point.y-final.latitude)<1e-8,"Mode ground endpoint differs from report"); else require(std::abs((plot=="XY" ? point.y : point.x)-final.x)<1e-8,"Mode state endpoint differs from report");
            if ((plot=="Orb" || plot=="Ground") && (!model->endOfRun || model->firstVisibleFrame(curve(*model),model->frame)!=0)) issues.append(plot+": terminated Latest view still shows only live recent segments");
            require(window.plotReceiver()->show(plot),"Mode viewer activation failed"); auto *view=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(view && !view->canvas()->captureImage().isNull(),"Mode viewer capture failed");
            auto *replay=view->findChild<QSlider *>("plotTimeline"); require(replay && replay->maximum()>1,"Mode replay unavailable"); const auto latest=view->canvas()->captureImage(); replay->setValue(replay->minimum()); QApplication::processEvents(); replay->setValue(replay->maximum()); QApplication::processEvents(); require(view->canvas()->captureImage()==latest,"Mode Latest replay not restored");
            const auto count=points.size(); area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show(plot) && window.plotReceiver()->model(plot)==model && curve(*model).points.size()==count,"Mode viewer close/reopen lost history");
         }
         std::cout<<type.toStdString()<<" "<<solve.toStdString()<<" "<<exit.toStdString()<<" report rows="<<independent.size()<<" epoch="<<final.elapsed<<" alpha="<<final.alpha<<'\n';
         if (!capture.isEmpty() && stopping && solve=="Solve") { area->tileSubWindows(); QApplication::processEvents(); require(window.grab().save(capture+"."+type+".png"),"Stopped mode native capture failed"); }
         require(issues.isEmpty(),qPrintable(issues.join('\n')));
         if (failure) {
            auto *manager=FileManager::Instance(); const auto original=manager->GetFullPathname("OUTPUT_PATH");
            struct RestoreOutput { FileManager *manager; std::string path; ~RestoreOutput() { manager->SetAbsPathname("OUTPUT_PATH",path); } } restore{manager,original};
            manager->SetAbsPathname("OUTPUT_PATH",(files.path()+"/").toStdString());
            require(window.runMission()==MainWindow::RunResult::Completed,"Failed output path could not recover after restoring a writable directory");
            require(editor->toPlainText()==candidate,"Output recovery changed mission source");
            require(QFileInfo::exists(files.filePath("Alpha.Variable.data")),"Output recovery did not save the expected Variable data file");
            const auto recovered=rows(read(report)); require(std::abs(recovered.back().elapsed-80)<1e-5 && std::abs(recovered.back().alpha-2)<1e-5,"Recovered solver report missed the known objective/final epoch");
            for (const auto &name:QStringList{"Orb","Ground"}) { const auto model=window.plotReceiver()->model(name); require(model && model->endOfRun && model->firstVisibleFrame(curve(*model),model->frame)==0,"Recovery lost full retained disabled history"); }
         }
      }
      if (disabled) std::cout<<"PASS: Target/Yukon completed solver followed by disabled Orbit/Ground/XY and intentional Stop or Save write failure; correct terminal status, independent exact partial reports, source/Undo/Redo/Unicode reopen, full retained Latest histories, partial summaries, replay and close/reopen. Write-failure variant restores output and verifies successful rerun/save and known final goal/epoch.\n";
      else std::cout<<"PASS: all 12 actual Target/Yukon Solve/RunInitialGuess and ExitMode combinations in repeated spacecraft propagation loops; documented Save/Discard recurrence, stopped classification and partial summaries, independent exact state/geodetic reports, source/Undo/Redo/Unicode reopen, terminated full Latest histories, replay and close/reopen.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
