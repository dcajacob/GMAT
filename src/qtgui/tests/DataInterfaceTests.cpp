#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "CommandForm.hpp"
#include "ScriptCompatibility.hpp"
#include "QtPlotReceiver.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QWindow>
#include <QRegularExpression>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString diagnostics(MainWindow &window) { QApplication::processEvents(); return window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(3500); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(diagnostics(window).toStdString()); }
static QByteArray bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Fixture/report unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &data) { QFile file(path); require(file.open(QIODevice::WriteOnly|QIODevice::Truncate) && file.write(data)==data.size(),"Fixture write failed"); }
static QVector<QVector<double>> rows(const QString &path)
{
   QVector<QVector<double>> result; for (const auto &line:QString::fromUtf8(bytes(path)).trimmed().split('\n')) { QVector<double> row; for (const auto &entry:line.simplified().split(' ')) { bool ok; const double value=entry.toDouble(&ok); require(ok && std::isfinite(value),"Numeric report malformed"); row.append(value); } result.append(row); } return result;
}
static void same(const QVector<QVector<double>> &a,const QVector<QVector<double>> &b)
{
   require(a.size()==b.size(),"Report row count changed"); for (int r=0;r<a.size();++r) { require(a[r].size()==b[r].size(),"Report column count changed"); for (int c=0;c<a[r].size();++c) require(std::abs(a[r][c]-b[r][c])<1e-8,"GUI import differs from independently initialized reference"); }
}
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static int row(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); for (int r=0;r<table->rowCount();++r) if (table->item(r,0)->text()==name) return r; throw std::runtime_error("Resource field unavailable"); }
static int importNode(const MissionSnapshot &snapshot) { for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Set") return i; throw std::runtime_error("Set command missing from mission tree"); }
static bool waitUntil(const std::function<bool()> &ready) { if (ready()) return true; QEventLoop loop; QElapsedTimer elapsed; elapsed.start(); QTimer timer; timer.setInterval(10); QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if (ready() || elapsed.elapsed()>3000) loop.quit(); }); timer.start(); loop.exec(); return ready(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtDataInterface"); if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Data-interface fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Data-interface runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto original=bytes(QDir::current().absoluteFilePath("../samples/SupportFiles/Ex_FileInterface_TVHF.sv"));
      const auto before=files.filePath("before.sv"),input=files.filePath("input ü.sv"),report=files.filePath("import.txt"),saved=files.filePath("file import ü.script"); write(before,original); write(input,original);
      const QString state="Create Spacecraft Sat OtherSat;\nSat.Cr = 1.9;\n";
      const QString interfaces="Create FileInterface Reader OtherReader;\nReader.Filename = '"+input+"';\nReader.Format = 'TVHF_ASCII';\nOtherReader.Filename = '"+input+"';\nOtherReader.Format = 'TVHF_ASCII';\n";
      const QString output="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 30;\nProp.MinStep = 30;\nProp.MaxStep = 30;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nValues.WriteHeaders = false;\n";
      const QString values="Report Values Sat.A1ModJulian Sat.Cr Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const QString command="Set 'Read initial state' Sat Reader; % keep import comment";
      const QString mission="BeginMissionSequence;\n"+command+"\n"+values+"Propagate 'Imported orbit' Prop(Sat) {Sat.ElapsedSecs = 600};\n"+values;
      const QString epoch="Sat.DateFormat = UTCGregorian;\nSat.Epoch = '20 Aug 2012 00:00:00.000';\n";
      const QString cartesian="Sat.DisplayStateType = Cartesian;\nSat.X = 42164;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 3.0746662829706;\nSat.VZ = 0;\n";
      auto reference=[&](const QStringList &selected) { return state+interfaces+output+(selected.contains("Epoch") ? epoch : "")+(selected.contains("CartesianState") ? cartesian : "")+(selected.contains("Cr") ? "Sat.Cr = 1.4;\n" : "")+QString(mission).replace(command+"\n",""); };
      editor->setPlainText(reference({"Epoch","CartesianState","Cr"})); run(window); const auto expected=rows(report); require(expected.size()==2 && expected[0].size()==8 && expected[0][1]==1.4 && expected[0][2]==42164 && expected[0][6]==3.0746662829706,"Imported-file literal reference wrong");
      auto base=state+interfaces+output+mission; base.replace("\nReader.Filename = '"+input+"';","\nReader.Filename = '"+before+"';"); editor->setPlainText(base); require(window.buildScript(),"Editable import mission failed build"); const auto source=editor->toPlainText(); QString error; std::exception_ptr failure;
      {
         QWidget owner; auto *object=Moderator::Instance()->GetConfiguredObject("Reader"); ResourceEditor panel(*object,[&](const auto &changes) { error=window.applyResourceChanges("Reader",changes,source); return error; },&owner,source); auto *table=panel.findChild<QTableWidget *>(); auto *format=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"Format"),1)); require(format && format->findText("TVHF_ASCII")>=0 && !format->isEditable(),"Interface format selector incorrect"); auto *filename=table->item(row(panel,"Filename"),1); auto *choose=panel.findChild<QPushButton *>("chooseProperty_Filename"); require(choose,"Interface input-file chooser missing");
         later(&panel,failure,[&] { panel.findChild<QFileDialog *>("resourceFileDialog")->reject(); }); choose->click(); if (failure) std::rethrow_exception(failure); require(filename->text()==before && !panel.hasChanges(),"Input chooser Cancel changed resource");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QFileDialog *>("resourceFileDialog"); dialog->selectFile(input); require(QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection),"Input chooser failed selection"); }); choose->click(); if (failure) std::rethrow_exception(failure); require(filename->text()==input && object->GetStringParameter("Filename")==before.toStdString(),"Pending filename changed configured resource"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto applied=editor->toPlainText(); require(applied.endsWith(mission),"File resource edit changed command labels/comments"); editor->undo(); require(editor->toPlainText()==source,"File edit Undo not exact"); editor->redo(); require(editor->toPlainText()==applied,"File edit Redo not exact"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode file import save/reopen failed"); run(window); same(rows(report),expected);
      auto *tree=window.findChild<QTreeWidget *>("Output"); const auto reports=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(reports.size()==1 && reports[0]->data(0,Qt::UserRole).toString()==report,"Import report not available in Output"); tree->itemDoubleClicked(reports[0],0); auto *preview=window.findChild<QPlainTextEdit *>("report:Values"); require(preview && preview->isReadOnly() && !preview->toPlainText().trimmed().isEmpty(),"Import report viewer unavailable");
      const auto current=editor->toPlainText(); for (const auto &bad:QList<QMap<QString,QString>>{{{"Filename",""}},{{"Filename",files.path()}},{{"Filename",files.filePath("missing.sv")}},{{"Format","WrongFormat"}}}) require(!window.applyResourceChanges("Reader",bad,current).isEmpty() && editor->toPlainText()==current,"Invalid interface configuration changed source"); run(window); same(rows(report),expected);
      const auto allFields=dataInterfaceFields(*Moderator::Instance()->GetConfiguredObject("Reader")); require(allFields==QStringList{"Epoch","CartesianState","Cr"},"Reader fields do not match shipped format");
      auto select=[&](CommandForm &form,const QStringList &selected,bool all,bool accept) {
         later(&form,failure,[&,selected,all,accept] { auto *dialog=form.findChild<QDialog *>("dataImportDialog"); require(dialog,"Import-field selector missing"); auto *list=dialog->findChild<QListWidget *>("dataImportFields"); require(list->count()==3,"Field selector uses unrelated target properties"); auto *importAll=dialog->findChild<QCheckBox *>("dataImportAll"); importAll->setChecked(false); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked); require(!dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Empty field subset accepted"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(selected.contains(list->item(i)->text()) ? Qt::Checked : Qt::Unchecked); importAll->setChecked(all); require(list->isEnabled()!=all,"Import-all dependency incorrect"); if (!capture.isEmpty() && selected==QStringList{"Cr"} && accept) { require(waitUntil([&] { return dialog->windowHandle() && dialog->windowHandle()->isExposed(); }),"Field selector did not expose"); require(dialog->grab().save(capture+".fields.png"),"Field selector capture failed"); } close(dialog,accept); }); form.findChild<QPushButton *>("commandChoose_Data")->click(); if (failure) std::rethrow_exception(failure);
      };
      for (int mask=0;mask<8;++mask) {
         QStringList selected; for (int bit=0;bit<3;++bit) if (!mask || (mask&(1<<bit))) selected.append(allFields[bit]);
         editor->setPlainText(reference(selected)); run(window); const auto referenceRows=rows(report); require(window.loadScript(saved) && window.buildScript(),"Import control fixture restore failed");
         QString changed=command; CommandForm form([&](const QString &text) { changed=text; }); form.setStatement(command); require(form.title()=="File import" && form.findChild<QPushButton *>("commandChoose_Target") && form.findChild<QPushButton *>("commandChoose_Data source"),"Set command lacks typed controls"); select(form,selected,!mask,false); require(changed==command,"Field selector Cancel changed source"); select(form,selected,!mask,true); require(changed.endsWith("; % keep import comment"),"Selection lost label/comment");
         const auto snapshot=window.missionSnapshot(); error=window.applyMissionChange(snapshot,importNode(snapshot),MissionEdit::Replace,changed); require(error.isEmpty(),qPrintable(error)); const auto edited=editor->toPlainText(); if (mask) { editor->undo(); require(editor->toPlainText()==snapshot.sourceScript,"Set field edit Undo not exact"); editor->redo(); require(editor->toPlainText()==edited,"Set field edit Redo not exact"); }
         const auto path=files.filePath("fields-"+QString::number(mask)+".script"); require(window.saveScriptTo(path) && window.loadScript(path),"Selected-field command save/reopen failed"); run(window); same(rows(report),referenceRows);
      }
      require(window.loadScript(saved) && window.buildScript(),"Existing-options fixture restore failed");
      {
         const auto subset="Set 'Read initial state' Sat Reader (Data = {'Cr'}); % keep import comment";
         QString changed=subset; CommandForm form([&](const QString &text) { changed=text; }); form.setStatement(subset);
         select(form,{"Epoch"},false,false); require(changed==subset,"Cancel discarded an existing selection"); select(form,{},true,true); require(changed==command,"Import all did not remove existing subset");
         form.setStatement("Set Sat Reader (Data = {'All'});"); select(form,{},true,true); require(changed=="Set Sat Reader;","Explicit All did not simplify to engine default");
         const QString unknown="Set Sat Reader (Data = {'UnknownField'}); % preserve"; form.setStatement(unknown); changed=unknown;
         later(&form,failure,[&] { auto *dialog=form.findChild<QDialog *>("dataImportDialog"); auto *list=dialog->findChild<QListWidget *>("dataImportFields"); require(list->count()==4 && list->item(3)->text()=="UnknownField" && list->item(3)->checkState()==Qt::Checked && !dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Unknown field silently removed or accepted"); close(dialog,false); }); form.findChild<QPushButton *>("commandChoose_Data")->click(); if (failure) std::rethrow_exception(failure); require(changed==unknown,"Unknown-field Cancel changed source");
         const auto snapshot=window.missionSnapshot(); require(window.applyMissionChange(snapshot,importNode(snapshot),MissionEdit::Replace,unknown).isEmpty(),"Engine-supported unknown-field diagnostic fixture rejected early"); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains("UnknownField"),"Unknown import field did not fail clearly");
         require(window.loadScript(saved),"Unknown-field recovery reopen failed"); run(window); same(rows(report),expected);
      }
      require(window.loadScript(saved) && window.buildScript(),"Target/source picker fixture restore failed"); QString changed=command; CommandForm form([&](const QString &text) { changed=text; }); form.setStatement(command);
      for (const auto &entry:QList<QPair<QString,QString>>{{"Target","OtherSat"},{"Data source","OtherReader"}}) {
         auto *button=form.findChild<QPushButton *>("commandChoose_"+entry.first); later(&form,failure,[&] { auto *dialog=form.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains(entry.second) && !dialog->comboBoxItems().contains("FM") && !dialog->comboBoxItems().contains("Earth"),"Set picker includes wrong resource types"); dialog->setTextValue(entry.second); dialog->accept(); }); button->click(); if (failure) std::rethrow_exception(failure);
      }
      require(changed=="Set 'Read initial state' OtherSat OtherReader; % keep import comment","Reference picker changed source spans"); const auto snapshot=window.missionSnapshot(); require(window.applyMissionChange(snapshot,importNode(snapshot),MissionEdit::Replace,changed).isEmpty(),"Picked Set references did not apply"); run(window); auto *other=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("OtherSat")); require(other && other->GetRealParameter("Cr")==1.4 && other->GetRealParameter("X")==42164,"Picked import target did not receive data");
      form.setStatement("Set Sat Missing;"); form.findChild<QPushButton *>("commandChoose_Data")->click(); require(!form.findChild<QLabel *>("commandDataError")->text().isEmpty(),"Missing interface field browse has no corrective message");
      require(window.loadScript(saved),"Failure fixture restore failed"); require(QFile::rename(input,input+".held"),"Missing-file fixture rename failed"); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains("does not exist"),"Missing import file did not fail clearly"); require(QFile::rename(input+".held",input),"Input restore failed"); run(window); same(rows(report),expected);
      for (const auto &bad:QList<QByteArray>{QByteArray("invalid input\n"),QByteArray(original).replace("12 08 20 00 00 00.000","12 13 20 00 00 00.000"),QByteArray(original).replace("EPOCH TIME FOR ELEMENTS:","NO EPOCH TIME:" )}) {
         write(input,bad); require(window.runMission()==MainWindow::RunResult::Failed,"Malformed/invalid-epoch/missing-field input accepted"); write(input,original); run(window); same(rows(report),expected); require(window.loadScript(saved),"Malformed-file recovery reopen failed"); run(window); same(rows(report),expected);
      }
      auto task9=QByteArray("0\n42164\n0\n0\n0\n3.0746662829706\n0\nTVHF ELEMENT SET SUMMARY\n"); task9+=original.mid(original.indexOf("  SATELLITE ID:")); write(input,task9); run(window); same(rows(report),expected); write(input,original);
      // Execute the shipped OF example after conversion and shortening its run.
      auto sample=QString::fromUtf8(bytes(QDir::current().absoluteFilePath("../samples/Ex_FileInterface.script"))); sample.replace("'../samples/SupportFiles/Ex_FileInterface_TVHF.sv'","'"+input+"'"); sample.replace("sc.ElapsedDays = 2","sc.ElapsedSecs = 600"); sample.replace("Propagate 2 days","Propagate 600 seconds"); const auto sampleReport=files.filePath("sample.txt");
      const QString sampleFields="Report Sample sc.A1ModJulian sc.Cr sc.EarthMJ2000Eq.X sc.EarthMJ2000Eq.Y sc.EarthMJ2000Eq.Z sc.EarthMJ2000Eq.VX sc.EarthMJ2000Eq.VY sc.EarthMJ2000Eq.VZ;\n";
      sample.replace("BeginMissionSequence","Create ReportFile Sample;\nSample.Filename = '"+sampleReport+"';\nSample.Precision = 16;\nSample.WriteHeaders = false;\nBeginMissionSequence"); sample.replace("Set 'Read initial state' sc tvhf","Set 'Read initial state' sc tvhf;\n"+sampleFields); sample+=sampleFields;
      const auto converted=convertOpenFramesViews(sample); require(converted.error.isEmpty(),qPrintable(converted.error)); editor->setPlainText(converted.script); run(window); const auto imported=rows(sampleReport); require(imported.size()==2 && imported[0][1]==1.4 && imported[0][2]==42164 && imported[0][6]==3.0746662829706 && std::abs(imported[0][0]-expected[0][0])<1e-10,"Converted shipped sample import incorrect"); require(window.plotReceiver()->model("OFI_EarthView")!=nullptr,"Converted shipped sample viewer missing");
      require(window.saveScriptTo(files.filePath("sample ü.script")) && window.loadScript(files.filePath("sample ü.script")),"Shipped example save/reopen failed"); run(window); same(rows(sampleReport),imported);
      std::cout<<"PASS: FileInterface format/input chooser and clone field metadata, Set target/source/subsets/all/Cancel/validation, eight field combinations, exact Undo/Redo/Unicode save/reopen, independent state/time/Cr reports, Output access, missing/malformed/invalid-epoch/missing-field recovery, Task-9 reader and converted shortened shipped OF example\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; } return 0;
}
