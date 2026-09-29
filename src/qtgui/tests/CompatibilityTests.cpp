#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotModel.hpp"
#include "TestSettings.hpp"
#include "CommandForm.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "ResourceProperties.hpp"
#include "ResourceEditor.hpp"
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
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
         const auto kernelRoot=QFileInfo(startup).dir().absoluteFilePath("../data/vehicle/ephem/spk/");
         QMap<QString,QString> kernels;
         for (const auto &pair:QList<QPair<QString,QString>>{
               {"OrbitSpiceKernelName","MarsExpress_Short.bsp"},
               {"AttitudeSpiceKernelName","MarsExpress_ATNM_PTR00012_100531_002.BC"},
               {"SCClockSpiceKernelName","MarsExpress_MEX_100921_STEP.TSC"}}) {
            const auto path=output.filePath(pair.second);
            require(QFile::copy(QDir(kernelRoot).filePath(pair.second),path),"Mars Express kernel fixture unavailable"); kernels[pair.first]=path;
         }
         auto source=read(samples.filePath("Ex_SPICEOrbitAndAttitudePropagation.script"));
         source.remove(QRegularExpression("^MarsExpress\\.(?:OrbitSpiceKernelName|AttitudeSpiceKernelName|SCClockSpiceKernelName|FrameSpiceKernelName|Attitude)[ \\t]*=[^\\n]*\\n",QRegularExpression::MultilineOption));
         source.replace("MarsExpress.ElapsedDays = 1.0","MarsExpress.ElapsedSecs = 60");
         const auto report=output.filePath("spice-view.csv");
         source.replace("BeginMissionSequence;","Create ReportFile SpiceReport;\nSpiceReport.Filename = '"+report+"';\n"
            "SpiceReport.Add = {MarsExpress.ElapsedSecs, MarsExpress.MarsMJ2000Eq.X, MarsExpress.MarsMJ2000Eq.Y, MarsExpress.MarsMJ2000Eq.Z};\n"
            "SpiceReport.FixedWidth = false;\nSpiceReport.WriteHeaders = false;\nSpiceReport.Delimiter = ',';\nSpiceReport.Precision = 16;\nBeginMissionSequence;");
         editor->setPlainText(source);
         require(window.convertOpenFramesScript(),"SPICE example Qt view conversion failed");
         const auto before=editor->toPlainText(); QString applyError="No Apply";
         {
            QWidget owner;
            ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("MarsExpress"),
               [&](const QMap<QString,QString> &changes) { applyError=window.applyResourceChanges("MarsExpress",changes,before); return applyError; },&owner);
            for (auto it=kernels.cbegin();it!=kernels.cend();++it) {
               const auto path=it.value();
               QTimer::singleShot(0,&panel,[&panel,path] {
                  auto *dialog=panel.findChild<QDialog *>("kernelFileDialog"); if (!dialog) return;
                  dialog->findChild<QListWidget *>("kernelFileList")->addItem(path); dialog->accept();
               });
               auto *button=panel.findChild<QPushButton *>("chooseProperty_"+it.key()); require(button,"SPICE input list control absent"); button->click();
            }
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(applyError.isEmpty(),qPrintable(applyError));
         require(window.applyResourceChanges("MarsExpress",{{"Attitude","SpiceAttitude"}},editor->toPlainText()).isEmpty(),"SpiceAttitude selection rejected");
         roundTrip("spice-kernels");
         require(window.runMission()==MainWindow::RunResult::Completed,"Configured SPICE orbit/attitude example failed");
         const auto expectedReport=read(report);
         const auto rows=expectedReport.trimmed().split('\n'); require(rows.size()>=13,"SPICE report lacks propagated samples");
         const auto final=rows.last().split(','); require(final.size()==4 && std::abs(final[0].toDouble()-60)<1e-5,"SPICE run did not reach requested stop");
         auto model=window.plotReceiver()->model("OFI_Mars3DView"); require(model!=nullptr,"Converted SPICE view absent");
         const PlotCurve *curve=nullptr; for (const auto &candidate:model->curves) if (candidate.name=="MarsExpress") curve=&candidate;
         require(curve && curve->points.size()>=13,"SPICE viewer has no orbit samples");
         const auto &last=curve->points.back();
         require(std::abs(last.x-final[1].toDouble())<1e-5 && std::abs(last.y-final[2].toDouble())<1e-5 && std::abs(last.z-final[3].toDouble())<1e-5,
            "SPICE viewer trajectory disagrees with mission report");
         double changed=0;
         for (int i=0;i<9;++i) { require(std::isfinite(last.bodyToView[i]),"SPICE attitude viewer matrix is invalid"); changed+=std::abs(last.bodyToView[i]-curve->points.front().bodyToView[i]); }
         auto *runtime=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("MarsExpress"));
         require(runtime && runtime->GetStringParameter("Attitude")=="SpiceAttitude","Mission lost selected SPICE attitude model");
         const auto firstAttitude=runtime->GetAttitude(curve->points.front().epoch);
         const auto finalAttitude=runtime->GetAttitude(last.epoch);
         double engineChange=0;
         for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
            engineChange+=std::abs(finalAttitude(i,j)-firstAttitude(i,j));
            require(std::abs(last.bodyToView[3*i+j]-finalAttitude(j,i))<1e-10,"SPICE viewer attitude disagrees with engine orientation");
         }
         std::cout<<"SPICE attitude change: engine="<<engineChange<<" viewer="<<changed<<'\n';
         require(engineChange<=1e-8 || changed>1e-8,"Changing SPICE attitude was not recorded by the viewer");
         for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
            double dot=0; for (int k=0;k<3;++k) dot+=last.bodyToView[3*i+k]*last.bodyToView[3*j+k];
            require(std::abs(dot-(i==j ? 1. : 0.))<1e-8,"SPICE attitude matrix is not orthonormal");
         }
         const auto clock=kernels.value("SCClockSpiceKernelName");
         require(QFile::rename(clock,clock+".away"),"Cannot stage unavailable clock kernel");
         require(window.runMission()==MainWindow::RunResult::Failed,"Missing clock kernel was silently accepted");
         require(QFile::rename(clock+".away",clock),"Cannot restore clock fixture");
         const auto recovered=window.runMission();
         if (recovered!=MainWindow::RunResult::Completed) for (auto *log:window.findChildren<QPlainTextEdit *>()) if (log!=editor) std::cerr<<log->toPlainText().right(9000).toStdString()<<'\n';
         require(recovered==MainWindow::RunResult::Completed,"SPICE mission did not recover after clock restoration");
         require(read(report)==expectedReport,"Recovered SPICE run changed report values");
         require(window.runMission()==MainWindow::RunResult::Completed && read(report)==expectedReport,"Repeated SPICE run after recovery changed output");
         std::cout<<"PASS: SPICE kernel GUI lists, Qt OF conversion, orbit/attitude samples, report agreement, round trips and unavailable-clock recovery\n";
      }
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
         auto *previewWindow=qobject_cast<QMdiSubWindow *>(preview->parentWidget()->parentWidget());
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
         editor->setPlainText("Create ChemicalTank BrowserTank SpareTank;\nBrowserTank.FuelMass = 123.5;\n"
            "Create ChemicalThruster BrowserThruster SpareThruster;\nBrowserThruster.Tank = {BrowserTank};\n"
            "BrowserThruster.C1 = 12.5;\nCreate Spacecraft BrowserSat EmptySat;\nBrowserSat.Tanks = {BrowserTank};\n"
            "BrowserSat.Thrusters = {BrowserThruster};\nBrowserSat.DisplayStateType = Cartesian;\n"
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
         choose("Q4",{},"BrowserSat.Q4");
         choose("FuelMass","BrowserTank","BrowserSat.BrowserTank.FuelMass");
         require(dependency->findText("SpareTank")<0,"Unattached tank offered by hardware browser");
         choose("C1","BrowserThruster","BrowserSat.BrowserThruster.C1");
         require(dependency->findText("SpareThruster")<0,"Unattached thruster offered by hardware browser");
         owner->setCurrentText("EmptySat"); property->setCurrentText("FuelMass");
         require(dependency->count()==0 && !browser.findChild<QPushButton *>("reportUseReference")->isEnabled(),
            "Hardware reference remained selectable on a spacecraft without attachments");
         owner->setCurrentText("BrowserSat");

         property->setCurrentText("PlanetodeticLAT");
         require(property->currentText()=="PlanetodeticLAT" && dependency->findText("BrowserFixed")>=0 &&
            dependency->findText("EarthMJ2000Eq")<0,"Body-fixed requirement was not applied to reference choices");
         require(editor->toPlainText()==before,"Browsing mutated the mission before Apply");
         const auto snapshot=window.missionSnapshot(); int index=-1;
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.startsWith("Report ")) index=i;
         require(index>=0 && window.applyMissionChange(snapshot,index,MissionEdit::Replace,
            "Report BrowserReport "+browser.selection().join(" ")+";").isEmpty(),"Browsed references failed to apply");
         roundTrip("browsed-report");
         require(window.runMission()==MainWindow::RunResult::Completed && read(path).trimmed()=="7000,7000,0,1,123.5,12.5",
            "Browsed coordinate, central-body or independent parameter calculation differs");
      }
      {
         const auto path=output.filePath("force-model-rates.txt");
         const double mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant();
         const double speed=std::sqrt(mu/7000.);
         editor->setPlainText("Create Spacecraft RateSat;\nRateSat.DisplayStateType = Cartesian;\n"
            "RateSat.X = 7000;\nRateSat.Y = 0;\nRateSat.Z = 0;\nRateSat.VX = 0;\nRateSat.VY = "+QString::number(speed,'g',17)+";\nRateSat.VZ = 0;\n"
            "Create ForceModel RateForces;\nRateForces.PrimaryBodies = {};\nRateForces.PointMasses = {Earth};\n"
            "Create Propagator RateProp;\nRateProp.FM = RateForces;\n"
            "Create ReportFile RateReport;\nRateReport.Filename = '"+path+"';\nRateReport.FixedWidth = false;\n"
            "RateReport.WriteHeaders = false;\nRateReport.Delimiter = ',';\nRateReport.Precision = 16;\n"
            "BeginMissionSequence;\nReport RateReport RateSat.X;\nPropagate RateProp(RateSat) {RateSat.ElapsedSecs = 1};\n");
         require(window.buildScript(),"Force-model rate fixture failed");
         ReportParameterDialog browser({});
         auto *owner=browser.findChild<QComboBox *>("reportPropertyObject"); owner->setCurrentText("RateSat");
         auto *property=browser.findChild<QComboBox *>("reportPropertyType");
         auto *dependency=browser.findChild<QComboBox *>("reportPropertyDependency");
         for (const auto *name:{"SMADot","TLONGDot"}) {
            require(property->findText(name)>=0,"Force-model rate absent from browser"); property->setCurrentText(name);
            require(dependency->findText("RateForces")>=0,"Force-model reference missing from browser"); dependency->setCurrentText("RateForces");
            browser.findChild<QPushButton *>("reportUseReference")->click(); browser.findChild<QPushButton *>("reportAddParameter")->click();
         }
         const auto snapshot=window.missionSnapshot(); int index=-1;
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.startsWith("Report ")) index=i;
         require(index>=0 && window.applyMissionChange(snapshot,index,MissionEdit::Replace,
            "Report RateReport "+browser.selection().join(" ")+";").isEmpty(),"Force-model parameter references rejected");
         roundTrip("force-model-rates");
         require(window.runMission()==MainWindow::RunResult::Completed,"Force-model rates mission failed");
         const auto values=read(path).trimmed().split(',');
         require(values.size()==2 && std::abs(values[0].toDouble())<1e-10,"Unperturbed semimajor-axis rate is not zero");
         const double expected=speed/7000.*180./std::acos(-1.);
         require(std::abs(values[1].toDouble()-expected)<1e-12,"Unperturbed true-longitude rate differs from circular angular velocity");
         const auto velocityError=window.applyResourceChanges("RateSat",{{"Element5",QString::number(speed*1.1,'g',17)}},editor->toPlainText());
         if (!velocityError.isEmpty()) throw std::runtime_error("Eccentric-orbit velocity edit failed: "+velocityError.toStdString());
         roundTrip("eccentric-force-model-rates");
         require(window.runMission()==MainWindow::RunResult::Completed,"Eccentric force-model rates mission failed");
         const auto eccentric=read(path).trimmed().split(',');
         require(eccentric.size()==2 && std::abs(eccentric[0].toDouble())<1e-10 &&
            std::abs(eccentric[1].toDouble()-expected*1.1)<1e-12,"Eccentric unperturbed rates differ from instantaneous angular velocity");
      }
      {
         editor->setPlainText("Create ElectricThruster ElectricEngine;\nBeginMissionSequence;\n");
         require(window.buildScript(),"Electric coefficient fixture failed");
         QWidget owner; QString applyError="Not applied";
         const auto before=editor->toPlainText();
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("ElectricEngine"),
            [&](const auto &changes) { applyError=window.applyResourceChanges("ElectricEngine",changes,before); return applyError; },&owner);
         QTimer::singleShot(0,[&] {
            auto *dialog=panel.findChild<QDialog *>("thrusterCoefficientDialog"); require(dialog,"Electric coefficient dialog absent");
            for (const auto *name:{"thrusterThrustCoefficients","thrusterSecondaryCoefficients"}) {
               auto *grid=dialog->findChild<QTableWidget *>(name);
               require(grid->rowCount()==5,"Electric coefficient count incorrect");
               grid->item(4,1)->setText("0.0125");
            }
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         });
         panel.findChild<QPushButton *>("thrusterCoefficients")->click();
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(applyError.isEmpty(),qPrintable(applyError));
         roundTrip("electric-coefficients");
         auto *engine=Moderator::Instance()->GetConfiguredObject("ElectricEngine");
         require(engine->GetRealParameter("ThrustCoeff5")==0.0125 && engine->GetRealParameter("MassFlowCoeff5")==0.0125,
            "Electric coefficients lost during save/reopen");
      }
      {
         const auto path=output.filePath("finite-burn-mass.txt");
         editor->setPlainText("Create ChemicalTank BurnTank BurnTank2;\nBurnTank.FuelMass = 150;\nBurnTank2.FuelMass = 150;\n"
            "Create ChemicalThruster BurnEngine;\nBurnEngine.Tank = {BurnTank};\nBurnEngine.DecrementMass = true;\n"
            "BurnEngine.C1 = 90;\nBurnEngine.K1 = 290;\nBurnEngine.CoordinateSystem = EarthMJ2000Eq;\n"
            "Create Spacecraft BurnSat OtherSat;\nBurnSat.Tanks = {BurnTank, BurnTank2};\nBurnSat.Thrusters = {BurnEngine};\n"
            "Create FiniteBurn Continuous;\nContinuous.Thrusters = {BurnEngine};\n"
            "Create ForceModel BurnForces;\nBurnForces.PrimaryBodies = {};\nBurnForces.PointMasses = {Earth};\n"
            "Create Propagator BurnProp;\nBurnProp.FM = BurnForces;\n"
            "Create ReportFile BurnReport;\nBurnReport.Filename = '"+path+"';\nBurnReport.FixedWidth = false;\n"
            "BurnReport.WriteHeaders = false;\nBurnReport.Precision = 16;\n"
            "BeginMissionSequence;\nBeginFiniteBurn 'Start engine' Continuous(OtherSat); % retain start\n"
            "Propagate BurnProp(BurnSat) {BurnSat.ElapsedSecs = 10};\n"
            "EndFiniteBurn 'Stop engine' Continuous(OtherSat); % retain end\n"
            "Report BurnReport BurnSat.BurnTank.FuelMass BurnSat.BurnTank2.FuelMass;\n"
            "Propagate BurnProp(BurnSat) {BurnSat.ElapsedSecs = 10};\nReport BurnReport BurnSat.BurnTank.FuelMass BurnSat.BurnTank2.FuelMass;\n");
         require(window.buildScript(),"Finite-burn selector fixture failed");
         {
            QWidget owner; QString applyError="Not applied";
            const auto before=editor->toPlainText();
            ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("BurnEngine"),
               [&](const auto &changes) { applyError=window.applyResourceChanges("BurnEngine",changes,before); return applyError; },&owner);
            auto *button=panel.findChild<QPushButton *>("thrusterCoefficients"); require(button,"Thruster coefficient control absent");
            auto *properties=panel.findChild<QTableWidget *>();
            auto property=[&](const QString &name) -> QTableWidgetItem * {
               for (int row=0;row<properties->rowCount();++row) if (properties->item(row,0)->text()==name) return properties->item(row,1);
               throw std::runtime_error("Coefficient property missing");
            };
            property("C1")->setText("95");
            QTimer::singleShot(0,[&] {
               auto *dialog=panel.findChild<QDialog *>("thrusterCoefficientDialog"); require(dialog,"Coefficient dialog missing");
               auto *grid=dialog->findChild<QTableWidget *>("thrusterThrustCoefficients");
               require(grid->rowCount()==16 && grid->item(0,1)->text()=="95","Coefficient dialog lost pending edits");
               require(!grid->item(0,2)->text().isEmpty() && !(grid->item(0,2)->flags() & Qt::ItemIsEditable),"Coefficient units absent or editable");
               require(grid->horizontalHeader()->sectionResizeMode(1)==QHeaderView::Interactive,"Coefficient widths locked");
               grid->setColumnWidth(1,251); require(grid->columnWidth(1)==251,"Coefficient width cannot be adjusted");
               grid->item(0,1)->setText("999"); dialog->reject();
            }); button->click();
            require(property("C1")->text()=="95","Coefficient Cancel changed pending properties");
            QTimer::singleShot(0,[&] {
               auto *dialog=panel.findChild<QDialog *>("thrusterCoefficientDialog");
               auto *thrust=dialog->findChild<QTableWidget *>("thrusterThrustCoefficients");
               auto *impulse=dialog->findChild<QTableWidget *>("thrusterSecondaryCoefficients");
               auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
               thrust->item(0,1)->setText("100"); impulse->item(0,1)->setText("1e999"); ok->click();
               require(dialog->isVisible() && !dialog->findChild<QLabel *>("thrusterCoefficientError")->text().isEmpty(),"Nonfinite coefficient accepted");
               require(property("C1")->text()=="95","Invalid coefficient partially copied to properties");
               impulse->item(0,1)->setText("300"); ok->click();
            }); button->click();
            require(property("C1")->text()=="100" && property("K1")->text()=="300","Coefficient values not copied together");
            require(Moderator::Instance()->GetConfiguredObject("BurnEngine")->GetRealParameter("C1")==90,"Coefficient dialog changed engine before Apply");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            require(applyError.isEmpty(),qPrintable(applyError));
         }

         {
            QWidget owner; QString error="Not applied"; const auto before=editor->toPlainText();
            ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("BurnEngine"),
               [&](const auto &changes) { error=window.applyResourceChanges("BurnEngine",changes,before); return error; },&owner);
            QTimer::singleShot(0,[&] {
               auto *dialog=panel.findChild<QDialog *>("tankMixtureDialog"); require(dialog,"Burn mixture dialog missing");
               dialog->findChild<QComboBox *>("tankMixtureAvailable")->setCurrentText("BurnTank2");
               dialog->findChild<QPushButton *>("tankMixtureAdd")->click();
               auto *grid=dialog->findChild<QTableWidget *>("tankMixtureTable");
               require(grid->rowCount()==2,"Second burn tank not added");
               grid->item(0,1)->setText("3"); grid->item(1,1)->setText("2");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            }); panel.findChild<QPushButton *>("thrusterTankMixtures")->click();
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         for (const auto *command:{"BeginFiniteBurn","EndFiniteBurn"}) {
            const auto snapshot=window.missionSnapshot(); int index=-1;
            for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==command) index=i;
            require(index>=0,"Finite-burn command missing");
            QString replacement; CommandForm form([&](const auto &value) { replacement=value; });
            form.setStatement(snapshot.nodes[index].statement);
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QInputDialog *>();
               require(dialog && dialog->comboBoxItems().contains("BurnSat") && !dialog->comboBoxItems().contains("BurnEngine"),
                  "Finite-burn spacecraft selector has wrong resource types");
               dialog->setTextValue("BurnSat"); dialog->accept();
            });
            auto *button=form.findChild<QPushButton *>("commandChoose_Spacecraft"); require(button,"Finite-burn spacecraft picker missing"); button->click();
            require(replacement.contains("Continuous(BurnSat)") && replacement.contains("'"),"Finite-burn selection lost reference or label");
            require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,replacement).isEmpty(),"Finite-burn spacecraft selection failed to apply");
         }
         roundTrip("finite-burn-selected-spacecraft");
         require(window.runMission()==MainWindow::RunResult::Completed,"Selected finite-burn mission failed");
         const auto mass=read(path).trimmed().split('\n');
         const double gravity=Moderator::Instance()->GetConfiguredObject("BurnEngine")->GetRealParameter("GravitationalAccel");
         const double used=100.*10./(300.*gravity);
         require(mass.size()==2,"Finite burn report row count differs");
         const auto firing=mass[0].trimmed().split(QRegularExpression("\\s+")),coasting=mass[1].trimmed().split(QRegularExpression("\\s+"));
         require(firing.size()==2 && coasting.size()==2 && std::abs(firing[0].toDouble()-(150.-used*.6))<1e-7 &&
            std::abs(firing[1].toDouble()-(150.-used*.4))<1e-7 && std::abs(firing[0].toDouble()-coasting[0].toDouble())<1e-10 &&
            std::abs(firing[1].toDouble()-coasting[1].toDouble())<1e-10,"GUI mixture ratios or EndFiniteBurn shutdown produced incorrect tank fuel use");
      }
      {
         const auto first=output.filePath("toggle-first.txt"),second=output.filePath("toggle-second.txt");
         editor->setPlainText("Create Spacecraft ToggleSat;\nCreate ForceModel ToggleForces;\nCreate Propagator ToggleProp;\nToggleProp.FM = ToggleForces;\n"
            "Create ReportFile FirstOutput SecondOutput;\nFirstOutput.Filename = '"+first+"';\nSecondOutput.Filename = '"+second+"';\n"
            "FirstOutput.Add = {ToggleSat.ElapsedSecs};\nSecondOutput.Add = {ToggleSat.ElapsedSecs};\n"
            "FirstOutput.WriteHeaders = false;\nSecondOutput.WriteHeaders = false;\n"
            "BeginMissionSequence;\nToggle 'Quiet' FirstOutput On; % pause outputs\n"
            "Propagate ToggleProp(ToggleSat) {ToggleSat.ElapsedSecs = 10};\n"
            "Toggle 'Resume' FirstOutput On; % resume outputs\nPropagate ToggleProp(ToggleSat) {ToggleSat.ElapsedSecs = 10};\n");
         require(window.buildScript(),"Toggle fixture failed");
         for (int ordinal=0;ordinal<2;++ordinal) {
            const auto snapshot=window.missionSnapshot(); int index=-1,seen=0;
            for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Toggle" && seen++==ordinal) { index=i; break; }
            require(index>=0,"Toggle command absent");
            QString replacement; CommandForm form([&](const auto &value) { replacement=value; }); form.setStatement(snapshot.nodes[index].statement);
            auto *button=form.findChild<QPushButton *>("commandChoose_Subscribers"); require(button,"Toggle subscriber selector absent");
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QDialog *>("toggleSubscriberDialog"); auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList");
               require(list->findItems("ToggleSat",Qt::MatchExactly).isEmpty(),"Toggle picker includes a spacecraft");
               for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked);
               require(!dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Empty Toggle selection can be accepted");
               for (const auto *name:{"FirstOutput","SecondOutput"}) {
                  const auto items=list->findItems(name,Qt::MatchExactly); require(items.size()==1,"Report absent from Toggle picker"); items.first()->setCheckState(Qt::Checked);
               }
               dialog->accept();
            }); button->click();
            auto *state=form.findChild<QComboBox *>("commandToggleState"); require(state,"Toggle state control absent"); state->setCurrentText(ordinal==0 ? "Off" : "On");
            const auto accepted=replacement;
            QTimer::singleShot(0,[&] { auto *dialog=form.findChild<QDialog *>("toggleSubscriberDialog"); dialog->findChild<QListWidget *>("toggleSubscriberList")->clear(); dialog->reject(); }); button->click();
            require(replacement==accepted && replacement.contains("FirstOutput SecondOutput"),"Toggle Cancel or selection lost references");
            require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,replacement).isEmpty(),"Toggle picker edit failed");
         }
         roundTrip("toggle-selected-outputs"); require(window.runMission()==MainWindow::RunResult::Completed,"Selected Toggle mission failed");
         require(read(first)==read(second),"Toggled outputs recorded different intervals");
         const auto rows=read(first).trimmed().split('\n'); require(rows.size()>=2,"Toggle On did not resume recording");
         for (const auto &row:rows) { bool ok=false; const double time=row.trimmed().toDouble(&ok); require(ok && time>=10.-1e-6 && time<=20.+1e-6,"Toggle Off failed to suppress first interval"); }
         require(std::abs(rows.last().trimmed().toDouble()-20)<1e-6,"Toggled report did not reach final epoch");
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
      require(window.applyResourceChanges("Eclipse",{{"RunMode","Manual"}},editor->toPlainText()).isEmpty(),"Manual locator mode failed");
      require(window.applyMissionChange(window.missionSnapshot(),-1,MissionEdit::Append,
         "FindEvents 'Manual search' Eclipse; % retain event comment").isEmpty(),"FindEvents insertion failed");
      auto editAppend=[&](bool append) {
         const auto snapshot=window.missionSnapshot(); int index=-1;
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="FindEvents") index=i;
         require(index>=0,"FindEvents command missing");
         QString replacement=snapshot.nodes[index].statement;
         CommandForm form([&](const auto &value) { replacement=value; }); form.setStatement(replacement);
         if (auto *defaults=form.findChild<QPushButton *>("commandAddOptions")) defaults->click();
         auto *appendControl=form.findChild<QCheckBox *>("commandCheck_Append"); require(appendControl,"Append control absent");
         appendControl->setChecked(append);
         require(replacement.contains("'Manual search'") && replacement.contains("% retain event comment"),"Append editing lost label/comment");
         require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,replacement).isEmpty(),"Append option edit failed");
      };
      editAppend(false); roundTrip("manual-event-replace");
      require(window.runMission()==MainWindow::RunResult::Completed,"Manual FindEvents execution failed");
      const auto replacedEvents=read(eventPath);
      require(replacedEvents.contains("Umbra") || replacedEvents.contains("Penumbra"),"Manual FindEvents report has no intervals");
      editAppend(true); roundTrip("manual-event-append");
      require(window.runMission()==MainWindow::RunResult::Completed,"Append FindEvents execution failed");
      const auto appendedEvents=read(eventPath);
      require(appendedEvents.startsWith(replacedEvents) && appendedEvents.size()>replacedEvents.size(),"Append did not preserve and extend event report");
      editAppend(false);
      require(window.runMission()==MainWindow::RunResult::Completed,"FindEvents replacement rerun failed");
      require(read(eventPath)==replacedEvents,"Clearing Append did not restore replacement behavior");
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
      QTimer::singleShot(0,[&] {
         auto *dialog=saveForm.findChild<QDialog *>("commandObjectDialog");
         auto *list=dialog->findChild<QListWidget *>("commandObjectList");
         require(list->findItems("EarthMJ2000Eq",Qt::MatchExactly).size()==1,"Save picker excluded automatic global resources");
         for (int i=0;i<list->count();++i) require(!list->item(i)->text().contains('.'),"Object picker included computed system parameters");
         const auto number=list->findItems("SavedNumber",Qt::MatchExactly); require(number.size()==1,"Save picker omitted variable");
         number.first()->setCheckState(Qt::Checked); dialog->accept();
      });
      saveForm.findChild<QPushButton *>("commandChoose_Objects")->click();
      require(objects->text()=="SavedSat SavedNumber","Save picker lost object selection order");
      {
         QString chosen; CommandForm globalForm([&](const auto &value) { chosen=value; }); globalForm.setStatement("Global SavedSat;");
         QTimer::singleShot(0,[&] {
            auto *dialog=globalForm.findChild<QDialog *>("commandObjectDialog"); auto *list=dialog->findChild<QListWidget *>("commandObjectList");
            require(list->findItems("EarthMJ2000Eq",Qt::MatchExactly).isEmpty() && list->findItems("Earth",Qt::MatchExactly).isEmpty(),
               "Global picker includes automatically global objects");
            const auto number=list->findItems("SavedNumber",Qt::MatchExactly); require(number.size()==1,"Global picker omitted user variable");
            number.first()->setCheckState(Qt::Checked); dialog->accept();
         }); globalForm.findChild<QPushButton *>("commandChoose_Objects")->click();
         require(chosen=="Global SavedSat SavedNumber;","Global picker changed command syntax or order");
         globalForm.setStatement("Clear SavedSat;");
         QTimer::singleShot(0,[&] { auto *dialog=globalForm.findChild<QDialog *>("commandObjectDialog"); dialog->findChild<QListWidget *>("commandObjectList")->clear(); dialog->reject(); });
         globalForm.findChild<QPushButton *>("commandChoose_Objects")->click();
         require(globalForm.findChild<QLineEdit *>("commandField_Objects")->text()=="SavedSat","Clear selector Cancel changed objects");
      }
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
