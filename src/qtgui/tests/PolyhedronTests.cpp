#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>
#include <QDialog>
#include <QComboBox>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void write(const QString &path,const QString &text) { QFile file(path); require(file.open(QIODevice::WriteOnly),"Fixture write failed"); require(file.write(text.toUtf8())==text.toUtf8().size(),"Fixture write incomplete"); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Report unavailable"); return QString::fromUtf8(file.readAll()); }
static QVector<double> numbers(const QString &path) { QVector<double> result; for (const auto &word:read(path).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok; const double number=word.toDouble(&ok); require(ok && std::isfinite(number),"Nonnumeric polyhedron report"); result.append(number); } return result; }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); }
static void equivalent(const QVector<double> &a,const QVector<double> &b) { require(a.size()==b.size(),"Polyhedron report dimensions changed"); for (int i=0;i<a.size();++i) require(std::abs(a[i]-b[i])<1e-11*std::max(1.0,std::abs(b[i])),"GUI polyhedron result differs from script reference"); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); require(table,"Properties table missing"); for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==name) return table->item(i,1); throw std::runtime_error(("Missing property "+name).toStdString()); }
static void pickShape(ResourceEditor &panel,const QString &path,bool accept) {
   std::exception_ptr failure; auto *button=panel.findChild<QPushButton *>("chooseProperty_PolyhedronGravityModel.Earth.ShapeFileName"); require(button,"Shape file picker missing");
   QTimer::singleShot(0,&panel,[&] { auto *dialog=panel.findChild<QFileDialog *>(); try { require(dialog && dialog->fileMode()==QFileDialog::ExistingFile,"Shape picker is not an existing-input picker"); dialog->selectFile(path); if (accept) QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); else dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   button->click(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPolyhedron");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Fixture setup failed"); const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto shape=files.filePath("cube shape ü.txt"),report=files.filePath("polyhedron state.txt");
      write(shape,"8\n1 -10 -10 -10\n2 10 -10 -10\n3 10 10 -10\n4 -10 10 -10\n5 -10 -10 10\n6 10 -10 10\n7 10 10 10\n8 -10 10 10\n12\n1 1 3 2\n2 1 4 3\n3 5 6 7\n4 5 7 8\n5 1 5 8\n6 1 8 4\n7 2 3 7\n8 2 7 6\n9 1 2 6\n10 1 6 5\n11 4 8 7\n12 4 7 3\n");
      const QString reference="Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 100;\nSat.Y = 50;\nSat.Z = 30;\nSat.VX = 0;\nSat.VY = 0;\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.CentralBody = Earth;\nFM.PolyhedralBodies = {Earth};\nFM.CreateForceBody = Earth;\nFM.ShapeFileName = '"+shape+"';\nFM.BodyDensity = 2000;\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Accuracy = 1e-12;\nProp.InitialStepSize = 1;\nProp.MaxStep = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60};\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Sat.FM.SurfaceHeight;\n";
      editor->setPlainText(reference); require(window.buildScript(),"Polyhedron reference failed to build"); run(window); const auto expected=numbers(report); require(expected.size()==7,"Polyhedron state report missing");
      require(expected[3]<0 && expected[4]<0 && expected[5]<0,"Cube gravity does not point toward its center");
      {
         ReportParameterDialog dialog({}); dialog.findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat");
         auto *property=dialog.findChild<QComboBox *>("reportPropertyType"); require(property->findText("SurfaceHeight")>=0,"Polyhedron output parameter absent from browser"); property->setCurrentText("SurfaceHeight");
         auto *dependency=dialog.findChild<QComboBox *>("reportPropertyDependency"); require(dependency->findText("FM")>=0,"Force-model parameter dependency absent"); dependency->setCurrentText("FM");
         dialog.findChild<QPushButton *>("reportUseReference")->click(); dialog.findChild<QPushButton *>("reportAddParameter")->click(); require(dialog.selection()==QStringList{"Sat.FM.SurfaceHeight"},"SurfaceHeight browser generated wrong reference"); dialog.reject(); require(editor->toPlainText()==reference,"Output browser Cancel changed mission");
      }
      // At this distance, the uniform cube approaches a point mass with its
      // independently calculated volume (20^3 km^3) and density (kg/m^3).
      const double radius=std::sqrt(100.0*100+50.0*50+30.0*30),mu=6.673e-20*8000.0*1e9*2000.0;
      const double initial[]={100,50,30};
      for (int i=0;i<3;++i) { const double velocity=-mu*initial[i]*60/(radius*radius*radius); require(std::abs(expected[i+3]-velocity)<0.002*std::abs(velocity),"Cube result disagrees with independent far-field mass check"); }
      const auto decoy=files.filePath("smaller cube.txt"),saved=files.filePath("polyhedron mission Δ.script");
      QString small=read(shape); small.replace("-10","-5"); small.replace(" 10"," 5"); write(decoy,small);
      QString source=reference; source.replace(shape,decoy); source.replace("BodyDensity = 2000","BodyDensity = 1000"); editor->setPlainText(source); require(window.buildScript(),"Decoy cube fixture failed");
      QString error;
      {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("FM"),[&](const auto &changes) { error=window.applyResourceChanges("FM",changes,source); return error; },&owner,source);
         auto *table=panel.findChild<QTableWidget *>(); const QString prefix="PolyhedronGravityModel.Earth.";
         for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==prefix+"BodyDensity") require(table->item(i,2)->text()=="kg/m^3","Density units absent or incorrect");
         require(field(panel,prefix+"ShapeFileName")->toolTip().contains("kilometres"),"Shape coordinate units unexplained");
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_"+prefix+"CreateForceBody"); require(choose,"Typed gravity-body selector missing");
         std::exception_ptr failure;
         QTimer::singleShot(0,&panel,[&] { auto *dialog=panel.findChild<QInputDialog *>(); try { require(dialog && dialog->comboBoxItems().contains("Earth") && !dialog->comboBoxItems().contains("Sat"),"Gravity body choices are not celestial bodies"); dialog->setTextValue("Mars"); dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); choose->click(); if (failure) std::rethrow_exception(failure);
         require(field(panel,prefix+"CreateForceBody")->text()=="Earth","Gravity-body Cancel changed pending value");
         pickShape(panel,shape,false); require(field(panel,prefix+"ShapeFileName")->text()==decoy,"Shape chooser Cancel changed pending path");
         pickShape(panel,shape,true); field(panel,prefix+"BodyDensity")->setText("2000"); require(editor->toPlainText()==source,"Pending polyhedron changes altered source");
         if (argc>2) { panel.resize(900,680); panel.show(); app.processEvents(); panel.grab().save(QString::fromLocal8Bit(argv[2])); }
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); if (!error.isEmpty()) { app.processEvents(); throw std::runtime_error((error+"\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString()); }
      }
      const auto configured=editor->toPlainText(); run(window); equivalent(numbers(report),expected);
      require(configured.count("FM.PolyhedralBodies = {Earth}")==1 && !configured.contains("FM.UserDefined = {PolyhedronGravityModel}"),"Serialized cube contributor duplicated");
      editor->undo(); require(editor->toPlainText()==source,"Polyhedron Undo not exact"); editor->redo(); require(editor->toPlainText()==configured,"Polyhedron Redo not exact"); run(window); equivalent(numbers(report),expected);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Polyhedron Unicode save/reopen failed"); run(window); equivalent(numbers(report),expected);
      const QString prefix="PolyhedronGravityModel.Earth.";
      require(window.applyResourceChanges("FM",{{prefix+"CreateForceBody","Mars"},{prefix+"ShapeFileName",shape}},editor->toPlainText()).isEmpty(),"Paired gravity-body/path edit failed");
      require(editor->toPlainText().contains("FM.PolyhedralBodies = {Mars}"),"Changed gravity body was not serialized");
      require(window.applyResourceChanges("FM",{{"PolyhedronGravityModel.Mars.CreateForceBody","Earth"},{"PolyhedronGravityModel.Mars.ShapeFileName",shape}},editor->toPlainText()).isEmpty(),"Gravity-body restoration failed"); run(window); equivalent(numbers(report),expected);
      auto *outputs=window.findChild<QTreeWidget *>("Output"); const auto reports=outputs->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(reports.size()==1,"Polyhedron report missing from Output"); outputs->itemDoubleClicked(reports.first(),0); app.processEvents();
      const auto viewers=window.findChild<QMdiArea *>()->subWindowList(); bool visibleReport=false; for (auto *viewer:viewers) if (viewer->windowTitle().contains("polyhedron state")) visibleReport=viewer->isVisible(); require(visibleReport,"Polyhedron report could not be opened through Output");
      const auto before=editor->toPlainText();
      for (const auto &value : {"0","-1","nan"}) require(!window.applyResourceChanges("FM",{{prefix+"BodyDensity",value}},before).isEmpty() && editor->toPlainText()==before,"Invalid density did not roll back");
      require(!window.applyResourceChanges("FM",{{prefix+"CreateForceBody","Sat"}},before).isEmpty() && editor->toPlainText()==before,"Invalid gravity-body type did not roll back");
      require(!window.applyResourceChanges("FM",{{prefix+"ShapeFileName",files.filePath("missing shape.txt")}},before).isEmpty() && editor->toPlainText()==before,"Missing shape did not roll back");
      const QString valid=read(shape); QString truncated=valid.left(valid.indexOf("12\n")); QString outOfRange=valid; outOfRange.replace("1 1 3 2","1 99 3 2"); QString degenerate=valid; degenerate.replace("1 1 3 2","1 1 1 2"); QString open=valid; open.replace("12\n","11\n"); open=open.left(open.lastIndexOf("12 4 7 3"));
      QString nonfinite=valid; nonfinite.replace("1 -10 -10 -10","1 nan -10 -10"); QString reversed=valid; reversed.replace("1 1 3 2","1 1 2 3");
      QString collinear=valid; collinear.replace("3 10 10 -10","3 30 -10 -10");
      auto reverseRows=valid.trimmed().split('\n'); for (int i=10;i<reverseRows.size();++i) { auto columns=reverseRows[i].split(' '); std::swap(columns[2],columns[3]); reverseRows[i]=columns.join(' '); }
      const QStringList badMeshes={"invalid count\n",truncated,outOfRange,degenerate,open,nonfinite,reversed,collinear,reverseRows.join('\n'),valid+"unexpected\n"};
      for (int i=0;i<badMeshes.size();++i) {
         const auto bad=files.filePath("malformed "+QString::number(i)+".txt"); write(bad,badMeshes[i]);
         const auto snapshot=editor->toPlainText(); require(!window.applyResourceChanges("FM",{{prefix+"ShapeFileName",bad}},snapshot).isEmpty(),"Malformed shape accepted by GUI"); require(editor->toPlainText()==snapshot,"Rejected shape changed source");
         editor->setPlainText(QString(reference).replace(shape,bad)); if (window.buildScript()) require(window.runMission()==MainWindow::RunResult::Failed,"Malformed mesh did not fail execution");
         require(window.loadScript(saved) && window.buildScript(),"Malformed mesh recovery failed"); run(window); equivalent(numbers(report),expected);
      }
      const auto missing=files.filePath("missing shape.txt"); editor->setPlainText(QString(reference).replace(shape,missing)); if (window.buildScript()) require(window.runMission()==MainWindow::RunResult::Failed,"Missing mesh did not fail execution"); require(window.loadScript(saved) && window.buildScript(),"Missing mesh recovery failed"); run(window); equivalent(numbers(report),expected);
      // The loader accepts normal whitespace and a final line without newline.
      const auto whitespace=files.filePath("cube CRLF tabs.txt"); QString changed=valid.trimmed(); changed.replace(' ','\t'); changed.replace("\n","\r\n"); write(whitespace,changed);
      require(window.applyResourceChanges("FM",{{prefix+"ShapeFileName",whitespace}},editor->toPlainText()).isEmpty(),"CRLF/tab/no-final-newline mesh rejected"); run(window); equivalent(numbers(report),expected);
      // Record labels are decorative in the legacy format; connectivity still
      // indexes vertex rows from one. Preserve files with nonsequential labels.
      auto relabeledRows=valid.trimmed().split('\n'); for (int i=1;i<relabeledRows.size();++i) if (i!=9) { auto columns=relabeledRows[i].split(' '); columns[0]=QString::number(100+i); relabeledRows[i]=columns.join(' '); }
      const auto relabeled=files.filePath("cube decorative labels.txt"); write(relabeled,relabeledRows.join('\n'));
      require(window.applyResourceChanges("FM",{{prefix+"ShapeFileName",relabeled}},editor->toPlainText()).isEmpty(),"Legacy decorative record labels rejected"); run(window); equivalent(numbers(report),expected);
      const auto relative=files.filePath("relative polyhedron.script"); write(relative,QString(reference).replace(shape,QFileInfo(shape).fileName())); require(window.loadScript(relative) && window.buildScript(),"Relative shape mission failed"); run(window); equivalent(numbers(report),expected);
      require(window.applyResourceChanges("Values",{{"Precision","15"}},editor->toPlainText()).isEmpty(),"Unrelated resource edit lost relative shape context"); run(window); equivalent(numbers(report),expected);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Relative shape round trip failed"); run(window); equivalent(numbers(report),expected);
      std::cout<<"PASS: typed body/shape selection and Cancel, density units, pending Apply, independent cube far-field check and script state agreement, exact Undo/Redo/Unicode save/reopen, invalid density/body/missing/malformed shape rollback and recovery, whitespace and legacy record labels, SurfaceHeight browser, Output access, relative paths and unrelated resource editing.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
