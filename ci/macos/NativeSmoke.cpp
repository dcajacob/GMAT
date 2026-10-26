#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QWindow>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
static void settle(int milliseconds) { QEventLoop e; QTimer::singleShot(milliseconds,&e,&QEventLoop::quit);e.exec(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if(argc!=4) return 2;
   const QString out=QString::fromLocal8Bit(argv[3]);QDir().mkpath(out);
   QApplication::setOrganizationName("GMAT-CI");QApplication::setApplicationName("GMAT Qt macOS native smoke");
   QSettings::setDefaultFormat(QSettings::IniFormat);
   QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,out+"/harness-settings");
   QJsonObject receipt{{"qt",qVersion()},{"qpa",QGuiApplication::platformName()},
      {"gui","NOT_RUN"},{"mission","NOT_RUN"},{"opengl","NOT_RUN"},{"orbit_pixels","NOT_RUN"},
      {"camera","NOT_RUN"},{"replay","NOT_RUN"},{"close_reopen","NOT_RUN"},{"normal_exit","NOT_RUN"}};
   auto save=[&] { QFile f(out+"/native.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(receipt).toJson()); };
   save();
   QTimer deadline;deadline.setSingleShot(true);
   QObject::connect(&deadline,&QTimer::timeout,[&] {receipt["failure"]="120-second native harness deadline";save();std::_Exit(124);});deadline.start(115000);
   int code=1;
   // Qt emits lastWindowClosed only while QGuiApplication::exec is active.
   // Nested QEventLoop instances used for rendering do not satisfy that contract.
   QTimer::singleShot(0,&app,[&] {
   try {
      require(QGuiApplication::platformName()=="cocoa","Native Cocoa platform required; offscreen does not qualify");
      MainWindow window;window.resize(1180,800);window.show();settle(300);
      require(window.windowHandle() && window.windowHandle()->isExposed(),"Cocoa top-level window is not exposed");
      receipt["gui"]="PASS";save();window.grab().save(out+"/native-before.png");
      require(window.initialize(QString::fromLocal8Bit(argv[1])),"GMAT engine initialization failed");
      require(window.loadScript(QString::fromLocal8Bit(argv[2])) && window.buildScript(),"Small mission load/build failed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Small mission did not complete");
      receipt["mission"]="PASS";save();
      if(auto *messages=window.findChild<QPlainTextEdit *>("messageWindow")) {
         QFile f(out+"/mission-messages.txt");if(f.open(QIODevice::WriteOnly))f.write(messages->toPlainText().toUtf8());
      }
      auto *receiver=window.plotReceiver();auto model=receiver->model("CiOrbit");
      require(model && model->frame>5,"Orbit contains insufficient propagated samples");
      receipt["propagated_frames"]=static_cast<double>(model->frame);
      require(receiver->show("CiOrbit"),"Orbit viewer did not open");settle(700);
      auto findPlot=[&]() -> PlotWidget * {
         for(auto *child:receiver->workspaceArea()->subWindowList())
            if(child->property("plotName").toString()=="CiOrbit") return dynamic_cast<PlotWidget *>(child->widget());
         return nullptr;
      };
      auto *plot=findPlot();require(plot && plot->isVisible(),"Actual orbit widget is not visible");
      OrbitRenderer *renderer=nullptr;
      for(auto *w:plot->findChildren<QOpenGLWidget *>()) if((renderer=dynamic_cast<OrbitRenderer *>(w))) break;
      if(!renderer || !renderer->context() || !renderer->context()->isValid()) {
         receipt["opengl"]="UNSUPPORTED_CONTEXT";save();throw std::runtime_error("Runner has no valid native OpenGL context; 3D unqualified");
      }
      renderer->makeCurrent();auto *gl=renderer->context()->functions();
      auto str=[&](GLenum key){const auto *s=gl->glGetString(key);return s?QString::fromLatin1(reinterpret_cast<const char *>(s)):QString();};
      receipt["gl_vendor"]=str(GL_VENDOR);receipt["gl_renderer"]=str(GL_RENDERER);receipt["gl_version"]=str(GL_VERSION);
      renderer->doneCurrent();require(!receipt["gl_version"].toString().isEmpty(),"OpenGL returned no version");
      receipt["opengl"]="PASS";save();
      auto capture=[&](const QString &name) { auto image=renderer->captureImage();require(!image.isNull(),"Orbit framebuffer is null");require(image.save(out+"/"+name+".png"),"Orbit image save failed");return image; };
      const auto initial=capture("orbit");
      int earth=0,trajectory=0;
      for(int y=0;y<initial.height();++y)for(int x=0;x<initial.width();++x) {
         const auto p=initial.pixelColor(x,y);
         if(p.blue()>35 && p.blue()>p.red()*1.25 && p.blue()>p.green()*1.05)++earth;
         if(p.red()>100 && p.red()>p.green()*1.5 && p.red()>p.blue()*1.5)++trajectory;
      }
      receipt["earth_pixels"]=earth;receipt["trajectory_pixels"]=trajectory;
      require(earth>500 && trajectory>15,"Textured Earth or trajectory pixels absent; blank context is not a pass");
      receipt["orbit_pixels"]="PASS";save();
      plot->canvas()->setViewAngles(1.1,.15);plot->canvas()->zoomBy(1);settle(300);
      require(capture("orbit-camera")!=initial,"Camera adjustment did not alter rendered image");receipt["camera"]="PASS";
      plot->canvas()->setFrame(0);settle(200);auto early=capture("orbit-early");
      plot->canvas()->setFrame(model->frame);settle(200);require(capture("orbit-latest")!=early,"Replay frames did not alter rendered image");receipt["replay"]="PASS";save();
      for(auto *child:receiver->workspaceArea()->subWindowList())if(child->property("plotName").toString()=="CiOrbit"){child->close();break;}
      settle(200);require(receiver->show("CiOrbit"),"Closed orbit did not reopen");settle(400);require(findPlot() && findPlot()->isVisible(),"Reopened orbit is not visible");receipt["close_reopen"]="PASS";
      window.grab().save(out+"/native-final.png");
      // Capture WindowServer's actual main display, including the system menu
      // bar/desktop. This is not a QWidget grab or a composited decoration.
      window.setGeometry(window.screen()->availableGeometry().adjusted(30,30,-30,-40));
      window.raise();window.activateWindow();settle(800);
      receipt["desktop_application_active"]=QGuiApplication::applicationState()==Qt::ApplicationActive;
      QProcess screenCapture;
      screenCapture.start("/usr/sbin/screencapture",{"-x","-m",out+"/macos-desktop.png"});
      require(screenCapture.waitForFinished(10000),"Native desktop capture did not finish within ten seconds");
      receipt["desktop_capture_exit"]=screenCapture.exitCode();
      QFile captureLog(out+"/desktop-capture.log");
      if(captureLog.open(QIODevice::WriteOnly))captureLog.write(screenCapture.readAllStandardError());
      if(screenCapture.exitStatus()!=QProcess::NormalExit || screenCapture.exitCode()!=0) {
         receipt["desktop_capture"]="UNAVAILABLE";save();
         throw std::runtime_error("Runner desktop capture unavailable; retained error without changing screen-recording permissions");
      }
      receipt["desktop_capture"]="CAPTURED_REQUIRES_VISUAL_INSPECTION";save();
      bool closed=false;QObject::connect(&app,&QGuiApplication::lastWindowClosed,[&]{closed=true;});
      require(window.close(),"Normal main-window close was rejected");app.processEvents();require(closed,"Normal close did not emit lastWindowClosed");receipt["normal_exit"]="PASS";save();code=0;
   }catch(const std::exception &e){receipt["failure"]=e.what();save();std::cerr<<e.what()<<std::endl;}
   receipt["exit"]=code;save();
   if(code) app.exit(code); // Success must exit through the normal last-window path.
   });
   const int eventLoopExit=app.exec();
   receipt["application_event_loop_exit"]=eventLoopExit;save();return eventLoopExit;
}
