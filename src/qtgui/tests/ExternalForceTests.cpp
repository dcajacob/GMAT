#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QComboBox>
#include <QTimer>
#include <QRegularExpression>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Fixture/report unavailable"); return QString::fromUtf8(file.readAll()); }
static QVector<double> numbers(const QString &path) { QVector<double> result; for (const auto &word:read(path).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok; const double number=word.toDouble(&ok); require(ok && std::isfinite(number),"Nonnumeric external force report"); result.append(number); } return result; }
static void equivalent(const QVector<double> &a,const QVector<double> &b) { require(a.size()==b.size(),"External force report dimensions changed"); for (int i=0;i<a.size();++i) require(std::abs(a[i]-b[i])<1e-10*std::max(1.0,std::abs(b[i])),"GUI external force result differs from script reference"); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); require(table,"Properties table missing"); for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==name) return table->item(i,1); throw std::runtime_error(("Missing property "+name).toStdString()); }
static void module(ResourceEditor &panel,const QString &name,bool accept) {
   std::exception_ptr failure; auto *choose=panel.findChild<QPushButton *>("chooseProperty_External.ScriptFileName"); require(choose,"Python module selector missing");
   QTimer::singleShot(0,&panel,[&] { auto *dialog=panel.findChild<QInputDialog *>(); try { require(dialog && dialog->comboBoxItems().contains(name),"Configured Python module missing from choices"); require(!dialog->comboBoxItems().contains(name+".py"),"Module selector includes file extension"); dialog->setTextValue(name); accept ? dialog->accept() : dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   choose->click(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtExternalForce");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Fixture setup failed"); const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto sample=QDir(QFileInfo(startup).absolutePath()).filePath("../samples/Ex_ExternalForceModel_NoAPI.script"),report=files.filePath("external state.txt"),saved=files.filePath("external mission ü.script");
      QString reference=read(sample); reference.replace("ElapsedDays = 1","ElapsedSecs = 600");
      reference.replace("BeginMissionSequence","Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence");
      reference+="\nReport Values SatInternal.EarthMJ2000Eq.X SatInternal.EarthMJ2000Eq.Y SatInternal.EarthMJ2000Eq.Z SatInternal.EarthMJ2000Eq.VX SatInternal.EarthMJ2000Eq.VY SatInternal.EarthMJ2000Eq.VZ SatExternal.EarthMJ2000Eq.X SatExternal.EarthMJ2000Eq.Y SatExternal.EarthMJ2000Eq.Z SatExternal.EarthMJ2000Eq.VX SatExternal.EarthMJ2000Eq.VY SatExternal.EarthMJ2000Eq.VZ;\n";
      editor->setPlainText(reference); require(window.buildScript(),"External force reference failed to build"); run(window); const auto expected=numbers(report); require(expected.size()==12,"External state report missing");
      for (int i=0;i<6;++i) require(std::abs(expected[i]-expected[i+6])<(i<3 ? 1e-5 : 1e-8),"Python two-body state disagrees with internal force model");
      QString source=reference; source.replace("SimpleExternalForceModel_NoAPI","MissingQtForceModule"); source.replace("'GetDerivatives'","'MissingFunction'"); source.replace("ExcludeOtherForces = True","ExcludeOtherForces = False"); editor->setPlainText(source); require(window.buildScript(),"Pending missing module fixture did not build");
      QString error;
      {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("ExternalFM"),[&](const auto &changes) { error=window.applyResourceChanges("ExternalFM",changes,source); return error; },&owner,source);
         require(field(panel,"External.ScriptFileName")->toolTip().contains("without .py"),"Python module semantics not explained");
         module(panel,"SimpleExternalForceModel_NoAPI",false); require(field(panel,"External.ScriptFileName")->text()=="MissingQtForceModule","Module Cancel changed pending value");
         module(panel,"SimpleExternalForceModel_NoAPI",true); field(panel,"External.DerivativesFunction")->setText("GetDerivatives"); auto *table=panel.findChild<QTableWidget *>();
         for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()=="External.ExcludeOtherForces") { auto *choice=qobject_cast<QComboBox *>(table->cellWidget(i,1)); require(choice,"Exclude-other-forces dropdown missing"); choice->setCurrentText("true"); }
         require(editor->toPlainText()==source,"Pending external force settings changed script");
         if (argc>2) { panel.resize(850,680); panel.show(); app.processEvents(); panel.grab().save(QString::fromLocal8Bit(argv[2])); }
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto configured=editor->toPlainText(); run(window); equivalent(numbers(report),expected);
      require(configured.contains("SimpleExternalForceModel_NoAPI") && !configured.contains("MissingQtForceModule"),"Python module not serialized");
      editor->undo(); require(editor->toPlainText()==source,"External force Undo not exact"); editor->redo(); require(editor->toPlainText()==configured,"External force Redo not exact"); run(window); equivalent(numbers(report),expected);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"External force save/reopen failed"); run(window); equivalent(numbers(report),expected);
      require(window.applyResourceChanges("ExternalFM",{{"External.DerivativesFunction","MissingFunction"}},editor->toPlainText()).isEmpty(),"Missing function could not be configured for runtime error test");
      require(window.runMission()==MainWindow::RunResult::Failed,"Missing Python function did not fail");
      require(window.loadScript(saved) && window.buildScript(),"Missing function recovery failed"); run(window); equivalent(numbers(report),expected);
      require(window.applyResourceChanges("ExternalFM",{{"External.ScriptFileName","MissingQtForceModule"}},editor->toPlainText()).isEmpty(),"Missing module could not be configured for runtime error test");
      require(window.runMission()==MainWindow::RunResult::Failed,"Missing Python module did not fail");
      require(window.loadScript(saved) && window.buildScript(),"Missing module recovery failed"); run(window); equivalent(numbers(report),expected);
      QString combined=reference; combined.replace("ExcludeOtherForces = True","ExcludeOtherForces = False"); editor->setPlainText(combined); require(window.buildScript(),"Combined reference failed"); run(window); const auto combinedExpected=numbers(report);
      require(std::abs(combinedExpected[6]-expected[6])>1e-3,"Exclude-other-forces did not affect state");
      require(window.loadScript(saved) && window.buildScript() && window.applyResourceChanges("ExternalFM",{{"External.ExcludeOtherForces","false"}},editor->toPlainText()).isEmpty(),"GUI combined-force selection failed"); run(window); equivalent(numbers(report),combinedExpected);
      const auto copy=files.filePath("combined external copy Δ.script"); require(window.saveScriptTo(copy) && window.loadScript(copy) && window.buildScript(),"Combined external force round trip failed"); run(window); equivalent(numbers(report),combinedExpected);
      const auto before=editor->toPlainText();
      require(!window.applyResourceChanges("ExternalFM",{{"External.ExcludeOtherForces","invalid"}},before).isEmpty() && editor->toPlainText()==before,"Invalid external setting did not roll back");
      require(window.applyResourceChanges("Values",{{"Precision","15"}},editor->toPlainText()).isEmpty(),"Unrelated report edit failed"); run(window); equivalent(numbers(report),combinedExpected);
      require(editor->toPlainText().contains("ExternalFM.External = 'SimpleExternalForceModel_NoAPI'") && editor->toPlainText().contains("ExternalFM.External.ExcludeOtherForces = false"),"Unrelated resource edit lost external contributor");
      std::cout<<"PASS: configured Python module selector and Cancel, pending function/exclusion Apply, shortened shipped no-API example with independent internal two-body states, exact Undo/Redo/Unicode save/reopen, missing module/function failure and recovery, and script-reference combined-force execution.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
