#include "MainWindow.hpp"
#include "ScriptCompatibility.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QEventLoop>
#include <QTimer>
#include <QTemporaryDir>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Visibility report missing"); return file.readAll(); }
static void write(const QString &path,const QByteArray &value) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(value)==value.size(),"Visibility evidence write failed"); }
static void renderLabels(const QString &capture)
{
   // Independently positioned label-only object, with no line/model/legend.
   // Ordinary curves retain the legacy rule: hidden object also hides its label.
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
   model->axes=false; model->grid=false; model->legend=false; model->scriptedCamera=true;
   model->perspective=true; model->fieldOfView=45;
   model->cameras.push_back({0,{0,-1000,0},{0,0,0},{0,0,1},false});
   auto &curve=model->curves[0]; curve.name="HiddenModelLabel"; curve.showObject=false; curve.lines=false;
   curve.importedLabel=true; model->append(0,100,0,0);
   PlotCanvas canvas(model); canvas.resize(640,480); canvas.show(); canvas.setFrame(model->frame);
   QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec();
   const auto labeled=canvas.captureImage(); require(!labeled.isNull(),"Visibility rendering missing");
   model->labels=false; canvas.refresh(); const auto unlabeled=canvas.captureImage();
   require(labeled!=unlabeled,"Independent label missing for a hidden object/model");
   model->labels=true; curve.importedLabel=false; canvas.refresh(); require(canvas.captureImage()==unlabeled,"Per-object label Off not respected");
   curve.importedLabel.reset(); canvas.refresh(); require(canvas.captureImage()==unlabeled,"Ordinary hidden-object labels changed");
   curve.importedLabel=true; curve.visible=false; canvas.refresh(); require(canvas.captureImage()==unlabeled,"Live visibility did not hide imported label");
   curve.visible=true; ++model->frame; model->append(0,100,-1100,0); canvas.setFrame(model->frame);
   require(canvas.captureImage()==unlabeled,"Latest hidden-object label behind camera reused an earlier visible pose");
   if (!capture.isEmpty()) require(labeled.save(capture+".label-only.png"),"Independent label capture failed");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtConvertedVisibility");
   try {
      if (argc==2 && (QString(argv[1])=="--fallback-render" || QString(argv[1])=="--render-only")) { if (QString(argv[1])=="--fallback-render") require(QGuiApplication::platformName()=="offscreen","Fallback probe needs offscreen platform"); renderLabels({}); std::cout<<"PASS isolated "<<QGuiApplication::platformName().toStdString()<<" independent label rendering, label Off, master Off, ordinary hidden-object defaults and latest-pose clipping; no engine mission repeated.\n"; return 0; }
      require(argc==2 || argc==3,"Visibility startup/capture arguments missing"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Visibility files unavailable"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("complete states Δ.txt"),saved=files.filePath("visibility Δ.script");
      QString resources="% preserve independent calculations α\nCreate Spacecraft A B C;\n";
      for (int i=0;i<3;++i) { const QString name=QString(QChar('A'+i)); resources+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".X = "+QString::number(7000+i*2000)+";\n"+name+".Y = 0;\n"+name+".Z = 0;\n"+name+".VX = 0;\n"+name+".VY = 7.54605329010754;\n"+name+".VZ = 0;\n"; }
      resources+="Create ForceModel Forces;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nForces.Drag = None;\nForces.SRP = Off;\nCreate Propagator Prop;\nProp.FM = Forces;\nProp.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\n";
      QString state="Report Values A.ElapsedSecs"; for (const auto &name:QStringList{"A","B","C"}) for (const auto &field:QStringList{"X","Y","Z","VX","VY","VZ"}) state+=" "+name+".EarthMJ2000Eq."+field; state+=";\n";
      const auto mission="BeginMissionSequence;\n"+state+"Propagate 'KeepCalculation' Prop(A, B, C) {A.ElapsedSecs = 120}; % calculation stays exact\n"+state;
      const QString views="Create OpenFramesView Camera;\nCamera.ViewFrame = CoordinateSystem;\nCamera.SetDefaultLocation = On;\nCamera.DefaultEye = [0 -24000 14000];\nCreate OpenFramesInterface Display;\nDisplay.Add = {A, B, C, Earth};\nDisplay.View = {Camera};\nDisplay.DrawObject = [false true false true];\nDisplay.DrawLabel = [false true true false];\nDisplay.DrawTrajectory = [true false false false];\n";
      const auto legacy=resources+views+mission; const auto converted=convertOpenFramesViews(legacy);
      require(converted.error.isEmpty() && converted.script.contains("objectLabels") && converted.script.contains("objectTrajectories"),"Independent OF labels/trajectories lost in conversion");
      const auto flags=qtCameraSettings(converted.script).value("Display"); require(flags.objectLabels.value("C") && !flags.objectLabels.value("A") && !flags.objectTrajectories.value("B") && flags.objectTrajectories.value("A"),"Visibility flag/object order changed"); require(converted.script.endsWith(mission),"Visibility conversion changed calculations/comments");
      const auto prefix=convertOpenFramesViews(QString(legacy).replace("[false true true false]","[false]").replace("[true false false false]","[]")); require(prefix.error.isEmpty() && !qtCameraSettings(prefix.script).value("Display").objectLabels.value("A") && qtCameraSettings(prefix.script).value("Display").objectLabels.value("B") && qtCameraSettings(prefix.script).value("Display").objectTrajectories.value("C"),"OF prefix/empty flags did not retain omitted true defaults");
      for (const auto &bad:QStringList{QString(legacy).replace("[true false false false]","[true bogus]"),QString(legacy).replace("[false true true false]","{false true}")}) { const auto rejected=convertOpenFramesViews(bad); require(!rejected.error.isEmpty() && rejected.script==bad,"Malformed visibility conversion lost original source"); }
      const auto reset=convertOpenFramesViews(QString(legacy).replace("Display.DrawTrajectory = [true false false false];","Display.DrawTrajectory = [true false false false];\nDisplay.Add = {A, B, C, Earth};")); require(reset.error.isEmpty() && qtCameraSettings(reset.script).value("Display").objectLabels.value("A") && qtCameraSettings(reset.script).value("Display").objectTrajectories.value("B"),"Repeated OF Add did not reset drawing defaults");
      const auto noView=convertOpenFramesViews(QString(legacy).replace("Display.View = {Camera};\n","")); require(noView.error.isEmpty() && qtCameraSettings(noView.script).value("Display").objectLabels==flags.objectLabels,"Visibility metadata lost without explicit View selection");
      MainWindow window; window.show(); require(window.initialize(startup),"Visibility runtime initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(resources+mission); require(window.runMission()==MainWindow::RunResult::Completed,"Independent visibility state mission failed"); const auto expected=read(report); require(expected.split('\n').size()==3,"Independent report not complete two rows");
      editor->setPlainText(legacy); bool offered=false; QTimer::singleShot(0,[&] { auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (prompt && prompt->objectName()=="openFramesConversionPrompt") { offered=true; prompt->done(QMessageBox::Yes); } else if (prompt) prompt->reject(); }); require(window.buildScript() && offered,"Visibility conversion offer failed"); const auto source=editor->toPlainText(); require(source==converted.script,"Actual visibility conversion differs"); editor->undo(); require(editor->toPlainText()==legacy,"Visibility conversion Undo lost OF flags"); editor->redo(); require(editor->toPlainText()==source && window.saveScriptTo(saved) && window.loadScript(saved),"Visibility Unicode Save/reopen/Redo failed"); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==expected,"Converted visibility changed complete numerical states");
      auto model=window.plotReceiver()->model("Display"); require(model && model->curves.size()>=4,"Visibility plot missing objects"); for (const auto &curve:model->curves) std::cout<<"Captured object: "<<curve.name.toStdString()<<" object="<<curve.showObject<<" trajectory="<<curve.lines<<" label="<<curve.drawsLabel()<<'\n'; const PlotCurve *a=nullptr,*b=nullptr,*c=nullptr,*earth=nullptr; for (const auto &curve:model->curves) { if (curve.name=="A") a=&curve; if (curve.name=="B") b=&curve; if (curve.name=="C") c=&curve; if (curve.name=="Earth") earth=&curve; }
      require(a && b && c && earth && a->lines && !a->showObject && !a->drawsLabel() && !b->lines && b->showObject && b->drawsLabel() && !c->lines && !c->showObject && c->drawsLabel() && earth->showObject && !earth->drawsLabel(),"Independent trajectory/object/label callback settings combined or ignored"); require(a->points.size()>2 && b->points.size()==a->points.size() && c->points.size()==a->points.size(),"Hidden drawing discarded numerical history");
      require(window.plotReceiver()->show("Display"),"Visibility viewer did not activate"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot,"Visibility viewer widget absent"); const auto before=plot->canvas()->captureImage(); require(!before.isNull(),"Visibility viewer not rendered"); area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show("Display") && window.plotReceiver()->model("Display")==model,"Visibility close/reopen lost settings/history");
      if (!capture.isEmpty()) { require(before.save(capture+".scene.png"),"Visibility scene capture failed"); QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec(); require(window.grab().save(capture+".png"),"Visibility workspace capture failed"); write(capture+".reference.txt",expected); write(capture+".viewer.txt",read(report)); }
      // Name-based metadata survives Add reordering/removal and exact Undo.
      QString applyError="Not applied"; std::exception_ptr dialogFailure;
      {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Display"),[&](const auto &changes) { applyError=window.applyResourceChanges("Display",changes,source); return applyError; },&owner,source);
         auto *setup=panel.findChild<QPushButton *>("editOrbitView"); require(setup,"Converted orbit setup missing");
         auto rearrange=[&](bool accept) {
            QTimer::singleShot(0,&panel,[&panel,&dialogFailure,accept] {
               auto *dialog=panel.findChild<QDialog *>("orbitViewDialog");
               try {
                  require(dialog,"Converted orbit dialog missing"); auto *objects=dialog->findChild<QListWidget *>("orbitObjects"); require(objects && objects->count()==4,"Converted explicit Add list missing");
                  objects->setCurrentRow(3); for (int i=0;i<3;++i) dialog->findChild<QPushButton *>("orbitUpObject")->click();
                  require(objects->item(0)->text()=="Earth","Converted object reorder failed");
                  dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
               } catch (...) { dialogFailure=std::current_exception(); if (dialog) dialog->reject(); }
            }); setup->click(); if (dialogFailure) std::rethrow_exception(dialogFailure);
         };
         rearrange(false); require(!panel.hasChanges() && editor->toPlainText()==source,"Converted visibility Cancel changed source or flags");
         rearrange(true); require(panel.hasChanges() && editor->toPlainText()==source,"Converted visibility pending edit changed source early"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(applyError.isEmpty(),qPrintable(applyError));
      }
      const auto reordered=editor->toPlainText(); require(qtCameraSettings(reordered).value("Display").objectLabels==flags.objectLabels && qtCameraSettings(reordered).value("Display").objectTrajectories==flags.objectTrajectories,"Visibility metadata followed row indices after reordering");
      require(window.applyResourceChanges("Display",{{"Add","Earth, A, B"},{"DrawObject","true false true"}},reordered).isEmpty() && !qtCameraSettings(editor->toPlainText()).value("Display").objectLabels.contains("C"),"Removed object display metadata blocked Apply or remained stale"); editor->undo(); require(editor->toPlainText()==reordered,"Visibility removal Undo lost source"); editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Visibility reorder Undo lost original source");
      for (const auto &bad:QStringList{QString(source).replace("\"C\":true","\"Missing\":true"),QString(source).replace("\"C\":true","\"C\":\"true\"")}) { editor->setPlainText(bad); require(!window.buildScript() && editor->toPlainText()==bad,"Bad imported flag silently accepted or source lost"); }
      require(window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && editor->toPlainText()==source && read(report)==expected,"Visibility failed-build recovery changed source/calculations");
      const auto deletion=window.deleteResource("Display",source); require(deletion.isEmpty(),qPrintable(deletion)); require(!qtCameraSettings(editor->toPlainText()).contains("Display") && editor->toPlainText().endsWith(mission),"Viewer deletion retained imported metadata or changed calculations"); require(window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Display") && read(report)==expected,"Deleted converted viewer altered calculations or still opened"); editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Converted viewer deletion Undo lost exact metadata/source");
      renderLabels(capture);
      std::cout<<"PASS independent OF per-object labels/trajectory/object flags, prefix defaults, automatic Build conversion, exact Undo/Redo/Unicode file recovery, complete independent 19-column state reports, named reorder/removal and hidden-model label rendering; no engine calculations changed.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
