#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "ExternalForceDialog.hpp"
#include "UserParameter.hpp"
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
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QEventLoop>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Fixture/report unavailable"); return QString::fromUtf8(file.readAll()); }
static QVector<double> numbers(const QString &path) { QVector<double> result; for (const auto &word:read(path).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok; const double number=word.toDouble(&ok); require(ok && std::isfinite(number),"Nonnumeric external force report"); result.append(number); } return result; }
static void equivalent(const QVector<double> &a,const QVector<double> &b) { require(a.size()==b.size(),"External force report dimensions changed"); for (int i=0;i<a.size();++i) require(std::abs(a[i]-b[i])<1e-10*std::max(1.0,std::abs(b[i])),"GUI external force result differs from script reference"); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); require(table,"Properties table missing"); for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==name) return table->item(i,1); throw std::runtime_error(("Missing property "+name).toStdString()); }
static void module(ResourceEditor &panel,const QString &name,bool accept) {
   std::exception_ptr failure; auto *choose=panel.findChild<QPushButton *>("chooseProperty_External.ScriptFileName"); require(choose,"Python module selector missing");
   QTimer::singleShot(0,&panel,[&] { auto *dialog=panel.findChild<QInputDialog *>(); try { require(dialog && dialog->comboBoxItems().contains(name),"Configured Python module missing from choices"); require(!dialog->comboBoxItems().contains(name+".py"),"Module selector includes file extension"); dialog->setTextValue(name); accept ? dialog->accept() : dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   choose->click(); if (failure) std::rethrow_exception(failure);
}
static void moduleSourceCases(MainWindow &window,QPlainTextEdit &editor,QTemporaryDir &files,const QString &report,const QString &reference)
{
   auto source=reference;
   source.remove(QRegularExpression("^ExternalFM\\.(?:Drag|SRP)[^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption));
   source.replace("ExternalFM.External                    = 'SimpleExternalForceModel_NoAPI'","% retain creator\n  GMAT ExternalFM.External = 'MissingQtForceModule'; % module comment");
   source.replace("'GetDerivatives'","'MissingFunction'"); source.replace("ExcludeOtherForces = True","ExcludeOtherForces = False");
   const auto suffix=source.mid(source.indexOf("BeginMissionSequence"));
   auto independent=source; independent.replace("MissingQtForceModule","SimpleExternalForceModel_NoAPI"); independent.replace("MissingFunction","GetDerivatives"); independent.replace("ExternalFM.ErrorControl                = 'RSSStep'","ExternalFM.ErrorControl                = 'LargestStep'");
   editor.setPlainText(independent); run(window); const auto expected=numbers(report); require(expected.size()==12,"Module source state report incomplete");
   editor.setPlainText(source); require(window.buildScript(),"Module source fixture failed");
   auto *model=Moderator::Instance()->GetConfiguredObject("ExternalFM"); const auto before=QString::fromStdString(model->GetGeneratingString(Gmat::SCRIPTING));
   auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto matches=tree->findItems("ExternalFM",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Module source resource missing"); tree->itemDoubleClicked(matches.first(),0);
   QMdiSubWindow *child=nullptr; for (auto *item:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (item->property("resourceName").toString()=="ExternalFM") child=item;
   require(child,"Module source MDI panel missing"); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Module source editor missing");
   module(*panel,"SimpleExternalForceModel_NoAPI",true); field(*panel,"External.DerivativesFunction")->setText("GetDerivatives");
   auto *table=panel->findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") qobject_cast<QComboBox *>(table->cellWidget(row,1))->setCurrentText("LargestStep");
   require(panel->hasChanges() && editor.toPlainText()==source,"Module changes did not remain pending"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
   const auto changed=editor.toPlainText(); require(changed!=source && child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Module Apply did not refresh retained panel");
   require(changed.contains("  GMAT ExternalFM.External = ") && changed.contains("% retain creator") && changed.contains("% module comment") && changed.contains("ExternalFM.PointMasses                 = {Earth}") && changed.endsWith(suffix),"Module rename lost unrelated source/comment/mission");
   for (const auto &field:QStringList{"PrimaryBodies","Drag","SRP","RelativisticCorrection","PolyhedralBodies","UserDefined"}) require(!changed.contains("ExternalFM."+field+" ="),"Module rename printed an unrelated force default");
   const auto after=QString::fromStdString(Moderator::Instance()->GetConfiguredObject("ExternalFM")->GetGeneratingString(Gmat::SCRIPTING));
   auto unknown=source; unknown.replace("= 'MissingQtForceModule'","= ... % mapped continuation\n 'MissingQtForceModule'"); unknown.insert(unknown.indexOf("BeginMissionSequence"),"ExternalFM.External.FutureOption = 'retain'; % unknown option\n");
   const auto mapped=patchResourceConfiguration(unknown,"ExternalFM",before,after,{},true);
   require(mapped.contains("ExternalFM.External.FutureOption = 'retain'; % unknown option") && mapped.contains("% mapped continuation") && !mapped.contains("MissingQtForceModule") && mapped.contains("SimpleExternalForceModel_NoAPI"),"Module rename dropped unknown owned source or continued creator");
   editor.undo(); require(editor.toPlainText()==source,"Module source Undo was not exact"); editor.redo(); require(editor.toPlainText()==changed,"Module source Redo was not exact");
   const auto saved=files.filePath("module source Δ.script"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Module source Unicode save/reopen failed"); run(window); equivalent(numbers(report),expected);
   std::cout<<"PASS: actual MDI pending module/function/mixed ErrorControl Apply, retained panel refresh, continued creator and legacy function/comment/source preservation, implicit unrelated force defaults, unknown owned source, exact Undo/Redo/Unicode save/reopen and independent 600-second twelve-state report.\n";
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
      if (argc==3 && QString::fromLocal8Bit(argv[2])=="--module-source") { moduleSourceCases(window,*editor,files,report,reference); return 0; }
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
      // Add the first contributor through the real force-model controls, with
      // an ordinary pending field in the same transaction, then remove it.
      QString without=reference;
      without.remove(QRegularExpression("^ExternalFM\\.(?:External(?:\\.[A-Za-z]+)?|DerivativesFunction)[ \\t]*=[^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption));
      without.prepend("% retained creation source α\n"); without.replace("ExternalFM.ErrorControl                = 'RSSStep'","ExternalFM.ErrorControl                = 'RSSStep' % retain mixed comment");
      editor->setPlainText(without); require(window.runMission()==MainWindow::RunResult::Completed,"Without-external reference failed");
      const auto withoutSource=editor->toPlainText(); require(externalForceSettings(*Moderator::Instance()->GetConfiguredObject("ExternalFM")).value("@ExternalForce.Enabled")=="false","Without-external fixture still has a contributor");
      auto removalReference=withoutSource; removalReference.replace("'RSSStep' % retain mixed comment","'RSSState' % retain mixed comment"); editor->setPlainText(removalReference); run(window); const auto removalExpected=numbers(report);
      QString mixedReference=reference; mixedReference.replace("ExternalFM.ErrorControl                = 'RSSStep'","ExternalFM.ErrorControl                = 'RSSState'"); editor->setPlainText(mixedReference); run(window); const auto mixedExpected=numbers(report);
      editor->setPlainText(withoutSource); require(window.buildScript(),"Creation source restore failed");
      auto open=[&](ResourceEditor &panel,std::function<void(ExternalForceDialog *)> inspect) {
         std::exception_ptr failure;
         QTimer::singleShot(0,&panel,[&] { auto *dialog=dynamic_cast<ExternalForceDialog *>(panel.findChild<QDialog *>("externalForceDialog")); try { require(dialog,"External force controls missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
         panel.findChild<QPushButton *>("forceExternal")->click(); if (failure) std::rethrow_exception(failure);
      };
      auto accept=[](ExternalForceDialog *dialog) { dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); };
      auto choose=[](ExternalForceDialog *dialog) {
         dialog->findChild<QGroupBox *>("externalForceEnabled")->setChecked(true);
         auto *module=dialog->findChild<QComboBox *>("externalForceModule"); require(module->findText("SimpleExternalForceModel_NoAPI")>=0,"Module choices missing from creation"); module->setCurrentText("SimpleExternalForceModel_NoAPI");
         dialog->findChild<QLineEdit *>("externalForceFunction")->setText("GetDerivatives"); dialog->findChild<QCheckBox *>("externalForceExclusive")->setChecked(true);
      };
      auto openPanel=[&] {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto matches=tree->findItems("ExternalFM",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"External force resource tree missing"); tree->itemDoubleClicked(matches.first(),0);
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="ExternalFM") return child;
         throw std::runtime_error("Actual external force panel unavailable");
      };
      {
         auto *child=openPanel(); auto *previous=dynamic_cast<ResourceEditor *>(child->widget()); require(previous,"Actual force editor unavailable"); auto &panel=*previous;
         open(panel,[&](ExternalForceDialog *dialog) { require(!dialog->findChild<QGroupBox *>("externalForceEnabled")->isChecked(),"First force creation already enabled"); choose(dialog); dialog->reject(); });
         require(!panel.hasChanges() && editor->toPlainText()==withoutSource,"Creation Cancel changed pending source");
         auto *table=panel.findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") { auto *control=qobject_cast<QComboBox *>(table->cellWidget(row,1)); require(control && control->count()==5,"Force-model error-control choices missing"); control->setCurrentText("RSSState"); }
         open(panel,[&](ExternalForceDialog *dialog) {
            choose(dialog); auto *module=dialog->findChild<QComboBox *>("externalForceModule"); module->setCurrentText("SimpleExternalForceModel_NoAPI.py"); accept(dialog);
            require(dialog->isVisible() && dialog->findChild<QLabel *>("externalForceError")->text().contains("without .py"),"Creation accepted a Python filename");
            module->setCurrentText("SimpleExternalForceModel_NoAPI"); dialog->findChild<QLineEdit *>("externalForceFunction")->setText("bad function"); accept(dialog); require(dialog->isVisible(),"Creation accepted an invalid function");
            dialog->findChild<QLineEdit *>("externalForceFunction")->setText("GetDerivatives");
            require(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Help),"External controls did not inherit Help");
            if (argc>2) { QEventLoop wait; QTimer::singleShot(250,&wait,&QEventLoop::quit); wait.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(QString::fromLocal8Bit(argv[2])+".create.png"),"External creation window not exposed or capture failed"); }
            accept(dialog);
         });
         require(panel.hasChanges() && editor->toPlainText()==withoutSource && externalForceSettings(*Moderator::Instance()->GetConfiguredObject("ExternalFM")).value("@ExternalForce.Enabled")=="false","Creation changed engine/source before Apply");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(child->isVisible() && child->widget()!=previous && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges() && child->property("sourceScript").toString()==editor->toPlainText(),"Creation did not refresh the retained real force panel");
      }
      const auto created=editor->toPlainText(); require(created.startsWith("% retained creation source α") && created.contains("% retain mixed comment") && created.contains("RSSState") && externalForceSettings(*Moderator::Instance()->GetConfiguredObject("ExternalFM")).value("@ExternalForce.Enabled")=="true","Creation lost mixed field/comment or force");
      require(!created.contains("ExternalFM.PrimaryBodies") && !created.contains("ExternalFM.PolyhedralBodies") && !created.contains("ExternalFM.RelativisticCorrection"),"Mixed creation made unrelated implicit force settings explicit");
      run(window); equivalent(numbers(report),mixedExpected); editor->undo(); require(editor->toPlainText()==withoutSource,"Mixed external creation was not one exact Undo"); editor->redo(); require(editor->toPlainText()==created,"Mixed external creation Redo failed");
      const auto createdFile=files.filePath("Created external ü.script"); require(window.saveScriptTo(createdFile) && window.loadScript(createdFile),"Created external save/reopen failed"); run(window); equivalent(numbers(report),mixedExpected);
      {
         auto *child=openPanel(); auto *previous=dynamic_cast<ResourceEditor *>(child->widget()); require(previous,"Created real force editor unavailable"); auto &panel=*previous;
         open(panel,[&](ExternalForceDialog *dialog) { require(dialog->findChild<QGroupBox *>("externalForceEnabled")->isChecked() && dialog->findChild<QComboBox *>("externalForceModule")->currentText()=="SimpleExternalForceModel_NoAPI","Created settings not restored"); dialog->findChild<QGroupBox *>("externalForceEnabled")->setChecked(false); dialog->reject(); });
         require(!panel.hasChanges() && editor->toPlainText()==created,"Removal Cancel changed pending settings");
         open(panel,[&](ExternalForceDialog *dialog) { dialog->findChild<QGroupBox *>("externalForceEnabled")->setChecked(false); accept(dialog); });
         require(panel.hasChanges() && editor->toPlainText()==created,"Removal mutated source before Apply"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(child->isVisible() && child->widget()!=previous && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges() && !child->widget()->findChild<QPushButton *>("chooseProperty_External.ScriptFileName"),"Removal did not refresh retained panel or remove owned fields");
      }
      const auto removed=editor->toPlainText(); require(!removed.contains("ExternalFM.External") && removed.contains("% retain mixed comment") && externalForceSettings(*Moderator::Instance()->GetConfiguredObject("ExternalFM")).value("@ExternalForce.Enabled")=="false","Removal retained contributor or lost unrelated comment");
      run(window); equivalent(numbers(report),removalExpected); editor->undo(); require(editor->toPlainText()==created,"Removal Undo was not exact"); editor->redo(); require(editor->toPlainText()==removed,"Removal Redo was not exact");
      const auto removedFile=files.filePath("Removed external Δ.script"); require(window.saveScriptTo(removedFile) && window.loadScript(removedFile),"Removal save/reopen failed"); run(window); equivalent(numbers(report),removalExpected);
      std::cout<<"PASS: configured Python module selector and Cancel, pending function/exclusion Apply, shortened shipped no-API example with independent internal two-body states, exact Undo/Redo/Unicode save/reopen, missing module/function failure and recovery, script-reference combined-force execution; first contributor creation/removal through pending controls, Cancel and invalid module/function recovery, atomic mixed force-model Apply with retained comments and independent reports.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
