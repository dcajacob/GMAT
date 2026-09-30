#include "BulkPropertyDialog.hpp"
#include "ParameterReferences.hpp"
#include <QListWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>

BulkPropertyDialog::BulkPropertyDialog(const QMap<QString,QStringList> &values,const QString &owner,const QString &property,QWidget *parent)
   : QDialog(parent),properties(values)
{
   setObjectName("bulkPropertyDialog"); setWindowTitle("Select object properties"); resize(760,540);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Select one or more objects, then check the properties to add. Only common properties are shown. Check all skips properties with no shared reference; select those separately with fewer objects.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *splitter=new QSplitter(this);
   owners=new QListWidget(splitter); owners->setObjectName("bulkPropertyObjects");
   owners->setSelectionMode(QAbstractItemView::ExtendedSelection); owners->addItems(properties.keys());
   table=new QTableWidget(splitter); table->setObjectName("bulkPropertyTable");
   table->setColumnCount(3); table->setHorizontalHeaderLabels({"Property","Reference type","Reference"});
   table->setEditTriggers(QAbstractItemView::NoEditTriggers);
   table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
   table->setColumnWidth(0,200); table->setColumnWidth(1,150); table->setColumnWidth(2,200);
   splitter->setStretchFactor(0,1); splitter->setStretchFactor(1,3); layout->addWidget(splitter,1);
   auto *actions=new QHBoxLayout;
   for (const auto &label:{QString("Select all objects"),QString("Check all properties"),QString("Uncheck all properties")}) {
      auto *button=new QPushButton(label,this); actions->addWidget(button);
      button->setObjectName(label=="Select all objects" ? "bulkSelectAllObjects" : label=="Check all properties" ? "bulkCheckAll" : "bulkUncheckAll");
      connect(button,&QPushButton::clicked,this,[this,label] {
         if (label=="Select all objects") { owners->selectAll(); return; }
         for (int row=0;row<table->rowCount();++row) {
            const auto *reference=qobject_cast<QComboBox *>(table->cellWidget(row,2));
            const bool available=!reference || reference->count()>0;
            table->item(row,0)->setCheckState(label=="Check all properties" && available ? Qt::Checked : Qt::Unchecked);
         }
      });
   }
   layout->addLayout(actions);
   status=new QLabel(this); status->setObjectName("bulkPropertyStatus"); status->setWordWrap(true); layout->addWidget(status);
   buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(owners,&QListWidget::itemSelectionChanged,this,[this] { refresh(); });
   connect(table,&QTableWidget::itemChanged,this,[this] { validate(); });
   const auto matches=owners->findItems(owner,Qt::MatchExactly);
   if (!matches.isEmpty()) matches.first()->setSelected(true); else refresh();
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==property) table->item(row,0)->setCheckState(Qt::Checked);
   validate();
}

void BulkPropertyDialog::refresh()
{
   QMap<QString,QPair<bool,QString>> previous;
   for (int row=0;row<table->rowCount();++row) {
      auto *reference=qobject_cast<QComboBox *>(table->cellWidget(row,2));
      previous[table->item(row,0)->text()]={table->item(row,0)->checkState()==Qt::Checked,reference ? reference->currentText() : QString()};
   }
   table->blockSignals(true); table->setRowCount(0);
   const auto selected=owners->selectedItems();
   QStringList common;
   if (!selected.isEmpty()) {
      common=properties.value(selected.first()->text()); common.removeDuplicates(); common.sort();
      for (auto *owner:selected) for (int i=common.size()-1;i>=0;--i)
         if (!properties.value(owner->text()).contains(common[i])) common.removeAt(i);
   }
   for (const auto &property:common) {
      const int row=table->rowCount(); table->insertRow(row);
      auto *item=new QTableWidgetItem(property); item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
      item->setCheckState(previous.value(property).first ? Qt::Checked : Qt::Unchecked); table->setItem(row,0,item);
      auto dep=parameterDependency(selected.first()->text(),property);
      for (auto *owner:selected) {
         const auto available=parameterDependency(owner->text(),property);
         for (int i=dep.choices.size()-1;i>=0;--i) if (!available.choices.contains(dep.choices[i])) dep.choices.removeAt(i);
      }
      table->setItem(row,1,new QTableWidgetItem(dep.needed ? dep.label : "None"));
      if (dep.needed) {
         auto *reference=new QComboBox(table); reference->setProperty("dependencyRequired",true);
         reference->addItems(dep.choices);
         const auto preferred=previous.contains(property) ? previous.value(property).second : dep.preferred;
         if (reference->findText(preferred)>=0) reference->setCurrentText(preferred);
         table->setCellWidget(row,2,reference);
         connect(reference,&QComboBox::currentTextChanged,this,[this] { validate(); });
      } else table->setItem(row,2,new QTableWidgetItem("—"));
   }
   table->blockSignals(false); validate();
}

void BulkPropertyDialog::validate()
{
   bool checked=false,missing=false;
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->checkState()==Qt::Checked) {
      checked=true;
      if (auto *reference=qobject_cast<QComboBox *>(table->cellWidget(row,2))) missing=missing || reference->count()==0;
   }
   const bool valid=!owners->selectedItems().isEmpty() && checked && !missing;
   buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
   status->setText(owners->selectedItems().isEmpty() ? "Select at least one object." : missing ? "A checked property has no reference shared by these objects. Select fewer objects or uncheck that property." : !checked ? "Check at least one property." : QString("%1 references will be added in object/property order.").arg(selection().size()));
}

QStringList BulkPropertyDialog::selection() const
{
   QStringList values;
   // List order, rather than selection-click order, makes report order stable.
   for (int index=0;index<owners->count();++index) if (owners->item(index)->isSelected())
      for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->checkState()==Qt::Checked) {
         auto *reference=qobject_cast<QComboBox *>(table->cellWidget(row,2));
         if (reference && reference->count()==0) continue;
         values.append(parameterReference(owners->item(index)->text(),table->item(row,0)->text(),reference ? reference->currentText() : QString()));
      }
   return values;
}
