#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "EphemerisDialog.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QTableWidget>
#include <QTreeWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QWindow>
#include <QClipboard>
#include <QStatusBar>
#include <QRegularExpression>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString diagnostics(MainWindow &window) { QApplication::processEvents(); return window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(4000); }
static void build(MainWindow &window) { if (!window.buildScript()) throw std::runtime_error(("Build failed: "+diagnostics(window)).toStdString()); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(("Run failed: "+diagnostics(window)).toStdString()); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static bool waitUntil(const std::function<bool()> &ready)
{
   if (ready()) return true;
   QEventLoop loop; QElapsedTimer elapsed; elapsed.start(); QTimer poll; poll.setInterval(10);
   QObject::connect(&poll,&QTimer::timeout,&loop,[&] { if (ready() || elapsed.elapsed()>3000) loop.quit(); }); poll.start(); loop.exec(); return ready();
}
static QVector<QVector<double>> numericRows(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Ephemeris report unavailable"); QVector<QVector<double>> result;
   for (const auto &line:QString::fromUtf8(file.readAll()).trimmed().split('\n')) { QVector<double> row; for (const auto &part:line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok=false; const double value=part.toDouble(&ok); if (!ok) { row.clear(); break; } row.append(value); } if (!row.isEmpty()) result.append(row); } return result;
}
static void same(const QVector<QVector<double>> &actual,const QVector<QVector<double>> &expected)
{
   require(actual.size()==expected.size(),"Export changed report row count"); for (int r=0;r<actual.size();++r) { require(actual[r].size()==expected[r].size(),"Export changed report fields"); for (int c=0;c<actual[r].size();++c) require(std::abs(actual[r][c]-expected[r][c])<1e-9,"Export GUI changed propagated report/state"); }
}
static void circle(const QVector<double> &state,double time,double mu,double positionTolerance,double velocityTolerance)
{
   require(state.size()==6,"Ephemeris state incomplete"); const double rate=std::sqrt(mu/(7000.*7000.*7000.)),angle=rate*time,speed=std::sqrt(mu/7000.);
   const QVector<double> expected={7000*std::cos(angle),7000*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0};
   double positionError=0,velocityError=0;
   for (int i=0;i<6;++i) {
      const auto error=std::abs(state[i]-expected[i]);
      if (i<3) positionError=std::max(positionError,error); else velocityError=std::max(velocityError,error);
      require(error<(i<3 ? positionTolerance : velocityTolerance),"Ephemeris state disagrees with analytic circular orbit");
   }
   std::cout<<"Analytic circular state at "<<time<<" s: max position error "<<positionError<<" km, max velocity error "<<velocityError<<" km/s\n";
}
static QTreeWidgetItem *exportItem(MainWindow &window,const QString &name)
{
   auto *tree=window.findChild<QTreeWidget *>("Output"); const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); for (auto *item:items) if (item->data(0,Qt::UserRole+1).toString()=="ephemeris") return item; throw std::runtime_error("Ephemeris export missing from Output tree");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtEphemeris");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   QString mode;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Ephemeris fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Ephemeris runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const double mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant(),speed=std::sqrt(mu/7000.);
      const QString state="Create Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.NAIFId = -100055;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(speed,'g',17)+";\nSat.VZ = 0;\n";
      for (const auto &caseName:QStringList{"CCSDS-OEM","STK-TimePosVel","Code-500-LittleEndian","Code-500-BigEndian","SPK"}) {
         mode=caseName; const auto format=mode.startsWith("Code-500") ? QString("Code-500") : mode;
         const auto suffix=format=="SPK" ? ".bsp" : format=="CCSDS-OEM" ? ".oem" : format=="STK-TimePosVel" ? ".e" : ".eph";
         const auto exported=files.filePath(mode+(format=="CCSDS-OEM" ? ".custom" : suffix)),referenceFile=files.filePath("reference-"+mode+suffix),report=files.filePath(mode+"-report.txt"),saved=files.filePath(mode+" ü.script");
         const QString mission="BeginMissionSequence;\nPropagate 'Circular arc' Prop(Sat) {Sat.ElapsedSecs = 600}; % retain export mission\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
         const QString base=state+"Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 5;\nProp.MaxStep = 5;\nProp.Accuracy = 1e-12;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nCreate EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.Filename = '"+files.filePath("inactive.oem")+"';\nExport.WriteEphemeris = false;\n";
         const QMap<QString,QString> desired={{"FileFormat",format},{"Filename",exported},{"CoordinateSystem","EarthMJ2000Eq"},{"WriteEphemeris","true"},{"InterpolationOrder","7"},{"StepSize",format=="SPK" ? "IntegratorSteps" : "20"},{"OutputFormat",mode.endsWith("BigEndian") ? "BigEndian" : "LittleEndian"},{"EpochFormat","UTCGregorian"},{"InitialEpoch","01 Jan 2000 12:01:00.000"},{"FinalEpoch","01 Jan 2000 12:09:00.000"}};
         QString reference=base+"Export.FileFormat = "+format+";\n";
         for (auto it=desired.cbegin();it!=desired.cend();++it) if (it.key()!="FileFormat") reference+="Export."+it.key()+" = "+(QStringList{"Filename","InitialEpoch","FinalEpoch"}.contains(it.key()) ? "'"+(it.key()=="Filename" ? referenceFile : it.value())+"'" : it.value())+";\n";
         if (format=="STK-TimePosVel") reference+="Export.DistanceUnit = Meters;\nExport.IncludeEventBoundaries = false;\n";
         editor->setPlainText(reference+mission); build(window); run(window); const auto expected=numericRows(report); require(expected.size()==1,"Reference export report incomplete"); circle(expected[0],600,mu,1e-6,1e-9);
         editor->setPlainText(base+mission); build(window); run(window); same(numericRows(report),expected); require(!QFileInfo::exists(files.filePath("inactive.oem")),"Disabled export created a file"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
         {
            QWidget owner; auto *resource=Moderator::Instance()->GetConfiguredObject("Export"); ResourceEditor panel(*resource,[&](const auto &changes) { error=window.applyResourceChanges("Export",changes,source); return error; },&owner,source); auto *setup=panel.findChild<QPushButton *>("editEphemeris"); require(setup,"Ephemeris output editor missing");
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("ephemerisDialog"); dialog->findChild<QLineEdit *>("ephemeris_Filename")->setText(exported); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Export Cancel changed pending values");
            later(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("ephemerisDialog"); require(waitUntil([dialog] { return dialog->windowHandle() && dialog->windowHandle()->isExposed(); }),"Ephemeris dialog was not exposed"); auto *fileFormat=dialog->findChild<QComboBox *>("ephemeris_FileFormat"),*frame=dialog->findChild<QComboBox *>("ephemeris_CoordinateSystem"),*step=dialog->findChild<QComboBox *>("ephemeris_StepSize"),*units=dialog->findChild<QComboBox *>("ephemeris_DistanceUnit"); auto *events=dialog->findChild<QCheckBox *>("ephemeris_IncludeEventBoundaries");
               fileFormat->setCurrentText("STK-TimePosVel"); require(units->isEnabled() && events->isEnabled(),"STK settings hidden after format change"); units->setCurrentText("Meters"); events->setChecked(false); fileFormat->setCurrentText("Code-500"); require(dialog->findChild<QComboBox *>("ephemeris_OutputFormat")->isEnabled() && step->currentText()!="IntegratorSteps","Code-500 did not choose numeric sampling"); fileFormat->setCurrentText("SPK"); require(!step->isEnabled() && frame->findText("EarthFixed")<0 && dialog->findChild<QLineEdit *>("ephemeris_Interpolator")->text()=="Hermite","SPK dependencies/frame filtering wrong"); fileFormat->setCurrentText("STK-TimePosVel"); require(units->currentText()=="Meters" && !events->isChecked(),"Format switching lost inactive STK settings");
               fileFormat->setCurrentText(format); for (auto it=desired.cbegin();it!=desired.cend();++it) {
                  if (it.key()=="FileFormat" || it.key()=="Filename") continue;
                  if (auto *combo=dialog->findChild<QComboBox *>("ephemeris_"+it.key())) combo->setCurrentText(it.value()); else if (auto *box=dialog->findChild<QCheckBox *>("ephemeris_"+it.key())) box->setChecked(it.value()=="true"); else dialog->findChild<QLineEdit *>("ephemeris_"+it.key())->setText(it.value());
               }
               auto *epochs=dialog->findChild<QComboBox *>("ephemeris_EpochFormat"),*start=dialog->findChild<QComboBox *>("ephemeris_InitialEpoch"),*end=dialog->findChild<QComboBox *>("ephemeris_FinalEpoch"); require(start->isEditable() && end->isEditable() && step->isEditable(),"Custom epochs/sampling cannot be typed");
               end->setCurrentText("invalid epoch"); epochs->setCurrentText("TAIGregorian"); require(epochs->currentText()=="UTCGregorian" && start->currentText()==desired.value("InitialEpoch") && end->currentText()=="invalid epoch","Failed epoch conversion did not roll back both endpoints"); end->setCurrentText(desired.value("FinalEpoch")); epochs->setCurrentText("TAIGregorian"); require(start->currentText().contains("12:01:32") && end->currentText().contains("12:09:32"),"Paired epoch conversion changed the interval");
               later(dialog,failure,[&] { auto *picker=dialog->findChild<QFileDialog *>("ephemerisFilePicker"); require(picker,"Export filename chooser missing"); picker->selectFile(exported); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); }); dialog->findChild<QPushButton *>("ephemerisBrowse")->click(); if (failure) std::rethrow_exception(failure); require(!QFileInfo::exists(exported),"Filename browsing wrote an export before Apply/run");
               if (format=="SPK") { auto *order=dialog->findChild<QLineEdit *>("ephemeris_InterpolationOrder"); order->setText("4"); close(dialog); require(dialog->isVisible(),"Even SPK Hermite order accepted"); order->setText("7"); }
               else { step->setCurrentText("0"); close(dialog); require(dialog->isVisible(),"Zero export sampling accepted"); step->setCurrentText("20"); }
               dialog->resize(660,430); auto *scroll=dialog->findChild<QScrollArea *>("ephemerisScroll"); waitUntil([dialog,scroll] { return dialog->height()<=430 && scroll->verticalScrollBar()->maximum()>0; });
               if (scroll->verticalScrollBar()->maximum()==0 && !capture.isEmpty()) dialog->grab().save(capture);
               require(scroll->verticalScrollBar()->maximum()>0,qPrintable(QString("Compact ephemeris dialog cannot scroll: dialog height %1, viewport %2, content %3").arg(dialog->height()).arg(scroll->viewport()->height()).arg(scroll->widget()->height()))); auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(ok->mapTo(dialog,ok->rect().bottomRight())),"Export buttons inaccessible");
               if (!capture.isEmpty() && format=="STK-TimePosVel") { dialog->resize(660,760); require(waitUntil([dialog] { return dialog->height()==760; }),"Capture layout did not settle"); require(dialog->grab().save(capture),"Export dialog capture failed"); }
               close(dialog); require(!dialog->isVisible(),qPrintable(dialog->findChild<QLabel *>("ephemerisError")->text()));
            }); setup->click(); if (failure) std::rethrow_exception(failure);
            require(panel.hasChanges() && !resource->GetBooleanParameter("WriteEphemeris"),"Export dialog mutated engine before Apply");
            later(&panel,failure,[&] { auto *dialog=dynamic_cast<EphemerisDialog *>(panel.findChild<QDialog *>("ephemerisDialog")); require(dialog->settings().value("Filename")==exported && dialog->settings().value("EpochFormat")=="TAIGregorian","Pending export values lost on reopen"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"Export Apply altered labeled mission/comments"); editor->undo(); require(editor->toPlainText()==source,"Export Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Export Redo not exact"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Export Unicode save/reopen failed"); build(window); run(window); same(numericRows(report),expected); require(QFileInfo(exported).size()>200,"Export file empty/missing");
         auto *item=exportItem(window,"Export"); require(item->data(0,Qt::UserRole).toString()==exported,"Output tree uses wrong export path"); auto *tree=window.findChild<QTreeWidget *>("Output"); QMetaObject::invokeMethod(tree,"itemDoubleClicked",Qt::DirectConnection,Q_ARG(QTreeWidgetItem *,item),Q_ARG(int,0));
         if (format=="CCSDS-OEM" || format=="STK-TimePosVel") { const auto views=window.findChildren<QPlainTextEdit *>("report:Export"); auto *view=views.isEmpty() ? nullptr : views.last(); require(view && view->toPlainText().contains(format=="CCSDS-OEM" ? "CCSDS_OEM_VERS" : "stk.v."),"Text ephemeris did not open in Output viewer"); }
         else { const auto views=window.findChildren<QWidget *>("ephemerisFileDetails"); auto *view=views.isEmpty() ? nullptr : views.last(); require(view && view->findChild<QLabel *>("ephemerisFileInfo")->text().contains(exported),"Binary export details missing"); view->findChild<QPushButton *>("ephemerisCopyPath")->click(); require(QApplication::clipboard()->text()==exported,"Binary export copy path incorrect"); }
         const auto current=editor->toPlainText();
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Export"),[](const auto &) { return QString(); },&owner,current);
            auto *table=panel.findChild<QTableWidget *>();
            auto combo=[table](const QString &name) { for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return qobject_cast<QComboBox *>(table->cellWidget(row,1)); return static_cast<QComboBox *>(nullptr); };
            auto *epochs=combo("EpochFormat"),*start=combo("InitialEpoch"),*end=combo("FinalEpoch"),*type=combo("FileFormat"),*step=combo("StepSize");
            require(start && end && epochs && type && step,"Generic export controls missing");
            const auto initial=start->currentText(),final=end->currentText(); epochs->setCurrentText("UTCGregorian"); require(start->currentText().contains("12:01:00") && end->currentText().contains("12:09:00"),"Generic epoch controls changed the interval");
            end->setCurrentText("bad date"); epochs->setCurrentText("TAIGregorian"); require(epochs->currentText()=="UTCGregorian" && start->currentText().contains("12:01:00") && end->currentText()=="bad date","Generic epoch conversion failed rollback");
            end->setCurrentText("01 Jan 2000 12:09:00.000"); epochs->setCurrentText("TAIGregorian"); require(start->currentText()==initial && end->currentText()==final && !panel.hasChanges(),"Generic paired epoch round trip drifted");
            type->setCurrentText("SPK"); require(!step->isEnabled(),"Generic SPK sampling enabled"); type->setCurrentText("Code-500"); require(step->isEnabled() && step->currentText()=="60" && combo("OutputFormat")->isEnabled(),"Generic Code-500 sampling/byte-order dependencies missing");
            require(!combo("DistanceUnit")->isEnabled() && !combo("IncludeEventBoundaries")->isEnabled(),"Generic inactive STK controls enabled");
         }
         for (const auto &bad:QList<QMap<QString,QString>>{{{"Filename",files.filePath("missing/out.eph")}},{{"Spacecraft","Earth"}},{{"CoordinateSystem","Sat"}},{{"StepSize","0"}},{{"InterpolationOrder","11"}},{{"InitialEpoch","02 Jan 2000 12:00:00.000"},{"FinalEpoch","01 Jan 2000 12:01:32.000"}},{{"FileFormat","CK"}}}) {
            const auto rejected=window.applyResourceChanges("Export",bad,current);
            require(!rejected.isEmpty() && editor->toPlainText()==current,qPrintable("Invalid export settings failed rollback: "+bad.firstKey()+" = "+bad.first()+"; rejection = "+rejected+"; source unchanged = "+QString::number(editor->toPlainText()==current)));
         }
         if (format=="SPK") require(!window.applyResourceChanges("Export",{{"CoordinateSystem","EarthFixed"}},current).isEmpty() && editor->toPlainText()==current,"SPK accepted a body-fixed frame");
         const auto blocked=files.filePath("blocked"+QString(suffix)); require(QDir().mkpath(blocked),"Blocked export fixture unavailable");
         require(!window.applyResourceChanges("Export",{{"Filename",blocked}},current).isEmpty() && editor->toPlainText()==current,"Directory filename was not rejected before Apply");
         require(QDir().rmdir(blocked),"Cannot prepare changed output target"); error=window.applyResourceChanges("Export",{{"Filename",blocked}},current); require(error.isEmpty(),qPrintable(error));
         require(QDir().mkpath(blocked),"Cannot replace output target with directory"); const auto blockedResult=window.runMission();
         require(blockedResult==MainWindow::RunResult::Failed,qPrintable("Export to a directory unexpectedly succeeded: "+diagnostics(window))); require(QFileInfo(blocked).isDir(),"Failed export removed/replaced a directory");
         error=window.applyResourceChanges("Export",{{"Filename",exported}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); run(window); same(numericRows(report),expected);
         const QString reader=format=="Code-500" ? "Code500" : format=="STK-TimePosVel" ? "STK" : format;
         const auto readReport=files.filePath(mode+"-read.txt");
         const QString replay="Create Spacecraft Replay;\nReplay.DateFormat = UTCGregorian;\nReplay.Epoch = '01 Jan 2000 12:01:00.000';\nReplay.NAIFId = -100055;\nCreate Propagator Read;\nRead.Type = "+QString(reader)+";\nRead.StepSize = 20;\nRead.StartEpoch = 'FromSpacecraft';\n"+(format=="SPK" ? "Replay.OrbitSpiceKernelName = {'"+exported+"'};\n" : QString())+"Create ReportFile ReadValues;\nReadValues.Filename = '"+readReport+"';\nReadValues.Precision = 16;\nBeginMissionSequence;\nPropagate Read(Replay) {Replay.ElapsedSecs = 240};\nReport ReadValues Replay.EarthMJ2000Eq.X Replay.EarthMJ2000Eq.Y Replay.EarthMJ2000Eq.Z Replay.EarthMJ2000Eq.VX Replay.EarthMJ2000Eq.VY Replay.EarthMJ2000Eq.VZ;\n";
         editor->setPlainText(replay); build(window); error=window.applyResourceChanges("Read",{{"StepSize","10"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error));
         if (format!="SPK") {
            QWidget owner; const auto inputSource=editor->toPlainText(); auto *sat=Moderator::Instance()->GetConfiguredObject("Replay");
            ResourceEditor panel(*sat,[&](const auto &changes) { error=window.applyResourceChanges("Replay",changes,inputSource); return error; },&owner,inputSource);
            auto *browse=panel.findChild<QPushButton *>("chooseProperty_EphemerisName"); require(browse,"First ephemeris input-file chooser missing");
            later(&panel,failure,[&] { auto *picker=panel.findChild<QFileDialog *>("resourceFileDialog"); require(picker && picker->fileMode()==QFileDialog::ExistingFile,"Ephemeris chooser does not require an input file"); picker->reject(); }); browse->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Input-file Cancel changed pending values");
            later(&panel,failure,[&] { auto *picker=panel.findChild<QFileDialog *>("resourceFileDialog"); picker->selectFile(exported); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); }); browse->click(); if (failure) std::rethrow_exception(failure);
            require(panel.hasChanges() && sat->GetStringParameter("EphemerisName").empty(),"Input chooser changed the engine before Apply"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved),"Ephemeris reader GUI step/input round trip failed"); build(window); run(window); const auto read=numericRows(readReport); require(read.size()==1,"Readback report incomplete"); circle(read[0],300,mu,2e-4,2e-7);
         // FromSpacecraft intentionally clamps epochs before file coverage to
         // the first ephemeris epoch; preserve that existing engine behavior.
         error=window.applyResourceChanges("Replay",{{"Epoch","01 Jan 2000 12:00:00.000"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); run(window); same(numericRows(readReport),read);
         error=window.applyResourceChanges("Replay",{{"Epoch","01 Jan 2000 12:11:00.000"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error));
         const auto outsideResult=window.runMission(); require(outsideResult==MainWindow::RunResult::Failed,qPrintable("Ephemeris reader accepted an epoch after the exported interval: "+diagnostics(window)));
         require(window.loadScript(saved),"Coverage failure recovery reopen failed"); build(window); run(window); same(numericRows(readReport),read);
         const auto hidden=exported+".hidden"; require(QFile::rename(exported,hidden),"Cannot hide readback file"); require(window.runMission()==MainWindow::RunResult::Failed,"Missing ephemeris file did not fail"); require(QFile::rename(hidden,exported),"Cannot restore readback file"); require(window.loadScript(saved),"Readback recovery reopen failed"); build(window); run(window); same(numericRows(readReport),read);
         std::cout<<"PASS: "<<mode.toStdString()<<" grouped/generic dependencies, browse/pending/Cancel, paired epoch conversion/rollback, compact layout, Undo/Redo/Unicode round trips, unchanged mission report, Output access, invalid settings, directory preservation/recovery, plugin readback/input chooser/analytic circle, coverage and missing-file recovery\n";
      }
   } catch (BaseException &error) { std::cerr<<"FAIL "<<mode.toStdString()<<": "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL "<<mode.toStdString()<<": "<<error.what()<<'\n'; return 1; }
   return 0;
}
