#include "MainWindow.hpp"
#include "GroundStationDialog.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "Moderator.hpp"
#include "BodyFixedPoint.hpp"
#include "BodyFixedStateConverter.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "RgbColor.hpp"
#include "BaseException.hpp"
#include "A1Mjd.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) { std::cerr<<"CHECK FAILED: "<<message<<'\n'; throw std::runtime_error(message); } }
static void later(QObject *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] {
      try { action(); } catch (...) { if (!failure) failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); if (auto *dialog=qobject_cast<QDialog *>(owner)) dialog->reject(); }
   });
}
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read station fixture/report"); return QString::fromUtf8(file.readAll()); }
static BodyFixedPoint *station() { return dynamic_cast<BodyFixedPoint *>(Moderator::Instance()->GetConfiguredObject("Site")); }
static void close(QDialog &dialog,bool accepted=true) { dialog.findChild<QDialogButtonBox *>()->button(accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static void compareContacts(const QString &actual,const QString &expected)
{
   // UTC endpoints are printed to milliseconds; event-root convergence can
   // change the last microsecond of a duration after a coordinate round trip.
   const QRegularExpression row("^(\\d{2} [A-Za-z]{3} \\d{4} [\\d:.]+)\\s+(\\d{2} [A-Za-z]{3} \\d{4} [\\d:.]+)\\s+([\\d.]+)\\s*$",QRegularExpression::MultilineOption);
   auto left=row.globalMatch(actual),right=row.globalMatch(expected); int count=0;
   while (left.hasNext() && right.hasNext()) {
      const auto a=left.next(),b=right.next(); ++count;
      require(a.captured(1)==b.captured(1) && a.captured(2)==b.captured(2),"GUI station changed contact endpoints");
      require(std::abs(a.captured(3).toDouble()-b.captured(3).toDouble())<1e-5,"GUI station changed contact duration by more than ten microseconds");
   }
   require(count>0 && !left.hasNext() && !right.hasNext(),"GUI station changed contact count");
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtStations");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Station output directory unavailable");
      const auto contact=files.filePath("contacts.txt"),saved=files.filePath("station.script"),mask=files.filePath("horizon mask.txt");
      require(QFile::copy("../samples/SupportFiles/Ex_Contact_Location_Station_Mask.txt",mask),"Station mask fixture missing");
      MainWindow window; window.show(); require(window.initialize(startup),"Station runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString mission="BeginMissionSequence;\nPropagate StationProp(Sat) {Sat.ElapsedSecs = 86400}; % preserve contact mission\n";
      const QString base="Create Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2015 11:59:28.000';\nSat.DisplayStateType = Keplerian;\n"
         "Sat.SMA = 7191.938817629017;\nSat.ECC = .02454974900598101;\nSat.INC = 56.7;\nSat.RAAN = 306.6148021947984;\nSat.AOP = 314.1905515359922;\nSat.TA = 99.88774933204861;\n"
         "Create GroundStation Site;\nSite.StateType = Spherical;\nSite.HorizonReference = Ellipsoid;\nSite.Location1 = 28.5383355;\nSite.Location2 = 278.6207635;\nSite.Location3 = 0;\n"
         "Create ForceModel StationFM;\nCreate Propagator StationProp;\nStationProp.FM = StationFM;\n"
         "Create ContactLocator Contacts;\nContacts.Target = Sat;\nContacts.Observers = {Site};\nContacts.StepSize = 60;\nContacts.UseLightTimeDelay = false;\nContacts.UseStellarAberration = false;\nContacts.Filename = '"+contact+"';\n"+mission;
      editor->setPlainText(base); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Station reference mission failed");
      const auto reference=read(contact); std::cout<<"REFERENCE CONTACT REPORT\n"<<reference.toStdString()<<'\n';
      require(reference.contains("Site") && reference.contains("Number of events : 5"),"Reference contact report incomplete");
      require(window.buildScript(),"Station configuration reset failed");
      const auto position=station()->GetBodyFixedLocation(A1Mjd(21545));
      QString applyError="Apply not invoked"; const auto source=editor->toPlainText();
      {
         QWidget owner; ResourceEditor panel(*station(),[&](const auto &changes) { applyError=window.applyResourceChanges("Site",changes,source); return applyError; },&owner,source);
         auto *button=panel.findChild<QPushButton *>("editGroundStation"); require(button,"Ground-station workflow button missing"); std::exception_ptr failure;
         bool cancelled=false,accepted=false;
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("groundStationDialog"); require(dialog,"Station dialog did not open"); dialog->findChild<QLineEdit *>("station_Id")->setText("Cancelled"); close(*dialog,false); cancelled=true; }); button->click(); if (failure) std::rethrow_exception(failure);
         require(cancelled,"Station dialog construction failed before Cancel");
         require(!panel.hasChanges() && editor->toPlainText()==source,"Station Cancel leaked edits");
         later(&panel,failure,[&] {
            auto *dialog=panel.findChild<QDialog *>("groundStationDialog"); require(dialog->findChildren<QGroupBox *>().size()==4,"Station wx groups missing");
            auto *type=dialog->findChild<QComboBox *>("station_StateType"),*horizon=dialog->findChild<QComboBox *>("station_HorizonReference"),*body=dialog->findChild<QComboBox *>("station_CentralBody");
            require(type->currentText()=="Spherical" && horizon->currentText()=="Ellipsoid" && body->findText("Mars")>=0,"Station typed selectors missing");
            auto *latitude=dialog->findChild<QLineEdit *>("station_Location1"); latitude->setText("95"); type->setCurrentText("Cartesian"); require(type->currentText()=="Spherical" && latitude->text()=="95" && !dialog->findChild<QLabel *>("stationError")->text().isEmpty(),"Invalid latitude did not recover selector/input");
            latitude->setText("28.5383355"); auto *elevation=dialog->findChild<QLineEdit *>("station_MinimumElevationAngle"); elevation->setText("91"); close(*dialog); require(dialog->isVisible(),"Out-of-range minimum elevation accepted"); elevation->setText("nan"); close(*dialog); require(dialog->isVisible(),"Nonfinite elevation accepted"); elevation->setText("7");
            type->setCurrentText("Cartesian"); require(dialog->findChild<QLabel *>("stationLabel_Location1")->text()=="X" && dialog->findChild<QLabel *>("stationUnit_Location1")->text()=="km" && !horizon->isEnabled(),"Cartesian station labels/units/horizon incorrect");
            for (int i=0;i<3;++i) require(std::abs(dialog->findChild<QLineEdit *>("station_Location"+QString::number(i+1))->text().toDouble()-position[i])<1e-9,"Cartesian preview changed physical station position");
            type->setCurrentText("Spherical"); horizon->setCurrentText("Sphere"); require(std::abs(latitude->text().toDouble()-28.5383355)>.01,"Horizon change did not convert station latitude"); horizon->setCurrentText("Ellipsoid");
            require(std::abs(latitude->text().toDouble()-28.5383355)<1e-8,"Horizon round trip changed station latitude");
            body->setCurrentText("Mars"); require(std::abs(latitude->text().toDouble()-28.5383355)<1e-8,"Changing station body altered numeric location");
            auto *mars=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Mars");
            const auto marsPosition=BodyFixedStateConverterUtil::Convert(Rvector3(28.5383355*std::acos(-1.0)/180,278.6207635*std::acos(-1.0)/180,0),"Spherical","Ellipsoid","Cartesian","Ellipsoid",mars->GetRealParameter(mars->GetParameterID("Flattening")),mars->GetRealParameter(mars->GetParameterID("EquatorialRadius")));
            type->setCurrentText("Cartesian");
            for (int i=0;i<3;++i) require(std::abs(dialog->findChild<QLineEdit *>("station_Location"+QString::number(i+1))->text().toDouble()-marsPosition[i])<1e-8,"Station conversion used Earth's radius/flattening for Mars");
            type->setCurrentText("Spherical"); body->setCurrentText("Earth");
            auto *file=dialog->findChild<QLineEdit *>("station_HorizonMaskFileName"); file->setText(files.filePath("missing mask.txt")); close(*dialog); require(dialog->isVisible(),"Missing horizon mask accepted");
            later(dialog,failure,[&] { dialog->findChild<QFileDialog *>("stationMaskDialog")->reject(); }); dialog->findChild<QPushButton *>("stationBrowse_Mask")->click(); require(file->text().endsWith("missing mask.txt"),"Mask chooser Cancel changed path");
            later(dialog,failure,[&] { auto *picker=dialog->findChild<QFileDialog *>("stationMaskDialog"); require(picker->fileMode()==QFileDialog::ExistingFile,"Mask picker is not an input picker"); picker->selectFile(mask); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); }); dialog->findChild<QPushButton *>("stationBrowse_Mask")->click(); require(file->text()==mask,"Mask picker lost chosen file");
            dialog->findChild<QPushButton *>("stationClear_Mask")->click(); require(file->text().isEmpty(),"Horizon mask clear failed");
            auto *color=dialog->findChild<QLineEdit *>("station_OrbitColor"); const auto previousColor=color->text();
            later(dialog,failure,[&] { auto *picker=dialog->findChild<QColorDialog *>("stationColorDialog"); require(picker->currentColor()==QColor(216,191,216),"Station picker lost named Thistle color"); picker->reject(); }); dialog->findChild<QPushButton *>("stationChoose_OrbitColor")->click(); require(color->text()==previousColor,"Color Cancel changed station color");
            later(dialog,failure,[&] { auto *picker=dialog->findChild<QColorDialog *>("stationColorDialog"); picker->setCurrentColor(QColor(45,135,205)); picker->accept(); }); dialog->findChild<QPushButton *>("stationChoose_OrbitColor")->click();
            dialog->findChild<QLineEdit *>("station_Id")->setText("Orlando site"); type->setCurrentText("Cartesian");
            const auto normalSize=dialog->size(); dialog->resize(600,440); app.processEvents();
            auto *scroll=dialog->findChild<QScrollArea *>("stationScroll"); require(scroll && scroll->verticalScrollBar()->maximum()>0,"Compact station dialog cannot scroll");
            auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(QRect(ok->mapTo(dialog,QPoint()),ok->size())),"Compact station OK button outside dialog"); dialog->resize(normalSize); app.processEvents();
            if (!capture.isEmpty()) require(dialog->grab().save(capture),"Station capture failed");
            close(*dialog); require(dialog->result()==QDialog::Accepted,"Station controls did not validate"); accepted=true;
         }); button->click(); if (failure) std::rethrow_exception(failure);
         require(accepted,"Station dialog construction failed before OK");
         require(panel.hasChanges(),"Station OK did not retain pending changes");
         require(editor->toPlainText()==source,"Station OK changed script before Apply");
         require(station()->GetStringParameter("StateType")=="Spherical","Station OK changed configured state before Apply");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("groundStationDialog"); require(dialog->findChild<QComboBox *>("station_StateType")->currentText()=="Cartesian" && dialog->findChild<QLineEdit *>("station_Id")->text()=="Orlando site","Station pending values lost on reopen"); close(*dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      }
      require(applyError.isEmpty(),qPrintable(applyError)); const auto applied=editor->toPlainText();
      require(applied.mid(applied.indexOf("BeginMissionSequence;"))==mission,"Station Apply rewrote mission commands/comments");
      require(station()->GetStringParameter("StateType")=="Cartesian" && station()->GetStringParameter("Id")=="Orlando site","Grouped state/identity Apply failed");
      require(RgbColor::ToIntColor(station()->GetStringParameter("OrbitColor"))==((45u<<16)|(135u<<8)|205u),"Station color did not reach configured resource");
      for (int i=0;i<3;++i) require(std::abs(station()->GetBodyFixedLocation(A1Mjd(21545))[i]-position[i])<1e-8,"Grouped station state lost physical location");
      editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Station Undo not atomic"); editor->redo(); require(editor->toPlainText()==applied && window.buildScript(),"Station Redo did not restore settings");
      require(window.saveScriptTo(saved) && read(saved)==applied && window.loadScript(saved) && window.buildScript(),"Station save/reopen changed source");
      require(window.runMission()==MainWindow::RunResult::Completed,"GUI-configured station contact execution failed"); const auto actual=read(contact); std::cout<<"GUI CONTACT REPORT\n"<<actual.toStdString()<<'\n';
      compareContacts(actual,reference);
      bool fileMetadata=false; for (const auto &property:resourceProperties(*station())) if (property.name=="HorizonMaskFileName") fileMetadata=property.filename && property.fileInput && !property.fileOutput;
      require(fileMetadata,"Station mask lacks input-file metadata");
      const auto stable=editor->toPlainText(); require(!window.applyResourceChanges("Site",{{"CentralBody","MissingBody"}},stable).isEmpty() && editor->toPlainText()==stable,"Unknown station body bypassed rollback");
      require(!window.applyResourceChanges("Site",{{"StateType","Spherical"},{"Location1","95"}},stable).isEmpty() && editor->toPlainText()==stable,"Invalid spherical latitude bypassed grouped Apply rollback");
      auto editStation=[&](const std::function<void(QDialog &)> &change) {
         const auto before=editor->toPlainText(); QString error="Apply not invoked"; QWidget owner;
         ResourceEditor panel(*station(),[&](const auto &values) { error=window.applyResourceChanges("Site",values,before); return error; },&owner,before);
         std::exception_ptr failure; bool opened=false;
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("groundStationDialog"); require(dialog,"Station dialog unavailable"); change(*dialog); close(*dialog); require(dialog->result()==QDialog::Accepted,"Station edit rejected"); opened=true; });
         panel.findChild<QPushButton *>("editGroundStation")->click(); if (failure) std::rethrow_exception(failure); require(opened && panel.hasChanges(),"Station edit did not remain pending");
         require(editor->toPlainText()==before,"Station edit changed source before Apply");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      };
      auto referenceWith=[&](const QString &assignments) {
         QString direct=base; direct.insert(direct.indexOf("BeginMissionSequence;"),assignments);
         editor->setPlainText(direct); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Station altered reference failed"); return read(contact);
      };
      const auto elevationReference=referenceWith("Site.MinimumElevationAngle = 25;\n");
      require(elevationReference!=reference,"Minimum elevation did not alter contact intervals");
      editor->setPlainText(applied); require(window.buildScript(),"Station reset before elevation edit failed");
      editStation([](QDialog &dialog) { dialog.findChild<QLineEdit *>("station_MinimumElevationAngle")->setText("25"); });
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Station elevation round trip/execution failed");
      compareContacts(read(contact),elevationReference);
      const auto maskReference=referenceWith("Site.HorizonMaskFileName = '"+mask+"';\n");
      require(maskReference!=reference,"Horizon mask did not alter contact intervals");
      editor->setPlainText(applied); require(window.buildScript(),"Station reset before mask edit failed");
      editStation([&](QDialog &dialog) { dialog.findChild<QLineEdit *>("station_HorizonMaskFileName")->setText(mask); });
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Station mask round trip/execution failed"); compareContacts(read(contact),maskReference);
      require(QFile::rename(mask,mask+".held"),"Cannot isolate missing-mask fixture");
      require(window.runMission()!=MainWindow::RunResult::Completed,"Missing station mask did not fail execution");
      require(QFile::rename(mask+".held",mask),"Cannot restore station mask");
      require(window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Station mask restore/reopen did not recover"); compareContacts(read(contact),maskReference);
      editStation([](QDialog &dialog) { dialog.findChild<QPushButton *>("stationClear_Mask")->click(); });
      require(station()->GetStringParameter("HorizonMaskFileName").empty(),"Clearing a configured mask failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Station clear-mask round trip/execution failed"); compareContacts(read(contact),reference);
      std::cout<<"PASS: ground-station selectors/labels, physical-state conversions, validation/Cancel, colors/mask pickers, grouped Apply/Undo/Redo, save/reopen and contact execution\n";
      return 0;
   } catch (BaseException &failure) { std::cerr<<"Station check failed: "<<failure.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &failure) { std::cerr<<"Station check failed: "<<failure.what()<<'\n'; return 1; }
}
