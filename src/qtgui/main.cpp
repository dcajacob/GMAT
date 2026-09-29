#include "MainWindow.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <cstdio>
#include <QTimer>
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
   parser.addPositionalArgument("script", "Optional GMAT script to open");
   parser.process(app);
   const QString startup = parser.isSet("startup") ? QFileInfo(parser.value("startup")).absoluteFilePath() : QCoreApplication::applicationDirPath() + "/gmat_startup_file.txt";
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
   if (!script.isEmpty()) loaded = window.loadScript(script) && window.buildScript();
   QTimer::singleShot(0, &window, [&] {
      bool succeeded = initialized && loaded;
      if (parser.isSet("run") && succeeded)
         succeeded = window.runMission() == MainWindow::RunResult::Completed;
      if (!screenshot.isEmpty())
         QTimer::singleShot(200, &window, [&, succeeded] {
            app.exit(window.grab().save(screenshot) && succeeded ? 0 : 1);
         });
   });
   return app.exec();
}
