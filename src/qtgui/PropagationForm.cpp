#include "PropagationForm.hpp"
#include "ReportParameterDialog.hpp"
#include <QComboBox>
#include <QCheckBox>
#include "Propagate.hpp"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace {
QPair<int,int> backwardSpan(const QString &statement)
{
   const auto prefix=QRegularExpression("^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?").match(statement);
   const int start=prefix.capturedEnd();
   const auto keyword=QRegularExpression("^BackProp\\s+").match(statement.mid(start));
   return {start,keyword.hasMatch() ? keyword.capturedLength() : 0};
}
bool eventStop(const QString &parameter) { return parameter.endsWith(".Periapsis") || parameter.endsWith(".Apoapsis"); }
const QRegularExpression &propagationPattern()
{
   static const QRegularExpression pattern(
      "^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?(?:BackProp\\s+)?([A-Za-z][A-Za-z0-9_]*)\\s*\\(\\s*([A-Za-z][A-Za-z0-9_]*)\\s*\\)"
      "\\s*\\{\\s*([A-Za-z][A-Za-z0-9_.]*)(?:\\s*=\\s*([^{};,%]+?))?(\\s*,\\s*StopTolerance\\s*=\\s*([^{};,%]+?))?\\s*\\}\\s*;[ \\t]*(?:%[^\\n]*)?\\s*$");
   return pattern;
}
}
PropagationForm::PropagationForm(const QStringList &propagators,const QStringList &satellites,
      std::function<void(const QString &)> changed,QWidget *parent) : QGroupBox("Propagation",parent)
{
   setObjectName("propagationForm");
   auto *layout=new QFormLayout(this);
   propagator=new QComboBox(this); propagator->setObjectName("propagationPropagator"); propagator->addItems(propagators);
   spacecraft=new QComboBox(this); spacecraft->setObjectName("propagationSpacecraft"); spacecraft->addItems(satellites);
   units=new QComboBox(this); units->setObjectName("propagationUnits");
   units->addItem("Seconds","ElapsedSecs"); units->addItem("Days","ElapsedDays"); units->addItem("Other parameter","");
   duration=new QLineEdit(this); duration->setObjectName("propagationDuration"); duration->setPlaceholderText("Stop value or parameter expression");
   stopParameter=new QLineEdit(this); stopParameter->setObjectName("propagationStopParameter");
   layout->addRow("Propagator",propagator); layout->addRow("Spacecraft",spacecraft); layout->addRow("Stop type",units);
   auto picker=[&](const QString &label,QLineEdit *input,const QString &name) {
      auto *row=new QWidget(this); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0); box->addWidget(input);
      auto *choose=new QPushButton("Choose…",row); choose->setObjectName(name); box->addWidget(choose); layout->addRow(label,row);
      connect(choose,&QPushButton::clicked,this,[this,input] {
         ReportParameterDialog dialog({input->text()},this,input==stopParameter ? ReportParameterDialog::Mode::StopParameter : ReportParameterDialog::Mode::Single); dialog.setObjectName("propagationParameterDialog");
         if (dialog.exec()==QDialog::Accepted && !dialog.selection().isEmpty()) input->setText(dialog.selection().first());
      });
   };
   picker("Stop parameter",stopParameter,"propagationChooseStop"); picker("Stop value",duration,"propagationChooseGoal");
   backward=new QCheckBox("Propagate backwards",this); backward->setObjectName("propagationBackwards"); layout->addRow(backward);
   tolerance=new QLineEdit(this); tolerance->setObjectName("propagationTolerance");
   Propagate defaults; tolerance->setPlaceholderText("Default: "+QString::number(defaults.GetRealParameter("StopTolerance"),'g',10));
   tolerance->setToolTip("Positive stop tolerance. Leave blank to use GMAT's default."); layout->addRow("Stop tolerance",tolerance);
   auto update=[this,changed] {
      if (synchronizing) return;
      const auto match=propagationPattern().match(original); if (!match.hasMatch()) return;
      QString result=original;
      if (match.capturedStart(5)>=0) {
         if (tolerance->text().trimmed().isEmpty()) result.remove(match.capturedStart(5),match.capturedLength(5));
         else result.replace(match.capturedStart(6),match.capturedLength(6),tolerance->text().trimmed());
      } else if (!tolerance->text().trimmed().isEmpty()) {
         const int end=match.capturedStart(4)<0 ? match.capturedEnd(3) : match.capturedEnd(4);
         result.insert(end,", StopTolerance = "+tolerance->text().trimmed());
      }

      const bool event=eventStop(stopParameter->text());
      duration->setEnabled(!event); findChild<QPushButton *>("propagationChooseGoal")->setEnabled(!event);
      if (event || match.capturedStart(4)<0) {
         const int end=match.capturedStart(4)<0 ? match.capturedEnd(3) : match.capturedEnd(4);
         result.replace(match.capturedStart(3),end-match.capturedStart(3),stopParameter->text()+(event ? QString() : " = "+duration->text()));
      } else {
         result.replace(match.capturedStart(4),match.capturedLength(4),duration->text());
         result.replace(match.capturedStart(3),match.capturedLength(3),stopParameter->text());
      }
      result.replace(match.capturedStart(2),match.capturedLength(2),spacecraft->currentText());
      result.replace(match.capturedStart(1),match.capturedLength(1),propagator->currentText());
      const auto span=backwardSpan(original);
      result.replace(span.first,span.second,backward->isChecked() ? (span.second ? original.mid(span.first,span.second) : QString("BackProp ")) : QString());
      synchronizing=true; changed(result); synchronizing=false;
   };
   connect(propagator,&QComboBox::currentTextChanged,this,update);
   connect(spacecraft,&QComboBox::currentTextChanged,this,[this,update] {
      if (synchronizing) return;
      if (!units->currentData().toString().isEmpty()) { const QSignalBlocker block(stopParameter); stopParameter->setText(spacecraft->currentText()+"."+units->currentData().toString()); }
      update();
   });
   connect(units,&QComboBox::currentIndexChanged,this,[this,update] {
      if (synchronizing || units->currentData().toString().isEmpty()) return;
      { const QSignalBlocker block(stopParameter); stopParameter->setText(spacecraft->currentText()+"."+units->currentData().toString()); }
      update();
   });
   connect(stopParameter,&QLineEdit::textChanged,this,[this,update] {
      const QSignalBlocker block(units);
      const auto suffix=stopParameter->text().section('.',-1);
      units->setCurrentIndex(suffix=="ElapsedSecs" ? 0 : suffix=="ElapsedDays" ? 1 : 2); update();
   });
   connect(duration,&QLineEdit::textChanged,this,update);
   connect(tolerance,&QLineEdit::textChanged,this,update);
   connect(backward,&QCheckBox::toggled,this,update);
}
void PropagationForm::setStatement(const QString &statement)
{
   if (synchronizing) return;
   // Multiple propagators/stops stay in the source
   // editor until their full structure is representable here.
   const auto match=propagationPattern().match(statement);
   if (!match.hasMatch() || (match.capturedStart(4)<0 && !eventStop(match.captured(3))) || propagator->findText(match.captured(1))<0 || spacecraft->findText(match.captured(2))<0) { hide(); return; }
   synchronizing=true; original=statement;
   backward->setChecked(backwardSpan(statement).second>0); tolerance->setText(match.captured(6).trimmed());
   propagator->setCurrentText(match.captured(1)); spacecraft->setCurrentText(match.captured(2));
   stopParameter->setText(match.captured(3));
   const auto suffix=match.captured(3).section('.',-1);
   units->setCurrentIndex(suffix=="ElapsedSecs" ? 0 : suffix=="ElapsedDays" ? 1 : 2);
   duration->setText(match.captured(4).trimmed());
   duration->setEnabled(!eventStop(match.captured(3))); findChild<QPushButton *>("propagationChooseGoal")->setEnabled(duration->isEnabled());
   synchronizing=false; show();
}
