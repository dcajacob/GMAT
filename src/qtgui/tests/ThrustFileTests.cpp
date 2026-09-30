#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "CommandForm.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QDockWidget>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QTimer>
#include <QRegularExpression>
#include <QLabel>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <functional>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Fixture/report unavailable"); return QString::fromUtf8(file.readAll()); }
static void write(const QString &path,const QString &text) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(text.toUtf8())==text.toUtf8().size(),"Fixture write failed"); }
static QVector<double> numbers(const QString &path) { QVector<double> result; for (const auto &cell:read(path).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok; const auto value=cell.toDouble(&ok); require(ok && std::isfinite(value),"Non-numeric report"); result.append(value); } return result; }
static QString messages(MainWindow &window) { QApplication::processEvents(); return window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(5000); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(messages(window).toStdString()); }
static void equivalent(const QVector<double> &a,const QVector<double> &b) { require(a.size()==b.size(),"Report dimensions changed"); for (int i=0;i<a.size();++i) require(std::abs(a[i]-b[i])<1e-8,"GUI/reference report differs"); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); require(table,"Property table missing"); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1); throw std::runtime_error(("Missing property: "+name).toStdString()); }
static void modal(QWidget &owner,const std::function<void(QDialog *)> &action,const std::function<void()> &open)
{
   std::exception_ptr error;
   QTimer::singleShot(0,&owner,[&] { auto *dialog=owner.findChild<QDialog *>(); try { require(dialog,"Expected dialog missing"); action(dialog); } catch (...) { error=std::current_exception(); if (dialog) dialog->reject(); } });
   open(); if (error) std::rethrow_exception(error);
}
static void selection(ResourceEditor &panel,const QString &property,const QStringList &names,bool accept=true)
{
   auto *button=panel.findChild<QPushButton *>("chooseProperty_"+property); require(button,"Typed list picker missing");
   modal(panel,[&](QDialog *dialog) { auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list,"Selection list missing");
      if (property=="AddThrustSegment") require(list->findItems("History",Qt::MatchExactly).isEmpty() && list->findItems("Segment",Qt::MatchExactly).size()==1,"Segment choices include nonsegments");
      if (property=="MassSource") require(list->findItems("History",Qt::MatchExactly).isEmpty() && list->findItems("Fuel",Qt::MatchExactly).size()==1,"Mass choices include nontanks");
      for (int row=0;row<list->count();++row) list->item(row)->setCheckState(names.contains(list->item(row)->text()) ? Qt::Checked : Qt::Unchecked);
      accept ? dialog->accept() : dialog->reject();
   },[button] { button->click(); });
}
static void vectorEdit(ResourceEditor &panel,const QString &property,const QStringList &values,bool accept=true)
{
   auto *button=panel.findChild<QPushButton *>("editCells_"+property); require(button,"Coefficient grid button missing");
   modal(panel,[&](QDialog *dialog) {
      auto *grid=dialog->findChild<QTableWidget *>("numericGrid"); auto *columns=dialog->findChild<QSpinBox *>("arrayColumns"); require(grid && columns,"Resizable coefficient controls missing");
      columns->setValue(values.size()); require(grid->rowCount()==1 && grid->columnCount()==values.size(),"Vector dimensions incorrect");
      for (int column=0;column<values.size();++column) grid->item(0,column)->setText(values[column]);
      if (!accept) dialog->reject(); else dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
   },[button] { button->click(); });
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtThrustFiles");
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable"); MainWindow window;
      require(argc>1 && window.initialize(QString::fromLocal8Bit(argv[1])),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto history=files.filePath("selected thrust history.thf"),decoy=files.filePath("decoy.thf"),report=files.filePath("values.txt");
      auto fileText=[](const QString &model="ModelAccelAndMassRate",const QString &method="None") { return "BeginThrust {Segment}\nStart_Epoch = 01 Jan 2000 12:00:00.000\nThrust_Vector_Coordinate_System = EarthMJ2000Eq\nThrust_Vector_Interpolation_Method = "+method+"\nMass_Flow_Rate_Interpolation_Method = "+method+"\n"+model+"\n0.0 0.1 0 0"+(model.contains("MassRate") ? " 0.5" : "")+"\n120.0 0.1 0 0"+(model.contains("MassRate") ? " 0.5" : "")+"\nEndThrust {Segment}\n"; };
      write(history,fileText()); write(decoy,fileText());
      const QString setup="Create Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 0;\nCreate ChemicalTank Fuel DecoyFuel;\nFuel.FuelMass = 100;\nSat.Tanks = {Fuel};\nCreate ThrustSegment Segment Spare;\nSegment.MassSource = {Fuel};\nCreate ThrustHistoryFile History;\nHistory.FileName = '"+history+"';\nHistory.AddThrustSegment = {Segment};\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 1;\nProp.MaxStep = 1;\nProp.Accuracy = 1e-12;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nValues.WriteHeaders = false;\n";
      const QString reportCommand="Report Values Sat.ElapsedSecs Sat.Fuel.FuelMass Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const QString mission="BeginMissionSequence;\n"+reportCommand+"BeginFileThrust 'Start history' History(Sat); % retain start\nPropagate Prop(Sat) {Sat.ElapsedSecs = 10};\n"+reportCommand+"EndFileThrust 'Stop history' History(Sat); % retain stop\nPropagate Prop(Sat) {Sat.ElapsedSecs = 20};\n"+reportCommand;
      const QString base=setup+mission;
      editor->setPlainText(base); require(window.buildScript(),qPrintable(messages(window))); run(window); const auto initial=numbers(report);
      require(initial.size()==24 && std::abs(initial[9]-95)<1e-8 && std::abs(initial[17]-95)<1e-8,"Analytic five-kg fuel consumption/coast wrong");
      const QString adjustments="Segment.ThrustScaleFactor = 0.5;\nSegment.MassFlowScaleFactor = 0.75;\nSegment.ApplyThrustScaleToMassFlow = true;\nSegment.ThrustAngle1 = [2 0];\nSegment.ThrustAngle2 = [1];\nSegment.ThrustAngle1Sigma = [0.1 0.2];\nSegment.ThrustAngle2Sigma = [0.3];\nSegment.ThrustScaleFactorSigma = 0.1;\n";
      const QString reference=setup+adjustments+mission;
      editor->setPlainText(reference); require(window.buildScript(),qPrintable(messages(window))); run(window); const auto expected=numbers(report);
      require(std::abs(expected[9]-98.125)<1e-8 && std::abs(expected[17]-98.125)<1e-8,"Scaled mass-flow analytic result wrong");
      require(std::abs(expected[13]-initial[13])>1e-6,"Thrust scale/angles did not change velocity");
      QString source=base; source.replace(history,decoy); source.replace("History.AddThrustSegment = {Segment}","History.AddThrustSegment = {Spare}"); source.replace("Segment.MassSource = {Fuel}","Segment.MassSource = {DecoyFuel}");
      editor->setPlainText(source); require(window.buildScript(),qPrintable(messages(window)));
      require(window.availableEngineTypes().contains("ThrustHistoryFile") && window.availableEngineTypes().contains("ThrustSegment") && window.availableEngineTypes().contains("BeginFileThrust"),"Thrust types unavailable");
      QString error;
      {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("History"),[&](const auto &changes) { error=window.applyResourceChanges("History",changes,source); return error; },&owner,source);
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_FileName"); require(choose,"Thrust input-file picker missing");
         modal(panel,[](QDialog *dialog) { dialog->reject(); },[choose] { choose->click(); }); require(field(panel,"FileName")->text()==decoy,"File picker Cancel changed pending path");
         modal(panel,[&](QDialog *dialog) { auto *picker=dynamic_cast<QFileDialog *>(dialog); require(picker && picker->fileMode()==QFileDialog::ExistingFile,"Thrust picker not an input picker"); picker->selectFile(history); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); },[choose] { choose->click(); });
         selection(panel,"AddThrustSegment",{"Segment"},false); require(field(panel,"AddThrustSegment")->text().contains("Spare"),"Segment Cancel changed pending selection");
         selection(panel,"AddThrustSegment",{"Segment"}); require(Moderator::Instance()->GetConfiguredObject("History")->GetStringParameter("FileName")==decoy.toStdString(),"Pending file selection changed engine");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      source=editor->toPlainText();
      {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Segment"),[&](const auto &changes) { error=window.applyResourceChanges("Segment",changes,source); return error; },&owner,source);
         selection(panel,"MassSource",{"Fuel"}); selection(panel,"SolveFors",{"ThrustAngle1"},false); require(field(panel,"SolveFors")->text().isEmpty(),"Solve-for Cancel changed selection");
         field(panel,"ThrustScaleFactor")->setText("0.5"); field(panel,"MassFlowScaleFactor")->setText("0.75"); field(panel,"ThrustScaleFactorSigma")->setText("0.1");
         auto *table=panel.findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ApplyThrustScaleToMassFlow") qobject_cast<QComboBox *>(table->cellWidget(row,1))->setCurrentText("true");
         vectorEdit(panel,"ThrustAngle1",{"99","1","2"},false); require(field(panel,"ThrustAngle1")->text()=="0","Coefficient Cancel changed values");
         vectorEdit(panel,"ThrustAngle1",{"2","0"}); vectorEdit(panel,"ThrustAngle2",{"1"}); vectorEdit(panel,"ThrustAngle1Sigma",{"0.1","0.2"}); vectorEdit(panel,"ThrustAngle2Sigma",{"0.3"});
         vectorEdit(panel,"ThrustAngle1",{"2","0"}); // Reopen reads pending dimensions.
         require(Moderator::Instance()->GetConfiguredObject("Segment")->GetRvectorParameter("ThrustAngle1").GetSize()==1,"Pending vector resize changed engine");
         panel.resize(780,680); panel.show(); QApplication::processEvents(); if (argc>2) panel.grab().save(QString::fromLocal8Bit(argv[2]));
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto configured=editor->toPlainText(); run(window); equivalent(numbers(report),expected);
      require(Moderator::Instance()->GetConfiguredObject("Segment")->GetRvectorParameter("ThrustAngle1").GetSize()==2,"Vector resize lost on Apply");
      editor->undo(); require(editor->toPlainText()==source,"Segment Undo not exact"); editor->redo(); require(editor->toPlainText()==configured,"Segment Redo not exact"); run(window); equivalent(numbers(report),expected);
      const auto saved=files.filePath("thrust mission ü.script"),copy=files.filePath("thrust mission copy Δ.script");
      require(window.saveScriptTo(saved) && window.saveScriptTo(copy) && read(saved)==configured && read(copy)==configured,"Unicode Save/Save As changed script");
      require(window.loadScript(saved) && window.buildScript(),"Saved thrust mission failed"); run(window); equivalent(numbers(report),expected);
      const auto before=editor->toPlainText();
      for (const auto &changes:QVector<QMap<QString,QString>>{{{"MassSource","History"}},{{"MassSource","Missing"}},{{"SolveFors","BadParameter"}},{{"ThrustAngleConstraintVector","0 0 0"}},{{"ThrustAngle1",""}},{{"ThrustAngle1","nan"}},{{"ThrustScaleFactorSigma","-1"}}}) {
         require(!window.applyResourceChanges("Segment",changes,before).isEmpty() && editor->toPlainText()==before,"Invalid thrust settings did not roll back");
      }
      require(!window.applyResourceChanges("History",{{"AddThrustSegment","Fuel"}},before).isEmpty() && editor->toPlainText()==before,"Wrong segment type accepted");
      require(!window.applyResourceChanges("History",{{"FileName",files.filePath("missing.thf")}},before).isEmpty() && editor->toPlainText()==before,"Missing input accepted by Apply");
      run(window); equivalent(numbers(report),expected);
      const auto snapshot=files.filePath("backup.thf"); require(QFile::rename(history,snapshot),"Input removal failed");
      require(window.runMission()==MainWindow::RunResult::Failed && editor->toPlainText()==before,"Missing thrust file was not a recoverable run failure");
      require(QFile::rename(snapshot,history) && window.loadScript(saved) && window.buildScript(),"Missing thrust file recovery failed"); run(window); equivalent(numbers(report),expected);
      write(history,"not a thrust file\n"); require(window.runMission()==MainWindow::RunResult::Failed && editor->toPlainText()==before,"Malformed thrust input not diagnosed");
      write(history,fileText()); require(window.loadScript(saved) && window.buildScript(),"Malformed input recovery failed"); run(window); equivalent(numbers(report),expected);
      auto *output=window.findChild<QTreeWidget *>("Output"); const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(reports.size()==1,"Thrust report unavailable in Output");
      QMetaObject::invokeMethod(output,"itemDoubleClicked",Qt::DirectConnection,Q_ARG(QTreeWidgetItem *,reports.front()),Q_ARG(int,0)); require(window.findChild<QPlainTextEdit *>("report:Values"),"Cannot open generated thrust report");
      // Both file-thrust commands provide typed controls without changing labels/comments.
      for (const auto &command:{QString("BeginFileThrust"),QString("EndFileThrust")}) {
         QString changed; const auto statement=command+" 'Named history' History(Sat); % keep comment"; CommandForm form([&](const auto &text) { changed=text; }); form.setStatement(statement);
         require(form.title()=="File thrust","File-thrust command form missing");
         modal(form,[](QDialog *dialog) { auto *picker=dynamic_cast<QInputDialog *>(dialog); require(picker,"History selector missing"); const auto names=picker->comboBoxItems(); require(names.contains("History") && !names.contains("Segment") && !names.contains("Fuel"),"History choices not type filtered"); picker->reject(); },[&] { form.findChild<QPushButton *>("commandChoose_Thrust history")->click(); });
         modal(form,[](QDialog *dialog) { auto *list=dialog->findChild<QListWidget *>("fileThrustSpacecraftList"); require(list && list->count()==1 && list->item(0)->text()=="Sat","Spacecraft choices not typed"); dialog->reject(); },[&] { form.findChild<QPushButton *>("commandChoose_Spacecraft")->click(); });
         require(changed.isEmpty(),"Command Cancel changed source");
         form.findChild<QLineEdit *>("commandField_Thrust history")->setText("OtherHistory"); require(changed.contains("OtherHistory(Sat)") && changed.contains("'Named history'") && changed.endsWith("% keep comment"),"Command controls lost wrapper text");
      }
      require(window.createResource("ThrustHistoryFile","GuiHistory",editor->toPlainText()).isEmpty(),"GUI history creation failed");
      require(window.applyResourceChanges("GuiHistory",{{"FileName",history},{"AddThrustSegment","Segment"}},editor->toPlainText()).isEmpty(),"Created history configuration failed");
      for (const auto &command:{QString("BeginFileThrust"),QString("EndFileThrust")}) {
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==command) index=i;
         require(index>=0,"File-thrust command missing in Mission"); QString changed; CommandForm form([&](const auto &text) { changed=text; }); form.setStatement(snapshot.nodes[index].statement);
         modal(form,[](QDialog *dialog) { auto *picker=dynamic_cast<QInputDialog *>(dialog); require(picker && picker->comboBoxItems().contains("GuiHistory"),"Created history missing from command selector"); picker->setTextValue("GuiHistory"); picker->accept(); },[&] { form.findChild<QPushButton *>("commandChoose_Thrust history")->click(); });
         modal(form,[](QDialog *dialog) { dialog->accept(); },[&] { form.findChild<QPushButton *>("commandChoose_Spacecraft")->click(); });
         require(changed.contains("GuiHistory(Sat)") && changed.contains(command=="BeginFileThrust" ? "'Start history'" : "'Stop history'"),"Selected history lost command label");
         require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed).isEmpty(),"Command history selection failed Apply");
      }
      run(window); equivalent(numbers(report),expected);
      // Empty segment/mass lists must replace existing lists rather than append.
      require(window.applyResourceChanges("GuiHistory",{{"AddThrustSegment",""}},editor->toPlainText()).isEmpty(),"Segment clear failed"); run(window); const auto inactive=numbers(report); require(std::abs(inactive[9]-100)<1e-8 && std::abs(inactive[17]-100)<1e-8,"Cleared segment still depleted mass");
      require(window.applyResourceChanges("GuiHistory",{{"AddThrustSegment","Segment"}},editor->toPlainText()).isEmpty(),"Segment restore failed"); run(window); equivalent(numbers(report),expected);
      require(window.applyResourceChanges("Segment",{{"MassSource",""}},editor->toPlainText()).isEmpty(),"Mass source clear failed"); run(window); const auto noMass=numbers(report); require(std::abs(noMass[9]-100)<1e-8,"Cleared mass source still depleted tank");
      require(window.loadScript(saved) && window.buildScript(),"Final restore failed"); run(window); equivalent(numbers(report),expected);
      for (const auto &model:{QString("ModelAccelOnly"),QString("ModelAccelAndMassRate"),QString("ModelThrustOnly"),QString("ModelThrustAndMassRate")}) for (const auto &method:{QString("None"),QString("Linear")}) {
         write(history,fileText(model,method)); editor->setPlainText(reference); require(window.buildScript(),"Reference format failed"); run(window); const auto truth=numbers(report);
         require(std::abs(truth[9]-(model.contains("MassRate") ? 98.125 : 100))<1e-8,"Format-specific analytic mass wrong");
         require(window.loadScript(saved) && window.buildScript(),"GUI format round trip failed"); run(window); equivalent(numbers(report),truth);
      }
      write(history,fileText()); require(window.loadScript(saved) && window.buildScript(),"Format restore failed"); run(window); equivalent(numbers(report),expected);
      // Input lookup follows the document folder for stream builds and Apply,
      // even when the process remains in application/bin for startup assets.
      const auto processDirectory=QDir::currentPath();
      QString relative=read(saved); relative.replace(history,QFileInfo(history).fileName());
      const auto relativeScript=files.filePath("relative inputs.script"); write(relativeScript,relative);
      require(window.loadScript(relativeScript) && window.buildScript(),"Relative input script failed"); run(window); equivalent(numbers(report),expected);
      require(window.applyResourceChanges("History",{{"AddThrustSegment","Segment"}},editor->toPlainText()).isEmpty(),"Relative input rejected by resource Apply");
      require(window.applyResourceChanges("Segment",{{"ThrustScaleFactor","0.5"}},editor->toPlainText()).isEmpty(),"Relative input lost during model reconstruction");
      require(window.saveScriptTo(relativeScript) && window.loadScript(relativeScript) && window.buildScript(),"Relative input save/reopen failed"); run(window); equivalent(numbers(report),expected);
      require(QDir::currentPath()==processDirectory,"Script context changed the process directory");
      // The bundled full-day example is also executed and reopened, with its
      // output redirected to temporary storage so no user report is replaced.
      const auto sample=QDir(QFileInfo(QString::fromLocal8Bit(argv[1])).absolutePath()).filePath("../samples/Ex_Propagate_ThrustHistoryFile.script");
      require(window.loadScript(sample),"Bundled example could not be opened");
      const auto sampleReport=files.filePath("shipped example.txt"); QString shipped=read(sample); shipped.replace("Ex_R2020a_Propagate_ThrustHistoryFile.txt",sampleReport);
      editor->setPlainText(shipped); require(window.buildScript(),qPrintable(messages(window))); run(window);
      const auto sampleValues=read(sampleReport); bool numeric; const auto finalMass=sampleValues.trimmed().split('\n').last().simplified().split(' ').last().toDouble(&numeric);
      require(numeric && std::abs(finalMass-1170)<1e-4,"Bundled 180-second file thrust did not deplete expected mass");
      // Select an absolute input before moving the document to another folder.
      // Raw Save As does not rewrite the script's relative asset references.
      const auto bundledInput=QFileInfo(QDir(QFileInfo(sample).absolutePath()).filePath("SupportFiles/Thrust_Sim.thf")).absoluteFilePath();
      require(window.applyResourceChanges("FiniteBurnModel",{{"FileName",bundledInput}},editor->toPlainText()).isEmpty(),"Bundled input selection failed");
      const auto sampleSaved=files.filePath("shipped thrust example ü.script"); require(window.saveScriptTo(sampleSaved) && window.loadScript(sampleSaved) && window.buildScript(),"Bundled example round trip failed"); run(window); require(read(sampleReport)==sampleValues,"Bundled example report changed after save/reopen");
      std::cout<<"PASS: typed thrust segment/tank/solve-for lists, input picker and Cancel, resizable pending angle/sigma vectors, scaled analytic fuel/coast and independent state reports, four data formats with None/Linear interpolation, exact Undo/Redo/Unicode Save/Save As/reopen, invalid-edit rollback, missing/malformed input recovery, report access, GUI history creation and Begin/EndFileThrust controls, relative input build/Apply/reopen and full-day bundled example.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
