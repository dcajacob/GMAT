#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSlider>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Invalid-data report missing"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Invalid-data evidence write failed"); }
static void run(MainWindow &window) { const auto result=window.runMission(); QApplication::processEvents(); require(result==MainWindow::RunResult::Completed && !window.isRunning(),qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText())); }
static const PlotCurve &curve(const PlotModel &model) { for (const auto &item:model.curves) if (item.name=="Sat" || model.kind==PlotModel::Kind::XY) return item; throw std::runtime_error("Invalid-data spacecraft curve absent"); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtInvalidPlotData");
   try {
      TestSettings settings; QTemporaryDir files; require((argc==3 || argc==4) && files.isValid(),"Invalid-data fixture setup failed");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),kernel=QFileInfo(argv[2]).absoluteFilePath(),capture=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString(); require(QFileInfo(kernel).isFile(),"Qualified two-arc SPK fixture unavailable"); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Invalid-data runtime initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("gap report.txt"),saved=files.filePath("gap Δ.script");
      const QString resources="% preserve unavailable reader state evidence α\nCreate Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.NAIFId = -100055;\nSat.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator Read;\nRead.Type = SPK;\nRead.StepSize = 10;\nRead.StartEpoch = 'FromSpacecraft';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\n";
      const QString state="Report Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const QString mission="BeginMissionSequence;\nPropagate Read(Sat) {Sat.ElapsedSecs = 60};\n"+state+"Propagate Read(Sat) {Sat.ElapsedSecs = 120};\n"+state+"Propagate Read(Sat) {Sat.ElapsedSecs = 120};\n"+state;
      editor->setPlainText(resources+mission); run(window); const auto expected=read(report);
      QVector<QVector<double>> states; for (const auto &line:QString::fromUtf8(expected).trimmed().split('\n')) { QVector<double> row; for (const auto &field:line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok=false; row.append(field.toDouble(&ok)); require(ok,"Unavailable-state report is not numeric"); } states.append(row); }
      require(states.size()==3 && states[0].size()==7 && states[1].size()==7 && states[2].size()==7,"Independent gap report incomplete"); require(std::abs(states[0][0]-60)<.001 && std::abs(states[1][0]-180)<.001 && std::abs(states[2][0]-300)<.001,"Gap reference epochs wrong");
      for (int column=1;column<7;++column) require(states[1][column]<-1e300,"SPK gap did not reproduce unavailable state marker");
      if (!capture.isEmpty()) write(capture+".reference.txt",expected);
      for (const auto kind:{PlotModel::Kind::XY,PlotModel::Kind::Orbit}) {
         PlotModel model(kind); model.frame=1; model.append(0,states[0][1],states[0][2],kind==PlotModel::Kind::Orbit ? states[0][3] : 0);
         model.frame=2; model.append(0,states[1][1],states[1][2],kind==PlotModel::Kind::Orbit ? states[1][3] : 0);
         require(model.curves[0].points.size()==1,"Unavailable finite reader marker entered retained plot history");
         model.frame=3; model.append(0,states[2][1],states[2][2],kind==PlotModel::Kind::Orbit ? states[2][3] : 0); require(model.curves[0].points.size()==2 && !model.curves[0].points.back().connect,"Recovered valid sample joined unavailable history");
      }
      const QString plots="Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Sat;\nOrb.NumPointsToRedraw = 1;\nCreate GroundTrack Ground;\nGround.Add = {Sat};\nGround.NumPointsToRedraw = 1;\nCreate XYPlot XY;\nXY.XVariable = Sat.ElapsedSecs;\nXY.YVariables = {Sat.EarthMJ2000Eq.X};\n";
      auto source=resources+plots+mission; QtCameraSetting settingsCamera; settingsCamera.primaryName="Tracked"; QtCameraPreset alternate; alternate.name="Alternate"; alternate.reference="Earth"; alternate.target="Sat"; alternate.eye={0,-20000,0}; settingsCamera.views.append(alternate); source=setQtCameraSetting(source,"Orb",settingsCamera);
      editor->setPlainText(source); require(window.saveScriptTo(saved) && read(saved)==source.toUtf8() && window.loadScript(saved),"Unavailable-data source Unicode round trip changed bytes"); run(window); require(read(report)==expected && editor->toPlainText()==source,"Viewer filtering changed numerical report/source");
      auto *area=window.findChild<QMdiArea *>("workspace"); const double initial=window.plotReceiver()->model("Orb")->curves[0].points.front().epoch; int validCounts[3]={}; int which=0;
      for (const auto &name:QStringList{"Orb","Ground","XY"}) {
         auto model=window.plotReceiver()->model(name); require(model && !curve(*model).points.empty(),"Recovered valid plot history missing"); const auto &points=curve(*model).points; bool resumed=false;
         for (const auto &point:points) { const double time=model->kind==PlotModel::Kind::XY ? point.x : (point.epoch-initial)*86400.; require(!(time>120.001 && time<239.999),"Unavailable gap produced a fake trajectory sample"); require(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z) && std::abs(point.x)<1e9 && std::abs(point.y)<1e9 && std::abs(point.z)<1e9,"Invalid coordinates survived viewer filtering"); if (time>=239.999 && !resumed) { require(!point.connect,"Recovered trajectory bridges unavailable SPK interval"); resumed=true; } }
         require(resumed && points.size()>=18,"Both valid SPK arcs were not retained"); validCounts[which++]=points.size();
         if (name=="Orb") { require(!model->cameras.empty() && model->cameraViews.size()==2 && !model->cameraViews[1].cameras.empty(),"Tracked/alternate camera history missing"); for (const auto *history:{&model->cameras,&model->cameraViews[1].cameras}) for (const auto &camera:*history) { for (int axis=0;axis<3;++axis) require(std::isfinite(camera.eye[axis]) && std::isfinite(camera.target[axis]) && std::abs(camera.eye[axis])<1e9 && std::abs(camera.target[axis])<1e9,"Unavailable state entered scripted camera history"); } const auto &last=model->cameras.back(); require(last.frame==points.back().frame && std::abs(last.target[0]-points.back().x)<1e-8 && std::abs(last.target[1]-points.back().y)<1e-8 && std::abs(last.target[2]-points.back().z)<1e-8,"Recovered camera does not follow final valid state"); }
         require(window.plotReceiver()->show(name),"Invalid-data viewer cannot reopen"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot,"Invalid-data viewer widget missing"); const auto latest=plot->canvas()->captureImage(); require(!latest.isNull(),"Recovered history failed to render");
         if (name!="XY") { auto *timeline=plot->findChild<QSlider *>("plotTimeline"); timeline->setValue(0); QApplication::processEvents(); require(plot->canvas()->captureImage()!=latest,"Unavailable-data replay did not change pixels"); timeline->setValue(timeline->maximum()); QApplication::processEvents(); require(plot->canvas()->captureImage()==latest,"Latest changed valid recovered history"); }
         if (name=="Orb") { auto *selector=plot->findChild<QComboBox *>("orbitCameraView"); require(selector && selector->count()==2,"Alternate camera selector missing"); selector->setCurrentIndex(1); require(plot->canvas()->captureImage()!=latest,"Alternate camera cannot render retained valid history"); selector->setCurrentIndex(0); }
         if (!capture.isEmpty()) require(latest.save(capture+"."+name+".png"),"Unavailable-data scene capture failed"); const auto count=points.size(); area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show(name) && window.plotReceiver()->model(name)==model && curve(*model).points.size()==count,"Unavailable-data close/reopen lost history");
         std::cout<<name.toStdString()<<" valid retained="<<count<<"; SPK unavailable interval omitted with separated resumed arc.\n";
      }
      require(validCounts[0]==validCounts[1] && validCounts[1]==validCounts[2],"Orbit/Ground/XY disagree on available trajectory sample count");
      if (!capture.isEmpty()) { write(capture+".viewer.txt",read(report)); write(capture+".messages.txt",window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toUtf8()); QEventLoop exposed; QTimer::singleShot(200,&exposed,&QEventLoop::quit); exposed.exec(); require(window.grab().save(capture+".png"),"Unavailable-data workspace capture failed"); }
      run(window); require(read(report)==expected && editor->toPlainText()==source,"Unavailable-data rerun changed numerical/source evidence"); which=0; for (const auto &name:QStringList{"Orb","Ground","XY"}) require(curve(*window.plotReceiver()->model(name)).points.size()==validCounts[which++],"Unavailable-data rerun accumulated/lost valid points");
      std::cout<<"PASS finite SPK unavailable marker omitted from XY/Orbit/primary+alternate cameras and Ground conversion; actual reader retains both valid arcs, reports unchanged including invalid engine row, exact Unicode reopen, native replay/close-reopen and clean rerun. This does not qualify numerical gap traversal.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
