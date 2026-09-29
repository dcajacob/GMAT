#include "CommandForm.hpp"
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpression>
#include <algorithm>
#include "ReportParameterDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include <QPushButton>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>

CommandForm::CommandForm(std::function<void(const QString &)> callback,QWidget *parent)
   : QGroupBox("Command settings",parent),layout(new QFormLayout(this)),changed(std::move(callback))
{
   setObjectName("commandForm"); hide();
}

void CommandForm::updateSource()
{
   if (synchronizing) return;
   QString result=original;
   // Spans refer to the original snapshot. Replace from right to left so a
   // changed field length cannot displace another field, or a branch body.
   auto ordered=fields;
   std::sort(ordered.begin(),ordered.end(),[](const Field &a,const Field &b) { return a.start==b.start ? a.length>b.length : a.start>b.start; });
   for (const auto &field:ordered) result.replace(field.start,field.length,field.input->text());
   synchronizing=true; changed(result); synchronizing=false;
}

void CommandForm::setStatement(const QString &statement)
{
   if (synchronizing) return;
   while (layout->rowCount()) layout->removeRow(0);
   fields.clear(); original=statement;
   const QString label="(?:'[^'\\n]*'\\s+)?";
   const QString name="([A-Za-z][A-Za-z0-9_]*)";
   const QString expression="([^,;{}\\n]+?)";
   const QString end="\\s*;[ \\t]*(?:%[^\\n]*)?\\s*$";
   struct Spec { QString type,pattern; QStringList labels; };
   const QVector<Spec> specs={
      {"Maneuver","Maneuver\\s+"+label+"(?:BackProp\\s+)?"+name+"\\s*\\(\\s*"+name+"\\s*\\)"+end,{"Burn","Spacecraft"}},
      {"Finite burn","(?:BeginFiniteBurn|EndFiniteBurn)\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*\\)"+end,{"Burn","Spacecraft"}},
      {"Vary","Vary\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*=\\s*"+expression+"\\s*(?:,\\s*\\{([^{};]*)\\})?\\s*\\)"+end,{"Solver","Variable","Initial value"}},
      {"Achieve","Achieve\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*=\\s*"+expression+"\\s*(?:,\\s*\\{([^{};]*)\\})?\\s*\\)"+end,{"Solver","Goal","Value"}},
      {"Minimize","Minimize\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*\\)"+end,{"Solver","Objective"}},
      {"Constraint","NonlinearConstraint\\s+"+label+name+"\\s*\\(\\s*([^,;{}\\n]+?)\\s*(<=|>=|=)\\s*"+expression+"\\s*\\)"+end,{"Solver","Left side","Relation","Right side"}},
      {"Report","Report\\s+"+label+name+"\\s+([^;%\\n]+?)"+end,{"Report file","Parameters"}},
      {"Event search","FindEvents\\s+"+label+name+"(?:\\s*\\{([^{};]*)\\})?"+end,{"Locator"}},
      {"Function call",label+"(\\[[^\\];\\n]*\\]|[A-Za-z][A-Za-z0-9_]*)\\s*=\\s*"+name+"\\s*\\(([^;\\n]*?)\\)"+end,{"Outputs","Function","Inputs"}},
      {"Function call",label+name+"\\s*\\(([^;\\n]*?)\\)"+end,{"Function","Inputs"}},
      {"Toggle","Toggle\\s+"+label+"([A-Za-z][A-Za-z0-9_]*(?:\\s+[A-Za-z][A-Za-z0-9_]*)*)\\s+(On|Off)"+end,{"Subscribers","State"}},
      {"Objects","(?:Global|Clear|Save)\\s+"+label+"([^;%\\n]+?)"+end,{"Objects"}},
      {"Assignment","(?:GMAT\\s+)?"+label+"([A-Za-z][A-Za-z0-9_.]*(?:\\([^;\\n]*?\\))?)\\s*=\\s*([^;\\n]+?)"+end,{"Destination","Expression"}},
      // Branch forms replace only header spans, never their nested commands.
      {"For loop","For\\s+"+label+name+"\\s*=\\s*([^:;\\n]+?)\\s*:\\s*([^:;\\n]+?)\\s*:\\s*([^;\\n]+?)\\s*;[\\s\\S]*$",{"Index","Start","Step","End"}},
      {"For loop","For\\s+"+label+name+"\\s*=\\s*([^:;\\n]+?)\\s*:\\s*([^:;\\n]+?)\\s*;[\\s\\S]*$",{"Index","Start","End"}},
      {"Condition","(?:If|While)\\s+"+label+"([^;%\\n]+?)[ \\t]*(?:;[^\\n]*|%[^\\n]*)?(?:\\n[\\s\\S]*)?$",{"Condition"}},
      // Only the first line is captured; the entire branch remains untouched.
      {"Solver branch","(?:Target|Optimize)\\s+"+label+name+"(?:\\s*\\{([^{};]*)\\})?\\s*;[\\s\\S]*$",{"Solver"}}
   };
   auto add=[&](const QString &name,qsizetype start,qsizetype length) {
      if (start<0) return;
      auto *input=new QLineEdit(statement.mid(start,length),this);
      input->setObjectName("commandField_"+name);
      QString resourceType;
      if (name=="Report file") resourceType="ReportFile";
      else if (name=="Burn") resourceType=title()=="Maneuver" ? "ImpulsiveBurn" : "FiniteBurn";
      else if (name=="Spacecraft" && (title()=="Maneuver" || title()=="Finite burn")) resourceType="Spacecraft";
      else if (name=="Locator") resourceType="EventLocator";
      else if (name=="Function") resourceType="Function";
      else if (name=="Solver") {
         if (title()=="Minimize" || title()=="Constraint" || QRegularExpression("^\\s*Optimize\\b").match(statement).hasMatch()) resourceType="Optimizer";
         else if (title()=="Achieve" || QRegularExpression("^\\s*Target\\b").match(statement).hasMatch()) resourceType="BoundaryValueSolver";
         else resourceType="Solver";
      }
      if (title()=="Toggle" && name=="State") {
         auto *state=new QComboBox(this); state->setObjectName("commandToggleState"); state->addItems({"On","Off"});
         state->setCurrentText(input->text()); input->setParent(state); input->hide(); layout->addRow(name,state);
         connect(state,&QComboBox::currentTextChanged,input,&QLineEdit::setText);
         connect(input,&QLineEdit::textChanged,state,&QComboBox::setCurrentText);
      } else if ((title()=="Toggle" && name=="Subscribers") || (title()=="Objects" && name=="Objects")) {
         const bool subscribers=title()=="Toggle";
         const bool global=QRegularExpression("^\\s*Global\\b").match(statement).hasMatch();
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input);
         auto *choose=new QPushButton("Select…",container); choose->setObjectName("commandChoose_"+name); row->addWidget(choose); layout->addRow(name,container);
         connect(choose,&QPushButton::clicked,this,[this,input,subscribers,global] {
            QDialog dialog(this); dialog.setObjectName(subscribers ? "toggleSubscriberDialog" : "commandObjectDialog"); dialog.setWindowTitle(subscribers ? "Select outputs" : "Select objects"); dialog.resize(400,350);
            auto *layout=new QVBoxLayout(&dialog); auto *list=new QListWidget(&dialog); list->setObjectName(subscribers ? "toggleSubscriberList" : "commandObjectList");
            list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
            const auto selected=input->text().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
            QStringList names=selected,available;
            auto *moderator=Moderator::Instance();
            auto candidates=moderator->GetListOfObjects(subscribers ? Gmat::SUBSCRIBER : Gmat::UNKNOWN_OBJECT);
            if (!subscribers) {
               const auto bodies=moderator->GetListOfObjects(Gmat::CELESTIAL_BODY);
               candidates.insert(candidates.end(),bodies.begin(),bodies.end());
            }
            for (const auto &name:candidates) {
               auto *object=moderator->GetConfiguredObject(name); if (!object) continue;
               if (global && object->IsAutomaticGlobal()) continue;
               if (!subscribers && object->IsOfType(Gmat::PARAMETER) && !object->IsOfType("Variable") &&
                  !object->IsOfType("Array") && !object->IsOfType("String")) continue;
               available.append(QString::fromStdString(name));
            }
            available.removeDuplicates();
            if (global) {
               auto *note=new QLabel("Automatically global resources are omitted from new choices.",&dialog);
               note->setWordWrap(true); layout->insertWidget(0,note);
            }
            available.sort(); for (const auto &name:available) if (!names.contains(name)) names.append(name);
            for (const auto &name:names) {
               auto *item=new QListWidgetItem(name,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
               item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked);
            }
            auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
            auto validate=[list,buttons] {
               bool any=false; for (int i=0;i<list->count();++i) any=any || list->item(i)->checkState()==Qt::Checked;
               buttons->button(QDialogButtonBox::Ok)->setEnabled(any);
            };
            connect(list,&QListWidget::itemChanged,&dialog,[validate] { validate(); }); validate();
            connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
            if (dialog.exec()==QDialog::Accepted) {
               QStringList names; for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) names.append(list->item(i)->text());
               input->setText(names.join(" "));
            }
         });
      } else if (!resourceType.isEmpty() || (title()=="Report" && name=="Parameters")) {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0);
         row->addWidget(input); auto *choose=new QPushButton("Select…",container);
         choose->setObjectName("commandChoose_"+name); row->addWidget(choose); layout->addRow(name,container);
         connect(choose,&QPushButton::clicked,this,[this,input,name,resourceType] {
            if (!resourceType.isEmpty()) {
               QStringList resources;
               for (const auto &value:Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT)) {
                  auto *object=Moderator::Instance()->GetConfiguredObject(value);
                  if (!object || !object->IsOfType(resourceType.toStdString())) continue;
                  if (resourceType=="Solver" && !object->IsOfType("BoundaryValueSolver") && !object->IsOfType("Optimizer")) continue;
                  resources.append(QString::fromStdString(value));
               }
               resources.sort(); bool accepted=false;
               const auto chosen=QInputDialog::getItem(this,"Select "+name,name,resources,
                  std::max(0,static_cast<int>(resources.indexOf(input->text()))),false,&accepted);
               if (accepted && !chosen.isEmpty()) input->setText(chosen);
            } else {
               ReportParameterDialog dialog(input->text().split(QRegularExpression("\\s+(?![^()]*\\))"),Qt::SkipEmptyParts),this);
               if (dialog.exec()==QDialog::Accepted) input->setText(dialog.selection().join(" "));
            }
         });
      } else layout->addRow(name,input);
      fields.append({input,start,length});
      connect(input,&QLineEdit::textChanged,this,[this] { updateSource(); });
   };
   for (const auto &spec:specs) {
      const auto match=QRegularExpression("^\\s*"+spec.pattern).match(statement);
      if (!match.hasMatch()) continue;
      setTitle(spec.type);
      if (spec.type=="Maneuver") {
         const auto prefix=QRegularExpression("^\\s*Maneuver\\s+"+label).match(statement);
         const auto keyword=QRegularExpression("^BackProp\\s+").match(statement.mid(prefix.capturedEnd()));
         const QString originalKeyword=keyword.hasMatch() ? keyword.captured() : QString();
         auto *value=new QLineEdit(originalKeyword,this); value->hide();
         fields.append({value,prefix.capturedEnd(),originalKeyword.size()});
         auto *backprop=new QCheckBox("Apply maneuver backwards in time",this); backprop->setObjectName("commandBackProp");
         value->setParent(backprop);
         backprop->setChecked(!originalKeyword.isEmpty()); layout->addRow("Backprop",backprop);
         connect(value,&QLineEdit::textChanged,this,[this] { updateSource(); });
         connect(backprop,&QCheckBox::toggled,this,[value,originalKeyword](bool checked) {
            value->setText(checked ? (originalKeyword.isEmpty() ? "BackProp " : originalKeyword) : QString());
         });
      }
      for (int i=0;i<spec.labels.size();++i) add(spec.labels[i],match.capturedStart(i+1),match.capturedLength(i+1));
      // Optional keyword values can occur in any order. Only recognized keys
      // become controls; all other option text is retained verbatim.
      const int options=spec.labels.size()+1;
      if (match.lastCapturedIndex()>=options && match.capturedStart(options)>=0) {
         const QRegularExpression option("(?:^|,)\\s*(Perturbation|Lower|Upper|MaxStep|AdditiveScaleFactor|MultiplicativeScaleFactor|Tolerance|SolveMode|ExitMode|ShowProgressWindow|Append)\\s*=\\s*([^,]+?)\\s*(?=,|$)");
         auto matches=option.globalMatch(match.captured(options));
         while (matches.hasNext()) {
            const auto setting=matches.next();
            add(setting.captured(1),match.capturedStart(options)+setting.capturedStart(2),setting.capturedLength(2));
         }
      }
      show(); return;
   }
   hide();
}
