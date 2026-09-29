#include "ReportParameterDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
#include "BaseException.hpp"
#include <QFormLayout>
#include <QGroupBox>
#include <QMap>
#include "Array.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QDialogButtonBox>

ReportParameterDialog::ReportParameterDialog(const QStringList &selected,QWidget *parent) : QDialog(parent)
{
   setObjectName("reportParameterDialog"); setWindowTitle("Report parameters"); resize(600,450);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose a configured parameter, or enter a reference such as Sat.EarthMJ2000Eq.X. Apply validates references.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *entry=new QComboBox(this); entry->setObjectName("reportParameterEntry");
   entry->setEditable(true); entry->setInsertPolicy(QComboBox::NoInsert);
   auto *moderator=Moderator::Instance();
   QStringList names;
   for (const auto &name:moderator->GetListOfObjects(Gmat::PARAMETER)) {
      auto *parameter=moderator->GetParameter(name);
      if (parameter && parameter->IsReportable()) names.append(QString::fromStdString(name));
   }
   names.sort(); names.removeDuplicates(); entry->addItems(names); layout->addWidget(entry);
   auto *browser=new QGroupBox("Browse object properties",this);
   auto *form=new QFormLayout(browser);
   auto *owner=new QComboBox(browser); owner->setObjectName("reportPropertyObject");
   auto *property=new QComboBox(browser); property->setObjectName("reportPropertyType");
   auto *dependency=new QComboBox(browser); dependency->setObjectName("reportPropertyDependency");
   auto *dependencyLabel=new QLabel("Reference",browser);
   auto *use=new QPushButton("Use reference",browser); use->setObjectName("reportUseReference");
   form->addRow("Object",owner); form->addRow("Property",property); form->addRow(dependencyLabel,dependency); form->addRow(use);
   layout->addWidget(browser);
   auto *info=ParameterInfo::Instance();
   QMap<QString,QStringList> properties;
   for (const auto &type:info->GetTypesOfParameters()) {
      if (!info->IsReportable(type)) continue;
      const auto dep=info->GetDepObjectType(type);
      if (dep==GmatParam::OWNED_OBJ || dep==GmatParam::ATTACHED_OBJ || info->IsForOwnedObject(type) || info->IsForAttachedObject(type)) continue;
      const auto ownerType=info->GetObjectType(type);
      if (ownerType==Gmat::UNKNOWN_OBJECT || ownerType==Gmat::PARAMETER) continue;
      for (const auto &name:moderator->GetListOfObjects(ownerType)) properties[QString::fromStdString(name)].append(QString::fromStdString(type));
   }
   owner->addItems(properties.keys());
   auto updateDependency=[property,dependency,dependencyLabel,use,owner,info,moderator] {
      dependency->clear();
      const auto type=property->currentText().toStdString();
      const auto dep=info->GetDepObjectType(type);
      UnsignedInt objectType=Gmat::UNKNOWN_OBJECT;
      if (dep==GmatParam::COORD_SYS) { objectType=Gmat::COORDINATE_SYSTEM; dependencyLabel->setText("Coordinate system"); }
      else if (dep==GmatParam::ORIGIN) { objectType=Gmat::CELESTIAL_BODY; dependencyLabel->setText("Central body"); }
      else if (dep==GmatParam::ODE_MODEL) { objectType=Gmat::ODE_MODEL; dependencyLabel->setText("Force model"); }
      const bool needed=objectType!=Gmat::UNKNOWN_OBJECT;
      dependency->setVisible(needed); dependencyLabel->setVisible(needed);
      if (needed) for (const auto &name:moderator->GetListOfObjects(objectType)) {
         auto *object=moderator->GetConfiguredObject(name);
         if (!object) continue;
         if (dep==GmatParam::COORD_SYS) {
            try {
               if (info->RequiresBodyFixedCS(type) && object->GetStringParameter("Axes")!="BodyFixed") continue;
               if (info->RequiresCelestialBodyCSOrigin(type)) {
                  auto *origin=moderator->GetConfiguredObject(object->GetStringParameter("Origin"));
                  if (!origin || !origin->IsOfType(Gmat::CELESTIAL_BODY)) continue;
               }
            } catch (BaseException &) { continue; }
         }
         dependency->addItem(QString::fromStdString(name));
      }
      const auto preferred=dep==GmatParam::COORD_SYS ? "EarthMJ2000Eq" : "Earth";
      if (dependency->findText(preferred)>=0) dependency->setCurrentText(preferred);
      use->setEnabled(!owner->currentText().isEmpty() && !property->currentText().isEmpty() && (!needed || dependency->count()>0));
   };
   auto updateProperties=[owner,property,properties,updateDependency] {
      property->clear(); auto choices=properties.value(owner->currentText()); choices.removeDuplicates(); choices.sort();
      property->addItems(choices); updateDependency();
   };
   connect(owner,&QComboBox::currentTextChanged,this,[updateProperties] { updateProperties(); });
   connect(property,&QComboBox::currentTextChanged,this,[updateDependency] { updateDependency(); });
   connect(use,&QPushButton::clicked,this,[owner,property,dependency,entry] {
      QStringList parts{owner->currentText()};
      if (!dependency->isHidden()) parts.append(dependency->currentText());
      parts.append(property->currentText()); entry->setEditText(parts.join('.'));
   });
   updateProperties();
   auto *indices=new QHBoxLayout;
   auto *row=new QSpinBox(this); row->setObjectName("reportArrayRow"); row->setMinimum(1);
   auto *column=new QSpinBox(this); column->setObjectName("reportArrayColumn"); column->setMinimum(1);
   indices->addWidget(new QLabel("Array row",this)); indices->addWidget(row);
   indices->addWidget(new QLabel("Column",this)); indices->addWidget(column);
   auto *element=new QPushButton("Add element",this); element->setObjectName("reportAddElement"); indices->addWidget(element);
   auto *add=new QPushButton("Add parameter",this); add->setObjectName("reportAddParameter"); indices->addWidget(add);
   layout->addLayout(indices);
   list=new QListWidget(this); list->setObjectName("reportSelectedParameters"); list->addItems(selected);
   list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
   auto addName=[this](const QString &name) {
      if (name.isEmpty()) return;
      list->addItem(name); list->setCurrentRow(list->count()-1);
   };
   connect(add,&QPushButton::clicked,this,[entry,addName] { addName(entry->currentText().trimmed()); });
   connect(element,&QPushButton::clicked,this,[entry,row,column,addName] {
      addName(entry->currentText()+QString("(%1,%2)").arg(row->value()).arg(column->value()));
   });
   auto updateArray=[entry,row,column,element,moderator] {
      auto *array=dynamic_cast<Array *>(moderator->GetConfiguredObject(entry->currentText().toStdString()));
      row->setEnabled(array); column->setEnabled(array); element->setEnabled(array);
      if (array) {
         row->setMaximum(array->GetIntegerParameter("NumRows"));
         column->setMaximum(array->GetIntegerParameter("NumCols"));
      }
   };
   connect(entry,&QComboBox::currentTextChanged,this,[updateArray] { updateArray(); }); updateArray();
   auto *actions=new QHBoxLayout;
   for (const auto &label:{QString("Remove"),QString("Up"),QString("Down")}) {
      auto *button=new QPushButton(label,this); button->setObjectName("reportParameter"+label); actions->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,label] {
         const int index=list->currentRow(); if (index<0) return;
         if (label=="Remove") { delete list->takeItem(index); return; }
         const int destination=index+(label=="Up" ? -1 : 1);
         if (destination<0 || destination>=list->count()) return;
         auto *item=list->takeItem(index); list->insertItem(destination,item); list->setCurrentRow(destination);
      });
   }
   layout->addLayout(actions);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
QStringList ReportParameterDialog::selection() const
{
   QStringList names;
   for (int i=0;i<list->count();++i) names.append(list->item(i)->text());
   return names;
}
