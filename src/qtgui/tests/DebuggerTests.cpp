#include "MainWindow.hpp"
#include "Debugger.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Parameter.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTemporaryDir>
#include <QTimer>
#include <QWindow>
#include <QTextBrowser>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Debugger report unavailable"); return file.readAll(); }
static void key(QWidget *widget,int code,Qt::KeyboardModifiers modifiers={})
{
   QKeyEvent down(QEvent::KeyPress,code,modifiers),up(QEvent::KeyRelease,code,modifiers);
   QApplication::sendEvent(widget,&down); QApplication::sendEvent(widget,&up);
}
struct Observer final : CommandExecutionObserver {
   int count=0;
   void BeforeExecution(GmatCommand *) override { ++count; }
};

int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtDebugger");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Debugger fixtures unavailable");
      MainWindow window; window.show(); require(window.initialize(startup),"Debugger runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); Debugger *controller=nullptr;
      for (auto *child:window.children()) if (auto *candidate=dynamic_cast<Debugger *>(child)) controller=candidate;
      require(controller,"Debugger controller unavailable");
      const auto report=files.filePath("values.txt");
      const QString source="% retain debugger source α\nCreate Spacecraft Sat;\nCreate Propagator P;\nCreate Variable V I;\nV = 1; % implicit resource defaults remain implicit\nCreate Array Grid[1,2];\nGrid(1,2) = 7;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n"
         "V = V + 1; % first execution\nReport 'first sample' Values V Grid(1,2) Sat.EarthMJ2000Eq.X;\n"
         "For 'loop' I = 1:2;\nIf I > 0;\nV = V + I;\nReport 'loop sample' Values V I Grid(1,2);\nEndIf;\nEndFor;\n"
         "Propagate 'advance' P(Sat) {Sat.ElapsedSecs = 60};\nReport 'last sample' Values V Sat.EarthMJ2000Eq.X;\n";
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed,"Independent debugger reference failed"); const auto reference=read(report);
      const auto find=[&](const QString &label) {
         const auto &nodes=window.missionSnapshot().nodes;
         for (int i=0;i<nodes.size();++i) if ((nodes[i].type=="Report" || nodes[i].type=="Propagate") && nodes[i].statement.contains(label)) return i;
         throw std::runtime_error("Debugger command unavailable");
      };
      const int first=find("first sample"),inside=find("loop sample"),propagate=find("advance"),last=find("last sample");
      auto *tree=window.findChild<QTreeWidget *>("Mission");
      const auto item=[&](int index) {
         for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) return *it;
         throw std::runtime_error("Debugger mission item unavailable");
      };
      // Use the actual mission context menu to place the first breakpoint.
      std::exception_ptr failure; bool contextVisited=false;
      tree->scrollToItem(item(first)); QApplication::processEvents();
      QTimer::singleShot(0,&window,[&] {
         try {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Mission context menu unavailable");
            QAction *action=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()=="missionBreakpoint") action=candidate;
            require(action && action->isEnabled() && action->isCheckable(),"Breakpoint context action unavailable"); contextVisited=true;
            const auto point=menu->actionGeometry(action).center(); QMouseEvent down(QEvent::MouseButtonPress,point,menu->mapToGlobal(point),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier),up(QEvent::MouseButtonRelease,point,menu->mapToGlobal(point),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(menu,&down); QApplication::sendEvent(menu,&up);
         } catch (...) { failure=std::current_exception(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); }
      });
      tree->customContextMenuRequested(tree->visualItemRect(item(first)).center()); if (failure) std::rethrow_exception(failure);
      require(contextVisited && window.breakpointIndices().contains(first) && item(first)->text(0).startsWith("● "),"Context breakpoint did not mark the mission tree");
      require(window.setBreakpoint(inside,true) && window.setBreakpoint(propagate,true),"Loop/propagate breakpoints unavailable");
      require(window.buildScript() && window.breakpointIndices()==QSet<int>{first,inside,propagate},"Same-source rebuild discarded breakpoints");
      auto *step=window.findChild<QAction *>("stepMission"); require(step && !step->isEnabled(),"Step enabled while idle");
      auto *debugAction=window.findChild<QAction *>("debugMission"); require(debugAction && debugAction->shortcut()==QKeySequence("Ctrl+F5"),"Debug shortcut collides with Build");
      const auto value=[] { auto *v=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject("V")); require(v,"Runtime variable unavailable"); return v->EvaluateReal(); };
      const auto inspect=[](QDialog *dialog,const QString &name) {
         dialog->findChild<QComboBox *>("debuggerFilter")->setCurrentIndex(1);
         auto *list=dialog->findChild<QListWidget *>("debuggerObjects"); const auto entries=list->findItems(name,Qt::MatchExactly);
         require(entries.size()==1,"Runtime object missing from inspector"); list->setCurrentItem(entries.first());
         auto *text=dialog->findChild<QPlainTextEdit *>("debuggerObjectText"); require(text->isReadOnly() && !text->toPlainText().isEmpty(),"Inspector not read-only or empty"); return text->toPlainText();
      };
      // Timer actions run in the debugger's nested event loop. Failures always
      // release the engine so this test cannot leave a mission paused forever.
      auto run=[&](std::function<void(QDialog *,int)> visit,bool forceDebug=true) {
         int calls=0; failure=nullptr; QTimer timer,timeout; timer.setInterval(10); timeout.setSingleShot(true); timeout.setInterval(12000);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            if (!controller->isWaiting()) return;
            try { auto *dialog=window.findChild<QDialog *>("debuggerDialog"); require(dialog && dialog->isVisible() && window.isRunning() && step->isEnabled(),"Paused debugger state inconsistent"); visit(dialog,calls++); }
            catch (...) { failure=std::current_exception(); window.stopMission(); }
         });
         QObject::connect(&timeout,&QTimer::timeout,&window,[&] { failure=std::make_exception_ptr(std::runtime_error("Debugger did not release the engine")); window.stopMission(); });
         timer.start(); timeout.start(); const auto result=forceDebug ? window.debugMission() : window.runMission(); timer.stop(); timeout.stop();
         if (failure) std::rethrow_exception(failure); require(!window.isRunning() && !controller->isActive() && !controller->isWaiting() && !step->isEnabled(),"Debugger did not clean up after run");
         auto *dialog=window.findChild<QDialog *>("debuggerDialog"); require(!dialog || !dialog->isVisible(),"Finished debugger remains visible"); return std::make_pair(result,calls);
      };
      Observer sentinel; auto *prior=GmatCommand::SetExecutionObserver(&sentinel);
      const auto result=run([&](QDialog *dialog,int number) {
         const int index=dialog->property("commandIndex").toInt();
         require(!window.setBreakpoint(index,false) && window.debugMission()==MainWindow::RunResult::Busy,"Paused run allows breakpoint editing or recursive execution");
         if (number==0) {
            require(index==first && value()==2,"First breakpoint is not before Report or first assignment executed incorrectly");
            auto *filter=dialog->findChild<QComboBox *>("debuggerFilter"); require(filter->currentIndex()==0 && dialog->findChild<QListWidget *>("debuggerObjects")->count()==1,"Spacecraft inspector filter incorrect");
            require(inspect(dialog,"V").section("------- Current Value -------",-1).trimmed()=="2" && inspect(dialog,"Grid").contains("7"),"Variable/array runtime values missing");
            filter->setCurrentIndex(0); require(dialog->findChild<QListWidget *>("debuggerObjects")->count()==1 && inspect(dialog,"Sat").contains("Sat"),"Runtime spacecraft unavailable");
            require(Moderator::Instance()->GetConfiguredObject("V")!=Moderator::Instance()->GetInternalObject("V"),"Inspector used configured rather than runtime object");
            require(editor->isReadOnly() && !tree->isEnabled(),"Paused debugger permits configuration editing");
            dialog->findChild<QPushButton *>("debuggerHelp")->click(); auto *help=window.findChild<QDialog *>("helpDialog");
            require(help && help->isVisible() && help->findChild<QTextBrowser *>()->source().toString().contains("Breakpoint.html") && controller->isWaiting(),"Debugger Help lost topic or resumed execution"); help->close();
            if (!capture.isEmpty()) { QApplication::processEvents(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture+".paused.png"),"Native debugger window not exposed"); }
            dialog->findChild<QPushButton *>("debuggerStep")->click();
         } else if (number==1) {
            require(window.missionSnapshot().nodes[index].type=="For" && value()==2,"Step skipped the For boundary or duplicated the prior command");
            dialog->findChild<QPushButton *>("debuggerResume")->click();
         } else if (number==2 || number==3) {
            require(index==inside && value()==(number==2 ? 3 : 5),"Loop breakpoint skipped or duplicated normal branch execution");
            dialog->findChild<QPushButton *>("debuggerResume")->click();
         } else if (number==4) {
            require(index==propagate && value()==5,"Propagation breakpoint value incorrect"); window.stepMission();
         } else {
            require(number==5 && index==last && value()==5,"Step propagation did not stop before final report");
            dialog->reject();
         }
      },false);
      require(result.first==MainWindow::RunResult::Completed && result.second==6 && read(report)==reference && editor->toPlainText()==source,"Stepped report/source differs from independent execution");
      require(GmatCommand::SetExecutionObserver(prior)==&sentinel && sentinel.count==0,"Mission did not restore the previous execution observer");
      window.findChild<QAction *>("clearBreakpoints")->trigger(); require(window.breakpointIndices().isEmpty() && !item(first)->text(0).startsWith("● "),"Clear breakpoints retained markers");
      const auto initial=run([&](QDialog *dialog,int count) { require(count==0 && value()==1 && dialog->findChild<QPlainTextEdit *>("debuggerCommand")->toPlainText().contains("V = V + 1"),"Debug without breakpoints did not stop before the first command"); dialog->findChild<QPushButton *>("debuggerClose")->click(); });
      require(initial.first==MainWindow::RunResult::Completed && initial.second==1 && read(report)==reference,"Close and resume changed execution");
      const auto keyboard=run([&](QDialog *dialog,int count) {
         dialog->activateWindow(); dialog->findChild<QPushButton *>("debuggerStep")->setFocus(); QApplication::processEvents();
         if (count==0) {
            if (capture.isEmpty()) key(dialog,Qt::Key_F10); else step->trigger();
            require(!controller->isWaiting(),"F10 did not step from the inspector");
         } else {
            require(count==1 && dialog->property("commandIndex").toInt()==first && value()==2,"Keyboard step did not execute exactly one assignment");
            if (capture.isEmpty()) key(dialog,Qt::Key_F5); else dialog->findChild<QPushButton *>("debuggerResume")->click();
            require(!controller->isWaiting(),"F5 did not resume from the inspector");
         }
      });
      require(keyboard.first==MainWindow::RunResult::Completed && keyboard.second==2 && read(report)==reference,"Keyboard debugger controls changed execution");
      const auto escaped=run([&](QDialog *dialog,int count) { require(count==0,"Escape left debugger paused"); key(dialog,Qt::Key_Escape); });
      require(escaped.first==MainWindow::RunResult::Completed && escaped.second==1 && read(report)==reference,"Escape did not close and resume");
      const auto closedMain=run([&](QDialog *,int count) { require(count==0,"Main close left debugger paused"); window.close(); });
      require(closedMain.first==MainWindow::RunResult::Stopped && closedMain.second==1 && window.isVisible() && !editor->isReadOnly(),"Closing paused main window did not stop safely and retain the document");
      require(window.setBreakpoint(inside,true),"Cannot set End fixture breakpoint");
      const auto ended=run([&](QDialog *dialog,int count) { require(count==0 && value()==3,"End fixture breakpoint incorrect"); dialog->activateWindow(); QApplication::processEvents(); if (capture.isEmpty()) key(dialog,Qt::Key_F5,Qt::ShiftModifier); else dialog->findChild<QPushButton *>("debuggerEnd")->click(); require(!controller->isWaiting(),"End shortcut did not release debugger"); });
      require(ended.first==MainWindow::RunResult::Stopped && ended.second==1,"End did not stop debug execution");
      window.findChild<QAction *>("clearBreakpoints")->trigger(); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Stopped debug mission cannot rerun correctly");
      // Source changes invalidate transient placements; neither the inspector
      // nor breakpoints become serialized mission statements or source edits.
      require(window.setBreakpoint(first,true),"Cannot set source invalidation breakpoint"); editor->appendPlainText("% pending debugger comment"); const auto changed=editor->toPlainText();
      require(!window.setBreakpoint(first,false) && window.buildScript() && window.breakpointIndices().isEmpty(),"Changed source retained stale command placements");
      editor->undo(); require(editor->toPlainText()==source,"Debugging damaged source Undo"); editor->redo(); require(editor->toPlainText()==changed,"Debugging damaged source Redo");
      const auto saved=files.filePath("Debugger ü.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==changed && window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Debugger save/reopen changed source or calculations");
      auto *resources=window.findChild<QTreeWidget *>("Resources"); const auto entries=resources->findItems("V",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Pending debug resource unavailable"); resources->itemDoubleClicked(entries.first(),0);
      EditablePanel *panel=nullptr; for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="V") panel=dynamic_cast<EditablePanel *>(child->widget());
      require(panel,"Pending debug resource panel unavailable"); panel->findChild<QLineEdit *>("parameterValue")->setText("9"); require(panel->hasChanges() && window.debugMission()==MainWindow::RunResult::Failed && panel->hasChanges() && editor->toPlainText()==changed,"Debug lost or ran pending resource edits"); panel->discardChanges();
      editor->setPlainText("Create Variable V;\nBeginMissionSequence;\nV = Missing + 1;\n"); require(window.debugMission()==MainWindow::RunResult::Failed && !controller->isActive(),"Invalid debug build did not recover");
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Invalid debug build prevented correction/rerun");
      // Step over a function call as a single mission command; inspecting its
      // caller must not expose half-executed function-local scopes.
      const auto functionPath=files.filePath("Double.gmf"); QFile function(functionPath); require(function.open(QIODevice::WriteOnly),"Debugger function fixture unavailable"); function.write("function [y] = Double(x)\nCreate Variable y;\nBeginMissionSequence;\ny = x * 2;\n"); function.close();
      const auto functionReport=files.filePath("function.txt");
      const QString functionSource="Create GmatFunction Double;\nDouble.FunctionPath = '"+functionPath+"';\nCreate Variable x y;\nx = 3;\nCreate ReportFile Result;\nResult.Filename = '"+functionReport+"';\nResult.WriteHeaders = false;\nBeginMissionSequence;\n[y] = Double(x);\nReport Result y;\n";
      editor->setPlainText(functionSource); require(window.runMission()==MainWindow::RunResult::Completed,"Independent function fixture failed"); const auto functionReference=read(functionReport);
      const auto functionResult=run([&](QDialog *dialog,int count) {
         if (count==0) { require(dialog->findChild<QPlainTextEdit *>("debuggerCommand")->toPlainText().contains("Double(x)"),"Debug did not stop before function call"); window.stepMission(); }
         else { require(count==1 && dialog->findChild<QPlainTextEdit *>("debuggerCommand")->toPlainText().contains("Report Result y") && inspect(dialog,"y").section("------- Current Value -------",-1).trimmed()=="6","Step entered function-local scope or lost output"); dialog->close(); }
      });
      require(functionResult.first==MainWindow::RunResult::Completed && functionResult.second==2 && read(functionReport)==functionReference && editor->toPlainText()==functionSource,"Stepped function changed calculations/source");
      // Solver branches dispatch their children repeatedly; the observer must
      // not execute one directly, disturb solver state, or skip iterations.
      for (bool optimize:{false,true}) {
         const QString solverType=optimize ? "Yukon" : "DifferentialCorrector",branch=optimize ? "Optimize" : "Target";
         const auto solverReport=files.filePath(optimize ? "optimizer.txt" : "target.txt");
         const QString solverSource="Create "+solverType+" Solver;\nSolver.ShowProgress = false;\nSolver.ReportFile = '"+files.filePath("solver.log")+"';\nCreate Variable x goal;\nCreate ReportFile Values;\nValues.Filename = '"+solverReport+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nValues.SolverIterations = All;\nBeginMissionSequence;\n"+branch+" Solver {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Solver(x = 1, {Perturbation = 0.001, MaxStep = 10});\n"+(optimize ? "goal = (x - 2)^2;\nMinimize Solver(goal);\n" : "goal = x * x;\nAchieve Solver(goal = 9, {Tolerance = 0.00000001});\n")+"Report 'inside solver' Values x goal;\nEnd"+branch+";\nReport 'solved' Values x goal;\n";
         editor->setPlainText(solverSource); require(window.runMission()==MainWindow::RunResult::Completed,"Independent solver debugger fixture failed"); const auto solverReference=read(solverReport);
         const auto &nodes=window.missionSnapshot().nodes; int insideSolver=-1,entry=-1;
         for (int i=0;i<nodes.size();++i) { if (nodes[i].type==branch) entry=i; if (nodes[i].type=="Report" && nodes[i].statement.contains("inside solver")) insideSolver=i; }
         require(entry>=0 && insideSolver>=0 && window.setBreakpoint(entry,true) && window.setBreakpoint(insideSolver,true),"Solver breakpoints unavailable");
         const auto solverResult=run([&](QDialog *dialog,int count) {
            const auto index=dialog->property("commandIndex").toInt();
            if (count==0) { require(index==entry,"Solver breakpoint not before entry"); window.stepMission(); }
            else {
               require(count<128 && (count==1 ? window.missionSnapshot().nodes[index].type=="Vary" : index==insideSolver),qPrintable(QString("Solver step boundary count=%1 index=%2 type=%3 command=%4").arg(count).arg(index).arg(index>=0 ? window.missionSnapshot().nodes[index].type : "unmapped").arg(dialog->findChild<QPlainTextEdit *>("debuggerCommand")->toPlainText())));
               if (index==insideSolver) {
                  bool xValid=false,goalValid=false;
                  const double x=inspect(dialog,"x").section("------- Current Value -------",-1).trimmed().toDouble(&xValid),goal=inspect(dialog,"goal").section("------- Current Value -------",-1).trimmed().toDouble(&goalValid);
                  require(xValid && goalValid && std::abs(goal-(optimize ? (x-2)*(x-2) : x*x))<1e-8,"Solver inspector did not show current iteration values");
               }
               dialog->findChild<QPushButton *>("debuggerResume")->click();
            }
         });
         require(solverResult.first==MainWindow::RunResult::Completed && solverResult.second>3 && read(solverReport)==solverReference && editor->toPlainText()==solverSource,"Stepped solver changed independent iteration reports/source");
         auto *x=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject("x")); require(x && std::abs(x->EvaluateReal()-(optimize ? 2.0 : 3.0))<0.005,"Stepped solver lost analytic solution"); window.findChild<QAction *>("clearBreakpoints")->trigger();
      }
      auto eventSource=source; eventSource.replace("V = V + 1; % first execution","BeginScript;\nV = V + 1; % first execution\nEndScript;"); editor->setPlainText(eventSource);
      require(window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Independent script event fixture changed report");
      const auto eventResult=run([&](QDialog *dialog,int count) { require(count<5,"Script-event stepping failed to advance"); if (count<3) window.stepMission(); else dialog->findChild<QPushButton *>("debuggerResume")->click(); });
      require(eventResult.first==MainWindow::RunResult::Completed && eventResult.second==4 && read(report)==reference && editor->toPlainText()==eventSource,"Script-event stepping changed execution/source");
      // Request Pause while a debug mission is actively dispatching a loop.
      editor->setPlainText("Create Variable V;\nBeginMissionSequence;\nWhile V < 1e12;\nV = V + 1;\nEndWhile;\n");
      const auto pausedResult=run([&](QDialog *dialog,int count) {
         if (count==0) { dialog->findChild<QPushButton *>("debuggerResume")->click(); QTimer::singleShot(30,&window,[&] { window.pauseMission(); }); }
         else { require(count==1 && value()>0,"Pause did not reach a subsequent command boundary"); window.stopMission(); }
      });
      require(pausedResult.first==MainWindow::RunResult::Stopped && pausedResult.second==2,"Pause/Stop during debug loop did not recover");
      // Failed initialization and execution restore actions and the observer.
      auto initialization=source; initialization.replace(report,files.filePath("missing/values.txt")); editor->setPlainText(initialization);
      const auto initializationResult=run([&](QDialog *,int) { throw std::runtime_error("Invalid report path reached execution"); });
      require(initializationResult.first==MainWindow::RunResult::Failed && initializationResult.second==0,"Initialization failure retained debugger state");
      editor->setPlainText("Create Variable V;\nBeginMissionSequence;\nV = 2;\nSave V;\n");
      auto *fm=FileManager::Instance(); const auto output=fm->GetAbsPathname("OUTPUT_PATH"); fm->SetAbsPathname("OUTPUT_PATH",files.filePath("missing/nested").toStdString());
      const auto executionResult=run([&](QDialog *dialog,int count) { require(count==0,"Execution failure fixture paused twice"); dialog->findChild<QPushButton *>("debuggerResume")->click(); }); fm->SetAbsPathname("OUTPUT_PATH",output);
      require(executionResult.first==MainWindow::RunResult::Failed && executionResult.second==1,"Execution failure retained debugger state");
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Failed debug initialization/execution prevented ordinary rerun");
      std::cout<<"PASS: actual mission-context breakpoints/markers and normal Run; before-command stepping through For/If and propagation; live spacecraft/variable/array inspection; read-only runtime objects; byte-exact independent report/source; no duplicate execution; transient rebuild/source invalidation; F10/F5/Shift+F5 routing, Close/Escape/resume and End/Stop; step over function with independent output; Pause at next command; build/initialization/execution correction/rerun; previous observer restoration; Undo/Redo/Unicode save/reopen and pending resource protection\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
