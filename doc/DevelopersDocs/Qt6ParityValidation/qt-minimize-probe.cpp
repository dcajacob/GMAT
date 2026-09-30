#include <QApplication>
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QOpenGLWidget>
#include <QTimer>
#include <iostream>
int main(int argc,char **argv) {
 QApplication app(argc,argv); QMainWindow win; win.resize(800,600); win.setWindowTitle("Temporary Qt minimize check");
 if(argc>1) { auto *area=new QMdiArea; win.setCentralWidget(area); auto *child=area->addSubWindow(new QWidget); child->showMaximized(); }
 if(argc>2) { auto *anchor=new QOpenGLWidget(&win); anchor->resize(1,1); anchor->hide(); }
 win.show();
 QTimer::singleShot(1000,&win,[&] { win.showMinimized(); std::cerr<<"immediate state="<<int(win.windowState())<<'\n'; });
 QTimer::singleShot(2000,&win,[&] { std::cerr<<"settled state="<<int(win.windowState())<<'\n'; app.exit(win.isMinimized() ? 0 : 1); });
 return app.exec();
}
