#include "PropagationGroupsDialog.hpp"
#include "TableColumns.hpp"
#include "Propagate.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSet>

namespace {
struct Groups { bool valid=false,backward=false,synchronized=false,stm=false,aMatrix=false; int start=0,length=0; QList<QPair<QString,QString>> rows; };
QStringList names(const QString &text) { return text.split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts); }
Groups parse(const QString &source)
{
   Groups result;
   const auto prefix=QRegularExpression("^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?").match(source); if (!prefix.hasMatch()) return result;
   result.start=prefix.capturedEnd();
   const auto end=QRegularExpression("(?:\\{[^{};]*\\}\\s*)?;[ \\t]*(?:%[^\\n]*)?\\s*$").match(source,result.start); if (!end.hasMatch()) return result;
   result.length=end.capturedStart()-result.start; QString header=source.mid(result.start,result.length).trimmed();
   if (header.startsWith("BackProp ")) { result.backward=true; header=header.mid(9).trimmed(); }
   if (header.startsWith("Synchronized ")) { result.synchronized=true; header=header.mid(13).trimmed(); }
   const QRegularExpression group("^([A-Za-z][A-Za-z0-9_]*)\\s*\\(\\s*((?:[A-Za-z][A-Za-z0-9_]*|'STM'|'AMatrix')(?:\\s*,\\s*(?:[A-Za-z][A-Za-z0-9_]*|'STM'|'AMatrix'))*)\\s*\\)");
   while (!header.isEmpty()) {
      const auto match=group.match(header); if (!match.hasMatch()) return result;
      QStringList objects;
      for (const auto &name:names(match.captured(2))) {
         if (name=="STM" || name=="'STM'") result.stm=true;
         else if (name=="AMatrix" || name=="'AMatrix'") result.aMatrix=true;
         else if (name=="Covariance") return result;
         else objects.append(name);
      }
      if (objects.isEmpty()) return result;
      result.rows.append({match.captured(1),objects.join(", ")}); header=header.mid(match.capturedLength()).trimmed();
   }
   result.valid=!result.rows.isEmpty(); return result;
}
}
bool PropagationGroupsDialog::supports(const QString &source) { return parse(source).valid; }
PropagationGroupsDialog::PropagationGroupsDialog(const QString &source,const QStringList &propagators,const QStringList &objects,QWidget *parent)
   : QDialog(parent),source(source)
{
   setObjectName("propagationGroupsDialog"); setWindowTitle("Propagators and spacecraft"); resize(720,430);
   const auto parsed=parse(source); start=parsed.start; length=parsed.length;
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Assign each spacecraft or formation to a propagator. Synchronized mode coordinates the propagators' steps. OK updates the command; Apply validates the mission.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *settings=new QHBoxLayout; layout->addLayout(settings); settings->addWidget(new QLabel("Mode",this));
   mode=new QComboBox(this); mode->setObjectName("propagationGroupMode"); Propagate defaults;
   for (const auto &name:defaults.GetStringArrayParameter(defaults.GetParameterID("AvailablePropModes"))) if (name!="BackProp") mode->addItem(name.empty() ? "Independent" : QString::fromStdString(name),QString::fromStdString(name));
   mode->setCurrentIndex(mode->findData(parsed.synchronized ? "Synchronized" : "")); settings->addWidget(mode);
   backward=new QCheckBox("Propagate backwards",this); backward->setObjectName("propagationGroupBackwards"); backward->setChecked(parsed.backward); settings->addWidget(backward); settings->addStretch();
   auto *variational=new QHBoxLayout; layout->addLayout(variational);
   stm=new QCheckBox("Propagate STM",this); stm->setObjectName("propagationGroupSTM"); stm->setChecked(parsed.stm); variational->addWidget(stm);
   aMatrix=new QCheckBox("Compute A-matrix",this); aMatrix->setObjectName("propagationGroupAMatrix"); aMatrix->setChecked(parsed.aMatrix); variational->addWidget(aMatrix); variational->addStretch();
   table=new QTableWidget(0,2,this); table->setObjectName("propagationGroupsTable"); table->setHorizontalHeaderLabels({"Propagator","Spacecraft / formations"}); table->verticalHeader()->hide();
   table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection); configureTableColumns(table,{24,40}); layout->addWidget(table);
   auto addRow=[=](const QString &prop,const QString &sats) {
      const int row=table->rowCount(); table->insertRow(row); auto *combo=new QComboBox(table); combo->addItems(propagators);
      if (combo->findText(prop)<0) combo->addItem(prop); combo->setCurrentText(prop); table->setCellWidget(row,0,combo);
      table->setItem(row,1,new QTableWidgetItem(sats)); table->selectRow(row);
   };
   for (const auto &row:parsed.rows) addRow(row.first,row.second); fitTableColumns(table);
   auto *controls=new QHBoxLayout; layout->addLayout(controls);
   auto button=[&](const QString &label,const QString &name) { auto *b=new QPushButton(label,this); b->setObjectName(name); controls->addWidget(b); return b; };
   auto *add=button("Add propagator","propagationGroupAdd"),*remove=button("Remove","propagationGroupRemove"),*choose=button("Choose spacecraft…","propagationGroupChoose"); controls->addStretch();
   auto *error=new QLabel(this); error->setObjectName("propagationGroupsError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   auto refresh=[=] {
      remove->setEnabled(table->currentRow()>=0); choose->setEnabled(table->currentRow()>=0); bool valid=parsed.valid && table->rowCount()>0; QSet<QString> assigned;
      for (int row=0;row<table->rowCount();++row) {
         auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,0)); auto *item=table->item(row,1);
         if (!combo || !item) { valid=false; continue; }
         valid=valid && !combo->currentText().isEmpty() && !names(item->text()).isEmpty();
         for (const auto &name:names(item->text())) { valid=valid && objects.contains(name) && !assigned.contains(name); assigned.insert(name); }
      }
      buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
      error->setText(valid ? "" : "Choose at least one object per row; each object may appear only once.");
   };
   auto connectCombo=[=](int row) {
      auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,0));
      connect(combo,&QComboBox::currentTextChanged,this,[=] {
         for (int r=0;r<table->rowCount();++r) if (table->cellWidget(r,0)==combo) { table->selectRow(r); break; }
         refresh();
      });
   };
   for (int row=0;row<table->rowCount();++row) connectCombo(row);
   connect(add,&QPushButton::clicked,this,[=] { addRow(propagators.value(0),{}); connectCombo(table->rowCount()-1); refresh(); });
   connect(remove,&QPushButton::clicked,this,[=] { table->removeRow(table->currentRow()); refresh(); });
   connect(choose,&QPushButton::clicked,this,[=] {
      const int row=table->currentRow(); if (row<0) return; const auto selected=names(table->item(row,1)->text());
      QDialog dialog(this); dialog.setObjectName("propagationObjectsDialog"); dialog.setWindowTitle("Select spacecraft and formations"); auto *box=new QVBoxLayout(&dialog);
      auto *list=new QListWidget(&dialog); list->setObjectName("propagationObjectsList"); list->setDragDropMode(QAbstractItemView::InternalMove); box->addWidget(list);
      auto candidates=selected; for (const auto &name:objects) if (!candidates.contains(name)) candidates.append(name);
      for (const auto &name:candidates) { auto *item=new QListWidgetItem(name,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked); }
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); box->addWidget(buttons);
      connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      if (dialog.exec()==QDialog::Accepted) { QStringList values; for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) values.append(list->item(i)->text()); table->item(row,1)->setText(values.join(", ")); }
   });
   connect(table,&QTableWidget::itemChanged,this,[=] { refresh(); }); connect(table,&QTableWidget::itemSelectionChanged,this,refresh);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject); connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept); refresh();
}
QString PropagationGroupsDialog::statement() const
{
   QStringList groups; if (backward->isChecked()) groups.append("BackProp"); if (!mode->currentData().toString().isEmpty()) groups.append(mode->currentData().toString());
   for (int row=0;row<table->rowCount();++row) {
      auto objects=names(table->item(row,1)->text());
      // These flags apply to the whole command in GMAT, including all groups.
      if (row==0 && stm->isChecked()) objects.append("'STM'");
      if (row==0 && aMatrix->isChecked()) objects.append("'AMatrix'");
      groups.append(qobject_cast<QComboBox *>(table->cellWidget(row,0))->currentText()+"("+objects.join(", ")+")");
   }
   QString result=source; result.replace(start,length,groups.join(" ")+" "); return result;
}
