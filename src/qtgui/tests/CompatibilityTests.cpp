#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "CommandForm.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "ResourceProperties.hpp"
#include "ResourceEditor.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMdiSubWindow>
#include <QStatusBar>
#include "ReportParameterDialog.hpp"
#include <QListWidget>
#include <QSpinBox>
#include <QTimer>
#include <QInputDialog>
#include "FileManager.hpp"
#include <QRegularExpression>
#include <array>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read fixture/report"); return QString::fromUtf8(file.readAll()); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtCompatibility");
   if (argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(); const QDir samples(QFileInfo(argv[2]).absoluteFilePath());
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir output; require(output.isValid(),"Temporary output directory failed");
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto roundTrip=[&](const QString &label) {
         const auto source=editor->toPlainText();
         const auto first=output.filePath(label+" original.script");
         const auto second=output.filePath(label+QString::fromUtf8(" saved Δ.script"));
         require(window.saveScriptTo(first),"Plugin script save failed");
         require(window.saveScriptTo(second),"Plugin script Save As failed");
         require(read(first)==source && read(second)==source,"Plugin save changed script calculations or properties");
         editor->setPlainText("Create MissingPluginType Broken;\nBeginMissionSequence;\n");
         require(!window.buildScript(),"Invalid plugin type unexpectedly built");
         require(window.loadScript(second) && editor->toPlainText()==source && window.buildScript(),
            "Plugin script did not recover unchanged after failed interpretation and reopen");
      };
      {
         const auto reportPath=output.filePath("configured-report.txt");
         editor->setPlainText("Create Variable Number;\nCreate Array Values[2,2];\n"
            "Create ReportFile ConfiguredReport;\nConfiguredReport.Filename = '"+reportPath+"';\n"
            "BeginMissionSequence;\nGMAT Number = 12.3456789;\nGMAT Values(1,2) = -7.25;\n"
            "Report ConfiguredReport Number Values(1,2);\n");
         require(window.buildScript(),"Report fixture failed");
         require(window.applyResourceChanges("ConfiguredReport",{{"Add","Values(1,2), Number"},
            {"FixedWidth","false"},{"WriteHeaders","false"},{"Precision","6"},{"Delimiter",","}},editor->toPlainText()).isEmpty(),
            "Report configuration or array-element list rejected");
         {
            QString replacement;
            CommandForm form([&](const QString &value) { replacement=value; });
            form.setStatement("Report 'Saved label' ConfiguredReport Number Values(1,2); % keep comment");
            bool choseReport=false;
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QInputDialog *>();
               require(dialog && dialog->comboBoxItems().contains("ConfiguredReport"),"Report file picker missing configured report");
               dialog->setTextValue("ConfiguredReport"); choseReport=true; dialog->accept();
            });
            form.findChild<QPushButton *>("commandChoose_Report file")->click();
            require(choseReport,"Report picker was not used");
            bool selected=false;
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QDialog *>("reportParameterDialog");
               require(dialog,"Report parameter selector did not open");
               auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");
               entry->setCurrentText("Values");
               auto *row=dialog->findChild<QSpinBox *>("reportArrayRow");
               auto *column=dialog->findChild<QSpinBox *>("reportArrayColumn");
               require(row->isEnabled() && row->maximum()==2 && column->maximum()==2,"Array bounds absent from picker");
               row->setValue(1); column->setValue(2);
               auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters");
               list->setCurrentRow(1); dialog->findChild<QPushButton *>("reportParameterRemove")->click();
               dialog->findChild<QPushButton *>("reportAddElement")->click();
               dialog->findChild<QPushButton *>("reportParameterUp")->click();
               selected=true; dialog->accept();
            });
            form.findChild<QPushButton *>("commandChoose_Parameters")->click();
            require(selected && replacement=="Report 'Saved label' ConfiguredReport Values(1,2) Number; % keep comment",
               "Parameter selection lost order, label or comment");
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QDialog *>("reportParameterDialog");
               dialog->findChild<QListWidget *>("reportSelectedParameters")->clear(); dialog->reject();
            });
            form.findChild<QPushButton *>("commandChoose_Parameters")->click();
            require(replacement.contains("Values(1,2) Number"),"Cancelling selection changed command");
            const auto snapshot=window.missionSnapshot(); int reportIndex=-1;
            for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.startsWith("Report ")) reportIndex=i;
            require(reportIndex>=0 && window.applyMissionChange(snapshot,reportIndex,MissionEdit::Replace,replacement).isEmpty(),
               "Selected report parameters did not apply to mission");
         }
         const auto valid=editor->toPlainText();
         for (const auto &changes:QVector<QMap<QString,QString>>{
            {{"Delimiter","::"}},{{"Precision","0"}},{{"ColumnWidth","0"}},{{"Add","Values(1,2); Stop"}}})
            require(!window.applyResourceChanges("ConfiguredReport",changes,valid).isEmpty() && editor->toPlainText()==valid,
               "Invalid report setting changed the script");
         auto *report=Moderator::Instance()->GetConfiguredObject("ConfiguredReport");
         require(report->GetStringArrayParameter("Add")==StringArray({"Values(1,2)","Number"}),"Report parameter order changed");
         {
            QWidget host;
            QMap<QString,QString> changes;
            ResourceEditor panel(*report,[&](const auto &value) { changes=value; return QString(); },&host);
            auto *delimiter=panel.findChild<QComboBox *>("reportDelimiter");
            require(delimiter && delimiter->currentText()=="Comma" && !panel.hasChanges(),"Report delimiter label or initial dirty state wrong");
            delimiter->setCurrentText("Tab");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            require(changes.value("Delimiter")=="\t","Tab label was not translated to a literal tab");
            delimiter->setEditText(":");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            require(changes.value("Delimiter")==":" && delimiter->insertPolicy()==QComboBox::NoInsert,
               "Custom delimiter was lost or can be inserted without its value");
            bool reordered=false;
            QTimer::singleShot(0,[&] {
               auto *dialog=panel.findChild<QDialog *>("reportParameterDialog");
               auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters");
               require(list->count()==2 && list->item(0)->text()=="Values(1,2)","Resource picker split an array element");
               list->setCurrentRow(0); dialog->findChild<QPushButton *>("reportParameterDown")->click();
               reordered=true; dialog->accept();
            });
            panel.findChild<QPushButton *>("chooseProperty_Add")->click();
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            require(reordered && changes.value("Add")=="Number, Values(1,2)","Resource report picker lost reordered parameters");
         }
         require(splitResourceReferences("Values(1,2), Number")==QStringList({"Values(1,2)","Number"}),
            "Report picker split an indexed parameter");
         roundTrip("report-settings");
         require(window.runMission()==MainWindow::RunResult::Completed,"Configured report run failed");
         require(read(reportPath).trimmed()=="-7.25,12.3457","Report precision, delimiter or header settings not reflected in output");
         require(window.applyResourceChanges("ConfiguredReport",{{"Delimiter","\t"}},editor->toPlainText()).isEmpty(),"Tab delimiter edit failed");
         roundTrip("tab-report");
         require(window.runMission()==MainWindow::RunResult::Completed && read(reportPath).trimmed()=="-7.25\t12.3457",
            "Tab report save/reopen or output failed");
         auto automatic=editor->toPlainText();
         automatic.replace("BeginMissionSequence;","Create Spacecraft ReportSat;\nCreate ForceModel ReportForces;\n"
            "Create Propagator ReportProp;\nReportProp.FM = ReportForces;\nBeginMissionSequence;");
         automatic.replace("Report 'Saved label' ConfiguredReport Values(1,2) Number; % keep comment","Propagate ReportProp(ReportSat) {ReportSat.ElapsedSecs = 1};");
         editor->setPlainText(automatic); require(window.buildScript(),"Automatic report fixture failed");
         roundTrip("automatic-report");
         require(window.runMission()==MainWindow::RunResult::Completed,"Automatic report execution failed");
         const auto rows=read(reportPath).trimmed().split('\n');
         require(rows.size()>=2,"Automatic report did not record propagation");
         for (const auto &row:rows) require(row.trimmed()=="-7.25\t12.3457","Report Add order or array value did not reach output");

         // Format settings must survive reconstruction and affect actual output.
         require(window.applyResourceChanges("ConfiguredReport",{{"FixedWidth","true"},{"WriteHeaders","true"},
            {"ColumnWidth","18"},{"Delimiter"," "},{"LeftJustify","On"},{"ZeroFill","On"}},editor->toPlainText()).isEmpty(),
            "Fixed-width report configuration failed");
         roundTrip("fixed-width-report");
         require(window.runMission()==MainWindow::RunResult::Completed,"Fixed-width report failed");
         const auto fixed=read(reportPath);
         const auto fixedRows=fixed.split('\n',Qt::SkipEmptyParts);
         require(fixedRows.size()==rows.size()+1 && fixedRows.first()==QString("Values(1,2)").leftJustified(21)+QString("Number").leftJustified(21),
            "Fixed-width header or configured width differs");
         for (int i=1;i<fixedRows.size();++i)
            require(fixedRows[i]==QString("-7.25000").leftJustified(21)+QString("12.3457").leftJustified(21),
               "Fixed-width left alignment or zero fill differs");
         require(window.runMission()==MainWindow::RunResult::Completed && read(reportPath)==fixed,
            "Default repeat run appended instead of replacing report");
         require(window.applyResourceChanges("ConfiguredReport",{{"LeftJustify","Off"}},editor->toPlainText()).isEmpty(),
            "Right alignment edit failed");
         roundTrip("right-aligned-report");
         require(window.runMission()==MainWindow::RunResult::Completed,"Right-aligned report failed");
         const auto rightRows=read(reportPath).split('\n',Qt::SkipEmptyParts);
         require(rightRows.first()==QString("Values(1,2)").rightJustified(21)+QString("Number").rightJustified(21),
            "Right-aligned header differs");
         for (int i=1;i<rightRows.size();++i)
            require(rightRows[i]==QString("-7.25000").rightJustified(21)+QString("12.3457").rightJustified(21),
               "Right-aligned report data differs");
         const auto existing=read(reportPath);
         require(window.applyResourceChanges("ConfiguredReport",{{"AppendToExistingFile","true"}},editor->toPlainText()).isEmpty(),
            "Append setting edit failed");
         roundTrip("append-report");
         require(read(reportPath)==existing,"Configuring or reopening append report modified existing results");
         require(window.runMission()==MainWindow::RunResult::Completed && read(reportPath)==existing+existing,
            "Append run lost existing results or changed output");
         require(window.runMission()==MainWindow::RunResult::Completed && read(reportPath)==existing+existing+existing,
            "Repeated append run did not preserve all results");
         auto *outputTree=window.findChild<QTreeWidget *>("Output");
         const auto reportItems=outputTree->findItems("ConfiguredReport",Qt::MatchExactly|Qt::MatchRecursive);
         require(reportItems.size()==1,"Configured report missing from Output");
         outputTree->itemDoubleClicked(reportItems.first(),0);
         auto *preview=window.findChild<QPlainTextEdit *>("report:ConfiguredReport");
         require(preview && preview->isReadOnly() && preview->lineWrapMode()==QPlainTextEdit::NoWrap &&
            preview->toPlainText()==existing+existing+existing,"Report preview is editable, wrapped or differs from file");
         auto *previewWindow=qobject_cast<QMdiSubWindow *>(preview->parentWidget());
         require(previewWindow && previewWindow->windowTitle().contains(reportPath),"Report window does not identify its output path");
         preview->selectAll(); require(preview->textCursor().hasSelection(),"Report text cannot be selected for copying");
         previewWindow->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         require(!window.findChild<QPlainTextEdit *>("report:ConfiguredReport"),"Closing report preview left the window alive");
         require(QFile::remove(reportPath),"Cannot remove temporary report for unavailable-file check");
         outputTree->itemDoubleClicked(reportItems.first(),0);
         require(!window.findChild<QPlainTextEdit *>("report:ConfiguredReport") &&
            window.statusBar()->currentMessage().startsWith("Report is not available:"),"Missing report was displayed or not diagnosed");
         require(window.runMission()==MainWindow::RunResult::Completed && read(reportPath)==existing,
            "Append mode could not recreate a missing output file");


      }
      {
         const auto path=output.filePath("browsed-properties.txt");
         editor->setPlainText("Create Spacecraft BrowserSat;\nBrowserSat.DisplayStateType = Cartesian;\n"
            "BrowserSat.X = 7000;\nBrowserSat.Y = 0;\nBrowserSat.Z = 0;\n"
            "Create CoordinateSystem BrowserFixed;\nBrowserFixed.Origin = Earth;\nBrowserFixed.Axes = BodyFixed;\n"
            "Create ReportFile BrowserReport;\nBrowserReport.Filename = '"+path+"';\nBrowserReport.FixedWidth = false;\n"
            "BrowserReport.Delimiter = ',';\nBrowserReport.WriteHeaders = false;\nBeginMissionSequence;\n"
            "Report BrowserReport BrowserSat.X;\n");
         require(window.buildScript(),"Property browser fixture failed");
         const auto before=editor->toPlainText();
         ReportParameterDialog browser({});
         auto *owner=browser.findChild<QComboBox *>("reportPropertyObject");
         auto *property=browser.findChild<QComboBox *>("reportPropertyType");
         auto *dependency=browser.findChild<QComboBox *>("reportPropertyDependency");
         auto *entry=browser.findChild<QComboBox *>("reportParameterEntry");
         require(owner->findText("BrowserSat")>=0,"Property browser omitted spacecraft");
         owner->setCurrentText("BrowserSat");
         auto choose=[&](const QString &type,const QString &reference,const QString &expected) {
            require(property->findText(type)>=0,"Property browser omitted reportable parameter type");
            property->setCurrentText(type);
            if (!reference.isEmpty()) {
               require(dependency->findText(reference)>=0,"Property browser omitted required reference");
               dependency->setCurrentText(reference);
            } else require(dependency->isHidden(),"Independent property asks for a reference");
            browser.findChild<QPushButton *>("reportUseReference")->click();
            require(entry->currentText()==expected,"Property browser constructed wrong parameter name");
            browser.findChild<QPushButton *>("reportAddParameter")->click();
         };
         choose("X","EarthMJ2000Eq","BrowserSat.EarthMJ2000Eq.X");
         choose("RMAG","Earth","BrowserSat.Earth.RMAG");
         choose("ElapsedSecs",{},"BrowserSat.ElapsedSecs");
         property->setCurrentText("PlanetodeticLAT");
         require(property->currentText()=="PlanetodeticLAT" && dependency->findText("BrowserFixed")>=0 &&
            dependency->findText("EarthMJ2000Eq")<0,"Body-fixed requirement was not applied to reference choices");
         require(editor->toPlainText()==before,"Browsing mutated the mission before Apply");
         const auto snapshot=window.missionSnapshot(); int index=-1;
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.startsWith("Report ")) index=i;
         require(index>=0 && window.applyMissionChange(snapshot,index,MissionEdit::Replace,
            "Report BrowserReport "+browser.selection().join(" ")+";").isEmpty(),"Browsed references failed to apply");
         roundTrip("browsed-report");
         require(window.runMission()==MainWindow::RunResult::Completed && read(path).trimmed()=="7000,7000,0",
            "Browsed coordinate, central-body or independent parameter calculation differs");
      }
      const auto types=window.availableEngineTypes();
      require(types.contains("GmatFunction") && types.contains("Yukon") && types.contains("EclipseLocator"),"Expected native plugins were not registered");
      auto function=read(samples.filePath("Ex_GMATFunction_Math.script"));
      function.replace("BeginMissionSequence;","rf.Filename = '"+output.filePath("cross.txt")+"';\nBeginMissionSequence;");
      editor->setPlainText(function); require(window.buildScript(),"Function sample did not build");
      const auto snapshot=window.missionSnapshot(); int call=-1;
      for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("cross(")) call=i;
      require(call>=0,"Function call not in mission tree");
      QString replacement; CommandForm form([&](const QString &value) { replacement=value; });
      form.setStatement(snapshot.nodes[call].statement);
      auto *inputs=form.findChild<QLineEdit *>("commandField_Inputs"); require(inputs,"Function-call form missing"); inputs->setText("vec2, vec1");
      require(window.applyMissionChange(snapshot,call,MissionEdit::Replace,replacement).isEmpty(),"Function form change rejected");
      roundTrip("function");
      require(window.runMission()==MainWindow::RunResult::Completed,"Function plugin execution failed");
      auto *product=Moderator::Instance()->GetInternalObject("crossProd");
      require(product && std::abs(product->GetRealParameter("SingleValue",0,0)+.25)<1e-12 &&
         std::abs(product->GetRealParameter("SingleValue",1,0)-2.75)<1e-12 &&
         std::abs(product->GetRealParameter("SingleValue",2,0)-2)<1e-12,"Function edited argument order produced wrong cross product");
      auto optimize=read(samples.filePath("Ex_Yukon_AlgebraicOptimization.script"));
      optimize.replace("'MinNLPadYukon1.data'","'"+output.filePath("optimizer.data")+"'");
      optimize.replace("'Ex_AlgebraicOptimization.report'","'"+output.filePath("optimize.txt")+"'");
      editor->setPlainText(optimize);
      require(window.buildScript(),"Yukon sample did not build");
      roundTrip("optimizer");
      require(window.runMission()==MainWindow::RunResult::Completed,"Yukon sample failed");
      for (const auto *name:{"X1","X2"}) {
         auto *variable=Moderator::Instance()->GetInternalObject(name);
         require(variable && std::abs(variable->GetRealParameter("Value")-2)<.01,"Yukon numerical result differs from analytic optimum");
      }
      require(!read(output.filePath("optimize.txt")).trimmed().isEmpty(),"Optimizer report missing");
      const auto eventPath=output.filePath("eclipses.txt");
      editor->setPlainText("Create Spacecraft EventSat;\nCreate ForceModel Forces;\nCreate Propagator Prop;\nProp.FM = Forces;\n"
         "Create EclipseLocator Eclipse;\nEclipse.Spacecraft = EventSat;\nEclipse.OccultingBodies = {Earth};\n"
         "Eclipse.Filename = '"+eventPath+"';\nEclipse.RunMode = Automatic;\nEclipse.WriteReport = true;\n"
         "BeginMissionSequence;\nPropagate Prop(EventSat) {EventSat.ElapsedSecs = 12000};\n");
      require(window.buildScript(),"Eclipse fixture failed to build");
      require(window.applyResourceChanges("Eclipse",{{"EclipseTypes","Umbra, Penumbra"},{"OccultingBodies","Earth, Luna"}},editor->toPlainText()).isEmpty(),
         "Event locator list settings failed");
      roundTrip("event");
      require(window.runMission()==MainWindow::RunResult::Completed,"Eclipse locator execution failed");
      const auto events=read(eventPath);
      require(events.contains("Umbra") || events.contains("Penumbra"),"Eclipse locator found no expected shadow intervals");
      auto *resources=window.findChild<QTreeWidget *>("Resources");
      require(resources->findItems("Event Locators",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Event locator resource category missing");
      auto *outputs=window.findChild<QTreeWidget *>("Output");
      const auto reports=outputs->findItems("Eclipse",Qt::MatchExactly|Qt::MatchRecursive);
      require(reports.size()==1 && reports.first()->data(0,Qt::UserRole).toString()==eventPath,"Event report absent from Output");
      auto python=read(samples.filePath("Ex_PythonInterface.script"));
      const auto pythonReport=output.filePath("python-cross.txt");
      python.replace("BeginMissionSequence;","Create ReportFile PythonReport;\nPythonReport.Filename = '"+pythonReport+"';\nBeginMissionSequence;");
      python+="\nReport PythonReport crossProd(1,1) crossProd(1,2) crossProd(1,3);\n";
      editor->setPlainText(python); require(window.buildScript(),"Python sample did not build");
      roundTrip("python");
      require(window.runMission()==MainWindow::RunResult::Completed,"Python interface execution failed");
      auto *position=Moderator::Instance()->GetInternalObject("state");
      auto *velocity=Moderator::Instance()->GetInternalObject("vel");
      auto *cross=Moderator::Instance()->GetInternalObject("crossProd");
      require(position && velocity && cross,"Python result arrays missing");
      for (int i=0;i<3;++i) {
         const int j=(i+1)%3,k=(i+2)%3;
         const auto expected=position->GetRealParameter("SingleValue",0,j)*velocity->GetRealParameter("SingleValue",0,k)-
            position->GetRealParameter("SingleValue",0,k)*velocity->GetRealParameter("SingleValue",0,j);
         require(std::abs(cross->GetRealParameter("SingleValue",0,i)-expected)<1e-9,"Python cross product differs from independent calculation");
      }
      require(!read(pythonReport).trimmed().isEmpty(),"Python report missing");
      editor->setPlainText("Create Spacecraft MemberA MemberB;\nCreate Formation Fleet;\nFleet.Add = {MemberA, MemberB};\n"
         "Create ForceModel Forces;\nCreate Propagator Prop;\nProp.FM = Forces;\n"
         "BeginMissionSequence;\nPropagate Prop(Fleet) {MemberA.ElapsedSecs = 60};\n");
      require(window.buildScript(),"Formation fixture did not build");
      const auto formationSource=editor->toPlainText();
      require(!window.applyResourceChanges("Fleet",{{"Add","Prop"}},formationSource).isEmpty() && editor->toPlainText()==formationSource,
         "Formation accepted a non-spacecraft member or changed source after rejection");
      require(window.applyResourceChanges("Fleet",{{"Add","MemberB, MemberA"}},editor->toPlainText()).isEmpty(),"Formation member configuration failed");
      roundTrip("formation");
      const auto epoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("MemberA"))->GetEpoch();
      require(window.runMission()==MainWindow::RunResult::Completed,"Formation propagation failed");
      for (const auto *name:{"MemberA","MemberB"}) {
         auto *member=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject(name));
         require(member && std::abs((member->GetEpoch()-epoch)*86400-60)<.01,"Formation did not propagate both members for 60 seconds");
      }
      auto tle=read(samples.filePath("Ex_TLE_Propagation.script"));
      const auto tleReport=output.filePath("tle.txt");
      tle.replace("'Ex_TLE_Propagation.txt'","'"+tleReport+"'");
      tle.replace("'../samples/SupportFiles/Ex_TLE_Propagation_TLE.txt'","'"+samples.filePath("SupportFiles/Ex_TLE_Propagation_TLE.txt")+"'");
      tle.replace(QRegularExpression("RF\\.Add\\s*=\\s*\\{[^}]*\\}"),
         "RF.Add = {ExampleSat.A1ModJulian, ExampleSat.X, ExampleSat.Y, ExampleSat.Z, ExampleSat.VX, ExampleSat.VY, ExampleSat.VZ}");
      editor->setPlainText(tle); require(window.buildScript(),"TLE example did not build");
      bool stepEditor=false;
      for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("TLEProp"))) stepEditor=stepEditor || field.name=="InitialStepSize";
      require(stepEditor,"TLE step configuration missing from Qt resource editor");
      roundTrip("tle");
      auto tleEndpoint=[&] {
         require(window.runMission()==MainWindow::RunResult::Completed,"TLE propagation failed");
         const auto report=read(tleReport).trimmed().split('\n');
         require(report.size()>200,"TLE report is missing propagated samples");
         const auto first=report[1].trimmed().split(QRegularExpression("\\s+"));
         const auto last=report.last().trimmed().split(QRegularExpression("\\s+"));
         require(first.size()==7 && last.size()==7,"Unexpected TLE report columns");
         std::array<double,7> endpoint;
         for (int i=0;i<7;++i) { bool valid=false; endpoint[i]=last[i].toDouble(&valid); require(valid && std::isfinite(endpoint[i]),"Nonfinite TLE state"); }
         require(std::abs(endpoint[0]-first[0].toDouble()-1)<1e-8,"TLE mission did not span one day");
         const double radius=std::hypot(endpoint[1],endpoint[2],endpoint[3]);
         require(radius>6800 && radius<7500,"TLE orbit radius inconsistent with the supplied low-Earth orbit");
         auto *spacecraft=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("ExampleSat"));
         require(spacecraft && std::abs(spacecraft->GetEpoch()-endpoint[0])<1e-9,"TLE report and propagated epoch disagree");
         return endpoint;
      };
      const auto baseline=tleEndpoint();
      require(window.applyResourceChanges("TLEProp",{{"InitialStepSize","120"}},editor->toPlainText()).isEmpty(),"TLE step edit failed");
      roundTrip("tle-step");
      const auto refined=tleEndpoint();
      for (int i=1;i<7;++i) require(std::abs(baseline[i]-refined[i])<1e-4,"TLE endpoint changed with output sampling step");
      const auto validTle=editor->toPlainText();
      auto missingTle=validTle; missingTle.replace(samples.filePath("SupportFiles/Ex_TLE_Propagation_TLE.txt"),output.filePath("missing-tle.txt"));
      editor->setPlainText(missingTle);
      require(window.runMission()==MainWindow::RunResult::Failed,"Missing TLE file was silently accepted");
      editor->setPlainText(validTle); const auto recovered=tleEndpoint();
      for (int i=1;i<7;++i) require(std::abs(refined[i]-recovered[i])<1e-8,"TLE failed-file recovery changed the result");
      const double mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant();
      const double radius=7000,speed=std::sqrt(mu/radius),duration=1200,angle=std::sqrt(mu/(radius*radius*radius))*duration;
      for (const auto *type:{"BulirschStoer","PrinceDormand853"}) {
         const auto integratorReport=output.filePath(QString(type)+".txt");
         editor->setPlainText(QString("Create Spacecraft Circle;\nCircle.X = 7000;\nCircle.Y = 0;\nCircle.Z = 0;\n"
            "Circle.VX = 0;\nCircle.VY = %1;\nCircle.VZ = 0;\n"
            "Create ForceModel PointMass;\nPointMass.PrimaryBodies = {};\nPointMass.PointMasses = {Earth};\n"
            "Create Propagator Prop;\nProp.FM = PointMass;\nProp.Type = %2;\nProp.InitialStepSize = 60;\nProp.Accuracy = 1e-12;\n"
            "Create ReportFile OrbitReport;\nOrbitReport.Filename = '%3';\n"
            "BeginMissionSequence;\nPropagate Prop(Circle) {Circle.ElapsedSecs = 1200};\n"
            "Report OrbitReport Circle.X Circle.Y Circle.Z;\n").arg(QString::number(speed,'g',17),type,integratorReport));
         require(window.buildScript(),"Plugin integrator fixture did not build");
         const auto settingError=window.applyResourceChanges("Prop",{{"InitialStepSize","30"}},editor->toPlainText());
         if (!settingError.isEmpty()) std::cerr<<type<<": "<<settingError.toStdString()<<std::endl;
         require(settingError.isEmpty(),"Owned plugin integrator setting failed");
         roundTrip(type);
         require(window.runMission()==MainWindow::RunResult::Completed,"Plugin integrator propagation failed");
         auto *circle=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Circle"));
         require(circle && std::abs(circle->GetRealParameter("X")-radius*std::cos(angle))<1e-4 &&
            std::abs(circle->GetRealParameter("Y")-radius*std::sin(angle))<1e-4 && std::abs(circle->GetRealParameter("Z"))<1e-4,
            "Plugin integrator differs from analytic circular-orbit solution");
         require(!read(integratorReport).trimmed().isEmpty(),"Plugin integrator report missing");
      }
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",output.path().toStdString());
      require(types.contains("Save"),"Save plugin is missing from available command types");
      const QString saveFixture="Create Spacecraft SavedSat;\nCreate Variable SavedNumber;\n"
         "BeginMissionSequence;\nSavedSat.DryMass = 825.25;\nSavedNumber = 42.5;\nSave SavedSat;\n";
      editor->setPlainText(saveFixture); require(window.buildScript(),"Save command fixture failed to build");
      const auto saveSnapshot=window.missionSnapshot(); int saveIndex=-1;
      for (int i=0;i<saveSnapshot.nodes.size();++i) if (saveSnapshot.nodes[i].type=="Save") saveIndex=i;
      require(saveIndex>=0,"Save command missing from mission tree");
      QString saveStatement; CommandForm saveForm([&](const QString &value) { saveStatement=value; });
      saveForm.setStatement(saveSnapshot.nodes[saveIndex].statement);
      auto *objects=saveForm.findChild<QLineEdit *>("commandField_Objects"); require(objects,"Save object-list controls missing");
      objects->setText("SavedSat SavedNumber");
      require(window.applyMissionChange(saveSnapshot,saveIndex,MissionEdit::Replace,saveStatement).isEmpty(),"Save object-list edit failed");
      roundTrip("save-command");
      const auto validSave=editor->toPlainText();
      require(window.runMission()==MainWindow::RunResult::Completed,"Save command execution failed");
      const auto exportPath=output.filePath("SavedSat_SavedNumber.data");
      const auto savedObjects=read(exportPath);
      require(savedObjects.contains("SavedSat.DryMass = 825.25") && savedObjects.contains("SavedNumber = 42.5"),
         "Save exported initial rather than current object values");
      require(window.loadScript(exportPath) && window.buildScript(),"Saved object definitions could not reopen in Qt");
      require(std::abs(Moderator::Instance()->GetConfiguredObject("SavedSat")->GetRealParameter("DryMass")-825.25)<1e-12 &&
         std::abs(Moderator::Instance()->GetConfiguredObject("SavedNumber")->GetRealParameter("Value")-42.5)<1e-12,
         "Saved object reload changed numeric values");
      editor->setPlainText(validSave);
      require(window.runMission()==MainWindow::RunResult::Completed && read(exportPath)==savedObjects,"Repeat mission appended stale Save output");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",output.filePath("missing/nested").toStdString());
      require(window.runMission()==MainWindow::RunResult::Failed,"Save silently accepted an invalid output path");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",output.path().toStdString());
      require(window.runMission()==MainWindow::RunResult::Completed && read(exportPath)==savedObjects,"Save did not recover unchanged after file failure");
#ifdef __linux__
      require(QFile::remove(exportPath) && QFile::link("/dev/full",exportPath),"Cannot construct write-failure fixture");
      require(window.runMission()==MainWindow::RunResult::Failed,"Save silently accepted a disk write failure");
      require(QFile::remove(exportPath),"Cannot remove write-failure fixture");
      require(window.runMission()==MainWindow::RunResult::Completed && read(exportPath)==savedObjects,"Save did not recover after a disk write failure");
#endif
      editor->setPlainText("Create Variable SavedNumber Step;\nBeginMissionSequence;\nFor Step = 1:1:2;\nSavedNumber = Step * 10;\nSave SavedNumber;\nEndFor;\n");
      require(window.runMission()==MainWindow::RunResult::Completed,"Save inside a loop failed");
      const auto loopSaved=read(output.filePath("SavedNumber.Variable.data"));
      require(loopSaved.count("Create Variable SavedNumber")==2 && loopSaved.contains("SavedNumber = 10") && loopSaved.contains("SavedNumber = 20"),
         "Repeated Save command did not retain both snapshots");
      std::cout<<"PASS: Save object controls, exact round trips, exported runtime values, reopen, repeat execution, loop snapshots, bad-path and disk-write recovery\n";
      std::cout<<"PASS: BulirschStoer and PrinceDormand853 configuration, round trips and analytic circular-orbit checks\n";
      std::cout<<"PASS: TLE sample, step configuration, report epoch/state, sampling invariance and missing-file recovery\n";
      std::cout<<"PASS: Python cross product and report, configured formation members and simultaneous propagation\n";
      std::cout<<"PASS: plugin save, Save As, exact source round trip and failed-build recovery before numerical execution\n";
      std::cout<<"PASS: registered native plugins, edited GMAT function arguments and cross product, Yukon analytic optimum, automatic eclipse events and report access\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
