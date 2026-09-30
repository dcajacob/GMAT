#include "SpiceInterface.hpp"
#include "FileManager.hpp"
#include "MessageInterface.hpp"
#include "QtMessageReceiver.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Diagnostic file unreadable"); return f.readAll(); }
static void write(const QString &path,const QByteArray &data) { QFile f(path); require(f.open(QIODevice::WriteOnly) && f.write(data)==data.size(),"Diagnostic fixture write failed"); }
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      if (argc==5 && QString(argv[1])=="--child") {
         const auto startup=QString(argv[2]),output=QString(argv[3]),missing=QString(argv[4]);
         QDir::setCurrent(QFileInfo(startup).absolutePath());
         QtMessageReceiver messages; messages.SetLogEnable(false);
         messages.SetMessageCallback(&app,[](const QString &text) { std::cout<<text.toStdString()<<std::flush; });
         MessageInterface::SetMessageReceiver(&messages);
         auto *fm=FileManager::Instance(); fm->ReadStartupFile(startup.toStdString());
         fm->SetAbsPathname("OUTPUT_PATH",(output+"/").toStdString());
         {
            SpiceInterface interface;
            char device[256]; errdev_c("GET",sizeof(device),device); std::cout<<"DEVICE="<<device<<'\n';
            // An actual CSPICE failure exercises WRLINE, not just errdev SET.
            const auto path=missing.toStdString(); furnsh_c(path.c_str());
            require(failed_c(),"Missing kernel did not fail CSPICE");
            char error[1840]; getmsg_c("LONG",sizeof(error),error);
            require(QString::fromUtf8(error).contains(missing),"Underlying kernel error lost its path");
            std::cout<<"KERNEL_ERROR="<<error<<'\n'; reset_c();
            const auto leap=fm->GetFullPathname("LSK_FILE");
            require(interface.LoadKernel(leap) && interface.IsLoaded(leap),"Kernel correction did not recover");
            require(interface.UnloadKernel(leap),"Corrected kernel did not unload");
         }
         QCoreApplication::processEvents(); MessageInterface::SetMessageReceiver(nullptr);
         std::cout<<"CHILD_PASS\n"; return 0;
      }
      require(argc==2,"Startup argument required");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QTemporaryDir files;
      require(files.isValid(),"Diagnostic fixture directory missing");
      const auto missing=files.filePath("missing kernel Δ.bsp");
      auto run=[&](const QString &output) {
         QProcess process; process.setProcessChannelMode(QProcess::MergedChannels);
         process.start(QCoreApplication::applicationFilePath(),{"--child",startup,output,missing});
         require(process.waitForStarted() && process.waitForFinished(15000),"Diagnostic subprocess did not finish");
         const auto text=process.readAll(); std::cout<<text.toStdString();
         require(process.exitStatus()==QProcess::NormalExit && process.exitCode()==0 && text.contains("CHILD_PASS"),"Diagnostic subprocess failed");
         require(!text.contains("SPICE(FILEOPENFAILED)") && !text.contains("IOSTAT"),"Diagnostic output produced file-open errors");
         return text;
      };
      const auto existing=files.filePath("GMATSpiceKernelError.txt"); const QByteArray previous="Retain prior diagnostics exactly.\n";
      write(existing,previous); const auto output=run(files.path());
      require(read(existing)==previous,"Prior diagnostic file was overwritten");
      const auto next=files.filePath("GMATSpiceKernelError.1.txt");
      auto rawError=[&](const QString &path) { return QString::fromUtf8(read(path)).simplified().contains(missing); };
      require(output.contains(next.toUtf8()) && rawError(next),"New raw diagnostics were not retained");
      const auto first=read(next); run(files.path());
      require(read(existing)==previous && read(next)==first && rawError(files.filePath("GMATSpiceKernelError.2.txt")),"Repeated launch lost old or new diagnostics");
      QTemporaryDir fresh; run(fresh.path()); require(rawError(fresh.filePath("GMATSpiceKernelError.txt")),"Fresh output failed to retain diagnostics");
      // No existing file, but an unwritable directory or overlong errdev path.
      const auto blocked=files.filePath("read-only"); require(QDir().mkpath(blocked),"Read-only fixture missing");
      require(QFile::setPermissions(blocked,QFile::ReadOwner|QFile::ExeOwner),"Read-only fixture permissions failed");
      const auto fallback=run(blocked); require(QFile::setPermissions(blocked,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner),"Read-only fixture restore failed");
      require(fallback.contains("DEVICE=SCREEN") && fallback.contains("Cannot create") && fallback.contains("KERNEL_ERROR="),"Unwritable diagnostics hid the kernel error");
      const auto longPath=files.filePath(QString(110,'a')+"/"+QString(110,'b')); require(QDir().mkpath(longPath),"Long path fixture missing");
      require(run(longPath).contains("DEVICE=SCREEN"),"Overlong diagnostic path did not fall back safely");
      std::cout<<"PASS: existing/fresh/repeated SPICE diagnostics, preserved raw files, Unicode kernel errors, unwritable/long-path console fallback and corrected kernel load/unload\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
