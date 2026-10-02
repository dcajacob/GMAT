#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void setField(QWidget *owner,const QString &name,const QString &value)
{
   if (auto *choice=owner->findChild<QComboBox *>("resource_"+name)) { require(choice->findText(value)>=0,"Requested propagator selector absent"); choice->setCurrentText(value); }
   else if (auto *line=owner->findChild<QLineEdit *>("resource_"+name)) line->setText(value);
   else throw std::runtime_error(("Missing propagator field "+name).toStdString());
}
static QMdiSubWindow *propWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="DeepSpace") return child;
   throw std::runtime_error("Propagator settings window missing");
}
static ResourceEditor *propPanel(MainWindow &window)
{
   auto *panel=dynamic_cast<ResourceEditor *>(propWindow(window)->widget()); require(panel,"Propagator settings form missing"); return panel;
}
static void openPanel(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto items=tree->findItems("DeepSpace",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Propagator tree item missing"); tree->itemDoubleClicked(items.front(),0);
}
static void closePanel(MainWindow &window)
{
   require(propWindow(window)->close(),"Clean propagator form did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void checkProp(const char *type,const char *model,double maxStep)
{
   auto *prop=Moderator::Instance()->GetConfiguredObject("DeepSpace"); require(prop,"Propagator missing");
   const auto equal=[](double a,double b) { return std::abs(a-b)<=std::max(std::abs(b)*1e-13,1e-25); };
   if (prop->GetStringParameter("Type")!=type || prop->GetStringParameter("FM")!=model ||
       !equal(prop->GetRealParameter("InitialStepSize"),600) || !equal(prop->GetRealParameter("Accuracy"),1e-12) ||
       !equal(prop->GetRealParameter("MinStep"),0) || !equal(prop->GetRealParameter("MaxStep"),maxStep) ||
       prop->GetIntegerParameter("MaxStepAttempts")!=75 || prop->GetBooleanParameter("StopIfAccuracyIsViolated")) {
      std::cerr<<"Actual PropSetup Type="<<prop->GetStringParameter("Type")<<" FM="<<prop->GetStringParameter("FM")
         <<" InitialStepSize="<<prop->GetRealParameter("InitialStepSize")<<" Accuracy="<<prop->GetRealParameter("Accuracy")
         <<" MinStep="<<prop->GetRealParameter("MinStep")<<" MaxStep="<<prop->GetRealParameter("MaxStep")<<'\n';
      throw std::runtime_error("Propagator selectors or owned numeric settings were lost");
   }
}
static QString unrelatedSource(QString source)
{
   source.remove(QRegularExpression("^(?:GMAT[ \\t]+)?DeepSpace\\.(?:Type|FM|MaxStep)[ \\t]*=[ \\t]*[^;\\n]*;[ \\t]*\\n",QRegularExpression::MultilineOption)); return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("PropSetupApply");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Propagator runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString baseline="% Preserve source α and independent resources\nCreate Spacecraft Vehicle;\nCreate ForceModel DeepSpace_ForceModel;\nDeepSpace_ForceModel.PrimaryBodies = {};\nDeepSpace_ForceModel.PointMasses = {Earth};\nCreate Propagator DeepSpace;\nDeepSpace.InitialStepSize = 600; % retain exact input\nDeepSpace.Accuracy = 1e-12; % retain accuracy spelling\nDeepSpace.MinStep = 0;\nDeepSpace.MaxStep = 864000;\nDeepSpace.MaxStepAttempts = 75;\nDeepSpace.StopIfAccuracyIsViolated = false;\nCreate Variable Keep;\nKeep = 7; % preserve initializer\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Propagator baseline failed");
      const auto originalModel=Moderator::Instance()->GetConfiguredObject("DeepSpace")->GetStringParameter("FM");
      checkProp("RungeKutta89",originalModel.c_str(),864000);
      openPanel(window); auto *panel=propPanel(window);
      auto *type=panel->findChild<QComboBox *>("resource_Type"); auto *model=panel->findChild<QLineEdit *>("resource_FM");
      const auto fields=resourceProperties(*Moderator::Instance()->GetConfiguredObject("DeepSpace"));
      const auto fm=std::find_if(fields.cbegin(),fields.cend(),[](const auto &field) { return field.name=="FM"; });
      require(type && type->findText("PrinceDormand78")>=0 && model && fm!=fields.cend() && fm->references.contains("DeepSpace_ForceModel") && !fm->references.contains("Vehicle"),"Propagator factory and force-model choices incorrect");
      setField(panel,"Type","PrinceDormand78"); setField(panel,"FM","DeepSpace_ForceModel"); setField(panel,"MaxStep","720000");
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Pending propagator fields committed before Apply");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText();
      if (edited==baseline) std::cerr<<propPanel(window)->findChild<QLabel *>("resourceStatus")->text().toStdString()<<'\n';
      require(edited!=baseline && !propPanel(window)->hasChanges(),"Propagator Apply did not commit a clean form");
      checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      require(edited.contains("DeepSpace.Type = PrinceDormand78;") && edited.contains("DeepSpace.FM = DeepSpace_ForceModel;"),"Propagator Apply omitted Type or FM from source");
      require(edited.indexOf("DeepSpace.Type")<edited.indexOf("DeepSpace.InitialStepSize"),"Changed Type follows owned numeric settings and would reset them");
      require(unrelatedSource(edited)==unrelatedSource(baseline),"Propagator Apply changed unrelated source or unchanged owned settings");
      const auto undoSteps=editor->document()->availableUndoSteps(); auto *original=Moderator::Instance()->GetConfiguredObject("DeepSpace");
      require(!window.applyResourceChanges("DeepSpace",{{"Type","MissingPropagator"}},edited).isEmpty() &&
         !window.applyResourceChanges("DeepSpace",{{"FM","Vehicle"}},edited).isEmpty() && editor->toPlainText()==edited &&
         Moderator::Instance()->GetConfiguredObject("DeepSpace")==original && editor->document()->availableUndoSteps()==undoSteps,"Invalid selector changed source, model or Undo history");
      closePanel(window); editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Propagator Apply was not one exact Undo"); checkProp("RungeKutta89",originalModel.c_str(),864000);
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Propagator Apply Redo failed"); checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      // Rebuilding for an unrelated creation reproduced the lost selectors in
      // the actual GUI. Assert source and reopen controls retain both choices.
      require(window.createResource("GroundStation","Station",edited).isEmpty(),"Unrelated resource creation failed");
      const auto withStation=editor->toPlainText(); require(withStation=="Create GroundStation Station;\n"+edited,"Unrelated creation rewrote propagator source"); checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      openPanel(window); panel=propPanel(window);
      require(panel->findChild<QComboBox *>("resource_Type")->currentText()=="PrinceDormand78" && panel->findChild<QLineEdit *>("resource_FM")->text()=="DeepSpace_ForceModel" && !panel->hasChanges(),"Reopened propagator form lost Type or FM"); closePanel(window);
      const auto saved=files.filePath("propagator α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==withStation,"Propagator Unicode save/reopen changed source"); checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      std::cout<<"PASS pending PropSetup factory/force-model choices, mixed Type/FM/MaxStep Apply, unchanged step/accuracy and source retained, invalid-selector rollback, exact Undo/Redo, unrelated creation/rebuild, reopened controls and Unicode save/reopen; no mission numerical or native desktop run.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
