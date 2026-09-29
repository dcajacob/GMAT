#include "MainWindow.hpp"
#include <QApplication>
#include <QScreen>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <cstdio>
#include <QTimer>
#include <QSettings>
int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   QApplication::setOrganizationName("GMAT");
   QApplication::setApplicationName("GMAT Qt 6");
   QCommandLineParser parser;
   parser.setApplicationDescription("GMAT Qt 6 desktop interface");
   parser.addHelpOption();
   parser.addOption({"startup", "GMAT startup file", "file"});
   parser.addOption({"screenshot", "Save a startup screenshot and exit", "file"});
   parser.addOption({"run", "Run the loaded mission after opening the window"});
   parser.addOption({"convert-views", "Convert supported OpenFrames viewer definitions to Qt OrbitView in memory; review before saving"});
   parser.addOption({"settings-dir", "Use an isolated INI settings directory", "directory"});
   parser.addPositionalArgument("script", "Optional GMAT script to open");
   parser.process(app);
   if (parser.isSet("settings-dir")) {
      const auto directory=QFileInfo(parser.value("settings-dir")).absoluteFilePath();
      if (!QDir().mkpath(directory)) { std::fprintf(stderr,"Cannot create settings directory.\n"); return 2; }
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory);
      QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,directory+"/system");
   }
   QString defaultStartup=QCoreApplication::applicationDirPath()+"/gmat_startup_qt.txt";
#ifdef Q_OS_MACOS
   if (!QFileInfo::exists(defaultStartup))
      defaultStartup=QCoreApplication::applicationDirPath()+"/../../../gmat_startup_qt.txt";
#endif
   const QString startup = parser.isSet("startup") ? QFileInfo(parser.value("startup")).absoluteFilePath() : defaultStartup;
   const QString script = parser.positionalArguments().isEmpty() ? QString() : QFileInfo(parser.positionalArguments().first()).absoluteFilePath();
   const QString screenshot = parser.isSet("screenshot") ? QFileInfo(parser.value("screenshot")).absoluteFilePath() : QString();
   if (!QFileInfo::exists(startup) || (!script.isEmpty() && !QFileInfo::exists(script))) {
      std::fprintf(stderr, "Startup file or script does not exist.\n");
      return 2;
   }
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   MainWindow window;
   window.show();
   const bool initialized = window.initialize(startup);
   bool loaded = true;
   if (initialized && !script.isEmpty()) loaded = window.loadScript(script) &&
      (!parser.isSet("convert-views") || window.convertOpenFramesScript()) && window.buildScript();
   QTimer::singleShot(0, &window, [&] {
      bool succeeded = initialized && loaded;
      if (parser.isSet("run") && succeeded)
         succeeded = window.runMission() == MainWindow::RunResult::Completed;
      if (!screenshot.isEmpty())
         QTimer::singleShot(200, &window, [&, succeeded] {
            // QWidget::grab can repaint a QOpenGLWidget without its composed
            // overlays. On X11 capture the displayed native window instead.
            const auto image=QGuiApplication::platformName()=="xcb" && window.screen()
               ? window.screen()->grabWindow(window.winId()) : window.grab();
            app.exit(image.save(screenshot) && succeeded ? 0 : 1);
         });
   });
   return app.exec();
}
