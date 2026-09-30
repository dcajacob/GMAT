#include "XYPlotDialog.hpp"
#include "ReportParameterDialog.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSet>

XYPlotDialog::XYPlotDialog(GmatBase &plot,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("xyPlotDialog"); setWindowTitle("XY plot — "+QString::fromStdString(plot.GetName())); resize(620,500);
   QMap<QString,QString> initial;
   QStringList modes;
   for (const auto &field:resourceProperties(plot)) { initial.insert(field.name,field.value); if (field.name=="SolverIterations") modes=field.choices; }
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Select one X parameter and an ordered list of Y parameters. OK keeps edits pending; Apply updates the mission.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *options=new QGroupBox("Plot options",this); auto *form=new QFormLayout(options); layout->addWidget(options);
   show=new QCheckBox("Show plot",options); show->setObjectName("xyShowPlot"); show->setChecked(initial.value("ShowPlot")=="true"); form->addRow(show);
   grid=new QCheckBox("Show grid",options); grid->setObjectName("xyShowGrid"); grid->setChecked(initial.value("ShowGrid")=="true"); form->addRow(grid);
   solver=new QComboBox(options); solver->setObjectName("xySolverIterations"); solver->addItems(modes); solver->setCurrentText(initial.value("SolverIterations")); form->addRow("Solver iterations",solver);
   auto *xGroup=new QGroupBox("X parameter",this); auto *xLayout=new QHBoxLayout(xGroup); layout->addWidget(xGroup);
   x=new QLineEdit(initial.value("XVariable"),xGroup); x->setObjectName("xyXVariable"); xLayout->addWidget(x);
   auto *editX=new QPushButton("Select…",xGroup); editX->setObjectName("xySelectX"); xLayout->addWidget(editX);
   connect(editX,&QPushButton::clicked,this,[this] {
      ReportParameterDialog picker({x->text()},this,ReportParameterDialog::Mode::PlottableSingle);
      if (picker.exec()==QDialog::Accepted) x->setText(picker.selection().value(0));
   });
   auto *yGroup=new QGroupBox("Y parameters",this); auto *yLayout=new QVBoxLayout(yGroup); layout->addWidget(yGroup,1);
   y=new QListWidget(yGroup); y->setObjectName("xyYVariables"); y->addItems(splitResourceReferences(initial.value("YVariables")));
   y->setDragDropMode(QAbstractItemView::InternalMove); yLayout->addWidget(y);
   auto *actions=new QHBoxLayout; yLayout->addLayout(actions);
   auto *editY=new QPushButton("Select…",yGroup); editY->setObjectName("xySelectY"); actions->addWidget(editY);
   connect(editY,&QPushButton::clicked,this,[this] {
      ReportParameterDialog picker(splitResourceReferences(settings().value("YVariables")),this,ReportParameterDialog::Mode::PlottableMultiple);
      if (picker.exec()==QDialog::Accepted) { y->clear(); y->addItems(picker.selection()); }
   });
   for (const auto &label:QStringList{"Remove","Up","Down"}) {
      auto *button=new QPushButton(label,yGroup); button->setObjectName("xyY"+label); actions->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,label] {
         const int index=y->currentRow(); if (index<0) return;
         if (label=="Remove") { delete y->takeItem(index); return; }
         const int target=index+(label=="Up" ? -1 : 1); if (target<0 || target>=y->count()) return;
         auto *item=y->takeItem(index); y->insertItem(target,item); y->setCurrentRow(target);
      });
   }
   actions->addStretch();
   error=new QLabel(this); error->setObjectName("xyPlotError"); error->setWordWrap(true); layout->addWidget(error);
   connect(x,&QLineEdit::textChanged,error,&QLabel::clear);
   connect(show,&QCheckBox::toggled,error,&QLabel::clear);
   connect(y->model(),&QAbstractItemModel::rowsInserted,error,[this] { error->clear(); });
   connect(y->model(),&QAbstractItemModel::rowsRemoved,error,[this] { error->clear(); });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,previousX=initial.value("XVariable")] {
      if (show->isChecked() && (x->text().trimmed().isEmpty() || y->count()==0)) {
         error->setText("Select an X parameter and at least one Y parameter, or turn off Show plot."); return;
      }
      if (!previousX.isEmpty() && x->text().trimmed().isEmpty()) {
         error->setText("Select an X parameter. Turning off Show plot keeps the existing X parameter; it does not clear it."); return;
      }
      QStringList references; if (!x->text().trimmed().isEmpty()) references.append(x->text().trimmed());
      QSet<QString> selected;
      for (int i=0;i<y->count();++i) {
         const auto value=y->item(i)->text();
         if (selected.contains(value)) { error->setText("Each Y parameter must be unique."); return; }
         selected.insert(value); references.append(value);
      }
      for (const auto &value:references) if (!ReportParameterDialog::isPlottableReference(value)) {
         error->setText("Select a numeric plot parameter or a valid array element: "+value); return;
      }
      accept();
   });
}
QMap<QString,QString> XYPlotDialog::settings() const
{
   QStringList ys; for (int i=0;i<y->count();++i) ys.append(y->item(i)->text());
   return {{"XVariable",x->text().trimmed()},{"YVariables",ys.join(", ")},{"ShowPlot",show->isChecked() ? "true" : "false"},
      {"ShowGrid",grid->isChecked() ? "true" : "false"},{"SolverIterations",solver->currentText()}};
}
