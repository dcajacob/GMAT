#include "PropagationForm.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpression>

PropagationForm::PropagationForm(const QStringList &propagators,const QStringList &satellites,
      std::function<void(const QString &)> changed,QWidget *parent) : QGroupBox("Propagation",parent)
{
   setObjectName("propagationForm");
   auto *layout=new QFormLayout(this);
   propagator=new QComboBox(this); propagator->setObjectName("propagationPropagator"); propagator->addItems(propagators);
   spacecraft=new QComboBox(this); spacecraft->setObjectName("propagationSpacecraft"); spacecraft->addItems(satellites);
   units=new QComboBox(this); units->setObjectName("propagationUnits");
   units->addItem("Seconds","ElapsedSecs"); units->addItem("Days","ElapsedDays");
   duration=new QLineEdit(this); duration->setObjectName("propagationDuration");
   duration->setPlaceholderText("Duration or parameter expression");
   layout->addRow("Propagator",propagator); layout->addRow("Spacecraft",spacecraft);
   layout->addRow("Duration",duration); layout->addRow("Units",units);
   auto update=[this,changed] {
      if (synchronizing) return;
      synchronizing=true;
      changed(QString("Propagate %1(%2) {%2.%3 = %4};").arg(propagator->currentText(),
         spacecraft->currentText(),units->currentData().toString(),duration->text()));
      synchronizing=false;
   };
   connect(propagator,&QComboBox::currentTextChanged,this,update);
   connect(spacecraft,&QComboBox::currentTextChanged,this,update);
   connect(units,&QComboBox::currentTextChanged,this,update);
   connect(duration,&QLineEdit::textChanged,this,update);
}

void PropagationForm::setStatement(const QString &statement)
{
   if (synchronizing) return;
   // Only expose the form when the entire statement is representable. Advanced
   // propagation modes, multiple spacecraft/stops, labels and comments remain
   // intact in the script editor rather than being silently simplified.
   static const QRegularExpression simple(
      "^\\s*Propagate\\s+([A-Za-z][A-Za-z0-9_]*)\\s*\\(\\s*([A-Za-z][A-Za-z0-9_]*)\\s*\\)"
      "\\s*\\{\\s*\\2\\.(ElapsedSecs|ElapsedDays)\\s*=\\s*([^{};,%]+?)\\s*\\}\\s*;\\s*$");
   const auto match=simple.match(statement);
   if (!match.hasMatch() || propagator->findText(match.captured(1))<0 || spacecraft->findText(match.captured(2))<0) {
      hide(); return;
   }
   synchronizing=true;
   propagator->setCurrentText(match.captured(1)); spacecraft->setCurrentText(match.captured(2));
   units->setCurrentIndex(units->findData(match.captured(3))); duration->setText(match.captured(4).trimmed());
   synchronizing=false;
   show();
}
