#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextDocument>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QClipboard>
#include <QTabWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   QApplication::setOrganizationName("GMATTests");
   QApplication::setApplicationName("QtWorkflow");
   if (argc < 3 || argc > 4) return 2;
   const auto startup = QFileInfo(argv[1]).absoluteFilePath();
   const auto script = QFileInfo(argv[2]).absoluteFilePath();
   const auto screenshot = argc == 4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      MainWindow window;
      window.show();
      require(window.initialize(startup), "Runtime initialization failed");
      require(window.loadScript(script), "Script read failed");
      require(window.buildScript(), "Script interpretation failed");
      auto *sat = dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      require(sat != nullptr, "Configured spacecraft missing");
      const auto epoch = sat->GetEpoch();
      require(window.runMission() == MainWindow::RunResult::Completed, "Propagation did not complete");
      auto *propagated = dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"));
      require(propagated != nullptr, "Propagated spacecraft missing");
      require(std::abs((propagated->GetEpoch() - epoch) * 86400.0 - 600.0) < 0.01,
         "Mission did not propagate the requested 600 seconds");
      require(window.runMission() == MainWindow::RunResult::Completed, "Repeat execution failed");
      auto *editor = window.findChild<QPlainTextEdit *>("scriptEditor");
      require(editor != nullptr, "Script editor missing");
      const QString beforeEdit = editor->toPlainText();
      auto dryMass = [] { return Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("DryMass"); };
      const double originalMass = dryMass();
      require(!window.applyResourceChanges("QtSat", {{"DryMass", "not-a-number"}}, beforeEdit).isEmpty(),
         "Invalid numeric input was accepted");
      require(dryMass() == originalMass && editor->toPlainText() == beforeEdit,
         "Rejected edit changed the mission");
      const auto editError = window.applyResourceChanges("QtSat", {{"DryMass", "975.5"}}, beforeEdit);
      if (!editError.isEmpty()) std::cerr << editError.toStdString() << '\n';
      require(editError.isEmpty(), "Resource edit failed");
      require(dryMass() == 975.5 && editor->document()->isModified(), "Resource edit not synchronized to script");
      const auto afterEdit = editor->toPlainText();
      require(!window.applyResourceChanges("QtSat", {{"DryMass", "1000"}}, beforeEdit).isEmpty(),
         "Stale resource panel overwrote current mission");
      require(dryMass() == 975.5, "Stale resource edit mutated model");
      require(!window.applyResourceChanges("QtSat", {{"CoordinateSystem", "NoSuchFrame"}}, afterEdit).isEmpty(),
         "Invalid reference was accepted");
      require(dryMass() == 975.5 && editor->toPlainText() == afterEdit, "Failed model validation was not rolled back");
      editor->undo();
      require(editor->toPlainText() == beforeEdit, "Resource edit destroyed script undo history");
      require(window.buildScript() && dryMass() == originalMass, "Undo did not restore mission settings");
      editor->setPlainText("Create Variable count;\nBeginMissionSequence;\nWhile count < 1e12;\ncount = count + 1;\nEndWhile;\n");
      bool paused = false, resumed = false, protectedEdits = false;
      double countAtPause = 0;
      auto counter = [] { return Moderator::Instance()->GetInternalObject("count")->GetRealParameter("Value"); };
      QTimer::singleShot(40, &window, [&] {
         protectedEdits = window.isRunning() && editor->isReadOnly() && !window.buildScript()
            && window.runMission() == MainWindow::RunResult::Busy && !window.loadScript(script);
         window.pauseMission();
         paused = Moderator::Instance()->GetRunState() == Gmat::PAUSED;
         countAtPause = counter();
      });
      QTimer::singleShot(110, &window, [&] {
         paused = paused && counter() == countAtPause;
         window.resumeMission();
         resumed = Moderator::Instance()->GetRunState() == Gmat::RUNNING;
      });
      QTimer::singleShot(200, &window, [&] { window.stopMission(); });
      require(window.runMission() == MainWindow::RunResult::Stopped, "Stop was not reported as interruption");
      require(paused && resumed && protectedEdits, "Pause/resume or reentrancy protection failed");
      require(!window.isRunning() && !editor->isReadOnly(), "Run controls did not reset");
      bool closeBlocked = false;
      QTimer::singleShot(40, &window, [&] { closeBlocked = !window.close() && window.isVisible(); });
      require(window.runMission() == MainWindow::RunResult::Stopped && closeBlocked,
         "Window closed before stopping the active engine");
      editor->setPlainText("Not a valid GMAT command;\n");
      require(window.runMission() == MainWindow::RunResult::Failed, "Invalid script ran the previous mission");
      require(window.loadScript(script), "Could not recover after invalid script");
      require(window.runMission() == MainWindow::RunResult::Completed, "Recovery execution failed");
      auto *tree = window.findChild<QTreeWidget *>("Resources");
      require(tree != nullptr, "Resource tree missing");
      const auto items = tree->findItems("QtSat", Qt::MatchExactly | Qt::MatchRecursive);
      require(items.size() == 1, "Spacecraft resource not displayed");
      tree->itemDoubleClicked(items.first(), 0);
      ResourceEditor *panel = nullptr;
      for (auto *widget : window.findChildren<QWidget *>())
         if (auto *candidate = dynamic_cast<ResourceEditor *>(widget)) panel = candidate;
      require(panel != nullptr, "Resource double click did not open editable panel");
      auto *table = panel->findChild<QTableWidget *>();
      require(table != nullptr, "Resource property table missing");
      bool massFound = false;
      for (int row = 0; row < table->rowCount(); ++row) {
         if (table->item(row, 0)->text() == "DryMass") {
            const auto unchangedScript=editor->toPlainText();
            const auto originalText=table->item(row,1)->text();
            table->editItem(table->item(row,1)); QApplication::processEvents();
            auto *field=qobject_cast<QLineEdit *>(QApplication::focusWidget());
            require(field!=nullptr,"Property cell editor did not receive focus");
            field->selectAll(); QApplication::clipboard()->setText("888.25");
            window.findChild<QAction *>("edit_paste")->trigger();
            require(field->text()=="888.25" && editor->toPlainText()==unchangedScript,"Property Paste modified the script");
            window.findChild<QAction *>("edit_undo")->trigger();
            require(field->text()==originalText && editor->toPlainText()==unchangedScript,"Property Undo modified the script");
            window.findChild<QAction *>("edit_redo")->trigger();
            panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->setFocus();
            QApplication::processEvents();
            require(table->item(row,1)->text()=="888.25","Edited cell did not commit on focus change");
            massFound = true; break;
         }
      }
      require(massFound && panel->hasChanges(), "Mass is not editable in the panel");
      tree->itemDoubleClicked(items.first(),0);
      int resourcePanelCount=0;
      for (auto *widget:window.findChildren<QWidget *>())
         if (dynamic_cast<ResourceEditor *>(widget)) ++resourcePanelCount;
      require(resourcePanelCount==1 && panel->hasChanges(),"Reopening a resource duplicated its panel or lost pending edits");
      require(!window.buildScript() && window.runMission() == MainWindow::RunResult::Failed,
         "Unapplied resource settings were silently ignored during run");
      require(!window.loadScript(script), "Loading discarded pending resource settings");
      require(!window.createResource("Spacecraft","PendingSat",editor->toPlainText()).isEmpty(),"Creation ignored pending panel changes");
      if (!screenshot.isEmpty()) {
         QApplication::processEvents();
         require(window.grab().save(screenshot), "Resource screenshot failed");
      }
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(dryMass() == 888.25, "Apply button did not commit the entered value");
      const auto beforeCreate=editor->toPlainText();
      for (const auto &name:{"QtSat","Earth","bad name","Bad; Stop;"})
         require(!window.createResource("Spacecraft",name,beforeCreate).isEmpty(),"Invalid or duplicate name accepted");
      require(!window.createResource("NoSuchType","CreatedSat",beforeCreate).isEmpty(),"Unknown type accepted");
      require(window.createResource("Spacecraft","CreatedSat",beforeCreate).isEmpty(),"Spacecraft creation failed");
      require(Moderator::Instance()->GetConfiguredObject("CreatedSat") && editor->document()->isModified(),"Created spacecraft not synchronized");
      require(!window.createResource("Spacecraft","StaleSat",beforeCreate).isEmpty(),"Stale creation snapshot accepted");
      editor->undo(); require(editor->toPlainText()==beforeCreate && window.buildScript(),"Creation could not be undone");
      require(!Moderator::Instance()->GetConfiguredObject("CreatedSat"),"Undo retained created resource");
      bool createdThroughDialog=false;
      bool capturedCreateDialog=screenshot.isEmpty();
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         if (!dialog) return;
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("Spacecraft");
         dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedSat");
         if (!screenshot.isEmpty()) capturedCreateDialog=dialog->grab().save(screenshot+".create.png");
         dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         createdThroughDialog=dialog->result()==QDialog::Accepted;
         if (!createdThroughDialog) dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger();
      require(createdThroughDialog && tree->findItems("CreatedSat",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"New resource dialog did not populate tree");
      require(capturedCreateDialog,"Could not capture resource creation dialog");
      const auto createdEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("CreatedSat"))->GetEpoch();
      require(window.applyMissionChange(window.missionSnapshot(),-1,MissionEdit::Append,
         "Propagate QtProp(CreatedSat) {CreatedSat.ElapsedSecs = 60};").isEmpty(),"Created spacecraft could not be used by mission");
      require(window.runMission()==MainWindow::RunResult::Completed,"Mission with created spacecraft failed");
      auto *createdResult=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("CreatedSat"));
      require(createdResult && std::abs((createdResult->GetEpoch()-createdEpoch)*86400-60)<.01,"Created spacecraft did not propagate");
      require(window.applyResourceChanges("CreatedSat",{{"DryMass","932"}},editor->toPlainText()).isEmpty(),"Could not update resource behind clean panel");
      const auto currentCreated=tree->findItems("CreatedSat",Qt::MatchExactly|Qt::MatchRecursive);
      tree->itemDoubleClicked(currentCreated.first(),0);
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      bool refreshedMass=false;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *resource=dynamic_cast<ResourceEditor *>(widget)) {
         if (resource->parentWidget()->property("resourceName").toString()!="CreatedSat") continue;
         auto *properties=resource->findChild<QTableWidget *>();
         for (int row=0;row<properties->rowCount();++row)
            if (properties->item(row,0)->text()=="DryMass") refreshedMass=properties->item(row,1)->text()=="932";
      }
      require(refreshedMass,"Reopening a clean stale resource panel did not refresh its values");
      QApplication::processEvents();
      auto *missionTree=window.findChild<QTreeWidget *>("Mission");
      QTreeWidgetItem *propagationItem=nullptr;
      for (QTreeWidgetItemIterator it(missionTree);*it;++it) {
         if (!(*it)->data(0,Qt::UserRole).isValid()) continue;
         const auto &node=window.missionSnapshot().nodes[(*it)->data(0,Qt::UserRole).toInt()];
         if (node.type=="Propagate" && node.statement.contains("QtSat")) propagationItem=*it;
      }
      require(propagationItem!=nullptr,"Propagation command missing from tree");
      window.findChild<QTabWidget *>()->setCurrentIndex(1);
      missionTree->expandAll(); missionTree->itemDoubleClicked(propagationItem,0);
      CommandEditor *commandPanel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<CommandEditor *>(widget)) commandPanel=candidate;
      require(commandPanel!=nullptr,"Propagation command panel missing");
      auto *form=commandPanel->findChild<QWidget *>("propagationForm");
      auto *commandText=commandPanel->findChild<QPlainTextEdit *>("commandSource");
      require(form && !form->isHidden(),"Simple propagation form unavailable");
      const QString originalCommand=commandText->toPlainText();
      const QString advanced="Propagate QtProp(QtSat) {QtSat.ElapsedSecs = 600, QtSat.Earth.Periapsis};";
      commandText->setPlainText(advanced);
      require(form->isHidden() && commandText->toPlainText()==advanced,"Form simplified an advanced command");
      commandText->setPlainText(originalCommand);
      auto *duration=commandPanel->findChild<QLineEdit *>("propagationDuration");
      auto *units=commandPanel->findChild<QComboBox *>("propagationUnits");
      require(!form->isHidden() && duration->text().toDouble()==600,"Form did not load command duration");
      auto *selectedSpacecraft=commandPanel->findChild<QComboBox *>("propagationSpacecraft");
      selectedSpacecraft->setCurrentText("CreatedSat");
      require(commandText->toPlainText().contains("QtProp(CreatedSat) {CreatedSat.ElapsedSecs"),"Spacecraft selection did not update its stop condition");
      selectedSpacecraft->setCurrentText("QtSat");
      const auto beforeForm=editor->toPlainText();
      duration->setText("not-a-number");
      auto *applyCommand=commandPanel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
      applyCommand->click();
      require(commandPanel->hasChanges() && editor->toPlainText()==beforeForm,"Invalid form input altered the mission");
      units->setCurrentIndex(units->findData("ElapsedDays")); duration->setText("0.01");
      require(commandText->toPlainText().contains("QtSat.ElapsedDays = 0.01"),"Form did not synchronize duration and units");
      if (!screenshot.isEmpty()) { duration->setFocus(); QApplication::processEvents(); require(window.grab().save(screenshot+".propagation.png"),"Propagation form capture failed"); }
      applyCommand->click();
      const auto formEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetEpoch();
      require(window.runMission()==MainWindow::RunResult::Completed,"Form-edited propagation failed");
      const auto finalEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpoch();
      require(std::abs((finalEpoch-formEpoch)*86400-864)<.01,"Form-selected duration did not propagate for 0.01 days");
      std::cout<<"PASS: propagation form, advanced-command preservation, invalid-input rollback and actual elapsed-days execution\n";
      std::cout<<"PASS: resource creation, name/type validation, pending/stale protection, undo, dialog/tree integration and actual propagation\n";
      std::cout << "PASS: propagation, repeated runs, pause/resume/stop, edit protection, close protection, invalid-script recovery, resource apply/rollback/stale-panel protection/undo\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr << "FAIL: " << error.what() << '\n';
      return 1;
   }
}
