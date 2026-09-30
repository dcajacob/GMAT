#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "XYPlotDialog.hpp"
#include "GroundTrackDialog.hpp"
#include "BodyFixedStateConverter.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QEventLoop>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] {
      try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); if (auto *dialog=qobject_cast<QDialog *>(owner)) dialog->reject(); }
   });
}
static void close(QDialog *dialog,bool accepted=true) { dialog->findChild<QDialogButtonBox *>()->button(accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>();
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1);
   throw std::runtime_error("Plot field missing");
}
static const PlotCurve &namedCurve(const PlotModel &model,const QString &name)
{
   for (const auto &curve:model.curves) if (curve.name==name) return curve;
   throw std::runtime_error("Expected configured curve missing");
}
static void groundTrackWorkflow(MainWindow &window,const QString &capture,QTemporaryDir &files)
{
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto texture=files.filePath("custom map.png"),bad=files.filePath("broken map.png"),report=files.filePath("ground.txt"),saved=files.filePath("ground ü.script");
   QImage custom(32,16,QImage::Format_RGB32); custom.fill(QColor(80,120,170)); require(custom.save(texture),"Custom texture fixture unavailable");
   QFile valid(texture); require(valid.open(QIODevice::ReadOnly),"Custom texture fixture cannot be read");
   QFile invalid(bad); require(invalid.open(QIODevice::WriteOnly),"Corrupt texture fixture unavailable"); invalid.write(valid.read(33)); invalid.close();
   require(QImageReader(bad).canRead(),"Corrupt fixture lacks a recognizable PNG header");
   const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 600}; % preserve ground mission\nReport Values Sat.EarthFixed.X Sat.EarthFixed.Y Sat.EarthFixed.Z Sat.MarsFixed.X Sat.MarsFixed.Y Sat.MarsFixed.Z;\n";
   const QString base="Create Spacecraft Sat;\nCreate GroundStation Site;\nSite.HorizonReference = Sphere;\nSite.StateType = Spherical;\nSite.Location1 = 20;\nSite.Location2 = 30;\nSite.Location3 = 0;\n"
      "Create CoordinateSystem MarsFixed;\nMarsFixed.Origin = Mars;\nMarsFixed.Axes = BodyFixed;\n"
      "Create ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 60;\nProp.MinStep = 60;\nProp.MaxStep = 60;\n"
      "Create GroundTrackPlot Map;\nMap.Add = {Sat, Site};\nMap.DataCollectFrequency = 1;\nMap.MaxPlotPoints = 100;\n"
      "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.NumPointsToRedraw = 1;\nOrb.MaxPlotPoints = 3;\n"
      "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n"+mission;
   editor->setPlainText(base); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Ground-track baseline failed");
   auto baseline=window.plotReceiver()->model("Map"); require(baseline && !baseline->map.isNull() && baseline->curves[0].points.size()>5,"Ground baseline missing texture/samples");
   const auto count=baseline->curves[0].points.size();
   const auto orbit=window.plotReceiver()->model("Orb"); require(orbit && orbit->redrawPoints==1 && orbit->endOfRun && namedCurve(*orbit,"Sat").points.size()==3,"Orbit redraw/retention settings not passed by GL callback");
   require(baseline->stations.size()==1 && baseline->stations[0].name=="Site","Station marker missing");
   require(std::abs(baseline->stations[0].longitude-30)<1e-9 && std::abs(baseline->stations[0].latitude-20)<1e-9,"Station ground coordinates wrong");
   QFile data(report); require(data.open(QIODevice::ReadOnly),"Ground reference report missing"); const auto expected=QString::fromUtf8(data.readAll()).trimmed().split('\n').last().simplified().split(' '); require(expected.size()==6,"Ground reference frame report incomplete");
   require(window.buildScript(),"Ground-track reset failed"); const auto source=editor->toPlainText(); QString applyError="Not applied"; std::exception_ptr failure;
   {
      QWidget owner; auto *plot=Moderator::Instance()->GetConfiguredObject("Map");
      ResourceEditor panel(*plot,[&](const auto &changes) { applyError=window.applyResourceChanges("Map",changes,source); return applyError; },&owner,source);
      auto *button=panel.findChild<QPushButton *>("editGroundTrack"); require(button,"Ground-track setup missing");
      later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("groundTrackDialog"); dialog->findChild<QComboBox *>("groundCentralBody")->setCurrentText("Mars"); close(dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure);
      require(!panel.hasChanges() && editor->toPlainText()==source,"Ground Cancel leaked edits");
      later(&panel,failure,[&] {
         auto *dialog=panel.findChild<QDialog *>("groundTrackDialog"); auto *body=dialog->findChild<QComboBox *>("groundCentralBody"); auto *map=dialog->findChild<QLineEdit *>("groundTextureMap");
         require(body->findText("Mars")>=0 && dialog->findChild<QSpinBox *>("ground_MaxPlotPoints")->minimum()==1 && dialog->findChild<QSpinBox *>("ground_NumPointsToRedraw")->minimum()==0,"Ground typed data selectors incorrect");
         auto *objects=dialog->findChild<QListWidget *>("groundObjects"); require(objects->findItems("Site",Qt::MatchExactly).size()==1 && objects->findItems("Earth",Qt::MatchExactly).isEmpty(),"Ground object picker does not match wx types");
         dialog->findChild<QPushButton *>("groundClearSelection")->click(); close(dialog); require(dialog->isVisible(),"Shown ground plot accepted no objects"); dialog->findChild<QPushButton *>("groundSelectAll")->click();
         require(dialog->findChild<QLabel *>("groundTrackError")->text().isEmpty(),"Corrected object selection retained stale error");
         map->setText(bad); close(dialog); require(dialog->isVisible() && !dialog->findChild<QLabel *>("groundTrackError")->text().isEmpty(),"Unreadable ground texture accepted");
         map->setText(texture);
         later(dialog,failure,[&] { dialog->findChild<QFileDialog *>("groundTextureDialog")->reject(); }); dialog->findChild<QPushButton *>("groundBrowseTexture")->click(); if (failure) std::rethrow_exception(failure); require(map->text()==texture,"Texture picker Cancel changed map");
         body->setCurrentText("Mars"); const auto marsDefault=map->text(); require(marsDefault!=texture && !marsDefault.isEmpty(),"Central-body change did not choose its default map"); body->setCurrentText("Earth"); require(map->text()==texture,"Custom Earth map lost while changing bodies"); body->setCurrentText("Mars"); require(map->text()==marsDefault,"Body map round trip lost default");
         later(dialog,failure,[&] { auto *picker=dialog->findChild<QFileDialog *>("groundTextureDialog"); picker->selectFile(texture); require(QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection),"Texture picker did not accept selection"); }); dialog->findChild<QPushButton *>("groundBrowseTexture")->click(); if (failure) std::rethrow_exception(failure); require(map->text()==texture,"Texture picker did not apply file");
         for (int i=0;i<objects->count();++i) objects->item(i)->setCheckState(objects->item(i)->text()=="Sat" ? Qt::Checked : Qt::Unchecked);
         dialog->findChild<QSpinBox *>("ground_DataCollectFrequency")->setValue(2); dialog->findChild<QSpinBox *>("ground_UpdatePlotFrequency")->setValue(4); dialog->findChild<QSpinBox *>("ground_MaxPlotPoints")->setValue(3); dialog->findChild<QSpinBox *>("ground_NumPointsToRedraw")->setValue(1);
         dialog->findChild<QComboBox *>("groundSolverIterations")->setCurrentText("None");
         const auto normal=dialog->size(); dialog->resize(620,440); QEventLoop settle; QTimer::singleShot(80,&settle,&QEventLoop::quit); settle.exec();
         auto *scroll=dialog->findChild<QScrollArea *>("groundTrackScroll"); require(scroll->verticalScrollBar()->maximum()>0,"Compact ground dialog cannot scroll");
         auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(QRect(ok->mapTo(dialog,QPoint()),ok->size())),"Ground OK outside compact dialog");
         dialog->resize(normal); QTimer::singleShot(80,&settle,&QEventLoop::quit); settle.exec();
         if (!capture.isEmpty()) { scroll->verticalScrollBar()->setValue(0); require(dialog->grab().save(capture+".ground.png"),"Ground dialog capture failed"); }
         close(dialog);
      }); button->click(); if (failure) std::rethrow_exception(failure);
      require(panel.hasChanges() && plot->GetStringParameter("CentralBody")=="Earth" && plot->GetIntegerParameter("MaxPlotPoints")==100,"Ground setup modified configured values before Apply");
      later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("groundTrackDialog"); require(dialog->findChild<QComboBox *>("groundCentralBody")->currentText()=="Mars" && dialog->findChild<QLineEdit *>("groundTextureMap")->text()==texture,"Pending ground setup not retained"); close(dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure);
      panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(applyError.isEmpty(),qPrintable(applyError));
   }
   const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"Ground edit changed mission source"); editor->undo(); require(editor->toPlainText()==source,"Ground Undo failed"); editor->redo(); require(editor->toPlainText()==changed,"Ground Redo failed");
   require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==changed && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Ground save/reopen/execution failed");
   auto model=window.plotReceiver()->model("Map"); require(model && model->maxPoints==3 && model->redrawPoints==1 && model->updateFrequency==4 && model->endOfRun && namedCurve(*model,"Sat").points.size()==3,"Ground settings not applied to viewer");
   for (const auto &curve:model->curves) require(curve.name!="Site","Deselected ground station retained in viewer");
   require(model->stations.isEmpty(),"Deselected station marker retained");
   require(model->map.size()==custom.size() && model->map.pixelColor(0,0)==custom.pixelColor(0,0),"Custom texture not rendered in ground model");
   require(window.plotReceiver()->show("Map"),"Configured ground viewer unavailable"); QApplication::processEvents();
   PlotWidget *viewer=nullptr; for (auto *child:window.findChildren<QMdiSubWindow *>()) if (child->property("plotName").toString()=="Map") viewer=dynamic_cast<PlotWidget *>(child->widget());
   require(viewer,"Configured ground viewer widget missing"); const auto image=viewer->canvas()->captureImage(); int mapPixels=0;
   for (int row=0;row<image.height();++row) for (int col=0;col<image.width();++col) if (image.pixelColor(col,row)==custom.pixelColor(0,0)) ++mapPixels;
   require(mapPixels>1000,"Configured map image not drawn in ground viewer");
   if (!capture.isEmpty()) require(image.save(capture+".ground-view.png"),"Configured ground viewer capture failed");
   const auto &point=model->curves.constFind(0).value().points.back(); const double radians=180/std::acos(-1.0),x=expected[3].toDouble(),y=expected[4].toDouble(),z=expected[5].toDouble();
   auto *mars=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Mars");
   const auto defaultName=QString::fromStdString(mars->GetStringParameter(mars->GetParameterID("TextureMapFileName")));
   const auto spherical=BodyFixedStateConverterUtil::Convert(Rvector3(x,y,z),"Cartesian","Sphere","Spherical","Ellipsoid",mars->GetFlattening(),mars->GetEquatorialRadius());
   require(std::abs(point.x-std::atan2(y,x)*radians)<1e-9 && std::abs(point.y-spherical[0]*radians)<1e-5,"Selected Mars ground coordinates disagree with independent report/converter");
   const auto current=editor->toPlainText(); require(!window.applyResourceChanges("Map",{{"TextureMap",bad}},current).isEmpty() && editor->toPlainText()==current,"Invalid generic texture edit did not roll back");
   require(!window.applyResourceChanges("Map",{{"DataCollectFrequency","0"}},current).isEmpty() && editor->toPlainText()==current,"Invalid collect frequency accepted");
   require(window.applyResourceChanges("Map",{{"MaxPlotPoints","100"}},current).isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"Ground collect-frequency check failed");
   require(window.plotReceiver()->model("Map")->curves[0].points.size()<count,"Ground data collection frequency ignored");
   require(window.applyResourceChanges("Map",{{"MaxPlotPoints","1"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && window.plotReceiver()->model("Map")->curves[0].points.size()==1,"One-point ground retention ignored");
   require(window.applyResourceChanges("Map",{{"TextureMap",defaultName}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Map")->map.isNull() && window.plotReceiver()->model("Map")->map.size()!=custom.size(),"Body texture filename not resolved through texture path");
   require(window.applyResourceChanges("Map",{{"TextureMap",""}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Map")->map.isNull() && window.plotReceiver()->model("Map")->map.size()!=custom.size(),"Default body texture did not recover");
   require(window.applyResourceChanges("Map",{{"CentralBody","Earth"},{"Add","Site"},{"TextureMap",""}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed,"Station-only ground plot failed");
   const auto stationOnly=window.plotReceiver()->model("Map"); require(stationOnly && stationOnly->curves.isEmpty() && stationOnly->stations.size()==1 && stationOnly->stations[0].name=="Site","Station-only plot retained an old spacecraft curve or lost its station");
   require(window.applyResourceChanges("Map",{{"ShowPlot","false"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Map"),"Ground plot disable ignored");
   std::cout<<"PASS: ground-track typed objects/body maps, texture picker/validation/recovery, data settings, pending Apply/Cancel/Undo/Redo/save/reopen, Mars frame/report agreement and one-point retention\n";
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtPlotSetup");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Plot fixture directory missing");
      MainWindow window; window.show(); require(window.initialize(startup),"Plot setup runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("values.txt"),saved=files.filePath("XY ü.script");
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60}; % preserve plot mission\nReport Values Sat.ElapsedSecs V A(1,2) Sat.EarthMJ2000Eq.X;\n";
      const QString base="Create Spacecraft Sat;\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\n"
         "Create Variable V;\nV = 3;\nCreate String Label;\nLabel = 'not numeric';\nCreate Array A[1,2];\nA(1,1) = 2;\nA(1,2) = 7;\n"
         "Create XYPlot Graph;\nGraph.XVariable = Sat.ElapsedSecs;\nGraph.YVariables = {Sat.EarthMJ2000Eq.Y};\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n"+mission;
      editor->setPlainText(base); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Plot baseline failed");
      QFile data(report); require(data.open(QIODevice::ReadOnly),"Reference plot report missing");
      const auto lines=QString::fromUtf8(data.readAll()).trimmed().split('\n'); const auto values=lines.last().simplified().split(' ');
      std::cout<<"REFERENCE: "<<lines.last().toStdString()<<'\n';
      require(values.size()==4 && std::abs(values[0].toDouble()-60)<1e-6 && values[1].toDouble()==3 && values[2].toDouble()==7,"Plot reference report wrong");
      const double finalX=values[3].toDouble(); require(window.buildScript(),"Plot baseline reset failed"); const auto source=editor->toPlainText();
      auto *plot=Moderator::Instance()->GetConfiguredObject("Graph"); QString applyError="Not applied"; std::exception_ptr failure;
      {
         QWidget owner; ResourceEditor panel(*plot,[&](const auto &changes) { applyError=window.applyResourceChanges("Graph",changes,source); return applyError; },&owner,source);
         auto *setup=panel.findChild<QPushButton *>("editXYPlot"); require(setup,"XY setup button missing");
         later(&panel,failure,[&] { auto *dialog=dynamic_cast<XYPlotDialog *>(panel.findChild<QDialog *>("xyPlotDialog")); dialog->findChild<QLineEdit *>("xyXVariable")->setText("V"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(!panel.hasChanges() && editor->toPlainText()==source,"Plot Cancel leaked edits");
         later(&panel,failure,[&] {
            auto *dialog=dynamic_cast<XYPlotDialog *>(panel.findChild<QDialog *>("xyPlotDialog")); auto *x=dialog->findChild<QLineEdit *>("xyXVariable");
            x->setText("Label"); close(dialog); require(dialog->isVisible() && !dialog->findChild<QLabel *>("xyPlotError")->text().isEmpty(),"String accepted for plot X");
            x->clear(); close(dialog); require(dialog->isVisible(),"Shown plot accepted empty X"); x->setText("Sat.ElapsedSecs");
            later(dialog,failure,[&] {
               auto *picker=dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog")); auto *entry=picker->findChild<QComboBox *>("reportParameterEntry");
               require(entry->findText("Label")<0 && entry->findText("V")>=0 && entry->findText("A")>=0,"Plottable picker filtering wrong");
               entry->setEditText("A"); require(!picker->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Bare array accepted for X");
               picker->findChild<QSpinBox *>("reportArrayColumn")->setValue(1); picker->findChild<QPushButton *>("reportAddElement")->click(); close(picker);
            }); dialog->findChild<QPushButton *>("xySelectX")->click(); if (failure) std::rethrow_exception(failure);
            require(x->text()=="A(1,1)","X array element selection failed");
            later(dialog,failure,[&] {
               auto *picker=dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog"));
               picker->findChild<QComboBox *>("reportParameterEntry")->setEditText("V"); close(picker,false);
            }); dialog->findChild<QPushButton *>("xySelectX")->click(); if (failure) std::rethrow_exception(failure);
            require(x->text()=="A(1,1)","X picker Cancel changed selection");
            later(dialog,failure,[&] {
               auto *picker=dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog")); auto *list=picker->findChild<QListWidget *>("reportSelectedParameters"); list->clear();
               auto *entry=picker->findChild<QComboBox *>("reportParameterEntry"); auto *add=picker->findChild<QPushButton *>("reportAddParameter");
               entry->setEditText("Label"); add->click(); require(list->count()==0,"Y picker accepted string");
               entry->setEditText("V"); add->click(); add->click(); require(list->count()==1,"Y duplicate parameter accepted");
               entry->setEditText("A"); picker->findChild<QSpinBox *>("reportArrayColumn")->setValue(2); picker->findChild<QPushButton *>("reportAddElement")->click();
               require(list->count()==2 && list->item(1)->text()=="A(1,2)","Y array element selection failed");
               picker->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat");
               auto *property=picker->findChild<QComboBox *>("reportPropertyType"); require(property->findText("X")>=0 && property->findText("UTCGregorian")<0,"Plot property browser filtering wrong"); property->setCurrentText("X");
               picker->findChild<QComboBox *>("reportPropertyDependency")->setCurrentText("EarthMJ2000Eq");
               picker->findChild<QPushButton *>("reportUseReference")->click(); add->click(); require(list->count()==3 && list->item(2)->text()=="Sat.EarthMJ2000Eq.X","Y property/frame browsing failed"); close(picker);
            }); dialog->findChild<QPushButton *>("xySelectY")->click(); if (failure) std::rethrow_exception(failure);
            auto *ys=dialog->findChild<QListWidget *>("xyYVariables"); ys->setCurrentRow(2); dialog->findChild<QPushButton *>("xyYUp")->click(); dialog->findChild<QPushButton *>("xyYDown")->click();
            require(ys->item(2)->text()=="Sat.EarthMJ2000Eq.X","Y reordering failed");
            dialog->findChild<QCheckBox *>("xyShowGrid")->setChecked(false); dialog->findChild<QComboBox *>("xySolverIterations")->setCurrentText("None");
            require(dialog->findChild<QLabel *>("xyPlotError")->text().isEmpty(),"Corrected plot input retained stale validation error");
            if (!capture.isEmpty()) { dialog->resize(620,500); QApplication::processEvents(); require(dialog->grab().save(capture),"XY setup capture failed"); }
            close(dialog);
         }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(panel.hasChanges() && plot->GetStringParameter("XVariable")=="Sat.ElapsedSecs" && plot->GetStringArrayParameter("YVariables").size()==1,"XY setup modified configured resource before Apply");
         later(&panel,failure,[&] { auto *dialog=dynamic_cast<XYPlotDialog *>(panel.findChild<QDialog *>("xyPlotDialog")); require(dialog->settings().value("XVariable")=="A(1,1)" && dialog->findChild<QListWidget *>("xyYVariables")->count()==3,"Pending XY settings lost when reopening"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(applyError.isEmpty(),qPrintable(applyError));
      }
      const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"XY Apply altered mission comment/report"); editor->undo(); require(editor->toPlainText()==source,"XY Undo failed"); editor->redo(); require(editor->toPlainText()==changed,"XY Redo failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"GUI XY round trip/execution failed");
      auto model=window.plotReceiver()->model("Graph"); require(model && !model->grid && model->curves.size()==3,"XY grid/curve configuration did not reach viewer");
      int index=0;
      for (const auto &curve:model->curves) {
         require(!curve.points.empty(),"Configured XY curve has no samples");
         for (const auto &point:curve.points) {
            require(point.x==2,"XY X array reference changed value");
            if (index<2) require(point.y==(index==0 ? 3 : 7),"XY variable/array Y changed value");
         }
         if (index==2) require(std::abs(curve.points.back().y-finalX)<1e-8,"XY spacecraft Y differs from report"); ++index;
      }
      require(ReportParameterDialog::isPlottableReference("A(1,2)") && !ReportParameterDialog::isPlottableReference("A(1,3)") && !ReportParameterDialog::isPlottableReference("Label"),"Plottable reference bounds/type validation failed");
      const auto current=editor->toPlainText(); require(!window.applyResourceChanges("Graph",{{"YVariables","Label"}},current).isEmpty() && editor->toPlainText()==current,"Invalid plot Y did not roll back");
      require(!window.applyResourceChanges("Graph",{{"XVariable","Label"}},current).isEmpty() && editor->toPlainText()==current,"Invalid plot X did not roll back");
      require(!window.applyResourceChanges("Graph",{{"YVariables",""}},current).isEmpty() && editor->toPlainText()==current,"Shown plot accepted no Y parameters");
      require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Invalid plot reference recovery failed");
      require(window.applyResourceChanges("Graph",{{"ShowPlot","false"},{"YVariables",""}},editor->toPlainText()).isEmpty(),"Plot disable/clear failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Graph"),"Disabled XY plot still opens after save/reopen");
      const auto disabled=editor->toPlainText(); require(!window.applyResourceChanges("Graph",{{"ShowPlot","true"}},disabled).isEmpty() && editor->toPlainText()==disabled,"Empty XY plot enabled without Y parameters");
      std::cout<<"PASS: XY grouped controls, plottable parameter/array browser, Cancel/pending Apply, validation/recovery, exact Undo/Redo/save/reopen, grid/visibility and numerical curve/report agreement\n";
      groundTrackWorkflow(window,capture,files);
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
