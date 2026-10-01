#include "MainWindow.hpp"
#include "ScriptCompatibility.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "ResourceEditor.hpp"
#include "OrbitObjectDrawingDialog.hpp"
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
#include <QTreeWidget>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
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
static void objectDrawingControls(const QString &startup,const QString &source,const QString &report,const QString &reference,const QString &saved,const QString &capture)
{
   // Reuse the complete committed no-viewer reference; no independent mission rerun.
   const auto expected=read(reference); require(expected.split('\n').size()==3,"Committed object-drawing reference missing complete rows");
   MainWindow window; window.show(); require(window.initialize(startup),"Object drawing initialization failed");
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(source); require(window.buildScript(),"Object drawing fixture build failed");
   auto *area=window.findChild<QMdiArea *>("workspace"); auto *tree=window.findChild<QTreeWidget *>("Resources");
   auto open=[&] {
      const auto matches=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Object drawing resource unavailable");
      tree->itemDoubleClicked(matches.first(),0); auto *child=area->activeSubWindow(); require(child && dynamic_cast<ResourceEditor *>(child->widget()),"Object drawing MDI panel unavailable"); return child;
   };
   auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel->findChild<QPushButton *>("editOrbitDrawing"),"Persistent per-object drawing controls missing");
   auto dialog=[&](const std::function<void(QDialog *)> &action,bool accept) {
      std::exception_ptr failure;
      QTimer::singleShot(0,panel,[&] {
         auto *view=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
         try { require(view,"Object drawing dialog unavailable"); action(view); view->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
         catch (...) { failure=std::current_exception(); if (view) view->reject(); }
      }); panel->findChild<QPushButton *>("editOrbitDrawing")->click(); if (failure) std::rethrow_exception(failure);
   };
   auto choice=[](QDialog *view,const QString &kind,const QString &name) { auto *value=view->findChild<QComboBox *>("orbitDrawing_"+kind+"_"+name); require(value,"Object drawing choice missing"); return value; };
   auto edits=[&](QDialog *view) {
      choice(view,"trajectory","A")->setCurrentText("Off"); choice(view,"label","A")->setCurrentText("On");
      choice(view,"label","B")->setCurrentText("Default"); choice(view,"trajectory","C")->setCurrentText("On"); choice(view,"label","C")->setCurrentText("Off");
   };
   dialog(edits,false); require(!panel->hasChanges() && editor->toPlainText()==source,"Object drawing Cancel changed source/pending flags");
   dialog([](QDialog *) {},true); require(!panel->hasChanges(),"Untouched object drawing generated explicit edits");
   dialog(edits,true); require(panel->hasChanges() && editor->toPlainText()==source,"Object drawing OK changed source before Apply");
   dialog([&](QDialog *view) { require(choice(view,"label","A")->currentText()=="On" && choice(view,"trajectory","A")->currentText()=="Off" && choice(view,"label","B")->currentText()=="Default" && choice(view,"label","C")->currentText()=="Off","Object drawing pending choices lost on reopen"); },false);
   auto *table=panel->findChild<QTableWidget *>("resourceProperties"); int scale=-1; for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ViewScaleFactor") scale=row;
   require(scale>=0,"Object drawing paired camera field absent"); const auto originalScale=table->item(scale,1)->text(); table->item(scale,1)->setText("-1"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
   require(panel->hasChanges() && editor->toPlainText()==source && panel->findChild<QLabel *>("resourceStatus")->text().contains("greater than zero"),"Mixed invalid object drawing Apply lost source/pending edits");
   table->item(scale,1)->setText(originalScale); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
   require(child->isVisible() && area->subWindowList().contains(child),"Object drawing Apply closed its MDI window"); panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel && !panel->hasChanges(),"Object drawing Apply did not refresh clean controls"); const auto edited=editor->toPlainText();
   const auto flags=qtCameraSettings(edited).value("Display"),before=qtCameraSettings(source).value("Display"); require(flags.objectLabels.value("A") && !flags.objectLabels.contains("B") && !flags.objectLabels.value("C") && !flags.objectTrajectories.value("A") && flags.objectTrajectories.value("C") && flags.up==before.up && flags.primaryName==before.primaryName,"Typed object drawing lost flags/Default/camera metadata");
   require(removeQtCameraSetting(edited,"Display")==removeQtCameraSetting(source,"Display"),"Object drawing rewrote unrelated source/calculations/implicit settings");
   dialog([&](QDialog *view) {
      require(choice(view,"label","B")->currentText()=="Default" && choice(view,"label","A")->currentText()=="On","Applied object drawing not reconstructed");
      auto *objects=view->findChild<QTableWidget *>("orbitObjectDrawing"); require(objects->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Object drawing columns not user adjustable");
      objects->setColumnWidth(0,objects->columnWidth(0)+30); const auto width=objects->columnWidth(0); view->resize(400,320); QApplication::processEvents(); require(objects->columnWidth(0)==width,"Object drawing resize discarded manual column width");
      auto *ok=view->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(view->rect().contains(ok->mapTo(view,ok->rect().bottomRight())),"Compact object drawing hides OK");
      if (!capture.isEmpty()) { view->resize(560,380); QEventLoop exposed; QTimer::singleShot(100,&exposed,&QEventLoop::quit); exposed.exec(); require(view->grab().save(capture+".controls.png"),"Object drawing controls capture failed"); }
   },false);
   require(window.runMission()==MainWindow::RunResult::Completed && read(report)==expected,"Typed object drawing changed complete committed independent states");
   auto model=window.plotReceiver()->model("Display"); require(bool(model),"Typed object drawing viewer absent"); bool a=false,b=false,c=false;
   for (const auto &curve:model->curves) { if (curve.name=="A") a=!curve.lines && !curve.showObject && curve.drawsLabel(); if (curve.name=="B") b=!curve.lines && curve.showObject && curve.drawsLabel() && !curve.importedLabel; if (curve.name=="C") c=curve.lines && !curve.showObject && !curve.drawsLabel(); } require(a && b && c,"Typed drawing choices did not reach trajectory/model/label callbacks");
   require(window.plotReceiver()->show("Display"),"Typed object drawing viewer cannot activate"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot && !plot->canvas()->captureImage().isNull(),"Typed object drawing scene not rendered");
   if (!capture.isEmpty()) { require(plot->canvas()->captureImage().save(capture+".scene.png") && window.grab().save(capture+".png"),"Typed object drawing scene/workspace capture failed"); write(capture+".report.txt",read(report)); }
   // Drawing changes accepted before object removal must not retain stale names.
   child=open(); panel=dynamic_cast<ResourceEditor *>(child->widget()); dialog([&](QDialog *view) { choice(view,"label","A")->setCurrentText("Off"); },true);
   std::exception_ptr removeFailure; QTimer::singleShot(0,panel,[&] { auto *view=panel->findChild<QDialog *>("orbitViewDialog"); try { require(view,"Paired object setup unavailable"); auto *objects=view->findChild<QListWidget *>("orbitObjects"); const auto c=objects->findItems("C",Qt::MatchExactly); require(c.size()==1,"Paired removal object missing"); objects->setCurrentItem(c.first()); view->findChild<QPushButton *>("orbitRemoveObject")->click(); view->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); } catch (...) { removeFailure=std::current_exception(); if (view) view->reject(); } }); panel->findChild<QPushButton *>("editOrbitView")->click(); if (removeFailure) std::rethrow_exception(removeFailure);
   require(panel->hasChanges() && editor->toPlainText()==edited,"Paired drawing/removal did not stay pending"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto removed=qtCameraSettings(editor->toPlainText()).value("Display"); require(!removed.objectLabels.contains("C") && !removed.objectTrajectories.contains("C") && !removed.objectLabels.value("A"),"Paired object removal left stale pending drawing names"); editor->undo(); require(editor->toPlainText()==edited,"Paired drawing/removal Undo not exact");
   editor->undo(); require(editor->toPlainText()==source,"Typed drawing Undo lost imported flags/source"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Typed drawing Redo/rebuild not exact");
   // Closed clean panels can reopen after source undo; invalid typed API maps fail atomically.
   for (const auto &invalid:QList<QMap<QString,QString>>{{{"@QtObjectLabels","{\"Missing\":true}"}},{{"@QtObjectTrajectories","{\"A\":\"false\"}"}},{{"@QtObjectLabels","[]"}},{{"@QtObjectLabels","{\"A\":false}"},{"ViewScaleFactor","-1"}}}) require(!window.applyResourceChanges("Display",invalid,edited).isEmpty() && editor->toPlainText()==edited,"Invalid typed drawing map/paired camera did not roll back");
   require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && editor->toPlainText()==edited && read(report)==expected,"Typed object drawing Unicode save/reopen/recovery changed source/states");
   require(!window.applyResourceChanges("Values",{{"@QtObjectLabels","{\"A\":true}"}},edited).isEmpty() && editor->toPlainText()==edited,"Object drawing accepted a non-OrbitView resource");
   const auto ordinary=removeQtCameraSetting(edited,"Display"); editor->setPlainText(ordinary); require(window.buildScript(),"Ordinary OrbitView object-drawing build failed"); child=open(); panel=dynamic_cast<ResourceEditor *>(child->widget());
   dialog([&](QDialog *view) { require(choice(view,"trajectory","A")->currentText()=="Default" && choice(view,"label","C")->currentText()=="Default","Ordinary viewer did not initialize inherited defaults"); },true);
   require(!panel->hasChanges() && editor->toPlainText()==ordinary,"Untouched ordinary drawing made implicit defaults explicit");
   dialog([&](QDialog *view) { choice(view,"label","A")->setCurrentText("On"); },true); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
   require(qtCameraSettings(editor->toPlainText()).value("Display").objectLabels==QMap<QString,bool>{{"A",true}} && qtCameraSettings(editor->toPlainText()).value("Display").objectTrajectories.isEmpty() && removeQtCameraSetting(editor->toPlainText(),"Display")==ordinary,"Ordinary drawing override changed unrelated/implicit source"); editor->undo(); require(editor->toPlainText()==ordinary,"Ordinary drawing override Undo not exact");
   std::cout<<"PASS actual persistent object-drawing MDI controls, Default/On/Off, Cancel/unchanged/pending/reopen, mixed invalid Apply rollback, retained clean Apply, compact adjustable columns, named pending removal, exact Undo/Redo/Unicode file recovery and complete reused independent 19-column state reports; no old conversion/viewer matrix repeated.\n";
}
static void objectDrawingLayout(const QString &capture)
{
   OrbitObjectDrawingDialog dialog({"A","B","C","Earth"},{{"A",true},{"C",false},{"Earth",false}},{{"A",false},{"B",false},{"C",true},{"Earth",false}});
   dialog.show(); QEventLoop exposed; QTimer::singleShot(100,&exposed,&QEventLoop::quit); exposed.exec();
   auto *table=dialog.findChild<QTableWidget *>("orbitObjectDrawing");
   auto fits=[&] { return table->columnViewportPosition(2)+table->columnWidth(2)<=table->viewport()->width(); };
   require(fits(),"Initial object drawing clips its Label column");
   table->setColumnWidth(0,table->columnWidth(0)+30); const int width=table->columnWidth(0);
   dialog.resize(400,320); QApplication::processEvents();
   require(table->columnWidth(0)==width,"Compact drawing layout loses manual widths");
   auto *ok=dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
   require(dialog.rect().contains(ok->mapTo(&dialog,ok->rect().bottomRight())),"Compact drawing layout hides OK");
   dialog.resize(560,380); QApplication::processEvents(); require(fits(),"Restored drawing layout clips its Label column");
   if (!capture.isEmpty()) require(dialog.grab().save(capture),"Object drawing layout capture failed");
   std::cout<<"PASS isolated native/widget object-drawing layout: all three initial/restored columns fit, compact OK visible and manual widths preserved; no engine initialization or mission repeated.\n";
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtConvertedVisibility");
   try {
      if ((argc==2 || argc==3) && QString(argv[1])=="--drawing-layout") { objectDrawingLayout(argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString()); return 0; }
      if (argc==2 && (QString(argv[1])=="--fallback-render" || QString(argv[1])=="--render-only")) { if (QString(argv[1])=="--fallback-render") require(QGuiApplication::platformName()=="offscreen","Fallback probe needs offscreen platform"); renderLabels({}); std::cout<<"PASS isolated "<<QGuiApplication::platformName().toStdString()<<" independent label rendering, label Off, master Off, ordinary hidden-object defaults and latest-pose clipping; no engine mission repeated.\n"; return 0; }
      const bool controlsOnly=argc>1 && QString(argv[1])=="--controls";
      require(controlsOnly ? (argc==4 || argc==5) : (argc==2 || argc==3),"Visibility startup/capture arguments missing"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Visibility files unavailable"); const auto startup=QFileInfo(argv[controlsOnly ? 2 : 1]).absoluteFilePath(),capture=(controlsOnly ? argc==5 : argc==3) ? QFileInfo(argv[controlsOnly ? 4 : 2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("complete states Δ.txt"),saved=files.filePath("visibility Δ.script");
      QString resources="% preserve independent calculations α\nCreate Spacecraft A B C;\n";
      for (int i=0;i<3;++i) { const QString name=QString(QChar('A'+i)); resources+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".X = "+QString::number(7000+i*2000)+";\n"+name+".Y = 0;\n"+name+".Z = 0;\n"+name+".VX = 0;\n"+name+".VY = 7.54605329010754;\n"+name+".VZ = 0;\n"; }
      resources+="Create ForceModel Forces;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nForces.Drag = None;\nForces.SRP = Off;\nCreate Propagator Prop;\nProp.FM = Forces;\nProp.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\n";
      QString state="Report Values A.ElapsedSecs"; for (const auto &name:QStringList{"A","B","C"}) for (const auto &field:QStringList{"X","Y","Z","VX","VY","VZ"}) state+=" "+name+".EarthMJ2000Eq."+field; state+=";\n";
      const auto mission="BeginMissionSequence;\n"+state+"Propagate 'KeepCalculation' Prop(A, B, C) {A.ElapsedSecs = 120}; % calculation stays exact\n"+state;
      const QString views="Create OpenFramesView Camera;\nCamera.ViewFrame = CoordinateSystem;\nCamera.SetDefaultLocation = On;\nCamera.DefaultEye = [0 -24000 14000];\nCreate OpenFramesInterface Display;\nDisplay.Add = {A, B, C, Earth};\nDisplay.View = {Camera};\nDisplay.DrawObject = [false true false true];\nDisplay.DrawLabel = [false true true false];\nDisplay.DrawTrajectory = [true false false false];\n";
      const auto legacy=resources+views+mission; const auto converted=convertOpenFramesViews(legacy);
      if (controlsOnly) { require(converted.error.isEmpty(),"Object drawing fixture conversion failed"); objectDrawingControls(startup,converted.script,report,QFileInfo(argv[3]).absoluteFilePath(),saved,capture); return 0; }
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
