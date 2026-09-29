#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "CommandEditor.hpp"
#include "CommandForm.hpp"
#include "ConditionDialog.hpp"
#include "ReportParameterDialog.hpp"
#include <QSpinBox>
#include <QComboBox>
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
#include <QCheckBox>
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
         form.findChild<QPushButton *>("commandAddOptions")->click();
         require(result.contains("Upper = 25") && result.contains("x = 3.5") && result.contains("% keep this") &&
            result.count("Perturbation =")==1 && result.contains("MultiplicativeScaleFactor = 1.0"),"Adding Vary defaults lost pending values or duplicated options");
         require(!form.findChild<QPushButton *>("commandAddOptions"),"Vary defaults can be added twice");
         for (const auto &command:{QString("Vary"),QString("Achieve")}) {
            form.setStatement(command+" 'Keep name' DC(x = 2); % keep comment");
            form.findChild<QLineEdit *>("commandField_Solver")->setText("OtherDC");
            form.findChild<QPushButton *>("commandAddOptions")->click();
            require(result.startsWith(command+" 'Keep name' OtherDC(x = 2, {") && result.endsWith("}); % keep comment"),
               "Omitted solver options inserted outside argument list or lost source");
            auto *setting=form.findChild<QLineEdit *>(command=="Vary" ? "commandField_MaxStep" : "commandField_Tolerance");
            require(setting,"Added solver options did not expose editable controls"); setting->setText("0.02");
            require(result.contains(command=="Vary" ? "MaxStep = 0.02" : "Tolerance = 0.02"),"Added solver option cannot be edited");
         }
         const QString loop="For 'Keep loop' i = 1:2:7;\n   total = total + i; % body stays exact\nEndFor;";
         form.setStatement(loop);
         auto *step=form.findChild<QLineEdit *>("commandField_Step");
         require(step && !form.isHidden(),"For controls missing");
         step->setText("3"); expected=loop; expected.replace("1:2:7","1:3:7");
         require(result==expected,"For control changed branch contents or label");
         const QString condition="If 'Choose' total > 2 & i < 9;\n total = 8;\nElse;\n total = 4;\nEndIf;";
         form.setStatement(condition);
         auto *predicate=form.findChild<QLineEdit *>("commandField_Condition");
         require(predicate,"Conditional controls missing");
         auto *builder=form.findChild<QPushButton *>("commandChoose_Condition");
         require(builder && builder->isEnabled(),"Compound condition builder missing");
         QTimer::singleShot(0,&form,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
               dialog->findChild<QTableWidget *>("conditionTable")->item(0,1)->setText("discarded"); dialog->reject();
            }
         }); builder->click(); require(predicate->text()=="total > 2 & i < 9","Cancel changed the condition");
         bool emptyRejected=false;
         QTimer::singleShot(0,&form,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            auto *table=dialog->findChild<QTableWidget *>("conditionTable");
            table->setCurrentCell(1,1); dialog->findChild<QPushButton *>("conditionRemove")->click();
            dialog->findChild<QPushButton *>("conditionAdd")->click();
            emptyRejected=!dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled();
            dialog->findChild<QPushButton *>("conditionRemove")->click();
            static_cast<QComboBox *>(table->cellWidget(0,2))->setCurrentText(">="); table->item(0,3)->setText("3"); dialog->accept();
         }); builder->click(); require(emptyRejected,"Incomplete condition row was accepted");
         expected=condition; expected.replace("total > 2 & i < 9","total >= 3");
         require(result==expected,"Condition editor changed branch bodies");
         predicate->setText("(total > 2 | i < 9)");
         require(!builder->isEnabled(),"Unsupported grouped syntax was offered a lossy conversion");
         require(ConditionDialog::supports("A(1, 2) >= -1.2e-3 | total ~= 0"),"Array/numeric compound condition was rejected");

         const QString bareBranch="Target 'Keep label' DC; % keep header\n   total = total + 1; % keep body\nEndTarget;";
         form.setStatement(bareBranch);
         auto *defaults=form.findChild<QPushButton *>("commandAddOptions"); require(defaults,"Omitted solver options cannot be exposed"); defaults->click();
         require(result.contains("SolveMode = Solve") && result.contains("ExitMode = DiscardAndContinue") && result.contains("ShowProgressWindow = true") &&
            result.endsWith("; % keep header\n   total = total + 1; % keep body\nEndTarget;"),"Default options damaged branch source");
         auto *solveMode=form.findChild<QComboBox *>("commandChoice_SolveMode"),*exitChoice=form.findChild<QComboBox *>("commandChoice_ExitMode");
         require(solveMode && solveMode->findText("RunInitialGuess")>=0 && exitChoice && exitChoice->findText("Stop")>=0,"Solver option choices missing");
         solveMode->setCurrentText("RunInitialGuess"); exitChoice->setCurrentText("SaveAndContinue");
         form.findChild<QCheckBox *>("commandCheck_ShowProgressWindow")->setChecked(false);
         require(result.contains("SolveMode = RunInitialGuess") && result.contains("ExitMode = SaveAndContinue") && result.contains("ShowProgressWindow = false"),"Solver controls did not update source");
         form.setStatement("Optimize Opt {ExitMode = Stop};\nEndOptimize;");
         form.findChild<QLineEdit *>("commandField_Solver")->setText("OtherOpt");
         form.findChild<QPushButton *>("commandAddOptions")->click();
         require(result.contains("Optimize OtherOpt") && result.contains("ExitMode = Stop") && result.count("ExitMode")==1,"Adding missing options lost a pending field or replaced an existing option");
         form.setStatement("FindEvents 'Find' Locator; % keep");
         form.findChild<QLineEdit *>("commandField_Locator")->setText("OtherLocator");
         form.findChild<QPushButton *>("commandAddOptions")->click();
         require(result=="FindEvents 'Find' OtherLocator {Append = false}; % keep","Omitted Append lost pending locator or source");
         require(!form.findChild<QPushButton *>("commandAddOptions"),"Existing Append offered duplicate insertion");
         form.setStatement("FindEvents 'Find' Locator {Append = false}; % keep");
         form.findChild<QCheckBox *>("commandCheck_Append")->setChecked(true);
         require(result=="FindEvents 'Find' Locator {Append = true}; % keep","Append checkbox changed unrelated command text");
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
         const auto forward=window.missionSnapshot(); const int maneuver=find(forward,"Maneuver");
         form.setStatement(forward.nodes[maneuver].statement);
         auto *backprop=form.findChild<QCheckBox *>("commandBackProp"); require(backprop && !backprop->isChecked(),"Backprop control missing or initially enabled");
         backprop->setChecked(true);
         require(replacement.contains("BackProp SecondBurn(SecondSat)") && replacement.contains("'Named burn'"),
            "Backprop toggle displaced burn reference or command label");
         require(window.applyMissionChange(forward,maneuver,MissionEdit::Replace,replacement).isEmpty(),"Backprop command edit failed");
         require(window.saveScriptTo(selectedMission.filePath("backprop.script")) && window.loadScript(selectedMission.filePath("backprop.script")) &&
            window.runMission()==MainWindow::RunResult::Completed,"Backprop save/reopen or execution failed");
         require(std::abs(Moderator::Instance()->GetInternalObject("SecondSat")->GetRealParameter("VX")-vx+.01)<1e-10,
            "Backprop did not reverse inertial delta-V");
         form.setStatement(replacement); backprop=form.findChild<QCheckBox *>("commandBackProp");
         require(backprop->isChecked(),"Existing BackProp keyword not reflected in control"); backprop->setChecked(false);
         require(!replacement.contains("BackProp") && replacement.contains("SecondBurn(SecondSat)"),"Clearing Backprop damaged command");
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
         require(step,"Real mission For header has no step control");
         QTimer::singleShot(0,[&] {
            auto *dialog=form.findChild<QInputDialog *>();
            require(dialog && dialog->comboBoxItems().contains("count"),"For index picker omitted Variable");
            for (const auto &name:dialog->comboBoxItems())
               require(Moderator::Instance()->GetConfiguredObject(name.toStdString())->IsOfType("Variable"),"For index picker included a non-variable");
            dialog->setTextValue("total"); dialog->reject();
         }); form.findChild<QPushButton *>("commandChoose_Index")->click();
         require(form.findChild<QLineEdit *>("commandField_Index")->text()=="count","Cancelled For index changed source");
         for (const auto &field:{QString("Start"),QString("Step"),QString("End")}) {
            QTimer::singleShot(0,[&] {
               auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
               auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");
               require(entry && entry->findText("total")>=0,"For bound picker omitted configured parameter");
               entry->setEditText(field=="Step" ? "2" : field=="Start" ? "1" : "3");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            }); form.findChild<QPushButton *>("commandChoose_"+field)->click();
         }
         require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed).isEmpty() && total()==5,
            "For form edit did not execute with the new step");
         editor->undo(); require(window.buildScript() && total()==6,"For form undo changed mission results");
         snapshot=window.missionSnapshot();
         const auto condition=find(snapshot,"If");
         form.setStatement(snapshot.nodes[condition].statement);
         auto *predicate=form.findChild<QLineEdit *>("commandField_Condition");
         if (!predicate) std::cerr<<"Condition source: "<<snapshot.nodes[condition].statement.toStdString()<<std::endl;
         require(predicate,"Real mission If header has no condition control");
         QTimer::singleShot(0,&form,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            auto *table=dialog->findChild<QTableWidget *>("conditionTable");
            table->item(0,1)->setText("count"); static_cast<QComboBox *>(table->cellWidget(0,2))->setCurrentText("~=");
            table->item(0,3)->setText("0"); dialog->accept();
         }); form.findChild<QPushButton *>("commandChoose_Condition")->click();
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
         "Create Variable x goalValue;\nCreate String Note;\nCreate Spacecraft FilterSat;\nCreate Array Choice[2,3];\nGMAT x = 1;\nGMAT goalValue = 8;\nBeginMissionSequence;\n"
         "Target DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = true};\n"
         "Vary DC(goalValue = 1, {Perturbation = 0.001, Lower = -20, Upper = 20, MaxStep = 10});\n"
         "Achieve DC(x = 7, {Tolerance = 0.000001});\nEndTarget;\n");
      require(window.buildScript(),"Targeting fixture did not build");
      {
         ReportParameterDialog numeric({"x"},&window,ReportParameterDialog::Mode::WritableReal);
         auto *entry=numeric.findChild<QComboBox *>("reportParameterEntry");
         auto *ok=numeric.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
         require(entry->findText("x")>=0 && entry->findText("Choice")>=0 && entry->findText("Note")<0,"Vary picker offers nonnumeric user values");
         auto *owner=numeric.findChild<QComboBox *>("reportPropertyObject"); owner->setCurrentText("FilterSat");
         auto *properties=numeric.findChild<QComboBox *>("reportPropertyType");
         require(properties->findText("X")>=0 && properties->findText("ElapsedSecs")<0,"Writable property browser includes read-only elapsed time");
         entry->setEditText("FilterSat.ElapsedSecs"); require(!ok->isEnabled(),"Read-only typed parameter was accepted");
         entry->setEditText("42"); require(!ok->isEnabled(),"Numeric literal accepted as writable destination");
         ReportParameterDialog destination({"Choice"},&window,ReportParameterDialog::Mode::Writable);
         require(destination.findChild<QComboBox *>("reportParameterEntry")->findText("Note")>=0 &&
            destination.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Assignment picker excluded string or whole array");
         const auto current=window.missionSnapshot(); QString varied;
         CommandForm varyForm([&](const QString &value) { varied=value; }); varyForm.setStatement(current.nodes[find(current,"Vary")].statement);
         bool selected=false;
         QTimer::singleShot(0,&varyForm,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); selected=entry->findText("x")>=0;
            entry->setCurrentText("x"); dialog->accept();
         }); varyForm.findChild<QPushButton *>("commandChoose_Variable")->click();
         require(selected && window.applyMissionChange(current,find(current,"Vary"),MissionEdit::Replace,varied).isEmpty(),"Selected Vary variable did not apply");
      }

      {
         const auto current=window.missionSnapshot(); QString changed;
         CommandForm optionsForm([&](const QString &value) { changed=value; }); optionsForm.setStatement("Vary DC(x = 1);");
         optionsForm.findChild<QPushButton *>("commandAddOptions")->click();
         optionsForm.findChild<QLineEdit *>("commandField_Lower")->setText("-20");
         optionsForm.findChild<QLineEdit *>("commandField_Upper")->setText("20");
         optionsForm.findChild<QLineEdit *>("commandField_MaxStep")->setText("2");
         require(window.applyMissionChange(current,find(current,"Vary"),MissionEdit::Replace,changed).isEmpty(),"Added Vary options did not apply");
      }
      snapshot=window.missionSnapshot();
      const int achieve=find(snapshot,"Achieve");
      QString replacement;
      CommandForm goalForm([&](const QString &text) { replacement=text; });
      goalForm.setStatement("Achieve DC(x = 7);");
      goalForm.findChild<QPushButton *>("commandAddOptions")->click();
      goalForm.findChild<QLineEdit *>("commandField_Tolerance")->setText("0.000001");
      auto *goal=goalForm.findChild<QLineEdit *>("commandField_Value");
      require(goal,"Engine-generated Achieve command has no form");
      {
         ReportParameterDialog selector({"x"},&window,ReportParameterDialog::Mode::Single);
         auto *entry=selector.findChild<QComboBox *>("reportParameterEntry");
         auto *ok=selector.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
         require(selector.selection()==QStringList{"x"} && selector.findChild<QWidget *>("reportSelectedParameters")->isHidden(),"Single chooser displayed a report list or lost initial value");
         entry->setEditText(""); require(!ok->isEnabled(),"Empty single selection enabled OK");
         entry->setEditText("Choice"); require(!ok->isEnabled(),"Bare array accepted as a scalar operand");
         selector.findChild<QSpinBox *>("reportArrayRow")->setValue(2);
         selector.findChild<QSpinBox *>("reportArrayColumn")->setValue(3);
         selector.findChild<QPushButton *>("reportAddElement")->click();
         require(ok->isEnabled() && selector.selection()==QStringList{"Choice(2,3)"},"Single array-element selection failed");
      }
      bool picked=false;
      QTimer::singleShot(0,&goalForm,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
         auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");
         picked=entry && entry->findText("goalValue")>=0; if (entry) entry->setCurrentText("goalValue"); dialog->accept();
      }); goalForm.findChild<QPushButton *>("commandChoose_Value")->click();
      require(picked && goal->text()=="goalValue","Achieve target parameter was not selected");
      QTimer::singleShot(0,&goalForm,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
         dialog->findChild<QComboBox *>("reportParameterEntry")->setCurrentText("x"); dialog->reject();
      }); goalForm.findChild<QPushButton *>("commandChoose_Value")->click();
      require(goal->text()=="goalValue","Cancel changed the selected target");
      require(window.applyMissionChange(snapshot,achieve,MissionEdit::Replace,replacement).isEmpty(),"Achieve form edit rejected");
      {
         const auto before=editor->toPlainText(); const auto current=window.missionSnapshot(); QString changed;
         CommandForm modeForm([&](const QString &value) { changed=value; }); modeForm.setStatement(current.nodes[find(current,"Target")].statement);
         modeForm.findChild<QComboBox *>("commandChoice_SolveMode")->setCurrentText("RunInitialGuess");
         require(window.applyMissionChange(current,find(current,"Target"),MissionEdit::Replace,changed).isEmpty() &&
            window.runMission()==MainWindow::RunResult::Completed,"Selected initial-guess mode failed");
         require(std::abs(Moderator::Instance()->GetInternalObject("x")->GetRealParameter("Value")-1)<1e-12,"Initial-guess mode unexpectedly solved the target");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Solver mode Undo failed");
      }
      QTemporaryDir selectedGoal;
      require(selectedGoal.isValid() && window.saveScriptTo(selectedGoal.filePath("target.script")) &&
         window.loadScript(selectedGoal.filePath("target.script")) && window.buildScript(),"Selected target save/reopen failed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Targeting did not execute");
      auto solved=[&] { return Moderator::Instance()->GetInternalObject("x")->GetRealParameter("Value"); };
      require(std::abs(solved()-8)<1e-6,"Targeting did not achieve edited goal");
      require(window.findChildren<QTableWidget *>("solverProgress").size()==1,"Edited solver mode left a stale progress window");
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
