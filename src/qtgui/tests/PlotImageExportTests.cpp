#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTimer>
#include <sys/resource.h>
#include <signal.h>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Image evidence file unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Image fixture write failed"); }
struct FileSizeLimit {
   rlimit saved; decltype(SIG_IGN) handler;
   FileSizeLimit() { require(getrlimit(RLIMIT_FSIZE,&saved)==0,"Cannot read image write limit"); handler=signal(SIGXFSZ,SIG_IGN); require(handler!=SIG_ERR,"Cannot handle image write failure"); auto limit=saved; limit.rlim_cur=128; require(setrlimit(RLIMIT_FSIZE,&limit)==0,"Cannot set image write limit"); }
   ~FileSizeLimit() { setrlimit(RLIMIT_FSIZE,&saved); signal(SIGXFSZ,handler); }
};
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPlotImageExport");
   try {
      TestSettings settings; QTemporaryDir files; require(argc==2 && files.isValid(),"Image export setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Image export runtime unavailable"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *receiver=window.plotReceiver(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto mission=files.filePath("protected Δ.script"),report=files.filePath("protected report.txt"),ephemeris=files.filePath("protected ephemeris.eph");
      const QString source="% preserve source during image export α\nCreate Spacecraft Sat;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nCreate EphemerisFile State;\nState.Spacecraft = Sat;\nState.Filename = '"+ephemeris+"';\nBeginMissionSequence;\n";
      editor->setPlainText(source); require(window.buildScript() && window.saveScriptTo(mission),"Image source/output protection setup failed"); write(report,"retain engine report bytes\n"); write(ephemeris,"retain ephemeris bytes\n");
      const auto startupBytes=read(startup); const auto alias=files.filePath("mission alias.script"); require(QFile::link(mission,alias),"Image alias fixture unavailable");
      receiver->CreateXyPlotWindow("ImageXY","",0,0,0,0,false,"Image XY","Time","Value",true); receiver->AddXyPlotCurve("ImageXY",0,"Curve",0xff0000); receiver->UpdateXyPlotCurve("ImageXY",0,1,2); receiver->UpdateXyPlotCurve("ImageXY",0,2,5);
      receiver->CreateGroundTrackWindow("ImageGround","","Image ground",0,0,0,0,false); receiver->CreateGlPlotWindow("ImageOrbit","",0,0,0,0,false,0);
      for (const auto &name:QStringList{"ImageXY","ImageGround","ImageOrbit"}) {
         require(receiver->show(name),"Image export MDI viewer missing"); auto model=receiver->model(name); auto *child=area->activeSubWindow(); auto *plot=dynamic_cast<PlotWidget *>(child->widget()); require(plot && plot->findChild<QAction *>("plotSaveImage"),"Image action missing"); QApplication::processEvents(); const auto expected=plot->canvas()->captureImage(); require(!expected.isNull(),"Image canvas capture unavailable"); const auto generation=model->historyGeneration,frame=model->frame; const auto count=model->curves.size(); const auto output=files.filePath(name+" Δ.png");
         auto chooser=[&](bool accept) { std::exception_ptr failure; QTimer::singleShot(0,plot,[&] { auto *dialog=dynamic_cast<QFileDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Image save chooser missing"); dialog->selectFile(output); if (accept) QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); else dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); plot->findChild<QAction *>("plotSaveImage")->trigger(); if (failure) std::rethrow_exception(failure); };
         chooser(false); require(!QFileInfo::exists(output),"Image Cancel wrote a file"); chooser(true); require(QImage(output).convertToFormat(QImage::Format_ARGB32)==expected.convertToFormat(QImage::Format_ARGB32),"Saved PNG differs from current canvas"); const auto png=read(output); require(png.size()>128,"Image fixture cannot exceed induced write limit");
         for (const auto &path:QStringList{mission,alias,report,ephemeris,startup}) require(!plot->exportImage(path).isEmpty(),"Image export overwrote protected mission/output/startup");
         require(!plot->exportImage({}).isEmpty() && !plot->exportImage(files.filePath("missing/image.png")).isEmpty(),"Invalid image destination accepted");
         const auto failed=files.filePath(name+" failure.png"); write(failed,"keep previous destination\n"); QString error; { FileSizeLimit limit; error=plot->exportImage(failed); } require(!error.isEmpty() && read(failed)=="keep previous destination\n","Image post-write failure replaced previous destination"); require(plot->exportImage(failed).isEmpty() && read(failed)==png,"Image save did not recover after write failure");
         require(model->historyGeneration==generation && model->frame==frame && model->curves.size()==count && plot->canvas()->captureImage()==expected,"Image export changed retained model/display");
         child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(receiver->show(name) && receiver->model(name).get()==model.get(),"Image viewer reopen lost retained model"); plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot->exportImage(output).isEmpty() && !QImage(output).isNull(),"Reopened image export failed");
      }
      require(editor->toPlainText()==source && read(mission)==source.toUtf8() && read(report)=="retain engine report bytes\n" && read(ephemeris)=="retain ephemeris bytes\n" && read(startup)==startupBytes,"Image export changed protected source/output/startup");
      std::cout<<"PASS: actual MDI XY/Ground/Orbit Save image chooser Cancel/accept to Unicode PNG, decoded pixel equality with current canvas, live mission/report/ephemeris/startup and symlink protection, empty/missing destination diagnosis, induced post-write failure preserves old destination and recovers, retained model/display and close/reopen. Synthetic viewer data; no numerical or native renderer requalification.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
