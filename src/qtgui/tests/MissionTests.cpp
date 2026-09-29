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
#include <QHeaderView>
#include <QLabel>
#include <QMdiSubWindow>
#include <QTimer>
#include <QInputDialog>
#include <QTemporaryDir>
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
         const QString loop="For 'Keep loop' i = 1:2:7;\n   total = total + i; % body stays exact\nEndFor;";
         form.setStatement(loop);
         auto *step=form.findChild<QLineEdit *>("commandField_Step");
         require(step && !form.isHidden(),"For controls missing");
         step->setText("3"); expected=loop; expected.replace("1:2:7","1:3:7");
         require(result==expected,"For control changed branch contents or label");
         const QString condition="If 'Choose' total > 2 & i < 9;\n total = 8;\nElse;\n total = 4;\nEndIf;";
         form.setStatement(condition);
         auto *predicate=form.findChild<QLineEdit *>("commandField_Condition");
         require(predicate,"Conditional controls missing"); predicate->setText("total >= 3");
         expected=condition; expected.replace("total > 2 & i < 9","total >= 3");
         require(result==expected,"Condition editor changed branch bodies");
         const QString assignment="GMAT total = sqrt(4) + 3; % preserve comment\n";
         form.setStatement(assignment);
         auto *expression=form.findChild<QLineEdit *>("commandField_Expression");
         require(expression,"Assignment controls missing"); expression->setText("sqrt(9) + 4");
         expected=assignment; expected.replace("sqrt(4) + 3","sqrt(9) + 4");
         require(result==expected,"Assignment control changed other source");
         const QString branch="Target 'Keep label' DC {ExitMode = SaveAndContinue, SolveMode = Solve};\n   Vary DC(x = 1);\nEndTarget;";
         form.setStatement(branch);
         auto *exitMode=form.findChild<QLineEdit *>("commandField_ExitMode");
         require(exitMode,"Target branch form missing"); exitMode->setText("DiscardAndContinue");
         expected=branch; expected.replace("SaveAndContinue","DiscardAndContinue");
         require(result==expected,"Solver form changed nested command source");
         for (const auto &statement:{"Maneuver 'Keep label' Burn(Sat);","BeginFiniteBurn Burn(Sat);",
              "Achieve DC(x = 7, {Tolerance = 0.001});","Minimize Opt(cost);",
              "NonlinearConstraint Opt(x <= 4);","Report R x y;","FindEvents Locator {Append = true};",
              "[out] = cross(vec1, vec2);","myFunction(input);",
              "Toggle PlotA PlotB Off;","Global Sat total;","Clear Sat total;",
              "For i = 1:3;\nEndFor;","While total < 3;\nEndWhile;"}) {
            form.setStatement(statement); require(!form.isHidden(),"Command-specific form missing");
         }
         form.setStatement("% Preserve advanced script\nUnknown foo;"); require(form.isHidden(),"Unknown command was simplified");
      }
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime failed");
      require(window.loadScript(script) && window.buildScript(),"Mission failed to build");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString original=editor->toPlainText();
      {
         editor->setPlainText("Create Spacecraft FirstSat SecondSat;\nCreate ImpulsiveBurn FirstBurn SecondBurn;\n"
            "SecondBurn.CoordinateSystem = EarthMJ2000Eq;\nSecondBurn.Element1 = 0.01;\n"
            "Create FiniteBurn Continuous;\nCreate DifferentialCorrector DC;\nCreate Yukon Opt;\n"
            "Create EclipseLocator Eclipse;\nEclipse.Spacecraft = FirstSat;\nEclipse.RunMode = Manual;\n"
            "BeginMissionSequence;\nManeuver 'Named burn' FirstBurn(FirstSat); % keep\n");
         require(window.buildScript(),"Resource picker fixture failed");
         auto pick=[&](CommandForm &form,const QString &field,const QString &chosen,const QString &excluded,bool accept=true) {
            bool visited=false;
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QInputDialog *>();
               require(dialog && dialog->comboBoxItems().contains(chosen),"Resource picker omitted compatible resource");
               require(excluded.isEmpty() || !dialog->comboBoxItems().contains(excluded),"Resource picker included incompatible resource");
               dialog->setTextValue(chosen); visited=true;
               if (accept) dialog->accept(); else dialog->reject();
            });
            auto *button=form.findChild<QPushButton *>("commandChoose_"+field); require(button,"Command reference selector missing"); button->click();
            require(visited,"Command resource dialog did not open");
         };
         QString replacement; CommandForm form([&](const auto &value) { replacement=value; });
         form.setStatement("Maneuver 'Named burn' FirstBurn(FirstSat); % keep");
         pick(form,"Burn","SecondBurn","Continuous"); pick(form,"Spacecraft","SecondSat","SecondBurn");
         const auto accepted=replacement;
         pick(form,"Burn","FirstBurn","Continuous",false);
         require(replacement==accepted && replacement=="Maneuver 'Named burn' SecondBurn(SecondSat); % keep", "Resource pickers changed labels, comments or cancelled values");
         const auto snapshot=window.missionSnapshot();
         require(window.applyMissionChange(snapshot,find(snapshot,"Maneuver"),MissionEdit::Replace,replacement).isEmpty(),"Picked maneuver references failed to apply");
         QTemporaryDir selectedMission;
         require(selectedMission.isValid() && window.saveScriptTo(selectedMission.filePath("picked.script")) &&
            window.loadScript(selectedMission.filePath("picked.script")) && window.buildScript(),"Picked command save/reopen failed");
         auto *before=Moderator::Instance()->GetConfiguredObject("SecondSat"); const double vx=before->GetRealParameter("VX");
         require(window.runMission()==MainWindow::RunResult::Completed,"Picked maneuver failed to execute");
         auto *after=Moderator::Instance()->GetInternalObject("SecondSat");
         require(std::abs(after->GetRealParameter("VX")-vx-.01)<1e-10,"Picked inertial burn did not change selected spacecraft velocity");
         form.setStatement("BeginFiniteBurn Continuous(FirstSat);"); pick(form,"Burn","Continuous","SecondBurn");
         form.setStatement("Target DC;\nEndTarget;"); pick(form,"Solver","DC","Opt");
         form.setStatement("Optimize Opt;\nEndOptimize;"); pick(form,"Solver","Opt","DC");
         form.setStatement("Achieve DC(FirstSat.X = 1);"); pick(form,"Solver","DC","Opt");
         form.setStatement("Minimize Opt(FirstSat.X);"); pick(form,"Solver","Opt","DC");
         form.setStatement("NonlinearConstraint Opt(FirstSat.X >= 1);"); pick(form,"Solver","Opt","DC");
         form.setStatement("Vary DC(FirstSat.X = 1);"); pick(form,"Solver","Opt",{}); pick(form,"Solver","DC",{});
         form.setStatement("FindEvents Eclipse;"); pick(form,"Locator","Eclipse","DC");
         editor->setPlainText(original); require(window.buildScript(),"Picker fixture restoration failed");
      }

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
      {
         QString changed;
         CommandForm form([&](const QString &value) { changed=value; });
         snapshot=window.missionSnapshot();
         const auto index=find(snapshot,"For");
         form.setStatement(snapshot.nodes[index].statement);
         auto *step=form.findChild<QLineEdit *>("commandField_Step");
         require(step,"Real mission For header has no step control"); step->setText("2");
         require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed).isEmpty() && total()==5,
            "For form edit did not execute with the new step");
         editor->undo(); require(window.buildScript() && total()==6,"For form undo changed mission results");
         snapshot=window.missionSnapshot();
         const auto condition=find(snapshot,"If");
         form.setStatement(snapshot.nodes[condition].statement);
         auto *predicate=form.findChild<QLineEdit *>("commandField_Condition");
         if (!predicate) std::cerr<<"Condition source: "<<snapshot.nodes[condition].statement.toStdString()<<std::endl;
         require(predicate,"Real mission If header has no condition control"); predicate->setText("count ~= 0");
         require(window.applyMissionChange(snapshot,condition,MissionEdit::Replace,changed).isEmpty() && total()==-196,
            "Condition form edit did not execute the correct branch");
         editor->undo(); require(window.buildScript() && total()==6,"Condition form undo changed mission results");
      }
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
      for (int column=0;column<progress->columnCount();++column)
         require(progress->horizontalHeader()->sectionResizeMode(column)==QHeaderView::Interactive,"Solver column cannot be dragged");
      progress->setColumnWidth(1,301); progress->setColumnWidth(4,177);
      require(window.runMission()==MainWindow::RunResult::Completed,"Solver resize repeat mission failed");
      require(progress->columnWidth(1)==301 && progress->columnWidth(4)==177,"Live solver updates overwrote adjusted widths");
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
