#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "QtPlotReceiver.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "RgbColor.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Toggle report unavailable"); return file.readAll(); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
static const PlotCurve &curve(const PlotModel &model,const QString &name) { if (model.kind==PlotModel::Kind::XY && !model.curves.isEmpty()) return model.curves.first(); for (const auto &item:model.curves) if (item.name==name) return item; throw std::runtime_error("Toggle plot curve missing"); }
static void inspect(MainWindow &window)
{
   QStringList failures;
   for (const auto &name:QStringList{"Orb","Ground","XY"}) {
      const auto model=window.plotReceiver()->model(name); require(bool(model),"Toggled plot model missing");
      const auto &points=curve(*model,"Sat").points; require(points.size()>4,"Toggled plot samples missing");
      const double initial=points.front().epoch;
      int gap=0,resumed=0; bool green=false;
      for (size_t i=0;i<points.size();++i) {
         const auto &point=points[i]; const double time=model->kind==PlotModel::Kind::XY ? point.x : (point.epoch-initial)*86400.;
         if (time>30.00001 && time<59.99999) ++gap;
         if (i>0 && time>=59.99999 && ((model->kind==PlotModel::Kind::XY ? points[i-1].x : (points[i-1].epoch-initial)*86400.)<40)) {
            ++resumed; if (point.connect) failures.append(name+": joined trajectory over disabled interval");
         }
         green=green || (time>=59.99999 && point.color==QColor::fromRgb(RgbColor::ToIntColor("Green")&0xffffff));
      }
      require(gap==0 && resumed==1,"Toggle sample suppression/resumption differs from expected interval");
      if (name!="XY") require(green,"Late ToggleOn lost runtime spacecraft palette");
      if (model->kind!=PlotModel::Kind::XY && !model->endOfRun) failures.append(name+": retained history not finalized");
      if (model->kind==PlotModel::Kind::Orbit) {
         require(!model->cameras.empty(),"Toggled camera history missing");
         for (const auto &camera:model->cameras) require(camera.frame<=model->frame,"Toggled camera frame outside replay history");
      }
   }
   if (!failures.isEmpty()) throw std::runtime_error(failures.join('\n').toStdString());
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtTogglePlots");
   try {
      TestSettings settings; QTemporaryDir files; const bool completion=argc==3 && QString::fromLocal8Bit(argv[2])=="--finish-disabled",live=argc==3 && QString::fromLocal8Bit(argv[2])=="--live-flush"; require((argc==2 || completion || live) && files.isValid(),"Toggle plot setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Toggle plot runtime failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("toggle state.txt"),saved=files.filePath("toggle plots Δ.script");
      const QString prefix="% retain source α\nCreate Spacecraft Sat Peer;\nSat.OrbitColor = Red;\nPeer.OrbitColor = Yellow;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Peer, Earth};\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Sat;\nCreate GroundTrack Ground;\nGround.Add = {Sat, Peer};\nCreate XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Sat.EarthMJ2000Eq.X};\nCreate ReportFile Idle Values;\nIdle.Filename = '"+files.filePath("idle.txt")+"';\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n";
      const QString mission="Propagate P(Sat, Peer) {Sat.ElapsedSecs = 30};\nToggle 'Pause plots' Idle Off; % retain off comment\nPropagate P(Sat, Peer) {Sat.ElapsedSecs = 30};\nSat.OrbitColor = 'Green'; % changed during disabled interval\nToggle 'Resume plots' Idle On; % retain on comment\nPropagate P(Sat, Peer) {Sat.ElapsedSecs = 30};\n";
      const QString suffix="Report Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Peer.EarthMJ2000Eq.X Peer.EarthMJ2000Eq.Y Peer.EarthMJ2000Eq.Z Peer.EarthMJ2000Eq.VX Peer.EarthMJ2000Eq.VY Peer.EarthMJ2000Eq.VZ;\n";
      if (live) {
         auto livePrefix=prefix; livePrefix.replace("Orb.Add = {Sat, Peer, Earth};","Orb.Add = {Sat, Peer, Earth};\nOrb.NumPointsToRedraw = 1;"); livePrefix.replace("Ground.Add = {Sat, Peer};","Ground.Add = {Sat, Peer};\nGround.NumPointsToRedraw = 1;");
         const auto source=livePrefix+mission+suffix; editor->setPlainText(source); run(window); const auto expected=read(report);
         require(window.buildScript(),"Live-flush debugger fixture failed"); const auto snapshot=window.missionSnapshot();
         QSet<int> pauses; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Toggle" || snapshot.nodes[i].type=="Report") { pauses.insert(i); require(window.setBreakpoint(i,true),"Live-flush breakpoint unavailable"); }
         require(pauses.size()==3,"Live-flush command boundaries missing"); Debugger *controller=nullptr; for (auto *child:window.children()) if (auto *candidate=dynamic_cast<Debugger *>(child)) controller=candidate; require(controller,"Live-flush debugger unavailable");
         std::exception_ptr failure; int calls=0; QTimer timer,timeout; timer.setInterval(10); timeout.setSingleShot(true); timeout.setInterval(12000);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            if (!controller->isWaiting()) return;
            try {
               for (const auto &name:QStringList{"Orb","Ground"}) { const auto model=window.plotReceiver()->model(name); require(model && curve(*model,"Sat").points.size()>2,"Live-flush plot history unavailable"); if (model->endOfRun || model->firstVisibleFrame(curve(*model,"Sat"),model->frame)==0) throw std::runtime_error((name+": intermediate propagation flush marked live mission complete at pause "+QString::number(calls)).toStdString()); }
               require(editor->toPlainText()==source,"Live display flush modified script");
               if (calls==1) { const auto model=window.plotReceiver()->model("Ground"); const auto count=curve(*model,"Sat").points.size(); for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("plotName")=="Ground") child->close(); QApplication::processEvents(); require(window.plotReceiver()->show("Ground") && window.plotReceiver()->model("Ground")==model && !model->endOfRun && curve(*model,"Sat").points.size()==count,"Reopened live plot changed history or completion"); }
               ++calls; controller->resume();
            } catch (...) { failure=std::current_exception(); window.stopMission(); }
         });
         QObject::connect(&timeout,&QTimer::timeout,&window,[&] { failure=std::make_exception_ptr(std::runtime_error("Live-flush debugger did not release")); window.stopMission(); }); timer.start(); timeout.start(); const auto result=window.runMission(); timer.stop(); timeout.stop(); if (failure) std::rethrow_exception(failure);
         require(result==MainWindow::RunResult::Completed && calls==3 && read(report)==expected,"Live-flush debug run changed independent calculation");
         for (const auto &name:QStringList{"Orb","Ground"}) { const auto model=window.plotReceiver()->model(name); require(model->endOfRun && model->firstVisibleFrame(curve(*model,"Sat"),model->frame)==0,"True completion did not restore all retained display segments"); }
         std::cout<<"PASS: actual command-boundary pauses after three propagation blocks keep Orbit/Ground live with recent-segment limits, live Ground close/reopen retains state, and real completion shows full retained history; independent twelve-state report and exact script preserved.\n"; return 0;
      }
      if (completion) {
         const QString terminal="Toggle 'Freeze completed plots' Idle Off; % keep final state";
         auto completedPrefix=prefix; completedPrefix.replace("Orb.Add = {Sat, Peer, Earth};","Orb.Add = {Sat, Peer, Earth};\nOrb.NumPointsToRedraw = 1;"); completedPrefix.replace("Ground.Add = {Sat, Peer};","Ground.Add = {Sat, Peer};\nGround.NumPointsToRedraw = 1;");
         const auto source=completedPrefix+mission+terminal+"\n"+suffix;
         editor->setPlainText(source); run(window); const auto expected=read(report); QMap<QString,std::deque<PlotPoint>> paths;
         for (const auto &name:QStringList{"Orb","Ground","XY"}) paths[name]=curve(*window.plotReceiver()->model(name),"Sat").points;
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'Freeze completed plots'")) index=i;
         auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree); while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item; require(index>=0 && *item,"Completion Toggle command missing"); tree->itemDoubleClicked(*item,0);
         auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr; require(panel,"Completion Toggle MDI panel missing");
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] { auto *dialog=panel->findChild<QDialog *>("toggleSubscriberDialog"); try { require(dialog,"Completion output checklist missing"); auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(QStringList{"Orb","Ground","XY"}.contains(list->item(i)->text()) ? Qt::Checked : Qt::Unchecked); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); panel->findChild<QPushButton *>("commandChoose_Subscribers")->click(); if (failure) std::rethrow_exception(failure);
         require(panel->hasChanges() && editor->toPlainText()==source,"Completion edit changed source before Apply"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Completion Apply lost retained clean panel");
         const auto changed=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==source,"Completion Undo not exact"); editor->redo(); require(editor->toPlainText()==changed && changed.contains("% keep final state"),"Completion Redo/comment failed"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Completion Unicode save/reopen failed"); run(window); require(read(report)==expected,"Final plot Toggle changed independent report");
         QStringList failures;
         for (const auto &name:QStringList{"Orb","Ground","XY"}) {
            const auto model=window.plotReceiver()->model(name); const auto &actual=curve(*model,"Sat").points,&want=paths.value(name); require(actual.size()==want.size() && actual.size()>3,"Completed disabled plot lost retained points");
            for (size_t i=0;i<actual.size();++i) require(actual[i].color==want[i].color && std::abs(actual[i].x-want[i].x)<1e-8 && std::abs(actual[i].y-want[i].y)<1e-8 && std::abs(actual[i].z-want[i].z)<1e-8,"Completed disabled history differs from uninterrupted display");
            if (model->kind!=PlotModel::Kind::XY && (!model->endOfRun || model->firstVisibleFrame(curve(*model,"Sat"),model->frame)!=0)) failures.append(name+": disabled completed plot still limited to recent segments");
            if (name!="Orb") require(!model->active,"Mission completion reactivated a disabled plot");
            for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("plotName")==name) child->close(); QApplication::processEvents(); require(window.plotReceiver()->show(name) && window.plotReceiver()->model(name)==model,"Completed disabled history lost on reopen");
         }
         if (!failures.isEmpty()) throw std::runtime_error(failures.join('\n').toStdString());
         std::cout<<"PASS: actual MDI final ToggleOff, pending/retained Apply, exact Undo/Redo/Unicode save/reopen and unchanged independent twelve-state report; disabled completed Orbit/Ground show all retained points at Latest despite recent-segment setting; XY stays disabled and all histories survive close/reopen.\n"; return 0;
      }
      const auto source=prefix+mission+suffix;
      editor->setPlainText(source); run(window); const auto expected=read(report); require(!expected.isEmpty(),"Uninterrupted independent report empty");
      auto reference=source; reference.replace("' Idle Off","' Orb Ground XY Off"); reference.replace("' Idle On","' Orb Ground XY On"); editor->setPlainText(reference); run(window); require(read(report)==expected,"Toggling independent plots changed calculation"); inspect(window);
      QMap<QString,std::deque<PlotPoint>> paths; for (const auto &name:QStringList{"Orb","Ground","XY"}) paths[name]=curve(*window.plotReceiver()->model(name),"Sat").points;
      editor->setPlainText(source); require(window.buildScript(),"Toggle GUI fixture build failed");
      for (const auto &label:QStringList{"Pause plots","Resume plots"}) {
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'"+label+"'")) index=i;
         auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree); while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item; require(index>=0 && *item,"Toggle command tree item missing"); tree->itemDoubleClicked(*item,0);
         auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr; require(panel,"Toggle actual MDI panel missing"); const auto before=editor->toPlainText();
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] {
            auto *dialog=panel->findChild<QDialog *>("toggleSubscriberDialog"); try { require(dialog,"Toggle plot checklist missing"); auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(QStringList{"Orb","Ground","XY"}.contains(list->item(i)->text()) ? Qt::Checked : Qt::Unchecked); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         }); panel->findChild<QPushButton *>("commandChoose_Subscribers")->click(); if (failure) std::rethrow_exception(failure);
         require(panel->hasChanges() && editor->toPlainText()==before,"Toggle plots changed source before Apply"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Toggle plot Apply lost retained clean panel");
         const auto changed=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==before,"Toggle plot Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Toggle plot Redo not exact");
      }
      require(editor->toPlainText().startsWith(prefix) && editor->toPlainText().endsWith(suffix) && editor->toPlainText().contains("% retain off comment") && editor->toPlainText().contains("% retain on comment"),"Toggle plot edit lost unrelated source/comments");
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Toggle plot Unicode save/reopen failed"); run(window); require(read(report)==expected,"GUI plot toggles changed calculation"); inspect(window);
      for (const auto &name:QStringList{"Orb","Ground","XY"}) {
         const auto model=window.plotReceiver()->model(name); const auto &actual=curve(*model,"Sat").points,&want=paths.value(name); require(actual.size()==want.size(),"GUI Toggle history size differs from reference");
         for (size_t i=0;i<actual.size();++i) require(actual[i].connect==want[i].connect && actual[i].color==want[i].color && std::abs(actual[i].x-want[i].x)<1e-8 && std::abs(actual[i].y-want[i].y)<1e-8 && std::abs(actual[i].z-want[i].z)<1e-8,"GUI Toggle history differs from reference");
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("plotName")==name) child->close(); QApplication::processEvents(); require(window.plotReceiver()->show(name) && window.plotReceiver()->model(name)==model && curve(*model,"Sat").points.size()==want.size(),"Toggled history lost after close/reopen");
      }
      std::cout<<"PASS: actual Mission/MDI Orbit/Ground/XY Toggle selection, retained panels and exact Undo/Redo/Unicode save/reopen; independent twelve-state reports, suppressed interval and separated resumed histories, runtime palette while disabled, completed camera/replay history and viewer close/reopen retention.\n";
   } catch (BaseException &failure) { std::cerr<<"FAIL: "<<failure.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &failure) { std::cerr<<"FAIL: "<<failure.what()<<'\n'; return 1; }
}
