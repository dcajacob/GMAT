#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "CommandEditor.hpp"
#include "CommandForm.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QMenu>
#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QMdiSubWindow>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static int find(const MissionSnapshot &snapshot,const QString &type,const QString &parent={})
{
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i];
      if (node.type==type && (parent.isEmpty() || (node.parent>=0 && snapshot.nodes[node.parent].type==parent))) return i;
   }
   throw std::runtime_error("Expected mission node missing");
}
static int forAssignment(const MissionSnapshot &snapshot)
{
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i];
      if (node.parent>=0 && snapshot.nodes[node.parent].type=="For" && node.statement.contains("total = total + 1")) return i;
   }
   throw std::runtime_error("For-body assignment missing");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtMission");
   if (argc<3 || argc>4) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(), script=QFileInfo(argv[2]).absoluteFilePath();
   const auto screenshot=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings isolatedSettings;
      {
         QString result;
         CommandForm form([&](const QString &text) { result=text; });
         const QString vary="  Vary 'Change X' DC(x = 1, {Upper = 10, Perturbation = 0.001, Lower = -10}); % keep this\n";
         form.setStatement(vary);
         auto *upper=form.findChild<QLineEdit *>("commandField_Upper");
         require(upper && !form.isHidden(),"Vary form missing");
         upper->setText("25");
         QString expected=vary; expected.replace("Upper = 10","Upper = 25");
         require(result==expected,"Vary form changed unrelated source");
         form.findChild<QLineEdit *>("commandField_Initial value")->setText("3.5");
         expected.replace("x = 1","x = 3.5"); require(result==expected,"Multiple field edits lost a value");
         const QString branch="Target 'Keep label' DC {ExitMode = SaveAndContinue, SolveMode = Solve};\n   Vary DC(x = 1);\nEndTarget;";
         form.setStatement(branch);
         auto *exitMode=form.findChild<QLineEdit *>("commandField_ExitMode");
         require(exitMode,"Target branch form missing"); exitMode->setText("DiscardAndContinue");
         expected=branch; expected.replace("SaveAndContinue","DiscardAndContinue");
         require(result==expected,"Solver form changed nested command source");
         for (const auto &statement:{"Maneuver 'Keep label' Burn(Sat);","BeginFiniteBurn Burn(Sat);",
              "Achieve DC(x = 7, {Tolerance = 0.001});","Minimize Opt(cost);",
              "NonlinearConstraint Opt(x <= 4);","Report R x y;","FindEvents Locator {Append = true};"}) {
            form.setStatement(statement); require(!form.isHidden(),"Command-specific form missing");
         }
         form.setStatement("% Preserve advanced script\nUnknown foo;"); require(form.isHidden(),"Unknown command was simplified");
      }
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime failed");
      require(window.loadScript(script) && window.buildScript(),"Mission failed to build");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString original=editor->toPlainText();
      auto snapshot=window.missionSnapshot();
      for (const auto &node:snapshot.nodes) {
         if (node.start<0) std::cerr<<"Unmapped "<<node.type.toStdString()<<": "<<node.statement.toStdString()<<'\n';
         require(node.start>=0 && node.end>node.start,"Command source range not mapped");
      }
      require(snapshot.nodes[find(snapshot,"If","If")].parent>=0,"Nested branch not represented");
      require(find(snapshot,"Else","If")>=0 && find(snapshot,"EndFor","For")>=0,"Branch structure missing");
      auto total=[&] {
         require(window.runMission()==MainWindow::RunResult::Completed,"Mission did not run");
         auto *value=Moderator::Instance()->GetInternalObject("total"); require(value!=nullptr,"Result variable missing");
         return value->GetRealParameter("Value");
      };
      require(total()==6,"Initial mission result incorrect");
      snapshot=window.missionSnapshot();
      const int assignment=forAssignment(snapshot);
      const auto error=window.applyMissionChange(snapshot,assignment,MissionEdit::Replace,"total = total + 2;");
      if (!error.isEmpty()) std::cerr<<error.toStdString()<<'\n';
      require(error.isEmpty() && total()==9,"Repeated-command edit changed wrong branch");
      require(!window.applyMissionChange(snapshot,assignment,MissionEdit::Remove,{}).isEmpty(),"Stale mission editor was accepted");
      editor->undo(); require(editor->toPlainText()==original && window.buildScript() && total()==6,"Mission edit lost undo history");
      snapshot=window.missionSnapshot();
      require(!window.applyMissionChange(snapshot,forAssignment(snapshot),MissionEdit::Replace,"Propagate Missing(Spacecraft);").isEmpty(),"Invalid mission command accepted");
      require(editor->toPlainText()==snapshot.sourceScript && total()==6,"Invalid edit did not restore previous mission");
      require(window.applyMissionChange(snapshot,forAssignment(snapshot),MissionEdit::InsertBefore,"total = total + 10;").isEmpty(),"Corrected insertion after failure rejected");
      require(total()==36,"Inserted command did not execute inside selected loop");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"If"),MissionEdit::Remove,{}).isEmpty() && total()==4,"Deleting a branch did not remove its whole block");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"For"),MissionEdit::InsertAfter,"total = total + 10;").isEmpty() && total()==16,"Insert-after remained inside branch");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,-1,MissionEdit::Append,"total = total + 10;").isEmpty() && total()==16,"Append mission command failed");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"BeginScript"),MissionEdit::Replace,
         "BeginScript;\n   total = total + 5;\nEndScript;").isEmpty() && total()==10,
         "Script event replacement did not replace the complete block");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(!window.applyMissionChange(snapshot,find(snapshot,"EndIf"),MissionEdit::Remove,{}).isEmpty(),"Structural end command could be deleted alone");
      auto *tree=window.findChild<QTreeWidget *>("Mission"); require(tree!=nullptr,"Mission tree missing");
      QTreeWidgetItem *target=nullptr;
      const int selected=forAssignment(snapshot);
      for (QTreeWidgetItemIterator it(tree);*it;++it)
         if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==selected) target=*it;
      require(target && target->parent()->data(0,Qt::UserRole+1).toString()=="For","Tree hierarchy wrong");
      window.findChild<QTabWidget *>()->setCurrentIndex(1);
      tree->expandAll(); tree->setCurrentItem(target); tree->itemDoubleClicked(target,0);
      CommandEditor *panel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<CommandEditor *>(widget)) panel=candidate;
      require(panel!=nullptr,"Mission double click did not open editor");
      auto *commandSource=panel->findChild<QPlainTextEdit *>("commandSource");
      commandSource->setFocus(); QApplication::processEvents();
      const auto scriptBeforeTextEdit=editor->toPlainText();
      const auto commandBeforeTextEdit=commandSource->toPlainText();
      commandSource->selectAll(); QApplication::clipboard()->setText("total = total + 4;");
      window.findChild<QAction *>("edit_paste")->trigger();
      require(commandSource->toPlainText()=="total = total + 4;" && editor->toPlainText()==scriptBeforeTextEdit,"Paste targeted the wrong document");
      QMenu *editMenu=nullptr;
      for (auto *menu:window.findChildren<QMenu *>()) if (menu->title()=="&Edit") editMenu=menu;
      require(editMenu!=nullptr,"Edit menu missing");
      editMenu->popup(window.mapToGlobal(QPoint(100,30))); QApplication::processEvents();
      editMenu->setActiveAction(window.findChild<QAction *>("edit_undo"));
      QKeyEvent activate(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
      QApplication::sendEvent(editMenu,&activate); QApplication::processEvents();
      require(commandSource->toPlainText()==commandBeforeTextEdit && editor->toPlainText()==scriptBeforeTextEdit,"Undo targeted the script instead of the command panel");
      window.findChild<QAction *>("edit_redo")->trigger();
      require(commandSource->toPlainText()=="total = total + 4;","Redo did not target command editor");
      tree->setFocus(); QApplication::processEvents();
      window.findChild<QAction *>("edit_undo")->trigger();
      require(editor->toPlainText()==scriptBeforeTextEdit && commandSource->toPlainText()=="total = total + 4;","Undo with tree focus changed an editor");
      require(panel->hasChanges() && !window.buildScript(),"Unapplied mission changes did not protect Build");
      if (!screenshot.isEmpty()) { QApplication::processEvents(); require(window.grab().save(screenshot),"Mission screenshot failed"); }
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(total()==15,"Mission panel Apply did not update the loop");
      editor->setPlainText("Create DifferentialCorrector DC;\nGMAT DC.MaximumIterations = 20;\n"
         "Create Variable x;\nGMAT x = 1;\nBeginMissionSequence;\n"
         "Target DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = true};\n"
         "Vary DC(x = 1, {Perturbation = 0.001, Lower = -20, Upper = 20, MaxStep = 10});\n"
         "Achieve DC(x = 7, {Tolerance = 0.000001});\nEndTarget;\n");
      require(window.buildScript(),"Targeting fixture did not build");
      snapshot=window.missionSnapshot();
      const int achieve=find(snapshot,"Achieve");
      QString replacement;
      CommandForm goalForm([&](const QString &text) { replacement=text; });
      goalForm.setStatement(snapshot.nodes[achieve].statement);
      auto *goal=goalForm.findChild<QLineEdit *>("commandField_Value");
      require(goal,"Engine-generated Achieve command has no form"); goal->setText("8");
      require(window.applyMissionChange(snapshot,achieve,MissionEdit::Replace,replacement).isEmpty(),"Achieve form edit rejected");
      require(window.runMission()==MainWindow::RunResult::Completed,"Targeting did not execute");
      auto solved=[&] { return Moderator::Instance()->GetInternalObject("x")->GetRealParameter("Value"); };
      require(std::abs(solved()-8)<1e-6,"Targeting did not achieve edited goal");
      auto *progress=window.findChild<QTableWidget *>("solverProgress");
      auto *solverStatus=window.findChild<QLabel *>("solverStatus");
      require(progress && progress->rowCount()>=2 && solverStatus && solverStatus->property("converged").toBool(),
         "Solver progress or convergence feedback missing");
      bool goalSeen=false;
      for (int row=0;row<progress->rowCount();++row) if (progress->item(row,0)->text()=="Goal =")
         goalSeen=progress->item(row,3)->text().toDouble()==8 && std::abs(progress->item(row,4)->text().toDouble())<1e-6;
      require(goalSeen,"Solver table did not show final goal/residual");
      auto *progressWindow=qobject_cast<QMdiSubWindow *>(progress->parentWidget()->parentWidget());
      require(progressWindow,"Solver progress is not an MDI window"); progressWindow->close();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(window.runMission()==MainWindow::RunResult::Completed && std::abs(solved()-8)<1e-6,
         "Closing solver progress broke repeated execution");
      require(window.findChild<QTableWidget *>("solverProgress"),"Rerun did not recreate closed solver progress");
      std::cout<<"PASS: nested branches, duplicate-command identity, replace/insert/delete/append, invalid-edit rollback, stale panel, undo, Mission tree and Apply\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
