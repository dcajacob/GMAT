#include "MainWindow.hpp"
#include "PlotWidget.hpp"
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
#include <iostream>
#include <memory>

class SurfaceObserver final : public QObject
{
public:
   int destroyed=0;
   bool eventFilter(QObject *,QEvent *event) override {
      if (event->type()==QEvent::PlatformSurface &&
          static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType()==QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) ++destroyed;
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
   int stage=0;
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
         auto *tree=window.findChild<QTreeWidget *>("Output");
         for (auto *tabs:window.findChildren<QTabWidget *>()) if (tabs->indexOf(tree)>=0) tabs->setCurrentWidget(tree);
         auto items=tree->findItems("DefaultOrbitView",Qt::MatchExactly|Qt::MatchRecursive);
         if (items.size()!=1) { app.exit(1); return; }
         tree->itemDoubleClicked(items.first(),0); log("open returned"); break;
      }
      case 2: {
         if (area->activeSubWindow()!=orbit || orbit->isMinimized()) { log("FAIL: orbit not active"); app.exit(1); return; }
         auto *view=dynamic_cast<PlotWidget *>(orbit->widget());
         if (!view) { log("FAIL: OrbitView widget missing"); app.exit(1); return; }
         // A visible window alone can pass after a GPU reset while its scene is
         // blank. Require both the blue Earth texture and the red orbit.
         const auto pixels=view->canvas()->captureImage();
         if (argc>3) pixels.save(argv[3]);
         int ocean=0,trajectory=0;
         for (int y=0;y<pixels.height();++y) for (int x=0;x<pixels.width();++x) {
            const auto color=pixels.pixelColor(x,y);
            if (color.blue()>color.red()*2 && color.blue()>50 && color.green()>25) ++ocean;
            if (color.red()>150 && color.green()<100 && color.blue()<100) ++trajectory;
         }
         std::cerr<<"Rendered ocean="<<ocean<<" trajectory="<<trajectory<<std::endl;
         if (ocean<1000 || trajectory<100) { log("FAIL: default OrbitView lacks textured Earth or trajectory"); app.exit(1); return; }
         log("restore ground"); ground->showNormal(); area->setActiveSubWindow(ground); ground->raise(); break;
      }
      case 3: log("minimize ground again"); clickMinimize(ground); break;
      case 4:
         if (surface.destroyed) { log("FAIL: plot interaction replaced the native window"); app.exit(1); return; }
         if (!ground->isMinimized()) { log("FAIL: repeated minimize did not minimize"); app.exit(1); return; }
         log("PASS: desktop event loop remains responsive"); app.exit(0); break;
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
