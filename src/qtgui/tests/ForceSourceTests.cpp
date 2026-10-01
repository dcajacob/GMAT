#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "UserParameter.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read force report"); return QString::fromUtf8(file.readAll()); }
static void parserCases()
{
   const QString before="Create ForceModel FM;\nGMAT FM.Drag = MSISE90;\nGMAT FM.Drag.AtmosphereBody = Earth;\nGMAT FM.Drag.F107 = 150;\nGMAT FM.ErrorControl = RSSStep;\n";
   QString after=before; after.replace("MSISE90","NRLMSISE00").replace("F107 = 150","F107 = 160");
   const QString unchanged="GMAT FM.GravityField.Earth.Degree = 0; % untouched degree\nFM.PointMasses = {Luna}; % untouched force\n";
   const QString creator="  GMAT FM.AtmosphereModel  = ... % creator continuation\n   MSISE90 ; % creator comment\n";
   const QString source="Create ForceModel FM;\n"+unchanged+creator+"FM.F107 = 150; % old flux\nFM.Drag.AtmosphereBody = Earth;\nBeginMissionSequence;\nFM.Drag = MSISE90; % mission must stay\n";
   const auto result=patchResourceConfiguration(source,"FM",before,after,{},true);
   require(result.contains(unchanged) && result.contains("% creator continuation") && result.contains("% creator comment") && result.contains("% old flux"),"Drag transition rewrote other force source/comments");
   require(result.contains("FM.AtmosphereModel  =") && !result.contains("FM.F107 = 150") && result.contains("FM.Drag.F107 = 160"),"Legacy drag creator/leaf alias not patched safely");
   require(result.endsWith("BeginMissionSequence;\nFM.Drag = MSISE90; % mission must stay\n"),"Drag patch reached mission assignments");
   after=before; after.replace("AtmosphereBody = Earth","AtmosphereBody = Mars");
   require(patchResourceConfiguration(source,"FM",before,after,{},true).contains("FM.Drag.AtmosphereBody = Mars;"),"Body-only source transition not patched");
   QString missing=source; missing.remove(creator); missing.remove("FM.Drag.AtmosphereBody = Earth;\n");
   after=before; after.replace("MSISE90","NRLMSISE00").replace("AtmosphereBody = Earth","AtmosphereBody = Mars");
   const auto inserted=patchResourceConfiguration(missing,"FM",before,after,{},true);
   require(inserted.indexOf("FM.Drag = NRLMSISE00")<inserted.indexOf("FM.F107 = 150") && inserted.indexOf("FM.Drag.AtmosphereBody = Mars")<inserted.indexOf("FM.F107 = 150"),"New drag creator/body inserted after dependent fields");
   const auto none=patchResourceConfiguration(source,"FM",before,"Create ForceModel FM;\nFM.Drag = None;\nFM.ErrorControl = LargestStep;\n",{},true);
   require(none.contains(unchanged) && none.contains("FM.Drag = None;") && none.contains("FM.ErrorControl = LargestStep;") && !none.contains("FM.F107 = 150"),"Disable lost mixed field or retained legacy drag settings");
   const auto implicit=source.left(source.indexOf("BeginMissionSequence;"))+"Propagate P(S);\nFM.Drag = MSISE90;\n";
   require(patchResourceConfiguration(implicit,"FM",before,after,"Propagate P(S);",true).endsWith("Propagate P(S);\nFM.Drag = MSISE90;\n"),"Implicit mission boundary was not preserved");
   bool refused=false; try { patchResourceConfiguration(implicit,"FM",before,after,"Propagate Missing(S);",true); } catch (const std::exception &) { refused=true; }
   require(refused,"Unlocatable implicit mission boundary accepted");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtForceSource");
   try {
      require(argc==2,"Startup argument required"); parserCases(); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("states.txt");
      const QString untouched="DragFM.PrimaryBodies = {Earth}; % preserve primary selection\n  GMAT DragFM.GravityField.Earth.Degree = 0; % preserve gravity spelling\nDragFM.GravityField.Earth.Order = 0;\nDragFM.PointMasses = {Luna}; % preserve unrelated contributor\n";
      const QString mission="BeginMissionSequence;\nPropagate DragProp(DragSat) {DragSat.ElapsedSecs = 600}; % preserve mission α\nReport States DragSat.EarthMJ2000Eq.X DragSat.EarthMJ2000Eq.Y DragSat.EarthMJ2000Eq.Z DragSat.EarthMJ2000Eq.VX DragSat.EarthMJ2000Eq.VY DragSat.EarthMJ2000Eq.VZ;\n";
      const QString exponential=QFileInfo("../data/atmosphere/earth/EarthExponentialAtmosphereData.txt").absoluteFilePath();
      auto fixture=[&](const QString &model,const QString &error) {
         QString force;
         if (model!="None") force="DragFM.Drag.AtmosphereModel = "+model+"; % preserve creator comment\n";
         if (model=="NRLMSISE00") force+="DragFM.Drag.F107 = 160;\nDragFM.Drag.F107A = 155;\nDragFM.Drag.MagneticIndex = 3;\n";
         if (model=="Exponential") force+="DragFM.Drag.InputFile = '"+exponential+"';\n";
         return "Create Spacecraft DragSat;\nDragSat.DateFormat = UTCGregorian;\nDragSat.Epoch = '01 Jan 2024 12:00:00.000';\nDragSat.X = 6778.1363;\nDragSat.Y = 0;\nDragSat.Z = 0;\nDragSat.VX = 0;\nDragSat.VY = 7.668;\nDragSat.VZ = 0;\nCreate ForceModel DragFM;\n"+untouched+"DragFM.ErrorControl = "+error+"; % preserve control comment\n"+force+
            "Create Propagator DragProp;\nDragProp.FM = DragFM;\nDragProp.Accuracy = 1e-13;\nDragProp.InitialStepSize = 10;\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\n"+mission;
      };
      auto run=[&] { if (window.runMission()!=MainWindow::RunResult::Completed) { app.processEvents(); throw std::runtime_error("Force fixture execution failed: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } return read(report); };
      auto reference=[&](const QString &model,const QString &error) { editor->setPlainText(fixture(model,error)); return run(); };
      const auto nrl=reference("NRLMSISE00","LargestStep"),exp=reference("Exponential","LargestStep"),none=reference("None","RSSState");
      auto compare=[&](const QString &expected) {
         const auto actual=run().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts),wanted=expected.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
         require(actual.size()==6 && wanted.size()==6,"State report incomplete");
         for (int i=0;i<6;++i) { bool valid=false; const auto a=actual[i].toDouble(&valid),b=wanted[i].toDouble(); require(valid && std::isfinite(a) && std::abs(a-b)<1e-11*std::max(1.0,std::abs(b)),"GUI force transition changed independently configured state"); }
      };
      auto configure=[&](const QString &model,const QString &control,bool correct) {
         const auto source=editor->toPlainText(); auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto rows=tree->findItems("DragFM",Qt::MatchExactly|Qt::MatchRecursive); require(rows.size()==1,"Force resource tree missing"); tree->itemDoubleClicked(rows.first(),0);
         QMdiSubWindow *child=nullptr; for (auto *item:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (item->property("resourceName").toString()=="DragFM") child=item;
         require(child,"Actual force panel missing"); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Force panel editor missing");
         auto open=[&](bool cancel) {
            std::exception_ptr failure; QTimer::singleShot(0,panel,[&] {
               auto *dialog=panel->findChild<QDialog *>("atmosphereDialog"); try {
                  require(dialog,"Atmosphere controls missing"); auto *selection=dialog->findChild<QComboBox *>("atmosphere_AtmosphereModel"); require(selection && selection->findText(model)>=0,"Target model missing"); selection->setCurrentText(model);
                  if (cancel) { dialog->reject(); return; }
                  auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
                  if (model=="NRLMSISE00") {
                     auto *f107=dialog->findChild<QLineEdit *>("atmosphere_F107"); f107->setText(correct ? "-1" : "160");
                     if (correct) { ok->click(); require(dialog->isVisible() && !dialog->findChild<QLabel *>("atmosphereError")->text().isEmpty(),"Invalid creation was accepted"); f107->setText("160"); }
                     dialog->findChild<QLineEdit *>("atmosphere_F107A")->setText("155"); dialog->findChild<QLineEdit *>("atmosphere_MagneticIndex")->setText("3");
                  }
                  if (model=="Exponential") dialog->findChild<QLineEdit *>("atmosphere_InputFile")->setText(exponential);
                  ok->click(); require(dialog->result()==QDialog::Accepted,"Atmosphere transition was rejected");
               } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
            }); panel->findChild<QPushButton *>("forceAtmosphere")->click(); if (failure) std::rethrow_exception(failure);
         };
         open(true); require(!panel->hasChanges() && editor->toPlainText()==source,"Creator Cancel changed pending/source");
         if (!control.isEmpty()) { auto *table=panel->findChild<QTableWidget *>(); bool found=false; for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") { auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1)); require(choice,"ErrorControl choices missing"); choice->setCurrentText(control); found=true; } require(found,"Mixed error control field missing"); }
         open(false); require(panel->hasChanges() && editor->toPlainText()==source,"Creator acceptance bypassed Apply");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         const auto applied=editor->toPlainText(); if (applied==source) throw std::runtime_error("Force Apply failed: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString());
         require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Force Apply did not refresh retained panel");
         require(applied.contains(untouched) && applied.contains("% preserve control comment") && applied.endsWith(mission),"Force transition changed unrelated source or mission");
         require(!applied.contains("DragFM.SRP") && !applied.contains("DragFM.RelativisticCorrection") && !applied.contains("DragFM.PolyhedralBodies"),"Force transition made other defaults explicit");
         editor->undo(); require(editor->toPlainText()==source,"Creator Undo was not exact"); editor->redo(); require(editor->toPlainText()==applied,"Creator Redo was not exact");
         const auto saved=files.filePath(model+" Δ.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Creator Unicode save/reopen failed");
      };
      editor->setPlainText(fixture("None","RSSStep")); require(window.buildScript(),"Creation fixture failed"); configure("NRLMSISE00","LargestStep",true); compare(nrl);
      // Exercise the legacy owned creator spelling from shipped scripts.
      auto legacy=editor->toPlainText(); legacy.replace("GMAT DragFM.Drag = NRLMSISE00;","DragFM.Drag.AtmosphereModel = NRLMSISE00; % preserve creator comment"); editor->setPlainText(legacy); require(window.buildScript(),"Legacy transition fixture failed");
      configure("Exponential",{},false); require(editor->toPlainText().contains("DragFM.Drag.AtmosphereModel = Exponential;") && editor->toPlainText().contains("% preserve creator comment"),"Existing creator syntax/comment was lost"); compare(exp);
      configure("None","RSSState",false); require(!editor->toPlainText().contains("DragFM.Drag.InputFile") && !editor->toPlainText().contains("DragFM.Drag.F107"),"Disabled drag settings survived"); compare(none);
      configure("NRLMSISE00","LargestStep",false); compare(nrl);
      std::cout<<"PASS: drag creator/body source mapping, continued legacy aliases, implicit/explicit mission boundaries and protected failure; actual MDI first creation, existing model switch, disable/re-enable, Cancel/invalid correction, mixed Apply, unrelated force syntax/defaults/comments, exact Undo/Redo and Unicode save/reopen, six-state reports against independent script configurations.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
