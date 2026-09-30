#include "MainWindow.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QWindow>
#include <QPlatformSurfaceEvent>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTimer>
#include <QStyleOptionTitleBar>
#include <QStyle>
#include <QMouseEvent>
#include <QTreeWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <iostream>
#include <memory>

class SurfaceObserver final : public QObject
{
public:
   int destroyed=0;
   bool eventFilter(QObject *object,QEvent *event) override {
      if (event->type()==QEvent::PlatformSurface &&
          static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType()==QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) ++destroyed;
      if (event->type()==QEvent::WindowStateChange) {
         if (auto *widget=qobject_cast<QWidget *>(object)) std::cerr<<"Window state="<<int(widget->windowState())<<std::endl;
      }
      return false;
   }
};

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (argc<2) return 2;
   QApplication::setOrganizationName("GMAT"); QApplication::setApplicationName("GMAT Qt 6");
   std::unique_ptr<TestSettings> settings;
   if (argc<3 || QString(argv[2])!="--desktop-settings") settings=std::make_unique<TestSettings>();
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   MainWindow window; window.show();
   SurfaceObserver surface; window.windowHandle()->installEventFilter(&surface);
   window.installEventFilter(&surface);
   if (!window.initialize(startup)) return 1;
   QTemporaryDir output;
   FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",(output.path()+"/").toStdString());
   QElapsedTimer elapsed; elapsed.start();
   auto log=[&](const char *text) { std::cerr<<elapsed.elapsed()<<" ms: "<<text<<std::endl; };
   auto *area=window.findChild<QMdiArea *>("workspace");
   auto plot=[&](const QString &name)->QMdiSubWindow * {
      for (auto *child:area->subWindowList()) if (child->property("plotName")==name) return child;
      return nullptr;
   };
   auto clickMinimize=[&](QMdiSubWindow *child) {
      QStyleOptionTitleBar option; option.initFrom(child);
      option.titleBarFlags=child->windowFlags(); option.titleBarState=child->windowState();
      option.rect=QRect(0,0,child->width(),child->style()->pixelMetric(QStyle::PM_TitleBarHeight,&option,child));
      const auto button=child->style()->subControlRect(QStyle::CC_TitleBar,&option,QStyle::SC_TitleBarMinButton,child);
      const QPointF local=button.center(),global=child->mapToGlobal(button.center());
      QMouseEvent move(QEvent::MouseMove,local,global,Qt::NoButton,Qt::NoButton,Qt::NoModifier);
      QApplication::sendEvent(child,&move);
      QMouseEvent press(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
      QApplication::sendEvent(child,&press);
      QTimer::singleShot(50,child,[child,local,global] {
         QMouseEvent release(QEvent::MouseButtonRelease,local,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
         QApplication::sendEvent(child,&release);
      });
   };
   auto openOutput=[&](const QString &name) {
      auto *tree=window.findChild<QTreeWidget *>("Output");
      for (auto *tabs:window.findChildren<QTabWidget *>()) if (tabs->indexOf(tree)>=0) tabs->setCurrentWidget(tree);
      const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
      if (items.size()!=1) return false;
      tree->scrollToItem(items.first()); QApplication::processEvents();
      const QPointF position=tree->visualItemRect(items.first()).center();
      for (auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease,QEvent::MouseButtonDblClick,QEvent::MouseButtonRelease}) {
         QMouseEvent event(type,position,tree->viewport()->mapToGlobal(position.toPoint()),Qt::LeftButton,
            type==QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,Qt::NoModifier);
         QApplication::sendEvent(tree->viewport(),&event);
      }
      auto *child=plot(name);
      // QMdiArea reports no active child while the application is inactive.
      // currentSubWindow still identifies the viewer selected through Output.
      auto *selected=area->currentSubWindow();
      if (!child || selected!=child || child->isMinimized())
         std::cerr<<"Output activation failed for "<<name.toStdString()<<" at "<<position.x()<<","<<position.y()
            <<" current="<<(selected ? selected->windowTitle().toStdString() : "none")
            <<" viewport="<<tree->viewport()->width()<<"x"<<tree->viewport()->height()<<std::endl;
      return child && selected==child && !child->isMinimized() &&
         area->subWindowList(QMdiArea::StackingOrder).last()==child;
   };
   auto scene=[&](const QString &name,int minimumOcean=1000,int minimumTrajectory=100) {
      auto *child=plot(name);
      auto *view=child ? dynamic_cast<PlotWidget *>(child->widget()) : nullptr;
      const auto model=window.plotReceiver()->model(name);
      if (!view || !model || (model->kind==PlotModel::Kind::GroundTrack && model->map.isNull())) return false;
      bool samples=false;
      for (const auto &curve:model->curves) if (curve.name=="DefaultSC" && curve.points.size()>100) samples=true;
      if (!samples) return false;
      const auto pixels=view->canvas()->captureImage();
      int ocean=0,trajectory=0;
      for (int y=0;y<pixels.height();++y) for (int x=0;x<pixels.width();++x) {
         const auto color=pixels.pixelColor(x,y);
         if (color.blue()>color.red()*2 && color.blue()>50 && color.green()>25) ++ocean;
         if (color.red()>150 && color.green()<100 && color.blue()<100) ++trajectory;
      }
      std::cerr<<name.toStdString()<<" ocean="<<ocean<<" trajectory="<<trajectory<<std::endl;
      if (argc>3 && name=="DefaultOrbitView") pixels.save(argv[3]);
      return ocean>=minimumOcean && trajectory>=minimumTrajectory;
   };
   const auto source=window.findChild<QPlainTextEdit *>("scriptEditor")->toPlainText();
   const bool minimizeMain=!(argc>4 && QString(argv[4])=="--skip-main-minimize");
   int stage=0,cycle=0;
   QTimer actions;
   actions.setInterval(500);
   QObject::connect(&actions,&QTimer::timeout,&window,[&] {
      auto *ground=plot("DefaultGroundTrackPlot"),*orbit=plot("DefaultOrbitView");
      if (!ground || !orbit) { log("FAIL: missing default plot"); app.exit(1); return; }
      switch(stage++) {
      case 0: log("minimize ground"); clickMinimize(ground); log("minimize click sent"); break;
      case 1: {
         if (!ground->isMinimized()) { log("FAIL: title-bar minimize did not minimize"); app.exit(1); return; }
         log("open orbit from Output");
         if (!openOutput("DefaultOrbitView")) { log("FAIL: Output did not open orbit"); app.exit(1); return; }
         log("open returned"); break;
      }
      case 2: {
         if (area->activeSubWindow()!=orbit || orbit->isMinimized()) { log("FAIL: orbit not active"); app.exit(1); return; }
         // A visible window alone can pass after a GPU reset while its scene is
         // blank. Require both the blue Earth texture and the red orbit.
         if (!scene("DefaultOrbitView")) { log("FAIL: default OrbitView lacks textured Earth or trajectory"); app.exit(1); return; }
         log("restore ground"); ground->showNormal(); area->setActiveSubWindow(ground); ground->raise(); break;
      }
      case 3: log("minimize ground again"); clickMinimize(ground); break;
      case 4:
         if (surface.destroyed) { log("FAIL: plot interaction replaced the native window"); app.exit(1); return; }
         if (!ground->isMinimized()) { log("FAIL: repeated minimize did not minimize"); app.exit(1); return; }
         log("resize main and both viewers"); ground->showNormal(); orbit->showNormal();
         window.resize(900,640); ground->resize(420,320); orbit->resize(420,320); break;
      case 5:
         if (!scene("DefaultOrbitView",200,20) || !scene("DefaultGroundTrackPlot",200,20)) {
            log("FAIL: resized viewers lost textures or trajectory"); app.exit(1); return;
         }
         window.resize(1280,900); ground->resize(700,480); orbit->resize(700,480);
         log("maximize orbit"); orbit->showMaximized(); area->setActiveSubWindow(orbit); break;
      case 6:
         if (!scene("DefaultOrbitView")) { log("FAIL: maximized orbit lost scene"); app.exit(1); return; }
         log("close orbit and reopen immediately through Output"); orbit->close();
         if (!openOutput("DefaultOrbitView")) { log("FAIL: closed orbit did not reopen"); app.exit(1); return; }
         break;
      case 7:
         if (!scene("DefaultOrbitView")) { log("FAIL: reopened orbit lost scene"); app.exit(1); return; }
         log("close ground and reopen immediately through Output"); ground->close();
         if (!openOutput("DefaultGroundTrackPlot")) { log("FAIL: closed ground did not reopen"); app.exit(1); return; }
         break;
      case 8:
         if (!scene("DefaultGroundTrackPlot")) { log("FAIL: reopened ground lost scene"); app.exit(1); return; }
         // Allow Output's focus/activation requests to settle before asking the
         // compositor to minimize the top-level window.
         log("ground scene retained; settling focus before main minimize"); break;
      case 9:
         if (minimizeMain) { log("minimize main window"); window.showMinimized(); }
         else log("UNQUALIFIED: top-level compositor minimize/restore skipped; viewer lifecycle checks continue");
         break;
      case 10:
         if (minimizeMain) {
            if (!window.isMinimized()) { log("FAIL: main window did not minimize"); app.exit(1); return; }
            log("restore main window"); window.showNormal(); window.raise(); window.activateWindow();
         }
         break;
      case 11:
         if (!openOutput("DefaultOrbitView") || !scene("DefaultOrbitView")) {
            log("FAIL: main-window restore lost orbit"); app.exit(1); return;
         }
         if (surface.destroyed) { log("FAIL: lifecycle replaced native window"); app.exit(1); return; }
         log("rerun with minimized ground and closed orbit"); ground->showMinimized(); orbit->close();
         if (window.runMission()!=MainWindow::RunResult::Completed) { log("FAIL: rerun failed"); app.exit(1); return; }
         break;
      case 12:
         if (!openOutput("DefaultOrbitView") || !scene("DefaultOrbitView") ||
             !openOutput("DefaultGroundTrackPlot") || !scene("DefaultGroundTrackPlot")) {
            log("FAIL: rerun lost rendered output"); app.exit(1); return;
         }
         if (surface.destroyed || window.findChild<QPlainTextEdit *>("scriptEditor")->toPlainText()!=source) {
            log("FAIL: lifecycle changed native surface or mission source"); app.exit(1); return;
         }
         if (++cycle<3) { stage=0; log("repeat viewer lifecycle"); }
         else {
            log(minimizeMain ? "PASS: three viewer lifecycle cycles, textured orbit/map and trajectories, immediate close/reopen, resize/maximize, main minimize/restore and rerun remain responsive" :
               "PASS: three viewer lifecycle cycles, textured orbit/map and trajectories, immediate close/reopen, resize/maximize and rerun remain responsive; top-level minimize/restore remains unqualified");
            app.exit(0);
         }
         break;
      }
   });
   QTimer::singleShot(0,&window,[&] {
      log("run default mission");
      if (window.runMission()!=MainWindow::RunResult::Completed) { app.exit(1); return; }
      log("mission completed");
      if (surface.destroyed) { log("FAIL: first OrbitView destroyed the live native window"); app.exit(1); return; }
      actions.start();
   });
   return app.exec();
}
