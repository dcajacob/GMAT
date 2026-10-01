#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ResourceEditor.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),qPrintable("Toggle output missing: "+path)); return file.readAll(); }
static void write(const QString &path,const QByteArray &data) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Toggle evidence write failed"); }
static void run(MainWindow &window) { const auto result=window.runMission(); QApplication::processEvents(); require(result==MainWindow::RunResult::Completed,qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText())); require(!window.isRunning(),"Ephemeris Toggle kept runtime locked"); }
static QVector<QVector<double>> rows(const QByteArray &data,bool oem=false)
{
   QVector<QVector<double>> result; const QDateTime initial(QDate(2000,1,1),QTime(12,0),QTimeZone::UTC);
   for (const auto &line:QString::fromUtf8(data).split('\n')) {
      const auto parts=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); if (parts.size()!=7) continue;
      QVector<double> row; bool ok=false;
      if (oem) { const auto epoch=QDateTime::fromString(parts[0]+"Z",Qt::ISODateWithMs); if (!epoch.isValid()) continue; row.append(initial.msecsTo(epoch)/1000.); }
      else { const auto time=parts[0].toDouble(&ok); if (!ok) continue; row.append(time); }
      for (int i=1;i<parts.size();++i) { const auto value=parts[i].toDouble(&ok); if (!ok) break; row.append(value); }
      if (row.size()==7) result.append(row);
   }
   return result;
}
static void circle(const QVector<double> &state,double mu)
{
   require(state.size()==7,"Circular ephemeris state incomplete"); const auto time=state[0],rate=std::sqrt(mu/(7000.*7000.*7000.)),speed=std::sqrt(mu/7000.);
   const QVector<double> expected={7000*std::cos(rate*time),7000*std::sin(rate*time),0,-speed*std::sin(rate*time),speed*std::cos(rate*time),0};
   for (int i=0;i<6;++i) require(std::abs(state[i+1]-expected[i])<(i<3 ? 2e-5 : 2e-8),qPrintable(QString("Circular state mismatch at %1 s column %2: %3 vs %4").arg(time).arg(i).arg(state[i+1],0,'g',17).arg(expected[i],0,'g',17)));
}
static QVector<QVector<double>> ephemeris(const QByteArray &data,const QString &format,double mu)
{
   const bool oem=format=="CCSDS-OEM"; const auto points=rows(data,oem);
   require(points.size()>=24 && std::abs(points.front()[0])<.001 && std::abs(points.back()[0]-360)<.001,"Toggle ephemeris enabled arc endpoints/count wrong");
   int gaps=0; double previous=-1;
   for (const auto &point:points) {
      require(point[0]>=previous && !(point[0]>120.001 && point[0]<239.999) && point[0]<360.001,"Ephemeris wrote a disabled interval or unordered epoch");
      if (previous>=0 && point[0]-previous>20) ++gaps; previous=point[0]; circle(point,mu);
   }
   require(gaps==1,"Ephemeris did not retain exactly two separated enabled arcs");
   if (oem) require(data.count("META_START")==2 && data.count("META_STOP")==2,"OEM Toggle segment metadata not split into two arcs");
   else {
      require(data.contains("DistanceUnit            Kilometers") && data.contains("END Ephemeris"),"STK Toggle unit/finalization metadata missing");
      const auto match=QRegularExpression("BEGIN SegmentBoundaryTimes\\s+(.*?)END SegmentBoundaryTimes",QRegularExpression::DotMatchesEverythingOption).match(QString::fromUtf8(data)); require(match.hasMatch(),"STK Toggle segment metadata missing");
      const auto times=match.captured(1).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(times.size()==2 && std::abs(times[0].toDouble())<.001 && std::abs(times[1].toDouble()-240)<.001,"STK metadata does not identify actual resumed arc starts (0/240 s)");
   }
   return points;
}
static QMdiSubWindow *open(MainWindow &window,const QString &label)
{
   auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator it(tree); while (*it && !(*it)->text(0).endsWith(" — "+label)) ++it; require(*it,"Ephemeris Toggle command tree entry missing"); tree->itemDoubleClicked(*it,0);
   auto *area=window.findChild<QMdiArea *>("workspace"); auto *child=area->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Ephemeris Toggle MDI editor missing"); QApplication::processEvents(); require(area->viewport()->rect().contains(child->geometry()),qPrintable(QString("New command panel outside workspace: child %1,%2 %3x%4, viewport %5x%6").arg(child->x()).arg(child->y()).arg(child->width()).arg(child->height()).arg(area->viewport()->width()).arg(area->viewport()->height())));
   auto *apply=child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply); require(apply->isVisible() && area->viewport()->rect().contains(apply->mapTo(area->viewport(),apply->rect().bottomRight())),"New command panel hides Apply outside workspace"); return child;
}
static void closePanels(MainWindow &window) { for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<CommandEditor *>(child->widget())) { panel->discardChanges(); child->close(); } QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static void choose(QMdiSubWindow *child,bool accept,const QString &capture={})
{
   std::exception_ptr failure; bool visited=false; QTimer::singleShot(0,child,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); visited=true;
      try {
         require(dialog && dialog->objectName()=="toggleSubscriberDialog","Ephemeris Toggle actual subscriber checklist missing"); auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList"); const QStringList selected{"Export","Orb","Ground","XY"};
         for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked);
         require(!dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Empty Toggle selection accepted");
         require(list->findItems("Sat",Qt::MatchExactly).isEmpty(),"Toggle checklist offered a non-subscriber");
         for (int i=selected.size()-1;i>=0;--i) { const auto items=list->findItems(selected[i],Qt::MatchExactly); require(items.size()==1,"Ephemeris or plot subscriber absent from checklist"); auto *item=list->takeItem(list->row(items.first())); list->insertItem(0,item); item->setCheckState(Qt::Checked); }
         if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture),"Native Ephemeris Toggle checklist not exposed"); }
         dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   child->widget()->findChild<QPushButton *>("commandChoose_Subscribers")->click(); require(visited,"Ephemeris Toggle selection not visited"); if (failure) std::rethrow_exception(failure);
}
static void inspectPlots(MainWindow &window,const QString &capture,bool solver=false)
{
   auto *area=window.findChild<QMdiArea *>("workspace");
   for (const auto &name:QStringList{"Orb","Ground","XY"}) {
      auto model=window.plotReceiver()->model(name); require(model && !model->curves.isEmpty(),"Mixed Toggle plot/history missing");
      const PlotCurve *curve=nullptr; for (const auto &candidate:model->curves) if (candidate.name=="Sat" || model->kind==PlotModel::Kind::XY) { curve=&candidate; break; } require(curve && curve->points.size()>10,"Mixed Toggle spacecraft curve missing"); const auto &retained=curve->points; std::deque<PlotPoint> points; for (const auto &point:retained) if (!solver || !point.solver) points.push_back(point); require(points.size()==26,"Mixed Toggle accepted history is not exactly the two 13-point arcs"); if (solver && name!="Orb") require(retained.size()==points.size(),qPrintable(QString("Current %1 retained %2 points instead of its %3 accepted samples").arg(name).arg(retained.size()).arg(points.size()))); const double initial=points.front().epoch; int gaps=0;
      for (size_t i=0;i<points.size();++i) {
         const auto &point=points[i]; const double time=model->kind==PlotModel::Kind::XY ? point.x : (point.epoch-initial)*86400.; require(!(time>120.001 && time<239.999) && time<360.001,"Viewer wrote a disabled Ephemeris Toggle interval");
         if (i>0) { const double previous=model->kind==PlotModel::Kind::XY ? points[i-1].x : (points[i-1].epoch-initial)*86400.; if (time-previous>20) { ++gaps; require(!point.connect,"Viewer joined the disabled ephemeris interval"); } }
      }
      require(gaps==1,"Mixed Toggle viewer resumption boundary missing"); require(model->kind==PlotModel::Kind::XY || model->endOfRun,"Disabled mixed Toggle viewer not finalized");
      if (name=="Orb") { require(!model->cameras.empty(),"Mixed Toggle camera history missing"); const auto &camera=model->cameras.back(); const auto &point=retained.back(); require(camera.frame==point.frame && std::abs(camera.target[0]-point.x)<1e-8 && std::abs(camera.target[1]-point.y)<1e-8 && std::abs(camera.target[2]-point.z)<1e-8,qPrintable(QString("Mixed Toggle camera endpoint not retained: latest frame %1 XYZ(%2,%3,%4), solver=%5, camera frame %6 target(%7,%8,%9), model frame %10").arg(point.frame).arg(point.x,0,'g',17).arg(point.y,0,'g',17).arg(point.z,0,'g',17).arg(point.solver).arg(camera.frame).arg(camera.target[0],0,'g',17).arg(camera.target[1],0,'g',17).arg(camera.target[2],0,'g',17).arg(model->frame))); }
      require(window.plotReceiver()->show(name),"Mixed Toggle viewer cannot reopen"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot && !plot->canvas()->captureImage().isNull(),"Mixed Toggle viewer cannot render"); const auto latest=plot->canvas()->captureImage();
      if (name!="XY") { auto *timeline=plot->findChild<QSlider *>("plotTimeline"); require(timeline,"Mixed Toggle replay missing"); timeline->setValue(0); QApplication::processEvents(); require(plot->canvas()->captureImage()!=latest,"Mixed Toggle replay cannot show earlier history"); timeline->setValue(timeline->maximum()); QApplication::processEvents(); require(plot->canvas()->captureImage()==latest,"Mixed Toggle Latest pixels changed"); }
      if (!capture.isEmpty()) require(latest.save(capture+"."+name+".png"),"Mixed Toggle native scene evidence failed"); const auto count=retained.size(); area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show(name) && window.plotReceiver()->model(name)==model && curve->points.size()==count,"Mixed Toggle close/reopen changed history");
      std::cout<<name.toStdString()<<" accepted="<<points.size()<<" retained="<<count<<" separated enabled arcs=2 terminal disabled\n";
   }
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtEphemerisToggle");
   QString format;
   try {
      const bool solver=argc>=3 && QString::fromLocal8Bit(argv[2])=="--solver";
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3 || (solver && argc==4)) && files.isValid(),"Ephemeris Toggle setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=solver ? (argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString()) : (argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString()); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Ephemeris Toggle runtime failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant();
      for (const auto &mode:QStringList{"CCSDS-OEM","STK-TimePosVel"}) {
         format=mode; const auto output=files.filePath(format+" Δ"+(format=="CCSDS-OEM" ? ".oem" : ".e")),report=files.filePath(format+"-states.txt"),saved=files.filePath(format+" Δ.script"),saveAs=files.filePath(format+" ü copy.script"),prefixCapture=capture.isEmpty() ? QString() : capture+"."+format;
         const QString prefix="% retain ephemeris/viewer Toggle source α\nCreate Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(std::sqrt(mu/7000.),'g',17)+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 1e-12;\nCreate EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.FileFormat = "+format+";\nExport.Filename = '"+output+"';\nExport.CoordinateSystem = EarthMJ2000Eq;\nExport.WriteEphemeris = false;\nExport.StepSize = IntegratorSteps;\n"+(format=="STK-TimePosVel" ? "Export.DistanceUnit = Kilometers;\nExport.IncludeEventBoundaries = true;\n" : QString())+"Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Sat;\nOrb.NumPointsToRedraw = 1;\nCreate GroundTrack Ground;\nGround.Add = {Sat};\nGround.NumPointsToRedraw = 1;\nCreate XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Sat.EarthMJ2000Eq.X};\nCreate ReportFile Idle Values;\nIdle.Filename = '"+files.filePath("idle.txt")+"';\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\nBeginMissionSequence;\n";
         const QString state="Report Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
         const QString mission="Toggle 'Start outputs' Idle On; % activate initially disabled ephemeris\nPropagate P(Sat) {Sat.ElapsedSecs = 120};\n"+state+"Toggle 'Pause outputs' Idle Off; % retain off\nPropagate P(Sat) {Sat.ElapsedSecs = 120};\n"+state+"Toggle 'Resume outputs' Idle On; % retain on\nPropagate P(Sat) {Sat.ElapsedSecs = 120};\n"+state+"Toggle 'Freeze outputs' Idle Off; % retain terminal off\nPropagate P(Sat) {Sat.ElapsedSecs = 60};\n"+state;
         auto setup=prefix,body=mission; const bool optimizer=solver && format=="STK-TimePosVel";
         if (solver) {
            setup.replace("Orb.Add = {Sat, Earth};","Orb.Add = {Sat, Earth};\nOrb.SolverIterations = Current;"); setup.replace("Ground.Add = {Sat};","Ground.Add = {Sat};\nGround.SolverIterations = Current;"); setup.replace("XY.XVariable = Sat.ElapsedSecs;","XY.SolverIterations = Current;\nXY.XVariable = Sat.ElapsedSecs;"); setup.replace("Values.Precision = 17;","Values.Precision = 17;\nValues.SolverIterations = All;");
            const auto resources=optimizer ? "Create Variable Alpha Cost;\nCreate Yukon Opt;\nOpt.ShowProgress = false;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nOpt.ReportFile = '"+files.filePath("optimizer.txt")+"';\n" : "Create Variable Alpha;\nCreate DifferentialCorrector DC;\nDC.ReportFile = '"+files.filePath("targeter.txt")+"';\n";
            setup.replace("BeginMissionSequence;",resources+"BeginMissionSequence;\n"+(optimizer ? "Optimize 'solver output scope' Opt" : "Target 'solver output scope' DC")+" {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"+(optimizer ? "Vary Opt(Alpha = 1, {Perturbation = 0.000001, Lower = 0, Upper = 3, MaxStep = 1});\n" : "Vary DC(Alpha = 1, {Perturbation = 0.01, Lower = 0, Upper = 3, MaxStep = 1});\n"));
            body+=optimizer ? "Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);\nEndOptimize; % retain solver end\n" : "Achieve DC(Alpha = 2, {Tolerance = 0.000001});\nEndTarget; % retain solver end\n";
         }
         const auto source=setup+body; auto reference=source; reference.replace("' Idle On","' Export Orb Ground XY On"); reference.replace("' Idle Off","' Export Orb Ground XY Off"); editor->setPlainText(reference); run(window); const auto expectedReport=read(report),expectedExport=read(output); const auto expectedRows=ephemeris(expectedExport,format,mu); const auto states=rows(expectedReport); require((!solver && states.size()==4) || (solver && states.size()>=8 && states.size()%4==0),"Independent mixed Toggle state report incomplete"); for (int i=0;i<states.size();++i) { const int arc=i%4; require(std::abs(states[i][0]-(arc<3 ? 120*(arc+1) : 420))<1e-4,"Mixed Toggle propagation epoch wrong"); circle(states[i],mu); }
         auto objective=[&] { if (!solver) return; auto *alpha=Moderator::Instance()->GetInternalObject("Alpha"); require(alpha && std::abs(alpha->GetRealParameter("Value")-2)<1e-5,"Solver ephemeris missed known goal/optimum 2"); if (optimizer) { auto *cost=Moderator::Instance()->GetInternalObject("Cost"); require(cost && std::abs(cost->GetRealParameter("Value"))<1e-10,"Solver ephemeris quadratic cost not minimized"); } }; objective();
         editor->setPlainText(source); require(window.buildScript(),"Ephemeris Toggle source build failed");
         for (const auto &label:QStringList{"Start outputs","Pause outputs","Resume outputs","Freeze outputs"}) {
            auto *child=open(window,label); const auto before=editor->toPlainText(); choose(child,false); require(!dynamic_cast<CommandEditor *>(child->widget())->hasChanges() && editor->toPlainText()==before,"Ephemeris Toggle Cancel changed pending/source"); choose(child,true,label=="Start outputs" && !prefixCapture.isEmpty() ? prefixCapture+".picker.png" : QString()); require(dynamic_cast<CommandEditor *>(child->widget())->hasChanges() && editor->toPlainText()==before,"Mixed Toggle applied before Apply");
            child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto changed=editor->toPlainText(); require(child->isVisible() && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Mixed Toggle did not retain clean Apply panel"); const auto expected=QString(before).replace("Toggle '"+label+"' Idle ","Toggle '"+label+"' Export Orb Ground XY "); require(changed==expected,"Mixed Toggle lost command labels/comments/unrelated source"); editor->undo(); require(editor->toPlainText()==before,"Mixed Toggle Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Mixed Toggle Redo not exact"); closePanels(window);
         }
         require(editor->toPlainText()==reference && window.saveScriptTo(saved) && window.saveScriptTo(saveAs) && read(saved)==reference.toUtf8() && read(saveAs)==reference.toUtf8() && window.loadScript(saveAs),"Mixed Toggle Unicode Save/Save As/reopen source not exact"); run(window); objective(); require(read(report)==expectedReport && ephemeris(read(output),format,mu)==expectedRows,"GUI mixed Toggle differs from independent complete state/ephemeris rows"); inspectPlots(window,prefixCapture,solver);
         auto *tree=window.findChild<QTreeWidget *>("Output"); const auto items=tree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive); QTreeWidgetItem *item=nullptr; for (auto *candidate:items) if (candidate->data(0,Qt::UserRole+1).toString()=="ephemeris") item=candidate; require(item && item->data(0,Qt::UserRole).toString()==output,"Toggled ephemeris missing/wrong Output path"); tree->itemDoubleClicked(item,0); auto views=window.findChildren<QPlainTextEdit *>("report:Export"); require(!views.isEmpty() && views.last()->toPlainText().toUtf8()==read(output),"Toggled ephemeris Output viewer text differs from file");
         auto *area=window.findChild<QMdiArea *>("workspace"); QApplication::processEvents(); require(area->viewport()->rect().contains(area->activeSubWindow()->geometry()),"New ephemeris report window outside workspace");
         auto *resources=window.findChild<QTreeWidget *>("Resources"); const auto exports=resources->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive); require(exports.size()==1,"Ephemeris resource tree entry missing"); QMap<QMdiSubWindow *,QRect> positions; for (auto *existing:area->subWindowList()) if (!existing->isMaximized() && !existing->isMinimized()) positions[existing]=existing->geometry(); resources->itemDoubleClicked(exports.first(),0); QApplication::processEvents(); auto *resource=area->activeSubWindow(); require(resource && dynamic_cast<ResourceEditor *>(resource->widget()) && area->viewport()->rect().contains(resource->geometry()),"New Ephemeris resource panel outside workspace"); for (auto it=positions.cbegin();it!=positions.cend();++it) require(it.key()->geometry()==it.value(),"New resource placement moved an existing viewer/report"); resource->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(editor->toPlainText()==reference,"Opening/closing Ephemeris resource panel changed source");
         auto *child=open(window,"Start outputs"); if (!prefixCapture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(window.grab().save(prefixCapture+".png"),"Native mixed Toggle panel evidence failed"); write(prefixCapture+".state.txt",expectedReport); write(prefixCapture+(format=="CCSDS-OEM" ? ".oem" : ".e"),read(output)); }
         child->widget()->findChild<QLineEdit *>("commandField_Subscribers")->setText("MissingOutput"); child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(editor->toPlainText()==reference && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Unknown Toggle subscriber did not stay pending with source rollback"); choose(child,true); require(!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Toggle checklist correction did not restore clean command"); closePanels(window); run(window); objective(); require(read(report)==expectedReport && ephemeris(read(output),format,mu)==expectedRows,"Mixed Toggle correction/repeat-run changed reports/ephemeris or left old segments");
         if (solver) std::cout<<(optimizer ? "Yukon Optimize" : "DifferentialCorrector Target")<<": Current viewers, accepted-only ephemeris, "<<states.size()<<" complete trial/accepted state rows, known Alpha=2"<<(optimizer ? " Cost<1e-10" : "")<<"; ";
         std::cout<<format.toStdString()<<": PASS actual MDI Ephemeris/Orbit/Ground/XY ordered Toggle selection, empty filtering/Cancel/pending/retained Apply, exact source/Undo/Redo/Unicode Save/Save As/reopen; initially disabled ephemeris activation, two enabled arcs (0-120/240-360), final Off through 420 s, every ephemeris state analytically circular and full script-reference rows/reports equal, segment metadata/finalization/Output access, invalid-subscriber rollback/correction and clean repeat-run; viewer suppression/replay/camera/close-reopen.\n";
         if (solver) {
            QMap<QString,std::deque<PlotPoint>> acceptedPaths;
            for (const auto &name:QStringList{"Orb","Ground","XY"}) { const auto model=window.plotReceiver()->model(name); for (const auto &curve:model->curves) if (curve.name=="Sat" || model->kind==PlotModel::Kind::XY) { for (const auto &point:curve.points) if (!point.solver) acceptedPaths[name].push_back(point); break; } }
            editor->setPlainText(QString(reference).replace("SolverIterations = Current;","SolverIterations = None;")); run(window); objective(); require(read(report)==expectedReport && ephemeris(read(output),format,mu)==expectedRows,"Independent None display changed complete solver report/ephemeris");
            for (const auto &name:QStringList{"Orb","Ground","XY"}) { const auto model=window.plotReceiver()->model(name); const PlotCurve *curve=nullptr; for (const auto &candidate:model->curves) if (candidate.name=="Sat" || model->kind==PlotModel::Kind::XY) { curve=&candidate; break; } const auto &accepted=acceptedPaths[name]; require(curve && curve->points.size()==accepted.size() && accepted.size()==26,"Current accepted mixed history count differs from None"); for (size_t i=0;i<accepted.size();++i) { const auto &point=curve->points[i]; const auto &expected=accepted[i]; require(!point.solver && point.connect==expected.connect && std::abs(point.epoch-expected.epoch)<1e-10 && std::abs(point.x-expected.x)<1e-8 && std::abs(point.y-expected.y)<1e-8 && std::abs(point.z-expected.z)<1e-8,"Current accepted mixed history differs from independent None"); } }
            std::cout<<format.toStdString()<<": PASS independently filtered None Orbit/Ground/XY accepted histories equal all 26 Current accepted epochs/XYZ/connection boundaries; full solver report/ephemeris unchanged.\n";
         }
      }
   } catch (BaseException &error) { std::cerr<<"FAIL "<<format.toStdString()<<": "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL "<<format.toStdString()<<": "<<error.what()<<'\n'; return 1; }
}
