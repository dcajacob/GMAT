#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ResourceProperties.hpp"
#include "ScriptCompatibility.hpp"
#include "Rvector.hpp"
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
#include <QTabBar>
#include <QMenu>
#include <QMessageBox>
#include <QKeyEvent>
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
      TestSettings isolatedSettings;
      {
         const QString dynamics="BeginMissionSequence;\nGMAT total = 2 + 3; % scientific code stays exact\n";
         const QString input="% Create OpenFramesInterface ignored comment;\nCreate OpenFramesInterface Display;\n"
            "Display.Add = {Sat, Earth};\nDisplay.View = {Camera};\nDisplay.DrawLabel = [true false];\n"
            "Display.ShowVR = false;\nCreate OpenFramesView Camera;\nCamera.ViewFrame = CoordinateSystem;\n"
            "Camera.SetDefaultLocation = On;\nCamera.DefaultEye = [100 200 300];\nCamera.DefaultUp = [1 0 1];\n"+dynamics;
         const auto converted=convertOpenFramesViews(input);
         require(converted.error.isEmpty() && converted.plots==1 && converted.script.endsWith(dynamics),"View conversion changed mission calculations");
         require(converted.script.contains("Create OrbitView Display;") && converted.script.contains("Display.ViewPointVector = [100 200 300]") &&
            converted.script.contains("retained as a comment") && converted.script.contains("non-axis up vector"),"View conversion lost settings or omitted limitations");
         require(convertOpenFramesViews(dynamics).script==dynamics,"Converter changed a script without OFI");
         require(!convertOpenFramesViews(input+"GMAT total = Camera.FOVy;\n").error.isEmpty(),"Converter removed a view used by calculations");
         require(!convertOpenFramesViews("Create OpenFramesVector Vec;\n").error.isEmpty(),"Unsupported OFI declaration was silently removed");
         require(!convertOpenFramesViews(input+"Display.Axes = Off;\n").error.isEmpty(),"Dynamic viewer setting was silently rewritten");
      }
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
      const auto forceScript=editor->toPlainText();
      auto forceValue=[](const QString &name) {
         for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtForces")))
            if (field.name==name) return field.value;
         return QString();
      };
      require(!forceValue("GravityField.Earth.Degree").isEmpty(),"Owned gravity settings missing from force editor");
      require(!window.applyResourceChanges("QtForces",{{"GravityField.Earth.Degree","-1"}},forceScript).isEmpty(),
         "Invalid gravity degree accepted");
      require(editor->toPlainText()==forceScript,"Rejected owned-force edit changed script");
      const auto gravityError=window.applyResourceChanges("QtForces",{{"GravityField.Earth.Degree","6"},{"GravityField.Earth.Order","6"}},forceScript);
      if (!gravityError.isEmpty()) std::cerr<<gravityError.toStdString()<<'\n';
      require(gravityError.isEmpty() && forceValue("GravityField.Earth.Degree")=="6" && forceValue("GravityField.Earth.Order")=="6",
         "Owned force edits did not survive model reconstruction");
      require(window.runMission()==MainWindow::RunResult::Completed,"Edited gravity model failed actual propagation");
      editor->undo(); require(editor->toPlainText()==forceScript && window.buildScript(),"Owned force edit was not undoable");
      require(window.applyResourceChanges("QtSat",{{"Attitude","Spinner"},{"AttitudeDisplayStateType","EulerAngles"}},forceScript).isEmpty(),
         "Attitude representation change failed");
      const auto attitudeScript=editor->toPlainText();
      bool attitudeField=false;
      for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtSat")))
         if (field.name=="EulerAngle1") attitudeField=true;
      require(attitudeField,"Attitude state fields missing from spacecraft editor");
      const auto attitudeError=window.applyResourceChanges("QtSat",{{"EulerAngle1","12.5"}},attitudeScript);
      if (!attitudeError.isEmpty()) std::cerr<<attitudeError.toStdString()<<'\n';
      require(attitudeError.isEmpty(),"Attitude state edit failed");
      require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("EulerAngle1")-12.5)<1e-9,
         "Attitude edit did not survive reconstruction");
      require(window.runMission()==MainWindow::RunResult::Completed,"Edited attitude failed propagation");
      editor->undo(); require(editor->toPlainText()==attitudeScript && window.buildScript(),"Attitude edit was not undoable");
      editor->undo(); require(editor->toPlainText()==forceScript && window.buildScript(),"Attitude representation was not undoable");
      editor->setPlainText("Create Array QtMatrix[2,3];\nGMAT QtMatrix(1,1) = 4;\nGMAT QtMatrix(2,3) = 9;\nBeginMissionSequence;\n");
      require(window.buildScript(), "Array fixture did not build");
      const auto arrayScript=editor->toPlainText();
      const auto fields=resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtMatrix"));
      bool foundMatrix=false;
      for (const auto &field:fields) if (field.name=="RmatValue")
         foundMatrix=field.rows==2 && field.columns==3 && field.value=="4 0 0; 0 0 9";
      require(foundMatrix,"Array grid missing or transposed");
      {
         QWidget owner;
         ResourceEditor gridPanel(*Moderator::Instance()->GetConfiguredObject("QtMatrix"),
            [](const QMap<QString,QString>&) { return QString(); },&owner);
         auto *editCells=gridPanel.findChild<QPushButton *>("editCells_RmatValue");
         require(editCells,"Array has no cell editor button");
         bool gridOpened=false,invalidRejected=false;
         QTimer::singleShot(0,&gridPanel,[&] {
            auto *dialog=gridPanel.findChild<QDialog *>("numericGridDialog");
            if (!dialog) return;
            auto *grid=dialog->findChild<QTableWidget *>("numericGrid");
            gridOpened=grid && grid->rowCount()==2 && grid->columnCount()==3 && grid->item(1,2)->text()=="9";
            if (!gridOpened) { dialog->reject(); return; }
            auto *buttons=dialog->findChild<QDialogButtonBox *>();
            grid->item(1,2)->setText("nan"); buttons->button(QDialogButtonBox::Ok)->click();
            invalidRejected=dialog->isVisible();
            grid->item(1,2)->setText("-12.5"); buttons->button(QDialogButtonBox::Ok)->click();
         });
         editCells->click();
         require(gridOpened && invalidRejected && gridPanel.hasChanges(),"Array grid validation/edit failed");
         require(Moderator::Instance()->GetConfiguredObject("QtMatrix")->GetRealParameter("SingleValue",1,2)==9,
            "Grid mutated configured array before Apply");
         const auto value=gridPanel.findChild<QTableWidget *>()->findItems("4 0 0; 0 0 -12.5",Qt::MatchExactly);
         require(value.size()==1,"Grid did not preserve cell order");
         QTimer::singleShot(0,&gridPanel,[&] {
            auto *dialog=gridPanel.findChild<QDialog *>("numericGridDialog");
            dialog->findChild<QTableWidget *>("numericGrid")->item(0,0)->setText("777");
            dialog->reject();
         });
         editCells->click();
         require(value.first()->text()=="4 0 0; 0 0 -12.5","Cancel changed matrix values");
         gridPanel.discardChanges();
      }
      require(!window.applyResourceChanges("QtMatrix",{{"RmatValue","1 2; 3 4"}},arrayScript).isEmpty(),
         "Array dimension change was accepted");
      require(!window.applyResourceChanges("QtMatrix",{{"RmatValue","1 2 3; 4 nan 6"}},arrayScript).isEmpty(),
         "Nonfinite array value was accepted");
      require(editor->toPlainText()==arrayScript,"Rejected array edit changed script");
      const auto arrayError=window.applyResourceChanges("QtMatrix",{{"RmatValue","0 -2 3.125; 4 5 6"}},arrayScript);
      if (!arrayError.isEmpty()) std::cerr<<arrayError.toStdString()<<'\n';
      require(arrayError.isEmpty(),"Array edit failed");
      auto *array=Moderator::Instance()->GetConfiguredObject("QtMatrix");
      require(array->GetRealParameter("SingleValue",0,0)==0 && array->GetRealParameter("SingleValue",0,2)==3.125 &&
         array->GetRealParameter("SingleValue",1,2)==6,"Array values did not survive serialization");
      editor->undo();
      require(editor->toPlainText()==arrayScript && window.buildScript(),"Array edit was not one undoable change");
      require(Moderator::Instance()->GetConfiguredObject("QtMatrix")->GetRealParameter("SingleValue",1,2)==9,
         "Array undo failed to restore initial values");
      require(!window.createResource("Array","InvalidDimensions",editor->toPlainText(),0,3).isEmpty(),
         "Zero-sized array accepted");
      require(window.createResource("Array","CreatedArray",editor->toPlainText(),3,2).isEmpty(),"Array creation failed");
      auto *createdArray=Moderator::Instance()->GetConfiguredObject("CreatedArray");
      require(createdArray && createdArray->GetIntegerParameter("NumRows")==3 && createdArray->GetIntegerParameter("NumCols")==2,
         "Created array dimensions wrong");
      editor->setPlainText("Create ChemicalTank FuelA FuelB;\nCreate ChemicalThruster Engine;\n"
         "GMAT Engine.Tank = {FuelA, FuelB};\nGMAT Engine.MixRatio = [2 3];\n"
         "Create Spacecraft Vehicle;\nGMAT Vehicle.Tanks = {FuelA, FuelB};\nGMAT Vehicle.Thrusters = {Engine};\nBeginMissionSequence;\n");
      require(window.buildScript(),"Hardware fixture failed");
      const auto hardwareScript=editor->toPlainText();
      require(!window.applyResourceChanges("Vehicle",{{"Tanks","MissingTank"}},hardwareScript).isEmpty(),
         "Missing hardware reference accepted");
      require(editor->toPlainText()==hardwareScript,"Rejected hardware link changed script");
      const auto tankError=window.applyResourceChanges("Engine",{{"Tank","FuelB, FuelA"}},hardwareScript);
      if (!tankError.isEmpty()) std::cerr<<tankError.toStdString()<<'\n';
      require(tankError.isEmpty(),"Tank reorder failed");
      auto *engine=Moderator::Instance()->GetConfiguredObject("Engine");
      require(engine->GetStringArrayParameter("Tank").front()=="FuelB" && engine->GetRvectorParameter("MixRatio")[0]==3 &&
         engine->GetRvectorParameter("MixRatio")[1]==2,"Tank reorder detached mixture ratios from tanks");
      require(window.applyResourceChanges("Engine",{{"MixRatio","4 5"}},editor->toPlainText()).isEmpty(),
         "Mixture vector edit failed");
      require(Moderator::Instance()->GetConfiguredObject("Engine")->GetRvectorParameter("MixRatio")[1]==5,
         "Mixture vector did not survive script rebuild");
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
      auto *sections=panel->findChild<QTabBar *>("propertySections");
      require(sections && sections->tabText(sections->currentIndex())=="Orbit","Spacecraft did not open on Orbit");
      int massSection=-1;
      for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="Ballistic/Mass") massSection=i;
      require(massSection>=0,"Spacecraft mass section missing");
      for (int row=0;row<table->rowCount();++row) {
         if (table->item(row,0)->text()=="DryMass") require(table->isRowHidden(row),"Orbit includes mass properties");
         if (table->item(row,0)->text()=="X") require(!table->isRowHidden(row),"Orbit is missing its state elements");
      }
      sections->setCurrentIndex(massSection);
      bool massFound = false;
      for (int row = 0; row < table->rowCount(); ++row) {
         if (table->item(row, 0)->text() == "DryMass") {
            require(!table->isRowHidden(row),"Mass section did not reveal DryMass");
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
      sections->setCurrentIndex(0);
      require(panel->hasChanges(),"Switching sections discarded pending edits");
      sections->setCurrentIndex(sections->count()-1);
      auto *propertyFilter=panel->findChild<QLineEdit *>("propertyFilter");
      propertyFilter->setText("drymass");
      for (int row=0;row<table->rowCount();++row)
         require(table->isRowHidden(row)==(table->item(row,0)->text()!="DryMass"),"Property filter did not combine with sections");
      propertyFilter->clear(); sections->setCurrentIndex(massSection);
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
      const auto beforeDeletion=editor->toPlainText();
      require(!window.deleteResource("QtSat",beforeDeletion).isEmpty(),"Deletion removed a spacecraft used by a command");
      require(!window.deleteResource("QtForces",beforeDeletion).isEmpty(),"Deletion removed a force model used by a propagator");
      require(!window.deleteResource("EarthMJ2000Eq",beforeDeletion).isEmpty(),"Deletion removed a built-in coordinate system");
      require(editor->toPlainText()==beforeDeletion,"Rejected deletion changed the script");
      require(window.createResource("Spacecraft","UnusedSat",beforeDeletion).isEmpty(),"Deletion fixture creation failed");
      const auto beforeUnusedDelete=editor->toPlainText();
      const auto unusedItems=tree->findItems("UnusedSat",Qt::MatchExactly|Qt::MatchRecursive);
      require(unusedItems.size()==1,"Unused resource missing from tree");
      tree->itemDoubleClicked(unusedItems.first(),0);
      ResourceEditor *unusedPanel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *resource=dynamic_cast<ResourceEditor *>(widget))
         if (resource->parentWidget()->property("resourceName").toString()=="UnusedSat") unusedPanel=resource;
      require(unusedPanel!=nullptr,"Unused resource panel missing");
      auto *unusedTable=unusedPanel->findChild<QTableWidget *>(); QTableWidgetItem *unusedMass=nullptr;
      for (int row=0;row<unusedTable->rowCount();++row) if (unusedTable->item(row,0)->text()=="DryMass") unusedMass=unusedTable->item(row,1);
      require(unusedMass!=nullptr,"Unused spacecraft mass missing");
      const auto oldMass=unusedMass->text(); unusedMass->setText("999");
      require(!window.deleteResource("UnusedSat",beforeUnusedDelete).isEmpty(),"Deletion discarded its panel changes");
      unusedMass->setText(oldMass);
      require(window.deleteResource("UnusedSat",beforeUnusedDelete).isEmpty(),"Unrelated open resource panel blocked deletion");
      require(!Moderator::Instance()->GetConfiguredObject("UnusedSat") && tree->findItems("UnusedSat",Qt::MatchExactly|Qt::MatchRecursive).isEmpty(),"Deleted resource remained in model or tree");
      require(!window.deleteResource("CreatedSat",beforeUnusedDelete).isEmpty(),"Stale deletion snapshot accepted");
      editor->undo(); require(editor->toPlainText()==beforeUnusedDelete && window.buildScript(),"Deletion undo failed");
      require(Moderator::Instance()->GetConfiguredObject("UnusedSat")!=nullptr,"Deletion undo did not restore resource");
      require(window.deleteResource("UnusedSat",editor->toPlainText()).isEmpty(),"Repeated deletion failed");
      require(window.createResource("Variable","ScratchOne",editor->toPlainText()).isEmpty() &&
         window.createResource("Variable","ScratchTwo",editor->toPlainText()).isEmpty(),"Variable deletion fixtures failed");
      require(window.deleteResource("ScratchOne",editor->toPlainText()).isEmpty() &&
         !Moderator::Instance()->GetConfiguredObject("ScratchOne") && Moderator::Instance()->GetConfiguredObject("ScratchTwo"),
         "Variable deletion affected a different declaration");
      require(window.runMission()==MainWindow::RunResult::Completed,"Mission failed after unused-resource deletion");
      window.findChild<QTabWidget *>()->setCurrentIndex(0); tree->expandAll();
      const auto scratchItems=tree->findItems("ScratchTwo",Qt::MatchExactly|Qt::MatchRecursive);
      require(scratchItems.size()==1,"Deletion menu fixture missing"); tree->scrollToItem(scratchItems.first());
      bool confirmedDeletion=false;
      QTimer::singleShot(0,&window,[&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
         if (!menu) return;
         for (auto *action:menu->actions()) if (action->text()=="Delete resource…") {
            menu->setActiveAction(action);
            QTimer::singleShot(0,&window,[&] {
               if (auto *confirmation=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                  confirmedDeletion=true; confirmation->button(QMessageBox::Yes)->click();
               }
            });
            QKeyEvent activate(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
            QApplication::sendEvent(menu,&activate); return;
         }
         menu->close();
      });
      tree->customContextMenuRequested(tree->visualItemRect(scratchItems.first()).center());
      require(confirmedDeletion && !Moderator::Instance()->GetConfiguredObject("ScratchTwo"),"Context-menu deletion failed");
      const auto sample=QFileInfo(startup).dir().filePath("../samples/Ex_HohmannTransfer.script");
      require(window.loadScript(sample),"Could not load shipped OFI sample");
      const auto originalSample=editor->toPlainText();
      const auto dynamics=originalSample.mid(originalSample.indexOf("BeginMissionSequence;"));
      require(window.convertOpenFramesScript(),"Shipped OFI sample conversion failed");
      require(editor->toPlainText().endsWith(dynamics) && editor->document()->isModified(),"Conversion changed mission or failed to mark unsaved changes");
      require(window.runMission()==MainWindow::RunResult::Completed,"Converted Hohmann mission did not run");
      editor->undo(); require(editor->toPlainText()==originalSample,"Conversion was not one undoable edit");
      const auto capabilities=window.availableEngineTypes();
      require(capabilities.contains("OrbitView") && capabilities.contains("GmatFunction") && !capabilities.contains("OpenFramesInterface"),
         "Runtime engine capability inventory incorrect");
      std::cout<<"PASS: resource deletion, command/resource dependencies, target-panel protection, unrelated panels, undo and variable declarations\n";
      std::cout<<"PASS: propagation form, advanced-command preservation, invalid-input rollback and actual elapsed-days execution\n";
      std::cout<<"PASS: resource creation, name/type validation, pending/stale protection, undo, dialog/tree integration and actual propagation\n";
      std::cout << "PASS: propagation, repeated runs, pause/resume/stop, edit protection, close protection, invalid-script recovery, resource apply/rollback/stale-panel protection/undo\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr << "FAIL: " << error.what() << '\n';
      return 1;
   }
}
