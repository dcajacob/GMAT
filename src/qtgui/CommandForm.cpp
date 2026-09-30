#include "CommandForm.hpp"
#include "ConditionDialog.hpp"
#include "Target.hpp"
#include "Optimize.hpp"
#include "FindEvents.hpp"
#include "Vary.hpp"
#include "Achieve.hpp"
#include <memory>
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpression>
#include <algorithm>
#include "ReportParameterDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "DynamicDataDisplay.hpp"
#include "ResourceProperties.hpp"
#include "BaseException.hpp"
#include <stdexcept>
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

QString CommandForm::currentStatement() const
{
   QString result=original;
   // Spans refer to the original snapshot. Replace from right to left so a
   // changed field length cannot displace another field, or a branch body.
   auto ordered=fields;
   std::sort(ordered.begin(),ordered.end(),[](const Field &a,const Field &b) { return a.start==b.start ? a.length>b.length : a.start>b.start; });
   for (const auto &field:ordered) {
      auto value=field.input->text();
      if (((title()=="Dynamic update" && field.input->objectName()=="commandField_Parameters") || (title()=="File import" && field.input->objectName()=="commandField_Data")) && !value.isEmpty() && !value.front().isSpace()) value.prepend(' ');
      result.replace(field.start,field.length,value);
   }
   return result;
}
void CommandForm::updateSource()
{
   if (synchronizing) return;
   const auto result=currentStatement();
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
      {"File thrust","(?:BeginFileThrust|EndFileThrust)\\s+"+label+name+"\\s*\\(\\s*([^;()\\n]+?)\\s*\\)"+end,{"Thrust history","Spacecraft"}},
      {"Vary","Vary\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*=\\s*"+expression+"\\s*(?:,\\s*\\{([^{};]*)\\})?\\s*\\)"+end,{"Solver","Variable","Initial value"}},
      {"Achieve","Achieve\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*=\\s*"+expression+"\\s*(?:,\\s*\\{([^{};]*)\\})?\\s*\\)"+end,{"Solver","Goal","Value"}},
      {"Minimize","Minimize\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*\\)"+end,{"Solver","Objective"}},
      {"Constraint","NonlinearConstraint\\s+"+label+name+"\\s*\\(\\s*([^,;{}\\n]+?)\\s*(<=|>=|=)\\s*"+expression+"\\s*\\)"+end,{"Solver","Left side","Relation","Right side"}},
      {"Report","Report\\s+"+label+name+"\\s+([^;%\\n]+?)"+end,{"Report file","Parameters"}},
      {"Dynamic update","UpdateDynamicData\\s+"+label+name+"([^;%\\n]*?)"+end,{"Display","Parameters"}},
      {"File import","Set\\s+"+label+name+"\\s+"+name+"([ \\t]*(?:\\(\\s*Data\\s*=\\s*\\{[^{}();%\\n]*\\}\\s*\\))?)"+end,{"Target","Data source","Data"}},
      {"Measurement simulation","RunSimulator\\s+"+label+name+end,{"Simulator"}},
      {"Orbit estimation","RunEstimator\\s+"+label+name+end,{"Estimator"}},
      {"Orbit smoothing","RunSmoother\\s+"+label+name+end,{"Smoother"}},
      {"Event search","FindEvents\\s+"+label+name+"(?:\\s*\\{([^{};]*)\\})?"+end,{"Locator"}},
      {"Function call",label+"(\\[[^\\];\\n]*\\]|[A-Za-z][A-Za-z0-9_]*)\\s*=\\s*"+name+"\\s*\\(([^;\\n]*?)\\)"+end,{"Outputs","Function","Inputs"}},
      {"Function call",label+name+"\\s*\\(([^;\\n]*?)\\)"+end,{"Function","Inputs"}},
      {"Toggle","Toggle\\s+"+label+"([A-Za-z][A-Za-z0-9_]*(?:\\s+[A-Za-z][A-Za-z0-9_]*)*)\\s+(On|Off)"+end,{"Subscribers","State"}},
      {"Command echo","CommandEcho\\s+"+label+"(On|Off)"+end,{"State"}},
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
      if (title()=="For loop" && name=="Index") resourceType="Variable";
      else if (name=="Report file") resourceType="ReportFile";
      else if (title()=="Dynamic update" && name=="Display") resourceType="DynamicDataDisplay";
      else if (title()=="File import" && name=="Target") resourceType="Spacecraft";
      else if (title()=="File import" && name=="Data source") resourceType="DataInterface";
      else if (name=="Simulator") resourceType="Simulator";
      else if (name=="Estimator") resourceType="Estimator";
      else if (name=="Smoother") resourceType="Smoother";
      else if (name=="Thrust history") resourceType="ThrustHistoryFile";
      else if (name=="Burn") resourceType=title()=="Maneuver" ? "ImpulsiveBurn" : "FiniteBurn";
      else if (name=="Spacecraft" && (title()=="Maneuver" || title()=="Finite burn")) resourceType="Spacecraft";
      else if (name=="Locator") resourceType="EventLocator";
      else if (name=="Function") resourceType="Function";
      else if (name=="Solver") {
         if (title()=="Minimize" || title()=="Constraint" || QRegularExpression("^\\s*Optimize\\b").match(statement).hasMatch()) resourceType="Optimizer";
         else if (title()=="Achieve" || QRegularExpression("^\\s*Target\\b").match(statement).hasMatch()) resourceType="BoundaryValueSolver";
         else resourceType="Solver";
      }
      if (title()=="File thrust" && name=="Spacecraft") {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input);
         auto *choose=new QPushButton("Select spacecraft…",container); choose->setObjectName("commandChoose_Spacecraft"); row->addWidget(choose); layout->addRow(name,container);
         connect(choose,&QPushButton::clicked,this,[this,input] {
            QDialog dialog(this); dialog.setObjectName("fileThrustSpacecraftDialog"); dialog.setWindowTitle("File thrust spacecraft"); auto *layout=new QVBoxLayout(&dialog);
            auto *list=new QListWidget(&dialog); list->setObjectName("fileThrustSpacecraftList"); list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
            const auto selected=input->text().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts); QStringList names=selected;
            for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT)) if (!names.contains(QString::fromStdString(name))) names.append(QString::fromStdString(name));
            for (const auto &name:names) { auto *item=new QListWidgetItem(name,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked); }
            auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
            auto validate=[list,buttons] { bool any=false; for (int i=0;i<list->count();++i) any=any || list->item(i)->checkState()==Qt::Checked; buttons->button(QDialogButtonBox::Ok)->setEnabled(any); };
            connect(list,&QListWidget::itemChanged,&dialog,[validate] { validate(); }); validate();
            connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.resize(450,350);
            if (dialog.exec()==QDialog::Accepted) { QStringList values; for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) values.append(list->item(i)->text()); input->setText(values.join(", ")); }
         });
      } else if (title()=="File import" && name=="Data") {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input);
         input->setToolTip("Leave empty to import all fields, or select fields from the data interface.");
         auto *choose=new QPushButton("Select fields…",container); choose->setObjectName("commandChoose_Data"); row->addWidget(choose); layout->addRow("Data fields (empty imports all)",container);
         auto *error=new QLabel(this); error->setObjectName("commandDataError"); error->setWordWrap(true); layout->addRow(error);
         auto *source=findChild<QLineEdit *>("commandField_Data source");
         connect(source,&QLineEdit::textChanged,error,&QLabel::clear); connect(input,&QLineEdit::textChanged,error,&QLabel::clear);
         connect(choose,&QPushButton::clicked,this,[this,input,source,error] {
            try {
               auto *object=Moderator::Instance()->GetConfiguredObject(source->text().trimmed().toStdString());
               if (!object) throw std::runtime_error("Select an existing data-interface resource.");
               const auto fields=dataInterfaceFields(*object);
               const auto options=QRegularExpression("\\{([^{}]*)\\}").match(input->text());
               auto selected=options.captured(1).split(QRegularExpression("[\\s,'\\\"]+"),Qt::SkipEmptyParts);
               QDialog dialog(this); dialog.setObjectName("dataImportDialog"); dialog.setWindowTitle("Select data to import"); auto *layout=new QVBoxLayout(&dialog);
               auto *all=new QCheckBox("Import all fields",&dialog); all->setObjectName("dataImportAll"); all->setChecked(input->text().trimmed().isEmpty() || selected.contains("All")); layout->addWidget(all);
               auto *list=new QListWidget(&dialog); list->setObjectName("dataImportFields"); layout->addWidget(list);
               auto names=fields; for (const auto &name:selected) if (name!="All" && !names.contains(name)) names.append(name);
               for (const auto &name:names) { auto *item=new QListWidgetItem(name,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked); if (!fields.contains(name)) item->setToolTip("This field is not supported by the selected interface."); }
               auto *note=new QLabel("Select at least one supported field, or import all. Fields not selected leave the target's current values unchanged.",&dialog); note->setWordWrap(true); layout->addWidget(note);
               auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
               auto validate=[all,list,buttons,fields] {
                  list->setEnabled(!all->isChecked()); bool any=false,valid=true;
                  for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) { any=true; valid=valid && fields.contains(list->item(i)->text()); }
                  buttons->button(QDialogButtonBox::Ok)->setEnabled(all->isChecked() || (any && valid));
               };
               connect(all,&QCheckBox::toggled,&dialog,[validate] { validate(); }); connect(list,&QListWidget::itemChanged,&dialog,[validate] { validate(); }); validate();
               connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.resize(460,360);
               if (dialog.exec()==QDialog::Accepted) { QStringList names; for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) names.append("'"+list->item(i)->text()+"'"); input->setText(all->isChecked() ? QString() : " (Data = {"+names.join(", ")+"})"); }
               error->clear();
            } catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
            catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
         });
      } else if (title()=="Dynamic update" && name=="Parameters") {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input); auto *choose=new QPushButton("Select updates…",container); choose->setObjectName("commandChoose_Parameters"); row->addWidget(choose); layout->addRow("Parameters (empty updates all)",container);
         connect(choose,&QPushButton::clicked,this,[this,input] {
            auto *display=dynamic_cast<DynamicDataDisplay *>(Moderator::Instance()->GetConfiguredObject(findChild<QLineEdit *>("commandField_Display")->text().trimmed().toStdString()));
            if (!display) return;
            QDialog dialog(this); dialog.setObjectName("dynamicUpdateDialog"); dialog.setWindowTitle("Select parameters to update"); auto *layout=new QVBoxLayout(&dialog); layout->addWidget(new QLabel("Leave all unchecked to update every cell.",&dialog)); auto *list=new QListWidget(&dialog); list->setObjectName("dynamicUpdateList"); layout->addWidget(list);
            const auto selected=input->text().split(QRegularExpression("\\s+(?![^()]*\\))"),Qt::SkipEmptyParts);
            for (const auto &row:display->GetDynamicDataStruct()) for (const auto &cell:row) if (!cell.paramName.empty()) { const auto name=QString::fromStdString(cell.paramName); auto *item=new QListWidgetItem(name,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked); }
            auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.resize(450,350);
            if (dialog.exec()==QDialog::Accepted) { QStringList names; for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) names.append(list->item(i)->text()); input->setText(names.isEmpty() ? QString() : " "+names.join(" ")); }
         });
      } else if (title()=="Function call" && (name=="Inputs" || name=="Outputs")) {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input);
         auto *choose=new QPushButton("Select arguments…",container); choose->setObjectName("commandChoose_"+name); row->addWidget(choose); layout->addRow(name,container);
         connect(choose,&QPushButton::clicked,this,[this,input,name] {
            auto text=input->text().trimmed(); const bool outputs=name=="Outputs";
            if (outputs && text.startsWith('[') && text.endsWith(']')) text=text.mid(1,text.size()-2);
            QStringList selected; int start=0,depth=0; bool quoted=false;
            for (int i=0;i<text.size();++i) {
               const auto ch=text[i];
               if (ch=='\'') {
                  if (quoted && i+1<text.size() && text[i+1]=='\'') { ++i; continue; }
                  quoted=!quoted;
               } else if (!quoted) {
                  if (ch=='(' || ch=='[') ++depth;
                  else if (ch==')' || ch==']') --depth;
                  else if (ch==',' && depth==0) { selected.append(text.mid(start,i-start).trimmed()); start=i+1; }
               }
            }
            if (!text.isEmpty()) selected.append(text.mid(start).trimmed());
            ReportParameterDialog dialog(selected,this,outputs ? ReportParameterDialog::Mode::FunctionOutputs : ReportParameterDialog::Mode::FunctionInputs);
            if (dialog.exec()!=QDialog::Accepted) return;
            const auto values=dialog.selection().join(", "); input->setText(outputs ? "["+values+"]" : values);
         });
      } else if (title()=="Solver branch" && (name=="SolveMode" || name=="ExitMode")) {
         Target prototype;
         auto *choice=new QComboBox(this); choice->setObjectName("commandChoice_"+name);
         for (const auto &option:prototype.GetStringArrayParameter((name+"Options").toStdString())) choice->addItem(QString::fromStdString(option));
         if (choice->findText(input->text())<0) choice->addItem(input->text());
         choice->setCurrentText(input->text()); input->setParent(choice); input->hide(); layout->addRow(name,choice);
         connect(choice,&QComboBox::currentTextChanged,input,&QLineEdit::setText);
      } else if ((name=="ShowProgressWindow" || name=="Append") && (input->text()=="true" || input->text()=="false")) {
         auto *check=new QCheckBox(this); check->setObjectName("commandCheck_"+name); check->setChecked(input->text()=="true");
         input->setParent(check); input->hide(); layout->addRow(name,check);
         connect(check,&QCheckBox::toggled,input,[input](bool enabled) { input->setText(enabled ? "true" : "false"); });
      } else if (title()=="Condition" && name=="Condition") {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input);
         auto *edit=new QPushButton("Edit conditions…",container); edit->setObjectName("commandChoose_Condition"); row->addWidget(edit); layout->addRow(name,container);
         auto available=[input,edit] {
            edit->setEnabled(ConditionDialog::supports(input->text()));
            edit->setToolTip(edit->isEnabled() ? "Edit comparisons and logical joins" : "This syntax remains editable directly in the condition field");
         };
         connect(input,&QLineEdit::textChanged,edit,[available] { available(); }); available();
         connect(edit,&QPushButton::clicked,this,[this,input] {
            ConditionDialog dialog(input->text(),this);
            if (dialog.exec()==QDialog::Accepted) input->setText(dialog.condition());
         });
      } else if ((title()=="Toggle" || title()=="Command echo") && name=="State") {
         auto *state=new QComboBox(this); state->setObjectName(title()=="Toggle" ? "commandToggleState" : "commandEchoState"); state->addItems({"On","Off"});
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
      } else if ((title()=="Vary" && name=="Variable") || (title()=="Assignment" && name=="Destination") ||
                 (title()=="Achieve" && (name=="Goal" || name=="Value")) ||
                 (title()=="Minimize" && name=="Objective") ||
                 (title()=="Constraint" && (name=="Left side" || name=="Right side")) ||
                 (title()=="For loop" && (name=="Start" || name=="Step" || name=="End"))) {
         auto *container=new QWidget(this); auto *row=new QHBoxLayout(container); row->setContentsMargins(0,0,0,0); row->addWidget(input);
         auto *choose=new QPushButton("Select parameter…",container); choose->setObjectName("commandChoose_"+name); row->addWidget(choose); layout->addRow(name,container);
         const auto mode=title()=="Vary" ? ReportParameterDialog::Mode::WritableReal :
            title()=="Assignment" ? ReportParameterDialog::Mode::Writable : ReportParameterDialog::Mode::Single;
         connect(choose,&QPushButton::clicked,this,[this,input,mode] {
            ReportParameterDialog dialog({input->text()},this,mode);
            if (dialog.exec()==QDialog::Accepted) input->setText(dialog.selection().first());
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
                  if (resourceType=="Estimator" && object->IsOfType("Smoother")) continue;
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
      if (spec.type=="Solver branch" || spec.type=="Event search" || spec.type=="Vary" || spec.type=="Achieve") {
         std::unique_ptr<GmatCommand> prototype;
         if (spec.type=="Event search") prototype=std::make_unique<FindEvents>();
         else if (spec.type=="Vary") prototype=std::make_unique<Vary>();
         else if (spec.type=="Achieve") prototype=std::make_unique<Achieve>();
         else if (QRegularExpression("^\\s*Optimize\\b").match(statement).hasMatch()) prototype=std::make_unique<Optimize>();
         else prototype=std::make_unique<Target>();
         QStringList missing;
         const QStringList keys=spec.type=="Event search" ? QStringList{"Append"} :
            spec.type=="Vary" ? QStringList{"Perturbation","Lower","Upper","MaxStep","AdditiveScaleFactor","MultiplicativeScaleFactor"} :
            spec.type=="Achieve" ? QStringList{"Tolerance"} : QStringList{"SolveMode","ExitMode","ShowProgressWindow"};
         for (const auto &key:keys) {
            if (findChild<QLineEdit *>("commandField_"+key)) continue;
            const auto value=(key=="ShowProgressWindow" || key=="Append") ? (prototype->GetBooleanParameter(key.toStdString()) ? QString("true") : QString("false")) :
               QString::fromStdString(prototype->GetStringParameter(key.toStdString()));
            missing.append(key+" = "+value);
         }
         if (!missing.isEmpty()) {
            auto *defaults=new QPushButton("Add default options",this); defaults->setObjectName("commandAddOptions");
            defaults->setToolTip("Add omitted options using GMAT defaults; existing settings and command contents are preserved"); layout->addRow(defaults);
            const bool argumentOptions=spec.type=="Vary" || spec.type=="Achieve";
            connect(defaults,&QPushButton::clicked,this,[this,pattern=spec.pattern,options,missing,argumentOptions] {
               auto result=currentStatement(); const auto match=QRegularExpression("^\\s*"+pattern).match(result);
               if (!match.hasMatch()) return;
               if (match.capturedStart(options)>=0) result.insert(match.capturedEnd(options),(match.captured(options).trimmed().isEmpty() ? "" : ", ")+missing.join(", "));
               else result.insert(match.capturedEnd(argumentOptions ? 3 : 1),(argumentOptions ? ", {" : " {")+missing.join(", ")+"}");
               synchronizing=true; changed(result); synchronizing=false; setStatement(result);
            });
         }
      }
      if (spec.type=="Vary") {
         auto *solverField=findChild<QLineEdit *>("commandField_Solver");
         auto refreshCapabilities=[this,solverField] {
            auto *moderator=Moderator::Instance();
            auto *solver=moderator->IsInitialized() ? moderator->GetConfiguredObject(solverField->text().trimmed().toStdString()) : nullptr;
            if (solver && !solver->IsOfType("Solver")) solver=nullptr;
            const QList<QPair<QString,QString>> settings={
               {"Perturbation","AllowVariablePertSetting"},{"Lower","AllowRangeSettings"},
               {"Upper","AllowRangeSettings"},{"MaxStep","AllowStepsizeSetting"},
               {"AdditiveScaleFactor","AllowScaleSetting"},{"MultiplicativeScaleFactor","AllowScaleSetting"}};
            for (const auto &setting:settings) {
               auto *field=findChild<QLineEdit *>("commandField_"+setting.first); if (!field) continue;
               const bool enabled=solver && solver->GetBooleanParameter(setting.second.toStdString());
               field->setEnabled(enabled);
               field->setToolTip(enabled ? QString() : solver ? "The selected solver does not use this setting. Its value is retained." :
                  "Select a configured solver to edit this setting. Its value is retained.");
               if (auto *label=layout->labelForField(field)) label->setEnabled(enabled);
            }
         };
         connect(solverField,&QLineEdit::textChanged,this,[refreshCapabilities] { refreshCapabilities(); });
         refreshCapabilities();
      }
      show(); return;
   }
   hide();
}
