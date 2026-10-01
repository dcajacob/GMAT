#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <array>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Solver toggle report missing"); return file.readAll(); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
static const PlotCurve &curve(const PlotModel &model) { if (model.kind==PlotModel::Kind::XY) return model.curves.first(); for (const auto &item:model.curves) if (item.name=="Sat") return item; throw std::runtime_error("Solver toggle spacecraft curve missing"); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtSolverToggle");
   try {
      const bool optimizer=argc>=3 && QString::fromLocal8Bit(argv[2])=="--optimizer";
      const bool nested=argc>=3 && QString::fromLocal8Bit(argv[2])=="--nested";
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3 || ((optimizer || nested) && argc==4)) && files.isValid(),"Solver toggle setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : argc==3 && !optimizer && !nested ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Solver toggle runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("solver toggle states.txt"),saved=files.filePath("solver toggle Δ.script");
      QString source="% preserve solver toggle source α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate DifferentialCorrector DC;\nDC.MaximumIterations = 30;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.SolverIterations = Current;\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Sat;\nCreate GroundTrack Ground;\nGround.Add = {Sat};\nGround.SolverIterations = Current;\nCreate XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Sat.EarthMJ2000Eq.X};\nXY.SolverIterations = Current;\nCreate ReportFile Idle Values;\nIdle.Filename = '"+files.filePath("idle.txt")+"';\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\nValues.SolverIterations = All;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nTarget 'retain branch' DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(Sat.X = 7000, {Perturbation = 1, Lower = 6000, Upper = 8000, MaxStep = 100});\nPropagate P(Sat) {Sat.ElapsedSecs = 30};\nToggle 'Pause iteration plots' Idle Off; % retain off\nPropagate P(Sat) {Sat.ElapsedSecs = 30};\nToggle 'Resume iteration plots' Idle On; % retain on\nPropagate P(Sat) {Sat.ElapsedSecs = 60};\nAchieve DC(Sat.X = 7100, {Tolerance = 0.000001});\nReport Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\nEndTarget;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nReport Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\n";
      if (optimizer || nested) {
         source.replace("Create Spacecraft Sat;", "Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329;\nSat.VZ = 0;");
         source.replace("Orb.ViewPointVector = [0 0 20000];", "Orb.ViewPointVector = [0 0 40000];");
      }
      if (optimizer) {
         source.replace("Create DifferentialCorrector DC;\nDC.MaximumIterations = 30;", "Create Yukon Opt;\nOpt.ShowProgress = false;\nOpt.ReportFile = '"+files.filePath("yukon.txt")+"';\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nCreate Variable Alpha Cost;");
         source.replace("Target 'retain branch' DC", "Optimize 'retain branch' Opt").replace("EndTarget;", "EndOptimize;");
         source.replace("Vary DC(Sat.X = 7000, {Perturbation = 1, Lower = 6000, Upper = 8000, MaxStep = 100});", "Vary Opt(Alpha = 1, {Perturbation = 0.000001, Lower = 0, Upper = 3, MaxStep = 1});\nSat.X = 7000 + 100 * Alpha;");
         source.replace("Achieve DC(Sat.X = 7100, {Tolerance = 0.000001});", "Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);");
      }
      if (nested) {
         source.replace("Create DifferentialCorrector DC;", "Create DifferentialCorrector DC Inner;\nDC.ReportFile = '"+files.filePath("outer.txt")+"';\nInner.ReportFile = '"+files.filePath("inner.txt")+"';\nCreate Variable Alpha Beta;");
         source.replace("Vary DC(Sat.X = 7000, {Perturbation = 1, Lower = 6000, Upper = 8000, MaxStep = 100});", "Vary DC(Alpha = 1, {Perturbation = 0.01, Lower = 0, Upper = 4, MaxStep = 1});\nSat.X = 7000 + 100 * Alpha;");
         source.replace("Achieve DC(Sat.X = 7100, {Tolerance = 0.000001});", "Achieve DC(Alpha = 2, {Tolerance = 0.000001});");
         source.replace("Toggle 'Pause iteration plots' Idle Off; % retain off\nPropagate P(Sat) {Sat.ElapsedSecs = 30};", "Toggle 'Pause iteration plots' Idle Off; % retain off\nTarget 'retain inner solve' Inner {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Inner(Beta = 1, {Perturbation = 0.01, Lower = 0, Upper = 5, MaxStep = 1});\nSat.Y = 100 * Beta;\nPropagate P(Sat) {Sat.ElapsedSecs = 30};\nAchieve Inner(Beta = 3, {Tolerance = 0.000001});\nReport Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.Earth.Longitude Sat.Earth.Latitude;\nEndTarget; % retain inner solve\n");
      }
      editor->setPlainText(source); run(window); const auto expected=read(report); require(!expected.isEmpty(),"Independent solver report empty"); const auto rows=QString::fromUtf8(expected).trimmed().split('\n'); require(rows.size()>3,"Solver toggle fixture did not execute multiple trial/accepted reports"); const auto accepted=rows[rows.size()-2].split(QRegularExpression("\\s+"),Qt::SkipEmptyParts),last=rows.last().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(accepted.size()==4 && last.size()==4 && std::abs(accepted[0].toDouble()-140)<1e-5 && ((optimizer || nested) || std::abs(accepted[1].toDouble()-7100)<1e-6) && std::abs(last[0].toDouble()-160)<1e-5,"Solver toggle objective/timing not independently satisfied");
      if (!capture.isEmpty()) {
         QFile file(capture+".state.txt"); require(file.open(QIODevice::WriteOnly) && file.write(expected)==expected.size(),"Solver toggle report evidence failed");
      }
      std::cout<<"Reference rows="<<rows.size()<<" acceptedElapsed="<<accepted[0].toDouble()<<" finalElapsed="<<last[0].toDouble()<<'\n';
      auto checkObjective=[&] {
         if (nested) {
            auto *alpha=Moderator::Instance()->GetInternalObject("Alpha"),*beta=Moderator::Instance()->GetInternalObject("Beta");
            require(alpha && beta && std::abs(alpha->GetRealParameter("Value")-2)<1e-6 && std::abs(beta->GetRealParameter("Value")-3)<1e-6,"Nested toggled solver missed the independently known goals 2 and 3");
            return;
         }
         if (!optimizer) return;
         auto *alpha=Moderator::Instance()->GetInternalObject("Alpha"),*cost=Moderator::Instance()->GetInternalObject("Cost");
         require(alpha && cost && std::abs(alpha->GetRealParameter("Value")-2)<1e-5 && std::abs(cost->GetRealParameter("Value"))<1e-10,"Optimizer toggle missed the independently known quadratic optimum");
      };
      checkObjective();
      require(window.buildScript(),"Solver toggle edit build failed"); const auto initial=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"))->GetEpoch(); auto *area=window.findChild<QMdiArea *>("workspace");
      for (const auto &label:QStringList{"Pause iteration plots","Resume iteration plots"}) {
         const auto before=editor->toPlainText(); const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'"+label+"'")) index=i; auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree); while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item; require(index>=0 && *item,"Solver-loop Toggle tree command missing"); tree->itemDoubleClicked(*item,0); auto *child=area->activeSubWindow(); auto *panel=dynamic_cast<CommandEditor *>(child->widget()); require(panel,"Solver-loop Toggle editor missing");
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] { auto *dialog=panel->findChild<QDialog *>("toggleSubscriberDialog"); try { require(dialog,"Solver-loop subscriber checklist missing"); auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(QStringList{"Orb","Ground","XY"}.contains(list->item(i)->text()) ? Qt::Checked : Qt::Unchecked); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); panel->findChild<QPushButton *>("commandChoose_Subscribers")->click(); if (failure) std::rethrow_exception(failure); require(panel->hasChanges() && editor->toPlainText()==before,"Solver-loop Toggle edited source before Apply"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto changed=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==before,"Solver-loop Toggle Undo changed source"); editor->redo(); require(editor->toPlainText()==changed && changed.contains("% retain off") && changed.contains("% retain on"),"Solver-loop Toggle Redo lost comments");
      }
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Solver-loop Toggle Unicode save/reopen failed"); const auto changed=editor->toPlainText(); run(window); require(read(report)==expected && editor->toPlainText()==changed,"Solver-loop Toggle changed independent state report or source");
      checkObjective();
      auto expectedSource=source;
      for (const auto &label:QStringList{"Pause iteration plots","Resume iteration plots"}) {
         const auto state=label.startsWith("Pause") ? QString("Off") : QString("On");
         const auto match=QRegularExpression("Toggle '"+label+"' ([A-Za-z][A-Za-z0-9_ ]*?) "+state).match(changed);
         const auto selected=match.captured(1).split(' ',Qt::SkipEmptyParts); auto sorted=selected; sorted.sort();
         require(sorted==QStringList({"Ground","Orb","XY"}),"Solver Toggle did not select exactly the three plots");
         expectedSource.replace("Toggle '"+label+"' Idle "+state,"Toggle '"+label+"' "+selected.join(' ')+" "+state);
      }
      require(changed==expectedSource,"Solver Toggle changed unrelated configuration, mission source, labels or expressions");
      QMap<QString,std::vector<std::array<double,4>>> acceptedPaths;
      QStringList failures; for (const auto &name:QStringList{"Orb","Ground","XY"}) {
         const auto model=window.plotReceiver()->model(name); require(bool(model),"Solver-loop viewer model missing"); const auto &points=curve(*model).points; require(points.size()>5,"Solver-loop retained data insufficient"); int resumed=0; auto time=[&](const PlotPoint &point) { return model->kind==PlotModel::Kind::XY ? point.x : (point.epoch-initial)*86400.; };
         for (size_t i=0;i<points.size();++i) { const auto t=time(points[i]); if (t>50.00001 && t<79.99999) failures.append(name+": collected disabled sample at "+QString::number(t)); if (i && time(points[i-1])<=50.00001 && t>=79.99999 && t<=140.00001) { ++resumed; if (points[i].connect) failures.append(name+": joined buffered solve across disabled interval at "+QString::number(t)); } }
         if ((optimizer || nested) && model->kind!=PlotModel::Kind::Orbit) {
            for (const auto &point:points) {
               require(!point.solver,"Current toggled Ground/XY history retained solver trial samples");
               acceptedPaths[name].push_back({point.x,point.y,point.z,point.epoch});
            }
         }
         require(!points.back().solver && std::abs(time(points.back())-160)<1e-5,"Solver toggle final accepted point/epoch missing"); if (model->kind==PlotModel::Kind::Orbit) require(std::abs(points.back().x-last[1].toDouble())<1e-8 && !model->cameras.empty() && model->cameras.back().frame==points.back().frame && std::abs(model->cameras.back().target[0]-points.back().x)<1e-8,"Toggled solver camera/trajectory final state differs from report"); else if (model->kind==PlotModel::Kind::GroundTrack) require(std::abs(points.back().x-last[2].toDouble())<1e-8 && std::abs(points.back().y-last[3].toDouble())<1e-8,"Toggled solver ground endpoint differs from report"); else require(std::abs(points.back().y-last[1].toDouble())<1e-8,"Toggled solver XY endpoint differs from report");
         if (model->kind==PlotModel::Kind::Orbit) {
            const auto &target=model->cameras.back().target; const auto &point=points.back();
            require(std::abs(target[0]-point.x)<1e-8 && std::abs(target[1]-point.y)<1e-8 && std::abs(target[2]-point.z)<1e-8,"Toggled solver camera does not track the complete accepted position");
         }
         std::cout<<name.toStdString()<<" Current retained="<<points.size()<<" resumedBoundaries="<<resumed<<" finalElapsed="<<time(points.back())<<'\n';
         if (!resumed) { std::cerr<<name.toStdString()<<" points="<<points.size()<<" times:"; for (const auto &point:points) std::cerr<<" "<<time(point)<<"/"<<point.connect<<"/"<<point.solver; std::cerr<<'\n'; } require(resumed>=1,"Solver-loop resumption boundary missing"); require(window.plotReceiver()->show(name),"Solver-loop viewer cannot activate"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot && !plot->canvas()->captureImage().isNull(),"Solver-loop viewer capture unavailable"); const auto count=points.size();
         if (model->kind!=PlotModel::Kind::XY) {
            auto *timeline=plot->findChild<QSlider *>("plotTimeline"); require(timeline,"Solver toggle replay control missing");
            const auto latest=plot->canvas()->captureImage(); timeline->setValue(0); QApplication::processEvents();
            require(plot->canvas()->captureImage()!=latest,"Solver toggle replay did not expose earlier history");
            timeline->setValue(timeline->maximum()); QApplication::processEvents(); require(plot->canvas()->captureImage()==latest,"Solver toggle replay did not restore Latest pixels");
         }
         if (!capture.isEmpty()) require(plot->canvas()->captureImage().save(capture+"."+name+".png"),"Solver toggle native scene evidence failed");
         area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show(name) && window.plotReceiver()->model(name)==model && curve(*model).points.size()==count,"Solver-loop close/reopen lost retained history");
      }
      if (!capture.isEmpty()) { area->tileSubWindows(); QApplication::processEvents(); require(window.grab().save(capture+".png"),"Solver toggle native workspace capture failed"); }
      if (!failures.isEmpty()) throw std::runtime_error(failures.join('\n').toStdString());
      if (optimizer || nested) {
         // Independently filtered accepted history: preserve the exact toggles,
         // but suppress every solver trial at the subscriber instead of clearing
         // Current history. This detects wrongly classified retained samples.
         editor->setPlainText(QString(changed).replace("SolverIterations = Current;","SolverIterations = None;")); run(window);
         require(read(report)==expected,"Accepted-history reference changed numerical reports"); checkObjective();
         for (const auto &name:QStringList{"Ground","XY"}) {
            const auto model=window.plotReceiver()->model(name); require(bool(model),"Accepted-history reference plot missing");
            const auto &points=curve(*model).points; const auto &accepted=acceptedPaths[name]; require(points.size()==accepted.size(),"Current toggled history retained a different accepted sample count from None");
            for (size_t i=0;i<points.size();++i) {
               const std::array<double,4> point={points[i].x,points[i].y,points[i].z,points[i].epoch};
               for (int j=0;j<4;++j) require(std::abs(point[j]-accepted[i][j])<(j==3 ? 1e-10 : 1e-8),"Current toggled accepted history differs from independently filtered None");
            }
         }
      }
      std::cout<<(optimizer ? "Yukon: " : nested ? "Nested differential correctors: " : "Differential corrector: ")<<"PASS: actual solver-loop Orbit/Ground/XY Toggle controls, exact source/comments/Undo/Redo/Unicode reopen, independent full iteration state/geodetic report, suppressed interval and separated resumed buffered histories, render capture, replay/Latest and close/reopen retention. Current mode; earlier solver mode matrix was not repeated.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
