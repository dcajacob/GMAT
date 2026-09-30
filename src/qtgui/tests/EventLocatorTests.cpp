#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "EventLocatorDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include "ReportViewer.hpp"
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStatusBar>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QRegularExpression>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read event fixture/report"); return QString::fromUtf8(file.readAll()); }
static void later(QObject *owner,std::exception_ptr &failure,std::function<void()> action) { QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); if (auto *dialog=qobject_cast<QDialog *>(owner)) dialog->reject(); } }); }
static void close(QDialog &dialog,bool accepted=true) { dialog.findChild<QDialogButtonBox *>()->button(accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static void select(QDialog &dialog,const QString &name,const QStringList &selected) { auto *list=dialog.findChild<QListWidget *>("event_"+name); require(list,"Typed locator list missing"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(selected.contains(list->item(i)->text()) ? Qt::Checked : Qt::Unchecked); }
static void compareContacts(const QString &actual,const QString &expected)
{
   const QRegularExpression row("^(\\d{2} [A-Za-z]{3} \\d{4} [\\d:.]+)\\s+(\\d{2} [A-Za-z]{3} \\d{4} [\\d:.]+)\\s+([\\d.]+)\\s*$",QRegularExpression::MultilineOption);
   auto left=row.globalMatch(actual),right=row.globalMatch(expected); int count=0;
   while (left.hasNext() && right.hasNext()) { const auto a=left.next(),b=right.next(); ++count; require(a.captured(1)==b.captured(1) && a.captured(2)==b.captured(2) && std::abs(a.captured(3).toDouble()-b.captured(3).toDouble())<1e-5,"GUI locator changed contact intervals"); }
   require(count>0 && !left.hasNext() && !right.hasNext(),"GUI locator changed contact count");
}
static void compareReport(const QString &actual,const QString &expected)
{
   const auto a=actual.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts),b=expected.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
   require(a.size()==b.size(),"GUI event report changed its rows/columns");
   for (int i=0;i<a.size();++i) {
      bool an=false,bn=false; const auto av=a[i].toDouble(&an),bv=b[i].toDouble(&bn);
      if (an && bn) require(std::abs(av-bv)<1e-5,"GUI event report changed numeric results beyond precision tolerance");
      else require(a[i]==b[i],"GUI event report changed times, names, types or headings");
   }
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtEventLocators"); if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Event temporary directory unavailable");
      const auto contact=files.filePath("contacts.txt"),eclipse=files.filePath("eclipse.txt"),saved=files.filePath("events.script");
      MainWindow window; window.show(); require(window.initialize(startup),"Event runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 86400}; % retain event mission\n";
      const QString common="Create Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2015 11:59:28.000';\nSat.DisplayStateType = Keplerian;\nSat.SMA = 7191.938817629017;\nSat.ECC = .02454974900598101;\nSat.INC = 56.7;\nSat.RAAN = 306.6148021947984;\nSat.AOP = 314.1905515359922;\nSat.TA = 99.88774933204861;\nCreate GroundStation Site;\nSite.StateType = Spherical;\nSite.HorizonReference = Ellipsoid;\nSite.Location1 = 28.5383355;\nSite.Location2 = 278.6207635;\nSite.Location3 = 0;\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\n";
      const QString contactBase=common+"Create ContactLocator Events;\nEvents.Target = Sat;\nEvents.Observers = {Site};\nEvents.UseLightTimeDelay = false;\nEvents.UseStellarAberration = false;\nEvents.Filename = '"+contact+"';\n"+mission;
      const QString interval="Events.UseEntireInterval = false;\nEvents.InputEpochFormat = UTCGregorian;\nEvents.InitialEpoch = '01 Jan 2015 19:00:00.000';\nEvents.FinalEpoch = '02 Jan 2015 00:00:00.000';\nEvents.StepSize = 30;\n";
      auto runScript=[&](QString source,const QString &path) { editor->setPlainText(source); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Event script reference execution failed"); return read(path); };
      auto direct=[&](const QString &base,const QString &assignments,const QString &path) { auto source=base; source.insert(source.indexOf("BeginMissionSequence"),assignments); return runScript(source,path); };
      auto openOutput=[&](bool written,const QString &captureSuffix=QString()) {
         const auto source=editor->toPlainText(); auto *tree=window.findChild<QTreeWidget *>("Output"); const auto items=tree->findItems("Events",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Event report missing from Output"); tree->itemDoubleClicked(items.first(),0); QApplication::processEvents();
         auto *area=window.findChild<QMdiArea *>("workspace"); auto *child=area->activeSubWindow(); require(child,"Event output viewer did not open"); auto *viewer=qobject_cast<QPlainTextEdit *>(child->widget()); if (!viewer) viewer=child->findChild<QPlainTextEdit *>("report:Events"); require(viewer && viewer->isReadOnly() && viewer->lineWrapMode()==QPlainTextEdit::NoWrap,qPrintable(QString("Event report is not read-only/unwrapped (written=%1, title=%2, widget=%3, name=%4, status=%5)").arg(written).arg(child->windowTitle(),child->widget()->metaObject()->className(),child->widget()->objectName(),window.statusBar()->currentMessage())));
         if (written) require(dynamic_cast<ReportViewer *>(child->widget()) && viewer->toPlainText()==read(contact),"Generated event report did not display current complete contents"); else require(viewer->toPlainText().contains("No event report was written") && viewer->toPlainText().contains("FindEvents") && !viewer->toPlainText().contains("Number of events"),"Unwritten event locator displayed a stale report");
         if (!capture.isEmpty() && !captureSuffix.isEmpty()) require(child->grab().save(capture+captureSuffix+".png"),"Event output capture failed"); child->close(); QApplication::processEvents(); require(editor->toPlainText()==source,"Event output inspection changed script");
      };
      const auto expected=direct(contactBase,interval,contact); require(expected.contains("Number of events : 2"),"Selected contact interval reference is not two events");
      openOutput(true,".generated"); editor->setPlainText(editor->toPlainText()+"% pending source comment\n"); openOutput(false); editor->setPlainText(contactBase); require(window.buildScript(),"Contact GUI fixture failed"); openOutput(false,".unwritten");
      auto disabled=contactBase; disabled.insert(disabled.indexOf("BeginMissionSequence"),"Events.RunMode = Disabled;\n"); editor->setPlainText(disabled); require(window.runMission()==MainWindow::RunResult::Completed && read(contact)==expected,"Disabled locator unexpectedly changed the prior report"); openOutput(false);
      auto noReport=contactBase; noReport.insert(noReport.indexOf("BeginMissionSequence"),"Events.WriteReport = false;\n"); editor->setPlainText(noReport); require(window.runMission()==MainWindow::RunResult::Completed && read(contact)==expected,"WriteReport-off locator changed the prior report"); openOutput(false);
      auto manual=contactBase; manual.insert(manual.indexOf("BeginMissionSequence"),"Events.RunMode = Manual;\n"); editor->setPlainText(manual); require(window.runMission()==MainWindow::RunResult::Completed && read(contact)==expected,"Manual locator without FindEvents unexpectedly wrote a report"); openOutput(false);
      require(window.applyMissionChange(window.missionSnapshot(),0,MissionEdit::Append,"FindEvents Events {Append = false}; % manual report recovery").isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"Manual FindEvents did not recover event output"); openOutput(true);
      editor->setPlainText(contactBase); require(window.buildScript(),"Contact GUI fixture restore failed");
      auto edit=[&](const std::function<void(QDialog &)> &action,bool accepted=true) {
         const auto source=editor->toPlainText(); QString applyError="Apply not invoked"; QWidget owner; auto *object=Moderator::Instance()->GetConfiguredObject("Events"); const auto priorStep=object->GetRealParameter("StepSize"); const auto priorEntire=object->GetBooleanParameter("UseEntireInterval");
         ResourceEditor panel(*object,[&](const auto &values) { applyError=window.applyResourceChanges("Events",values,source); return applyError; },&owner,source); std::exception_ptr failure; bool opened=false; QString pendingStep;
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("eventLocatorDialog"); require(dialog,"Event locator dialog did not open"); action(*dialog); pendingStep=dialog->findChild<QLineEdit *>("event_StepSize")->text(); close(*dialog,accepted); require(dialog->result()==(accepted ? QDialog::Accepted : QDialog::Rejected),qPrintable("Event dialog validation: "+dialog->findChild<QLabel *>("eventLocatorError")->text())); opened=true; });
         auto *button=panel.findChild<QPushButton *>("editEventLocator"); require(button,"Event locator action missing"); button->click(); if (failure) std::rethrow_exception(failure); require(opened,"Event dialog constructor failed"); require(editor->toPlainText()==source,"Event dialog changed script before Apply");
         if (!accepted) { require(!panel.hasChanges(),"Event Cancel leaked pending values"); return; }
         require(panel.hasChanges() && object==Moderator::Instance()->GetConfiguredObject("Events") && object->GetRealParameter("StepSize")==priorStep && object->GetBooleanParameter("UseEntireInterval")==priorEntire,"Event OK bypassed pending Apply");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("eventLocatorDialog"); require(dialog->findChild<QLineEdit *>("event_StepSize")->text()==pendingStep,"Pending locator step lost on reopen"); close(*dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(applyError.isEmpty(),qPrintable(applyError));
      };
      edit([](QDialog &dialog) { dialog.findChild<QLineEdit *>("event_StepSize")->setText("120"); },false);
      const auto original=editor->toPlainText();
      edit([&](QDialog &dialog) {
         auto *format=dialog.findChild<QComboBox *>("event_InputEpochFormat"),*target=dialog.findChild<QComboBox *>("event_Target"); require(target && target->findText("Sat")>=0 && target->findText("Earth")==-1,"Locator target selector is not typed");
         require(dialog.findChild<QListWidget *>("event_Observers")->findItems("Sat",Qt::MatchExactly).isEmpty(),"Contact target offered as its own observer");
         auto *entire=dialog.findChild<QCheckBox *>("event_UseEntireInterval"),*light=dialog.findChild<QCheckBox *>("event_UseLightTimeDelay"),*aberration=dialog.findChild<QCheckBox *>("event_UseStellarAberration");
         auto *initial=dialog.findChild<QLineEdit *>("event_InitialEpoch"),*final=dialog.findChild<QLineEdit *>("event_FinalEpoch"); require(!format->isEnabled() && !initial->isEnabled(),"Entire-interval controls not disabled"); entire->setChecked(false); format->setCurrentText("UTCGregorian"); initial->setText("01 Jan 2015 19:00:00.000"); final->setText("02 Jan 2015 00:00:00.000");
         const auto initialUtc=initial->text(),finalUtc=final->text(); final->setText("invalid date"); format->setCurrentText("TAIModJulian"); require(format->currentText()=="UTCGregorian" && initial->text()==initialUtc && final->text()=="invalid date" && !dialog.findChild<QLabel *>("eventLocatorError")->text().isEmpty(),"Failed paired conversion changed one epoch"); final->setText(finalUtc);
         format->setCurrentText("TAIModJulian"); require(initial->text().toDouble()>27000 && final->text().toDouble()>initial->text().toDouble(),"Event epochs did not convert"); format->setCurrentText("UTCGregorian"); require(initial->text()==initialUtc && final->text()==finalUtc,"Event epoch round trip changed dates");
         final->setText("01 Jan 2015 18:00:00.000"); close(dialog); require(dialog.isVisible(),"Reversed event interval accepted"); final->setText(finalUtc);
         auto *step=dialog.findChild<QLineEdit *>("event_StepSize"); step->setText("-1"); close(dialog); require(dialog.isVisible(),"Negative event step accepted"); step->setText("nan"); close(dialog); require(dialog.isVisible(),"Nonfinite event step accepted"); step->setText("30");
         light->setChecked(true); aberration->setChecked(true); light->setChecked(false); require(!aberration->isChecked() && !aberration->isEnabled() && !dialog.findChild<QComboBox *>("event_LightTimeDirection")->isEnabled(),"Light-time disable did not clear dependent aberration");
         select(dialog,"OccultingBodies",{"Earth"}); select(dialog,"Observers",{"Site"});
         auto *file=dialog.findChild<QLineEdit *>("event_Filename"); const auto before=file->text();
         std::exception_ptr pickerFailure;
         later(&dialog,pickerFailure,[&] { dialog.findChild<QFileDialog *>("eventFileDialog")->reject(); }); dialog.findChild<QPushButton *>("eventBrowse_Filename")->click(); if (pickerFailure) std::rethrow_exception(pickerFailure); require(file->text()==before,"Report chooser Cancel changed path");
         const auto picked=files.filePath("chosen events.txt");
         later(&dialog,pickerFailure,[&] { auto *picker=dialog.findChild<QFileDialog *>("eventFileDialog"); require(picker->acceptMode()==QFileDialog::AcceptSave && picker->fileMode()==QFileDialog::AnyFile,"Event report picker is not an output picker"); picker->selectFile(picked); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); }); dialog.findChild<QPushButton *>("eventBrowse_Filename")->click(); if (pickerFailure) std::rethrow_exception(pickerFailure); require(file->text()==picked,"Report chooser lost selection"); file->setText(contact);
         const auto normal=dialog.size(); dialog.resize(600,440); app.processEvents(); auto *scroll=dialog.findChild<QScrollArea *>("eventLocatorScroll"); require(scroll && scroll->verticalScrollBar()->maximum()>0,"Compact event dialog cannot scroll"); auto *ok=dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog.rect().contains(QRect(ok->mapTo(&dialog,QPoint()),ok->size())),"Event OK outside compact window"); dialog.resize(normal); app.processEvents(); if (!capture.isEmpty()) require(dialog.grab().save(capture),"Event capture failed");
      });
      const auto applied=editor->toPlainText(); require(applied.mid(applied.indexOf("BeginMissionSequence;"))==mission,"Event Apply rewrote mission comments");
      editor->undo(); require(editor->toPlainText()==original && window.buildScript(),"Event Undo not atomic"); editor->redo(); require(editor->toPlainText()==applied && window.buildScript(),"Event Redo failed");
      auto roundTrip=[&] { const auto source=editor->toPlainText(); require(window.saveScriptTo(saved) && read(saved)==source && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==source,"Event save/reopen changed source"); };
      roundTrip(); require(window.runMission()==MainWindow::RunResult::Completed,"GUI contact locator execution failed"); compareContacts(read(contact),expected); openOutput(true);
      {
         const auto before=editor->toPlainText(); EventLocatorDialog pending(*Moderator::Instance()->GetConfiguredObject("Events"),{{"InputEpochFormat","TAIModJulian"}});
         require(pending.findChild<QLineEdit *>("event_InitialEpoch")->text().toDouble()>27000 && pending.findChild<QLineEdit *>("event_FinalEpoch")->text().toDouble()>27000,"Pending format-only edit left epochs in old representation"); require(editor->toPlainText()==before,"Pending format preview changed script");
      }
      require(window.applyResourceChanges("Events",{{"InputEpochFormat","TAIModJulian"}},editor->toPlainText()).isEmpty(),"Format-only event Apply failed"); roundTrip(); require(window.runMission()==MainWindow::RunResult::Completed,"Format-only event execution failed"); compareContacts(read(contact),expected);
      const auto stable=editor->toPlainText(); require(!window.applyResourceChanges("Events",{{"Target","Earth"}},stable).isEmpty() && editor->toPlainText()==stable,"Wrong locator target type bypassed rollback"); require(!window.applyResourceChanges("Events",{{"Observers","Earth"}},stable).isEmpty() && editor->toPlainText()==stable,"Wrong observer type bypassed rollback");
      require(!window.applyResourceChanges("Events",{{"Observers","Sat"}},stable).isEmpty() && editor->toPlainText()==stable,"Contact target accepted as its own observer");
      auto intervalControls=[](QDialog &dialog) { dialog.findChild<QCheckBox *>("event_UseEntireInterval")->setChecked(false); dialog.findChild<QComboBox *>("event_InputEpochFormat")->setCurrentText("UTCGregorian"); dialog.findChild<QLineEdit *>("event_InitialEpoch")->setText("01 Jan 2015 19:00:00.000"); dialog.findChild<QLineEdit *>("event_FinalEpoch")->setText("02 Jan 2015 00:00:00.000"); dialog.findChild<QLineEdit *>("event_StepSize")->setText("30"); };
      for (const auto &direction:QStringList{"Transmit","Receive"}) {
         const auto lightReference=direct(contactBase,interval+"Events.UseLightTimeDelay = true;\nEvents.UseStellarAberration = true;\nEvents.LightTimeDirection = "+direction+";\n",contact);
         require(lightReference!=expected,"Light-time correction did not alter contacts"); editor->setPlainText(contactBase); require(window.buildScript(),"Light-time GUI fixture failed");
         edit([&](QDialog &dialog) { intervalControls(dialog); dialog.findChild<QCheckBox *>("event_UseLightTimeDelay")->setChecked(true); dialog.findChild<QCheckBox *>("event_UseStellarAberration")->setChecked(true); dialog.findChild<QComboBox *>("event_LightTimeDirection")->setCurrentText(direction); }); roundTrip(); require(window.runMission()==MainWindow::RunResult::Completed,"GUI light-time locator execution failed"); compareContacts(read(contact),lightReference);
      }
      const auto recoveryReference=read(contact),unavailable=files.filePath("missing/report.txt");
      edit([&](QDialog &dialog) { dialog.findChild<QLineEdit *>("event_Filename")->setText(unavailable); }); roundTrip();
      require(window.runMission()!=MainWindow::RunResult::Completed,"Unavailable event output directory did not fail execution");
      require(QDir().mkpath(QFileInfo(unavailable).absolutePath()),"Cannot restore event output directory"); require(window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Event output restore/reopen failed"); compareContacts(read(unavailable),recoveryReference);
      for (const auto &reportFormat:QStringList{"SiteViewMaxElevationReport","AzimuthElevationRangeReport"}) {
         const auto reportReference=direct(contactBase,interval+"Events.ReportFormat = "+reportFormat+";\nEvents.ReportTimeFormat = ISOYD;\nEvents.ReportPrecision = 8;\nEvents.IntervalStepSize = 60;\n",contact);
         require(reportReference!=expected,"Detailed contact report did not change output"); editor->setPlainText(contactBase); require(window.buildScript(),"Contact report GUI fixture failed");
         edit([&](QDialog &dialog) { intervalControls(dialog); dialog.findChild<QComboBox *>("event_ReportFormat")->setCurrentText(reportFormat); dialog.findChild<QComboBox *>("event_ReportTimeFormat")->setCurrentText("ISOYD"); dialog.findChild<QLineEdit *>("event_ReportPrecision")->setText("8"); auto *intervalStep=dialog.findChild<QLineEdit *>("event_IntervalStepSize"); require(intervalStep->isEnabled()==reportFormat.startsWith("Azimuth"),"Report interval dependency incorrect"); if (intervalStep->isEnabled()) { close(dialog); require(dialog.isVisible(),"Azimuth report accepted zero interval step"); } intervalStep->setText("60"); }); roundTrip(); require(window.runMission()==MainWindow::RunResult::Completed,"GUI detailed contact report failed"); compareReport(read(contact),reportReference);
      }
      const QString eclipseBase=common+"Create EclipseLocator Events;\nEvents.Spacecraft = Sat;\nEvents.OccultingBodies = {Earth};\nEvents.UseLightTimeDelay = false;\nEvents.UseStellarAberration = false;\nEvents.Filename = '"+eclipse+"';\n"+mission;
      const auto eclipseReference=direct(eclipseBase,"Events.StepSize = 30;\nEvents.EclipseTypes = {Umbra, Penumbra};\n",eclipse); require(eclipseReference.contains("Umbra") && eclipseReference.contains("Penumbra"),"Eclipse reference lacks expected shadow types"); editor->setPlainText(eclipseBase); require(window.buildScript(),"Eclipse GUI fixture failed");
      edit([](QDialog &dialog) { require(!dialog.findChild<QComboBox *>("event_LightTimeDirection"),"Eclipse dialog exposed contact-only field"); select(dialog,"EclipseTypes",{"Umbra","Penumbra"}); select(dialog,"OccultingBodies",{"Earth"}); dialog.findChild<QLineEdit *>("event_StepSize")->setText("30"); }); roundTrip(); require(window.runMission()==MainWindow::RunResult::Completed,"GUI eclipse locator execution failed");
      compareReport(read(eclipse),eclipseReference);
      const auto intrusion=files.filePath("intrusion.txt");
      QString intrusionBase=read("../samples/Ex_IntrusionLocator_Mercury_Sun_Transit.script"); intrusionBase.replace(QRegularExpression("\\bIL\\b"),"Events"); intrusionBase.replace("Ex_IntrusionLocator_Mercury_Sun_Transit.txt",intrusion); intrusionBase.insert(intrusionBase.indexOf("BeginMissionSequence"),"Create Spacecraft Other;\n");
      const auto intrusionReference=direct(intrusionBase,"Events.StepSize = 120;\n",intrusion); require(intrusionReference.contains("Mercury") && !intrusionReference.contains("No intrusion events"),"Shipped transit reference found no Mercury intrusion"); editor->setPlainText(intrusionBase); require(window.buildScript(),"Intrusion GUI fixture failed");
      edit([&](QDialog &dialog) {
         auto *sensors=dialog.findChild<QListWidget *>("event_Sensors"),*bodies=dialog.findChild<QListWidget *>("event_IntrudingBodies"); require(sensors && sensors->findItems("Sensor1",Qt::MatchExactly).size()==1 && sensors->findItems("Watcher",Qt::MatchExactly).isEmpty(),"Intrusion sensor selector is not typed"); require(bodies->findItems("Watcher",Qt::MatchExactly).isEmpty(),"Intrusion target offered as intruder");
         auto *target=dialog.findChild<QComboBox *>("event_Spacecraft"); target->setCurrentText("Other"); require(bodies->findItems("Watcher",Qt::MatchExactly).size()==1 && bodies->findItems("Other",Qt::MatchExactly).isEmpty(),"Intrusion target switch did not update body exclusions"); target->setCurrentText("Watcher");
         select(dialog,"Sensors",{"Sensor1"}); select(dialog,"IntrudingBodies",{"Mercury"}); auto *phase=dialog.findChild<QLineEdit *>("event_MinimumPhase"); phase->setText("1.1"); close(dialog); require(dialog.isVisible(),"Invalid intrusion phase accepted"); phase->setText("0");
         auto *coordinates=dialog.findChild<QComboBox *>("event_ReportCoordinates"); auto *grid=dialog.findChild<QLineEdit *>("event_SpiceGridFrameFile"); require(!grid->isEnabled(),"Sensor-frame mode enabled grid file"); coordinates->setCurrentText("FixedGrid"); require(grid->isEnabled() && dialog.findChild<QPushButton *>("eventBrowse_SpiceGridFrameFile")->isEnabled(),"Fixed-grid controls not enabled"); grid->setText(files.filePath("missing grid.tf")); close(dialog); require(dialog.isVisible(),"Missing fixed-grid file accepted"); coordinates->setCurrentText("SensorFrame"); grid->clear(); dialog.findChild<QLineEdit *>("event_StepSize")->setText("120");
      }); roundTrip(); require(window.runMission()==MainWindow::RunResult::Completed,"GUI-configured shipped Mercury intrusion failed"); compareReport(read(intrusion),intrusionReference);
      std::cout<<"PASS: event-locator typed controls, paired epochs and rollback, light-time/report dependencies, pending Apply/Cancel/Undo/Redo, file picker, compact layout, save/reopen, bounded contacts, Transmit/Receive corrections, detailed reports, failed-output recovery, generated Output viewer and rebuilt/edited/Disabled/WriteReport-off/Manual stale-report guards, eclipse intervals and shipped Mercury intrusion\n";
      return 0;
   } catch (BaseException &failure) { std::cerr<<"Event check failed: "<<failure.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &failure) { std::cerr<<"Event check failed: "<<failure.what()<<'\n'; return 1; }
}
