#include "ExternalForceDialog.hpp"
#include "ResourceProperties.hpp"
#include "UserParameter.hpp"
#include "ScriptStatements.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>
#include <stdexcept>

namespace {
const QString prefix="@ExternalForce.";
QString invalid(const QMap<QString,QString> &settings)
{
   if (!QStringList{"true","false"}.contains(settings.value(prefix+"Enabled"))) return "Choose whether to use the external force.";
   if (settings.value(prefix+"Enabled")=="false") return {};
   static const QRegularExpression module("^[A-Za-z_][A-Za-z0-9_]*(?:\\.[A-Za-z_][A-Za-z0-9_]*)*$"),function("^[A-Za-z_][A-Za-z0-9_]*$");
   if (!module.match(settings.value(prefix+"Module")).hasMatch() || settings.value(prefix+"Module").endsWith(".py")) return "Enter a Python module name without .py or a file path.";
   if (!function.match(settings.value(prefix+"Function")).hasMatch()) return "Enter the derivative function name in that module.";
   if (!QStringList{"true","false"}.contains(settings.value(prefix+"ExcludeOtherForces"))) return "Choose whether to exclude other forces.";
   return {};
}
}
QString validateExternalForceSettings(const QMap<QString,QString> &settings) { return invalid(settings); }
QMap<QString,QString> externalForceSettings(GmatBase &model)
{
   QMap<QString,QString> settings={{prefix+"Enabled","false"},{prefix+"Module",{}},{prefix+"Function","GetDerivatives"},{prefix+"ExcludeOtherForces","false"}};
   for (int i=0;i<model.GetOwnedObjectCount();++i) if (auto *force=model.GetOwnedObject(i);force && force->IsOfType("ExternalModel")) {
      settings[prefix+"Enabled"]="true";
      for (const auto &field:resourceProperties(*force)) {
         if (field.name=="ScriptFileName") settings[prefix+"Module"]=field.value;
         if (field.name=="DerivativesFunction") settings[prefix+"Function"]=field.value;
         if (field.name=="ExcludeOtherForces") settings[prefix+"ExcludeOtherForces"]=field.value;
      }
   }
   return settings;
}
QString externalForceScript(const QString &source,const QString &model,const QMap<QString,QString> &settings,const QString &firstCommand)
{
   const auto error=invalid(settings); if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
   QStringList properties={"External","External.ScriptFileName","External.DerivativesFunction","External.ExcludeOtherForces","ScriptFileName","DerivativesFunction","ExcludeOtherForces"};
   QString block;
   if (settings.value(prefix+"Enabled")=="true") {
      block="GMAT "+model+".External = '"+settings.value(prefix+"Module")+"';\nGMAT "+model+".External.DerivativesFunction = '"+settings.value(prefix+"Function")+"';\nGMAT "+model+".External.ExcludeOtherForces = "+settings.value(prefix+"ExcludeOtherForces")+";\n";
   } else {
      // Removal intentionally removes all settings of this contributor. The
      // common source patcher retains comments and stops at the mission boundary.
      const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(model)+"\\.(External(?:\\.[A-Za-z0-9_.]+)?)\\s*=");
      for (const auto &statement:scriptStatements(source)) { const auto match=assignment.match(statement.code); if (match.hasMatch()) properties.append(match.captured(1)); }
      properties.removeDuplicates();
   }
   return setConfigurationBlock(source,model,properties,block,firstCommand);
}
QString externalForceSettingsError(GmatBase &model,const QMap<QString,QString> &expected)
{
   int count=0; for (int i=0;i<model.GetOwnedObjectCount();++i) if (auto *force=model.GetOwnedObject(i);force && force->IsOfType("ExternalModel")) ++count;
   if (count!=(expected.value(prefix+"Enabled")=="true" ? 1 : 0)) return "The model did not retain exactly the requested external contributor. The previous model was restored.";
   const auto actual=externalForceSettings(model);
   if (actual.value(prefix+"Enabled")!=expected.value(prefix+"Enabled")) return "The external force was not added or removed. The previous model was restored.";
   if (expected.value(prefix+"Enabled")=="true") for (auto it=expected.cbegin();it!=expected.cend();++it)
      if (actual.value(it.key())!=it.value()) return "The external force settings were not retained. The previous model was restored.";
   return {};
}
ExternalForceDialog::ExternalForceDialog(GmatBase &model,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("externalForceDialog"); setWindowTitle("Python external force"); resize(620,340);
   auto settings=externalForceSettings(model); for (auto it=pending.cbegin();it!=pending.cend();++it) if (settings.contains(it.key())) settings[it.key()]=it.value();
   auto *layout=new QVBoxLayout(this);
   auto *note=new QLabel("Uncheck Use external Python force to remove this contributor when you Apply the force model. Settings stay pending until Apply.",this); note->setWordWrap(true); layout->addWidget(note);
   enabled=new QGroupBox("Use external Python force",this); enabled->setObjectName("externalForceEnabled"); enabled->setCheckable(true); layout->addWidget(enabled);
   auto *form=new QFormLayout(enabled);
   module=new QComboBox(enabled); module->setObjectName("externalForceModule"); module->setEditable(true); module->addItems(pythonModuleNames()); module->setCurrentText(settings.value(prefix+"Module")); form->addRow("Module",module);
   module->setToolTip("Module name without .py, found in the configured Python search paths.");
   function=new QLineEdit(settings.value(prefix+"Function"),enabled); function->setObjectName("externalForceFunction"); form->addRow("Derivative function",function);
   exclusive=new QCheckBox("Exclude other forces",enabled); exclusive->setObjectName("externalForceExclusive"); exclusive->setChecked(settings.value(prefix+"ExcludeOtherForces")=="true"); form->addRow(exclusive); enabled->setChecked(settings.value(prefix+"Enabled")=="true");
   auto *cache=new QLabel("Python caches imported modules. Restart GMAT after changing the module's code.",this); cache->setWordWrap(true); layout->addWidget(cache);
   error=new QLabel(this); error->setObjectName("externalForceError"); error->setWordWrap(true); layout->addWidget(error); layout->addStretch();
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(module,&QComboBox::currentTextChanged,error,&QLabel::clear);
   connect(function,&QLineEdit::textChanged,error,&QLabel::clear);
   connect(enabled,&QGroupBox::toggled,error,&QLabel::clear);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons->button(QDialogButtonBox::Ok),&QPushButton::clicked,this,[this] {
      const QMap<QString,QString> settings={{prefix+"Enabled",enabled->isChecked() ? "true" : "false"},{prefix+"Module",module->currentText().trimmed()},{prefix+"Function",function->text().trimmed()},{prefix+"ExcludeOtherForces",exclusive->isChecked() ? "true" : "false"}};
      const auto message=invalid(settings); if (!message.isEmpty()) { error->setText(message); return; }
      accepted=settings; accept();
   });
}
