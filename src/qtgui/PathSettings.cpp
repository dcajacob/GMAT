#include "PathSettings.hpp"
#include "QtMessageReceiver.hpp"
#include "StartupCompatibility.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "BaseException.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <functional>
#include <stdexcept>
#include <vector>
namespace {
QString read(const QString &path)
{
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(file.errorString().toStdString());
   return QString::fromUtf8(file.readAll());
}
QStringList functionPaths()
{
   QStringList paths,seen;
   for (const auto &path:FileManager::Instance()->GetAllGmatFunctionPaths()) {
      const auto absolute=QDir::cleanPath(QFileInfo(QString::fromStdString(path)).absoluteFilePath());
      const auto canonical=QFileInfo(absolute).canonicalFilePath();
      const auto key=canonical.isEmpty() ? absolute : canonical;
      // Engine callers may add both slash spellings of the same directory.
      // Keep its first search position when presenting and saving the list.
      if (!seen.contains(key)) { seen.append(key); paths.append(absolute); }
   }
   return paths;
}
// FileManager parsing also changes global UI modes and the message receiver.
// Previewing another startup must restore all three, including unknown file
// aliases and ordered include/Python paths that serialization cannot restore.
class StartupTransaction
{
public:
   explicit StartupTransaction(QtMessageReceiver &receiver): receiver(receiver), state(FileManager::Instance()->CaptureState()),
      logFile(receiver.GetLogFileName()), logEnabled(receiver.GetLogEnable()), cwd(QDir::currentPath())
   {
      auto *global=GmatGlobal::Instance();
      auto remember=[&](auto getter,auto setter) {
         const auto value=(global->*getter)();
         globals.emplace_back([=] { (global->*setter)(value); });
      };
      remember(&GmatGlobal::GetRunMode,&GmatGlobal::SetRunMode);
      remember(&GmatGlobal::InTestingMode,&GmatGlobal::SetTestingMode);
      remember(&GmatGlobal::IsBatchMode,&GmatGlobal::SetBatchMode);
      remember(&GmatGlobal::GetPlotMode,&GmatGlobal::SetPlotMode);
      remember(&GmatGlobal::GetMatlabMode,&GmatGlobal::SetMatlabMode);
      remember(&GmatGlobal::IsMatlabAvailable,&GmatGlobal::SetMatlabAvailable);
      remember(&GmatGlobal::IsMatlabDebugOn,&GmatGlobal::SetMatlabDebug);
      remember(&GmatGlobal::IsMissionTreeDebugOn,&GmatGlobal::SetMissionTreeDebug);
      remember(&GmatGlobal::IsWritingParameterInfo,&GmatGlobal::SetWriteParameterInfo);
      remember(&GmatGlobal::IsWritingFilePathInfo,&GmatGlobal::SetWriteFilePathInfo);
      remember(&GmatGlobal::IsWritingGmatKeyword,&GmatGlobal::SetWriteGmatKeyword);
      remember(&GmatGlobal::GetWritePersonalizationFile,&GmatGlobal::SetWritePersonalizationFile);
      remember(&GmatGlobal::EchoCommands,&GmatGlobal::SetCommandEchoMode);
      remember(&GmatGlobal::SkipSplashMode,&GmatGlobal::SetSkipSplashMode);
      const auto hidden=global->GetHiddenCommands();
      const auto startupLog=global->GetLogfileName(GmatGlobal::STARTUP);
      globals.emplace_back([=] { global->ClearHiddenCommands(); for (const auto &command:hidden) global->AddHiddenCommand(command); global->SetLogfileName(GmatGlobal::STARTUP,startupLog); });
      receiver.SuspendLogging(true);
   }
   ~StartupTransaction()
   {
      if (!committed) {
         FileManager::Instance()->RestoreState(*state);
         for (const auto &restore:globals) restore();
         QDir::setCurrent(cwd);
         receiver.RestoreLog(logFile,logEnabled);
      }
      receiver.SuspendLogging(false);
   }
   void commit() { committed=true; }
private:
   QtMessageReceiver &receiver;
   std::shared_ptr<const FileManager::State> state;
   std::vector<std::function<void()>> globals;
   std::string logFile;
   bool logEnabled, committed=false;
   QString cwd;
};
}
PathSettings capturePathSettings(const QString &startupFile)
{
   QTemporaryDir directory;
   if (!directory.isValid()) throw std::runtime_error("Cannot prepare startup settings.");
   const auto path=directory.filePath("startup.txt");
   FileManager::Instance()->WriteStartupFile(path.toStdString());
   return {startupFile,read(path),QFileInfo(QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"))).absoluteFilePath(),functionPaths()};
}
PathSettings readPathSettings(const QString &path,QtMessageReceiver &receiver)
{
   const auto error=qtStartupCompatibilityError(path);
   if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
   StartupTransaction transaction(receiver);
   FileManager::Instance()->ReadStartupFile(QFileInfo(path).absoluteFilePath().toStdString());
   return capturePathSettings(QFileInfo(path).absoluteFilePath());
}
QString pathSettingsScript(const PathSettings &settings)
{
   QStringList result; bool output=false,functions=false;
   const QRegularExpression assignment("^\\s*(OUTPUT_PATH|GMAT_FUNCTION_PATH)\\s*=.*$");
   auto paths=[&] { QStringList lines; for (const auto &path:settings.functions) lines.append("GMAT_FUNCTION_PATH = "+QDir::cleanPath(path)+"/"); return lines; };
   for (const auto &line:settings.startupText.split('\n')) {
      const auto match=assignment.match(line);
      if (!match.hasMatch()) { result.append(line); continue; }
      if (match.captured(1)=="OUTPUT_PATH") { if (!output) result.append("OUTPUT_PATH = "+QDir::cleanPath(settings.output)+"/"); output=true; }
      else { if (!functions) result.append(paths()); functions=true; }
   }
   if (!output) result.append("OUTPUT_PATH = "+QDir::cleanPath(settings.output)+"/");
   if (!functions) result.append(paths());
   return result.join('\n');
}
QString validatePathSettings(const PathSettings &settings)
{
   QStringList directories=settings.functions; directories.prepend(settings.output);
   QStringList seen;
   for (int i=0;i<directories.size();++i) {
      const auto &path=directories[i];
      if (path.isEmpty() || !QDir::isAbsolutePath(path) || path.contains('=') || path.contains(QRegularExpression("[\\x00-\\x1f]")))
         return "Choose an absolute directory without control characters or '='.";
      const QFileInfo directory(path);
      if (!directory.isDir() || !directory.isReadable()) return "Directory is unavailable: "+path;
      const auto canonical=directory.canonicalFilePath();
      if (i>0 && seen.contains(canonical)) return "Each function directory should appear once.";
      if (i>0) seen.append(canonical);
   }
   QTemporaryFile probe(QDir(settings.output).filePath(".gmat-write-XXXXXX"));
   if (!probe.open() || probe.write("probe")!=5 || !probe.flush()) return "Cannot write to the output directory: "+probe.errorString();
   return {};
}
QString applyPathSettings(const PathSettings &settings,QtMessageReceiver &receiver)
{
   const auto error=validatePathSettings(settings); if (!error.isEmpty()) return error;
   try {
      QTemporaryDir directory; if (!directory.isValid()) return "Cannot prepare startup settings.";
      const auto path=directory.filePath("startup.txt"); QFile file(path);
      const auto text=pathSettingsScript(settings).toUtf8();
      if (!file.open(QIODevice::WriteOnly) || file.write(text)!=text.size() || !file.flush()) return "Cannot prepare startup settings: "+file.errorString();
      file.close();
      const auto compatibility=qtStartupCompatibilityError(path); if (!compatibility.isEmpty()) return compatibility;
      StartupTransaction transaction(receiver);
      FileManager::Instance()->ReadStartupFile(path.toStdString());
      QStringList actual; for (const auto &entry:functionPaths()) actual.append(QDir::cleanPath(entry));
      QStringList expected; for (const auto &entry:settings.functions) expected.append(QDir::cleanPath(entry));
      if (actual!=expected || QDir::cleanPath(QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH")))!=QDir::cleanPath(settings.output))
         return "GMAT could not preserve the requested paths and order.";
      receiver.SetLogPath(settings.output.toStdString(),true);
      FileManager::Instance()->SetStartupFilePath(settings.startupFile.toStdString());
      transaction.commit();
      return {};
   } catch (BaseException &exception) { return QString::fromStdString(exception.GetFullMessage()); }
   catch (const std::exception &exception) { return QString::fromUtf8(exception.what()); }
}
