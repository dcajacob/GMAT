#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "ODEModel.hpp"
#include "SolarRadiationPressure.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Saved string-array source missing"); return file.readAll(); }
static GmatBase *object(const char *name) { auto *value=Moderator::Instance()->GetConfiguredObject(name); require(value,"String-array resource missing"); return value; }
static void check(const QString &kernel)
{
   auto *force=dynamic_cast<ODEModel *>(object("FmGeo")); require(force,"String-array ForceModel unavailable");
   const auto id=force->GetParameterID("SRP.ExtraShadowBodies");
   require(id>999 && force->GetParameterType(id)==Gmat::STRINGARRAY_TYPE,"Decorated SRP array did not resolve to an owned force");
   // ODEModel's extended-id array/text getters are incomplete upstream. Query
   // the actual owned SRP getter; this test targets interpreter setter dispatch.
   const auto *srp=dynamic_cast<const SolarRadiationPressure *>(force->GetForce("SolarRadiationPressure"));
   require(srp && srp->GetStringArrayParameter("ExtraShadowBodies")==StringArray({"Luna"}),"Decorated SRP Luna shadow list was lost");
   require(object("Kept")->GetStringArrayParameter("OrbitSpiceKernelName")==StringArray({kernel.toStdString()}),"Nonempty spacecraft SPICE list was lost");
   require(object("Cleared")->GetStringArrayParameter("OrbitSpiceKernelName").empty(),"Explicit empty spacecraft SPICE list retained a prior kernel");
   require(object("DefaultPower")->GetStringArrayParameter("ShadowBodies")==StringArray({"Earth"}),"Implicit solar-shadow Earth default changed");
   require(object("NamedPower")->GetStringArrayParameter("ShadowBodies")==StringArray({"Earth","Luna"}),"Nonempty solar-shadow list was lost");
   require(object("EmptyPower")->GetStringArrayParameter("ShadowBodies").empty(),"Explicit empty solar-shadow list restored Earth or retained prior names");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtStringArrayDispatch");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","String-array regression requires offscreen and a startup path");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"String-array temporary directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto kernel=QDir(QFileInfo(startup).absolutePath()).absoluteFilePath("../data/vehicle/ephem/spk/GEOSat.bsp");
      require(QFileInfo::exists(kernel),"Shipped SPK required by string-array fixture missing");
      const QString source="% delegated and explicit-empty string arrays α\nCreate ForceModel FmGeo;\nFmGeo.CentralBody = Earth;\nFmGeo.PrimaryBodies = {Earth};\nFmGeo.PointMasses = {Sun, Luna};\nFmGeo.Drag = None;\nFmGeo.SRP = On;\nFmGeo.SRP.ExtraShadowBodies = {'Luna'};\nCreate Spacecraft Kept Cleared;\nKept.OrbitSpiceKernelName = {'"+kernel+"'};\nCleared.OrbitSpiceKernelName = {'"+kernel+"'};\nCleared.OrbitSpiceKernelName = {};\nCreate SolarPowerSystem DefaultPower NamedPower EmptyPower;\nNamedPower.ShadowBodies = {Earth, Luna};\nEmptyPower.ShadowBodies = {Earth, Luna};\nEmptyPower.ShadowBodies = {};\nBeginMissionSequence; % no numerical mission\n";
      MainWindow window; window.show(); require(window.initialize(startup),"String-array runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"String-array editor missing");
      editor->setPlainText(source); require(window.buildScript(),"Delegated SRP array or retained empty-list interpretation failed"); check(kernel);
      require(editor->toPlainText()==source,"String-array interpretation changed original source");
      const auto saved=files.filePath("string arrays Δ.script");
      require(window.saveScriptTo(saved) && read(saved)==source.toUtf8(),"String-array Save changed exact source bytes");
      require(window.loadScript(saved) && window.buildScript() && editor->toPlainText()==source,"String-array reopen/build changed source"); check(kernel);
      std::cout<<"PASS: actual interpreter delegated SRP.ExtraShadowBodies Luna list; spacecraft SPK nonempty and set/clear {}; SolarPowerSystem nonempty, explicit empty and implicit Earth distinction; exact Unicode Save/reopen/source and owned SRP getter round-trip. Offscreen only, no numerical mission.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
