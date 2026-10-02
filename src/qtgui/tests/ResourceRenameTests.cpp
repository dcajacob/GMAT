#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceRename.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Rmatrix.hpp"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static QTreeWidgetItem *item(QTreeWidget *tree,const QString &name)
{
   for (auto *found:tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive)) if (!found->data(0,Qt::UserRole).toString().isEmpty()) return found;
   throw std::runtime_error(("Missing resource "+name).toStdString());
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static QString withoutCamera(QString source)
{
   source.remove(QRegularExpression("^[ \\t]*% GMAT-Qt-Camera [^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption)); return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtResourceRename"); TestSettings settings;
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Rename test needs an offscreen startup argument");
      QTemporaryDir files; require(files.isValid(),"Rename temporary directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Rename initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources");
      const auto build=[&] { if (!window.buildScript()) throw std::runtime_error(("Rename fixture build failed\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString()); };
      const auto report=files.filePath("Craft data α.txt"),observations=files.filePath("observations.gmd");
      QFile observationsFile(observations); require(observationsFile.open(QIODevice::WriteOnly) && observationsFile.write("% GMAT Internal Measurement Data File\n")>0,"Temporary tracking input unavailable"); observationsFile.close();
      QString plain=
         "% Craft Frame Index remain literal comments α\n"
         "Create Spacecraft Craft Friend; % grouped declarations\nCraft.Cr = 1.9000; % Craft retains exact numeric spelling\n"
         "Create GroundStation Site;\nSite.Id = 'Craft Frame Index';\nCreate ErrorModel RangeError;\nRangeError.Type = 'Range';\nSite.ErrorModels = {RangeError};\nCreate TrackingFileSet Tracking;\nTracking.FileName = {'"+observations+"'};\nTracking.AddTrackingConfig = {{Site, Craft}, 'Range'};\n"
         "Create CoordinateSystem Frame;\nFrame.Origin = Earth;\nFrame.Axes = MJ2000Eq;\nFriend.CoordinateSystem = Frame;\n"
         "Create Formation Group;\nGroup.Add = {Craft, Friend};\n"
         "Create ForceModel P_ForceModel;\nP_ForceModel.PrimaryBodies = {};\nP_ForceModel.PointMasses = {Earth};\n"
         "Create Propagator P;\nP.FM = P_ForceModel;\n"
         "Create Variable Index Result X e On Tolerance;\ne = 1.e-9; % exponent spelling\nOn = 2;\nIndex = 1;\nResult = 0;\nX = 7.00; % resource named X must not replace a terminal property\n"
         "Create String Caption ReportName;\nReportName = '"+report+"';\nCaption = 'Craft Frame and Index';\n"
         "Create Array Cells[2,2];\nCells(1,1) = 2.5000; % preserve independent initialization\n"
         "Create OrbitView Orb;\nOrb.Add = {Craft, Earth};\nOrb.ViewDirection = Craft;\nOrb.ShowPlot = false;\n"
         "Create ReportFile Values Idle;\nIdle.Filename = '"+files.filePath("idle.txt")+"';\nIdle.Add = {X};\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Add = {Craft.Frame.X, X};\n"
         "BeginMissionSequence;\n"
         "Values.Filename = ReportName;\nToggle Idle On;\nIf Index == 1;\nBeginScript;\nCells(Index,1) = Cells(Index,1) + 1; % Index retained in comment\nEndScript;\nEndIf;\n"
         "Propagate 'Craft Frame label' P(Craft) {Craft.ElapsedSecs = 1};\n"
         "Result = Craft.Frame.X;\nReport 'Craft Frame report' Values X Cells(1,1);\n";
      QtCameraSetting setting; setting.automaticTrajectory="Craft"; setting.objectLabels["Craft"]=false;
      QtVectorSetting vector; vector.name="Craft"; vector.label="Craft label"; vector.type="Relative Position"; vector.source="Craft"; vector.destination="Friend"; setting.vectors.append(vector);
      QtCameraPreset view; view.name="Craft"; view.reference="Earth"; view.target="Craft"; view.lookAtRotation=true; setting.views.append(view);
      auto directive=qtCameraDirective("Orb",setting); auto data=QJsonDocument::fromJson(directive.mid(QString("% GMAT-Qt-Camera ").size()).toUtf8()).object(); data["futureLiteral"]="Craft";
      directive="% GMAT-Qt-Camera "+QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact))+"\n";
      plain=setArrayExpressions(plain,"Cells",R"([{"row":1,"column":2,"expression":"Index*2"}])",2,2);
      const auto baseline=directive+plain; editor->setPlainText(baseline); build();
      tree->itemDoubleClicked(item(tree,"Index"),0); ResourceEditor *pending=nullptr;
      for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="Index") pending=dynamic_cast<ResourceEditor *>(child->widget());
      require(pending,"Pending Rename guard fixture missing"); auto *value=pending->findChild<QLineEdit *>("parameterValue"); require(value,"Variable value field missing"); value->setText("2");
      require(pending->hasChanges() && window.renameResource("Craft","Blocked",baseline).contains("Apply or discard") && editor->toPlainText()==baseline && object("Craft"),"Rename ignored pending panel edits"); value->setText("1"); closePanels(window);
      auto *action=window.findChild<QAction *>("renameResource"); require(action && action->shortcut()==QKeySequence("F2"),"Rename action/F2 missing");
      const auto dialog=[&](const std::function<void(QDialog *)> &check) {
         tree->setCurrentItem(item(tree,"Craft")); std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] { auto *rename=window.findChild<QDialog *>("renameResourceDialog"); try { require(rename,"Rename dialog missing"); check(rename); } catch (...) { failure=std::current_exception(); } if (rename && rename->result()!=QDialog::Accepted) rename->reject(); });
         action->trigger(); if (failure) std::rethrow_exception(failure);
      };
      dialog([&](QDialog *rename) { auto *name=rename->findChild<QLineEdit *>("resourceName"); require(name && name->text()=="Craft","Rename name not prefilled"); name->setText("Cancelled"); rename->reject(); });
      require(editor->toPlainText()==baseline && object("Craft") && !object("Cancelled"),"Rename Cancel changed source/model");
      // The selected QAction is copied into a value; the QMenu is destroyed
      // before the modal name editor starts. This is an offscreen ownership
      // check, not a Wayland/native compositor qualification.
      QPointer<QMenu> popup; std::exception_ptr popupFailure;
      tree->scrollToItem(item(tree,"Craft")); QApplication::processEvents();
      QTimer::singleShot(0,&window,[&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); popup=menu;
         try {
            require(menu,"Rename context menu missing"); QAction *rename=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()=="renameResource") rename=candidate;
            require(rename && rename->isEnabled(),"Context Rename unavailable"); menu->setActiveAction(rename);
            QTimer::singleShot(0,&window,[&] { auto *form=window.findChild<QDialog *>("renameResourceDialog"); try { require(popup.isNull() && !QApplication::activePopupWidget() && form,"Popup survived into Rename dialog"); } catch (...) { popupFailure=std::current_exception(); } if (form) form->reject(); });
            QKeyEvent press(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&press);
         } catch (...) { popupFailure=std::current_exception(); if (menu) menu->close(); }
      });
      tree->customContextMenuRequested(tree->visualItemRect(item(tree,"Craft")).center()); if (popupFailure) std::rethrow_exception(popupFailure);
      dialog([&](QDialog *rename) {
         auto *name=rename->findChild<QLineEdit *>("resourceName"); auto *accept=rename->findChild<QDialogButtonBox *>("resourceRenameButtons")->button(QDialogButtonBox::Ok);
         name->setText("Friend"); accept->click(); require(rename->isVisible() && rename->findChild<QLabel *>("resourceRenameStatus")->text().contains("already") && editor->toPlainText()==baseline,"Duplicate Rename did not stay pending");
         name->setText("Vehicle"); accept->click(); require(rename->result()==QDialog::Accepted,qPrintable("Corrected Rename did not commit: "+rename->findChild<QLabel *>("resourceRenameStatus")->text()+"\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
      });
      auto expected=plain; expected.replace("Create Spacecraft Craft Friend;","Create Spacecraft Vehicle Friend;").replace("Craft.Cr","Vehicle.Cr").replace("{{Site, Craft}, 'Range'}","{{Site, Vehicle}, 'Range'}").replace("Group.Add = {Craft, Friend};","Group.Add = {Vehicle, Friend};").replace("Orb.Add = {Craft, Earth};","Orb.Add = {Vehicle, Earth};").replace("Orb.ViewDirection = Craft;","Orb.ViewDirection = Vehicle;").replace("{Craft.Frame.X, X}","{Vehicle.Frame.X, X}").replace("P(Craft) {Craft.ElapsedSecs","P(Vehicle) {Vehicle.ElapsedSecs").replace("Result = Craft.Frame.X;","Result = Vehicle.Frame.X;");
      const auto renamed=editor->toPlainText(); require(withoutCamera(renamed)==expected,"Rename rewrote unrelated source/literals/comments/property names");
      require(!object("Craft") && object("Vehicle") && object("Vehicle")->GetRealParameter("Cr")==1.9 && object("Site")->GetStringParameter("Id")=="Craft Frame Index","Rename changed configured values");
      const auto camera=qtCameraSettings(renamed).value("Orb"); require(camera.automaticTrajectory=="Vehicle" && camera.objectLabels.contains("Vehicle") && !camera.objectLabels.contains("Craft") && camera.vectors[0].source=="Vehicle" && camera.vectors[0].name=="Craft" && camera.vectors[0].label=="Craft label" && camera.views[0].target=="Vehicle" && camera.views[0].name=="Craft" && renamed.contains("\"futureLiteral\":\"Craft\""),"Rename corrupted camera references/labels/unknown metadata");
      editor->undo(); require(editor->toPlainText()==baseline,"Rename Undo not exact/atomic"); build(); editor->redo(); require(editor->toPlainText()==renamed,"Rename Redo not exact"); build();
      const auto rename=[&](const QString &old,const QString &name) { const auto before=editor->toPlainText(); const auto error=window.renameResource(old,name,before); require(error.isEmpty(),qPrintable(error)); return before; };
      rename("Frame","LocalFrame"); require(editor->toPlainText().contains("Vehicle.LocalFrame.X") && editor->toPlainText().contains("Friend.CoordinateSystem = LocalFrame;") && editor->toPlainText().contains("'Craft Frame and Index'"),"Dependent frame parameter/reference rename failed");
      rename("Index","Row"); require(editor->toPlainText().contains("Cells(Row,1) = Cells(Row,1) + 1") && arrayExpressions(editor->toPlainText(),"Cells").contains("Row*2") && editor->toPlainText().contains("If Row == 1") && editor->toPlainText().contains("% Index retained in comment"),"Array indices/formulas/nested commands were stranded");
      rename("Cells","Grid"); require(arrayExpressions(editor->toPlainText(),"Grid").contains("Row*2") && !editor->toPlainText().contains("% GMAT-Qt-Array-Expressions Cells ") && editor->toPlainText().contains("Report 'Craft Frame report' Values X Grid(1,1)"),"Managed Array owner/source/reference rename failed");
      rename("P","Integrator"); require(object("Integrator") && !object("P") && object("Integrator_ForceModel") && !object("P_ForceModel") && object("Integrator")->GetStringParameter("FM")=="Integrator_ForceModel" && editor->toPlainText().contains("Integrator(Vehicle)"),"PropSetup/owned ForceModel rename differs from wx");
      rename("ReportName","OutputName"); require(editor->toPlainText().contains("Values.Filename = OutputName;") && editor->toPlainText().contains(report),"Typed filename String expression was stranded or its literal changed");
      rename("Orb","Display"); require(qtCameraSettings(editor->toPlainText()).contains("Display") && !qtCameraSettings(editor->toPlainText()).contains("Orb"),"Renamed plot resurrected old camera metadata");
      rename("On","Mode"); require(editor->toPlainText().contains("Toggle Idle On;") && editor->toPlainText().contains("Mode = 2;"),"Resource Rename rewrote Toggle mode literal");
      rename("e","Exponent"); require(editor->toPlainText().contains("Exponent = 1.e-9; % exponent spelling"),"Resource Rename rewrote scientific numeric literal");
      rename("X","Scalar"); require(editor->toPlainText().contains("Vehicle.LocalFrame.X") && editor->toPlainText().contains("Report 'Craft Frame report' Values Scalar Grid(1,1)"),"Resource X renamed another object's property");
      const auto options=renameResourceSource(editor->toPlainText()+"Vary Solver(Tolerance = 1) {Perturbation = Tolerance, Lower = 0, Upper = 2, MaxStep = 1};\nAchieve Solver(Result = 0) {Tolerance = Tolerance};\n",*object("Tolerance"),"Limit");
      require(options.contains("Vary Solver(Limit = 1) {Perturbation = Limit, Lower = 0, Upper = 2, MaxStep = 1}") && options.contains("{Tolerance = Limit}"),"Rename rewrote solver option keys or stranded their expressions");
      const auto final=editor->toPlainText();
      for (const auto &name:QStringList{"Earth","SolarSystem","EarthMJ2000Eq"}) require(!window.renameResource(name,"Protected",final).isEmpty() && editor->toPlainText()==final,"Rename allowed protected resource");
      require(!window.renameResource("Vehicle","Bad Name",final).isEmpty() && !window.renameResource("Vehicle","Propagate",final).isEmpty() && editor->toPlainText()==final,"Rename accepted invalid/reserved name");
      // Rejected name validation leaves the source and prior model intact.
      const auto rejected=window.renameResource("Scalar","Vehicle",final); require(!rejected.isEmpty() && editor->toPlainText()==final && object("Scalar") && object("Vehicle"),"Rejected Rename changed source/model");
      editor->insertPlainText("% pending edit\n"); require(!window.renameResource("Vehicle","Stale",final).isEmpty(),"Rename accepted stale editor source"); editor->undo(); require(editor->toPlainText()==final,"Stale test Undo not exact");
      const auto saved=files.filePath("renamed mission α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==final,"Unicode Rename save/reopen lost exact source"); build();
      require(window.runMission()==MainWindow::RunResult::Completed,qPrintable("Renamed mission did not complete:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
      QFile output(report); require(output.open(QIODevice::ReadOnly),"Renamed literal report filename not retained"); const auto values=QString::fromUtf8(output.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==2 && values[0].toDouble()==7 && values[1].toDouble()==3.5,"Rename altered independent original calculations");
      // Refuse external source boundaries before any source/model mutation.
      closePanels(window); const auto included=final+"#Include 'missing-reference.inc'\n";
      bool refused=false; try { renameResourceSource(included,*object("Vehicle"),"External"); } catch (const std::exception &error) { refused=QString::fromUtf8(error.what()).contains("external"); }
      require(refused && editor->toPlainText()==final && object("Vehicle"),"External reference rename was silently accepted");
      std::cout<<"PASS actual Rename/F2/context popup destruction, Cancel/correction/protection/source literals, typed resource and dependent frame references, nested commands/array indices/formulas/markers, PropSetup owned FM, Qt metadata, exact Undo/Redo/Unicode reopen and independent calculation; external documents explicitly deferred.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
