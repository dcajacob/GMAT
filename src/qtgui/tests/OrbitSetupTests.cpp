#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "OrbitViewDialog.hpp"
#include "ScriptCompatibility.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "OrbitPlot.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMdiSubWindow>
#include <QTimer>
#include <QEventLoop>
#include <QMouseEvent>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void settle() { QEventLoop loop; QTimer::singleShot(80,&loop,&QEventLoop::quit); loop.exec(); }
static void close(QDialog *dialog,bool accepted=true) { dialog->findChild<QDialogButtonBox *>()->button(accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
static PlotWidget *viewer(MainWindow &window)
{
   for (auto *child:window.findChildren<QMdiSubWindow *>()) if (child->property("plotName")=="Orb") return dynamic_cast<PlotWidget *>(child->widget());
   throw std::runtime_error("Orbit viewer missing");
}
static QByteArray contents(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Report unavailable"); return file.readAll(); }
static const PlotCurve &curve(const PlotModel &model,const QString &name) { for (const auto &entry:model.curves) if (entry.name==name) return entry; throw std::runtime_error("Orbit object missing"); }
static void sameView(const PlotViewState &a,const PlotViewState &b)
{
   require(a.zoom==b.zoom && a.yaw==b.yaw && a.pitch==b.pitch && a.pan==b.pan && a.fit==b.fit && a.perspective==b.perspective && a.fieldOfView==b.fieldOfView && a.camera==b.camera,"Interactive camera changed across rerun/reopen");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtOrbitSetup");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Orbit fixture directory unavailable");
      MainWindow window; window.show(); require(window.initialize(startup),"Orbit setup runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("state.txt"),saved=files.filePath("Orbit ü.script");
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 600}; % keep orbit mission\nReport Values Sat.EarthFixed.X Sat.EarthFixed.Y Sat.EarthFixed.Z;\n";
      const QString base="Create Spacecraft Sat;\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 60;\nProp.MinStep = 60;\nProp.MaxStep = 60;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.DrawObject = [true false];\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n"+mission;
      editor->setPlainText(base); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Orbit baseline failed"); const auto expected=contents(report);
      require(window.buildScript(),"Orbit edit reset failed"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
      {
         QWidget owner; auto *plot=Moderator::Instance()->GetConfiguredObject("Orb"); ResourceEditor panel(*plot,[&](const auto &changes) { error=window.applyResourceChanges("Orb",changes,source); return error; },&owner,source);
         auto *setup=panel.findChild<QPushButton *>("editOrbitView"); require(setup,"Grouped orbit setup missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("orbitViewDialog"); dialog->findChild<QPushButton *>("orbitClearObject")->click(); close(dialog); require(dialog->isVisible(),"Shown empty orbit accepted"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(!panel.hasChanges() && editor->toPlainText()==source,"Orbit Cancel changed pending/source settings");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("orbitViewDialog"); close(dialog); require(!dialog->isVisible(),"Unchanged orbit setup rejected"); }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(!panel.hasChanges(),"Unchanged camera formatting generated an explicit edit");
         later(&panel,failure,[&] {
            auto *dialog=panel.findChild<QDialog *>("orbitViewDialog"); auto *objects=dialog->findChild<QListWidget *>("orbitObjects"); require(objects->count()==2 && objects->item(0)->checkState()==Qt::Checked && objects->item(1)->checkState()==Qt::Unchecked,"Orbit visibility initialization wrong");
            auto *available=dialog->findChild<QComboBox *>("orbitAvailableObjects"); require(available->findText("Luna")>=0 && available->findText("Sat")>=0 && available->findText("Prop")<0,"Orbit picker does not restrict space points");
            available->setCurrentText("Luna"); dialog->findChild<QPushButton *>("orbitAddObject")->click(); dialog->findChild<QPushButton *>("orbitAddObject")->click(); require(objects->count()==3,"Duplicate object added"); objects->setCurrentRow(2); dialog->findChild<QPushButton *>("orbitRemoveObject")->click(); require(objects->count()==2,"Object removal failed"); dialog->findChild<QPushButton *>("orbitAddObject")->click();
            objects->setCurrentRow(2); dialog->findChild<QPushButton *>("orbitUpObject")->click(); dialog->findChild<QPushButton *>("orbitUpObject")->click(); require(objects->item(0)->text()=="Luna" && objects->item(2)->checkState()==Qt::Unchecked,"Object movement lost draw flag");
            objects->item(1)->setCheckState(Qt::Unchecked);
            auto *ref=dialog->findChild<QComboBox *>("orbit_ViewPointReference"); auto *pos=dialog->findChild<QComboBox *>("orbit_ViewPointVector"); auto *direction=dialog->findChild<QComboBox *>("orbit_ViewDirection");
            ref->setCurrentText("Vector"); pos->setCurrentText("Vector"); direction->setCurrentText("Vector");
            for (const auto &name:QStringList{"ViewPointReference","ViewPointVector","ViewDirection"}) require(dialog->findChild<QLineEdit *>("orbit_"+name+"1")->isEnabled(),"Vector fields not enabled");
            dialog->findChild<QLineEdit *>("orbit_ViewPointVector1")->setText("nan"); close(dialog); require(dialog->isVisible() && !dialog->findChild<QLabel *>("orbitViewError")->text().isEmpty(),"Nonfinite vector accepted");
            const QStringList refs={"10","20","30"},positions={"1000","2000","30000"};
            for (int i=0;i<3;++i) { dialog->findChild<QLineEdit *>("orbit_ViewPointReference"+QString::number(i+1))->setText(refs[i]); dialog->findChild<QLineEdit *>("orbit_ViewPointVector"+QString::number(i+1))->setText(positions[i]); }
            pos->setCurrentText("Sat"); require(!dialog->findChild<QLineEdit *>("orbit_ViewPointVector1")->isEnabled(),"Object camera leaves vector fields enabled"); pos->setCurrentText("Vector"); require(dialog->findChild<QLineEdit *>("orbit_ViewPointVector1")->text()=="1000","Vector lost across object selection");
            auto *scale=dialog->findChild<QLineEdit *>("orbit_ViewScaleFactor"); scale->setText("0"); close(dialog); require(dialog->isVisible(),"Zero camera scale accepted"); scale->setText("2");
            dialog->findChild<QComboBox *>("orbit_CoordinateSystem")->setCurrentText("EarthFixed"); dialog->findChild<QComboBox *>("orbit_ViewUpCoordinateSystem")->setCurrentText("EarthFixed"); dialog->findChild<QComboBox *>("orbit_ViewUpAxis")->setCurrentText("-Y");
            dialog->findChild<QCheckBox *>("orbit_UseInitialView")->setChecked(false);
            for (const auto &name:QStringList{"Axes","Grid","EclipticPlane","XYPlane","WireFrame","SunLine","EnableStars","EnableConstellations"}) dialog->findChild<QCheckBox *>("orbit_"+name)->setChecked(true);
            dialog->findChild<QCheckBox *>("orbit_ShowLabels")->setChecked(false);
            dialog->findChild<QSpinBox *>("orbit_StarCount")->setValue(80); dialog->findChild<QSpinBox *>("orbit_DataCollectFrequency")->setValue(2); dialog->findChild<QSpinBox *>("orbit_UpdatePlotFrequency")->setValue(3); dialog->findChild<QSpinBox *>("orbit_MaxPlotPoints")->setValue(4); dialog->findChild<QSpinBox *>("orbit_NumPointsToRedraw")->setValue(1); dialog->findChild<QComboBox *>("orbit_SolverIterations")->setCurrentText("None");
            dialog->resize(640,440); settle(); require(dialog->findChild<QScrollArea *>("orbitViewScroll")->verticalScrollBar()->maximum()>0,"Compact orbit setup cannot scroll"); auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(ok->mapTo(dialog,ok->rect().bottomRight())),"Compact setup hides OK button");
            dialog->resize(720,800); settle(); if (!capture.isEmpty()) require(dialog->grab().save(capture),"Orbit setup capture failed"); close(dialog); require(!dialog->isVisible(),qPrintable(dialog->findChild<QLabel *>("orbitViewError")->text()));
         }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(panel.hasChanges() && plot->GetStringArrayParameter("Add").size()==2 && plot->GetRealParameter("ViewScaleFactor")==1,"Dialog changed model before Apply");
         later(&panel,failure,[&] { auto *dialog=dynamic_cast<OrbitViewDialog *>(panel.findChild<QDialog *>("orbitViewDialog")); auto values=dialog->settings(); require(values.value("Add")=="Luna, Sat, Earth" && values.value("DrawObject")=="true false false" && values.value("ViewScaleFactor")=="2","Pending orbit settings lost on reopen"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"Orbit Apply rewrote mission comments"); editor->undo(); require(editor->toPlainText()==source,"Orbit Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Orbit Redo not exact");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Orbit save/reopen execution failed"); require(contents(report)==expected,"GUI orbit settings changed spacecraft calculations");
      auto model=window.plotReceiver()->model("Orb"); require(model && model->coordinates=="EarthFixed" && model->grid && model->axes && model->xyPlane && model->eclipticPlane && model->wireframe && model->sunLine && !model->labels && model->starsEnabled && model->constellationsEnabled && model->starCount==80,"Drawing controls did not reach runtime");
      require(model->maxPoints==4 && model->redrawPoints==1 && model->updateFrequency==3 && model->endOfRun && curve(*model,"Sat").points.size()==4,"Sampling/retention settings not honored");
      require(!curve(*model,"Sat").showObject && curve(*model,"Sat").lines && curve(*model,"Sat").visible && !curve(*model,"Earth").showObject && curve(*model,"Luna").showObject,"DrawObject should hide bodies/models while retaining trajectories");
      require(!model->cameras.empty(),"Edited script camera history missing"); auto camera=model->cameras.back(); require(camera.eye==std::array<double,3>{2010,4020,60030} && camera.target==std::array<double,3>{0,0,-1} && camera.up==std::array<double,3>{0,-1,0},"Vector/scale/up-axis settings did not reach camera history");
      auto *view=viewer(window); view->canvas()->zoomBy(3); view->canvas()->setViewAngles(.4,-.2); view->canvas()->fit(); view->canvas()->zoomBy(2);
      QMouseEvent press(QEvent::MouseButtonPress,QPointF(20,20),QPointF(20,20),Qt::LeftButton,Qt::LeftButton,Qt::ShiftModifier); QApplication::sendEvent(view->canvas(),&press);
      QMouseEvent move(QEvent::MouseMove,QPointF(42,31),QPointF(42,31),Qt::NoButton,Qt::LeftButton,Qt::ShiftModifier); QApplication::sendEvent(view->canvas(),&move);
      view->findChild<QComboBox *>("orbitProjection")->setCurrentText("Perspective"); view->findChild<QDoubleSpinBox *>("orbitFieldOfView")->setValue(63); const auto adjusted=*model->userView;
      require(adjusted.pan==QPointF(22,11) && adjusted.fit && adjusted.zoom>1,"Interactive camera fixture incomplete");
      require(window.runMission()==MainWindow::RunResult::Completed,"UseInitialView Off rerun failed"); sameView(adjusted,*window.plotReceiver()->model("Orb")->userView);
      for (auto *child:window.findChildren<QMdiSubWindow *>()) if (child->property("plotName")=="Orb") child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show("Orb"),"Closed orbit cannot reopen"); sameView(adjusted,*window.plotReceiver()->model("Orb")->userView);
      require(viewer(window)->findChild<QComboBox *>("orbitProjection")->currentText()=="Perspective" && viewer(window)->findChild<QDoubleSpinBox *>("orbitFieldOfView")->value()==63,"Restored camera UI disagrees with view");
      require(window.runMission()==MainWindow::RunResult::Completed,"Closed/reopened orbit rerun failed"); sameView(adjusted,*window.plotReceiver()->model("Orb")->userView);
      require(window.applyResourceChanges("Orb",{{"UseInitialView","On"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"UseInitialView On failed"); const auto reset=*window.plotReceiver()->model("Orb")->userView; require(reset.zoom==1 && reset.pan.isNull() && reset.yaw==0 && reset.pitch==0 && !reset.fit && !reset.perspective,"UseInitialView On did not reset script camera/projection");
      const auto current=editor->toPlainText();
      for (const auto &bad:QList<QMap<QString,QString>>{{{"DrawObject","true"}},{{"DrawObject","true maybe false"}},{{"Add","Sat, Sat"}},{{"Add","Prop"}},{{"ViewPointReference","Prop"}},{{"ViewPointVector","[1 2 nan]"}},{{"ViewDirection","[1 2]"}},{{"ViewScaleFactor","-1"}},{{"ViewUpCoordinateSystem","Sat"}},{{"Add",""}}}) require(!window.applyResourceChanges("Orb",bad,current).isEmpty() && editor->toPlainText()==current,"Invalid orbit edit did not roll back");
      require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Orbit invalid-edit recovery failed");
      require(window.applyResourceChanges("Orb",{{"Add","Earth, Sat"}},editor->toPlainText()).isEmpty(),"Generic list edit failed"); auto *plot=Moderator::Instance()->GetConfiguredObject("Orb"); require(!static_cast<OrbitPlot *>(plot)->GetShowObject("Earth") && !static_cast<OrbitPlot *>(plot)->GetShowObject("Sat"),"Generic list edit lost flags by name");
      error=window.applyResourceChanges("Orb",{{"Add",""},{"DrawObject",""},{"ShowPlot","false"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); require(window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Orb"),"Disabled/empty orbit still opens");
      require(window.applyResourceChanges("Orb",{{"Add","Sat, Earth"},{"DrawObject","true true"},{"ShowPlot","true"},{"ViewPointReference","Earth"},{"ViewDirection","Sat"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"Object camera and re-enable failed");
      model=window.plotReceiver()->model("Orb"); require(!model->cameras.empty() && std::abs(model->cameras.back().target[0]-curve(*model,"Sat").points.back().x)<1e-8,"Object target does not track published spacecraft");
      require(window.applyResourceChanges("Orb",{{"ViewPointVector","Luna"},{"ViewScaleFactor","1"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"Object camera position failed");
      model=window.plotReceiver()->model("Orb"); require(std::abs(model->cameras.back().eye[0]-curve(*model,"Luna").points.back().x)<1e-8,"Object camera position does not follow Luna");
      require(window.applyResourceChanges("Orb",{{"ViewPointVector","[1000 2000 30000]"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"Vector camera restore failed");
      if (!capture.isEmpty()) { settle(); require(viewer(window)->canvas()->captureImage().save(QFileInfo(capture).dir().filePath("orbit-view-wayland.png")),"Orbit view capture failed"); }
      QtCameraSetting imported; imported.automaticBody="Earth"; imported.lookAtRotation=true; imported.perspective=true; imported.fieldOfView=45;
      editor->setPlainText(setQtCameraSetting(editor->toPlainText(),"Orb",imported)); require(window.buildScript(),"Imported primary-camera fixture failed");
      const auto importedSource=editor->toPlainText();
      {
         QWidget owner; auto *plot=Moderator::Instance()->GetConfiguredObject("Orb"); ResourceEditor panel(*plot,[&](const auto &changes) { error=window.applyResourceChanges("Orb",changes,importedSource); return error; },&owner,importedSource);
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("orbitViewDialog"); dialog->findChild<QSpinBox *>("orbit_StarCount")->setValue(81); close(dialog); require(!dialog->isVisible(),"Imported primary-camera setup rejected"); }); panel.findChild<QPushButton *>("editOrbitView")->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto retained=qtCameraSettings(editor->toPlainText()).value("Orb"); require(retained.automaticBody=="Earth" && retained.lookAtRotation && retained.perspective && retained.fieldOfView==45,"Drawing-only grouped edit discarded imported camera metadata");
      require(window.runMission()==MainWindow::RunResult::Completed && window.plotReceiver()->model("Orb")->automaticBody=="Earth","Retained imported camera failed execution");
      std::cout<<"PASS: grouped OrbitView objects/draw flags, typed object/vector/frame cameras, styles/stars/sampling, pending/Cancel/validation/rollback, exact Undo/Redo/Unicode save/reopen, numerical report invariance, UseInitialView Off camera/pan/fit/projection retention across reruns/close/reopen and On reset, object tracking and compact layout\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
