#include "ResourceEditor.hpp"
#include "FunctionFileDialog.hpp"
#include "Moderator.hpp"
#include "AxisSystem.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <memory>
#include <stdexcept>
#include "ResourceProperties.hpp"
#include "ReportParameterDialog.hpp"
#include "RgbColor.hpp"
#include <QColorDialog>
#include <QPixmap>
#include "ScriptCompatibility.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "TableColumns.hpp"
#include <QTimer>
#include <QSignalBlocker>
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTabBar>
#include <QTabWidget>
#include <QSet>
#include <QDialog>
#include <QSpinBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QListWidget>
#include <QFormLayout>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>

namespace {
QString comboValue(const QComboBox *combo)
{
   if (!combo->property("resourceValueData").toBool()) return combo->currentText();
   const int index=combo->findText(combo->currentText());
   return index>=0 ? combo->itemData(index).toString() : combo->currentText();
}
}

ResourceEditor::ResourceEditor(GmatBase &object, Apply apply, QWidget *parent,const QString &script,bool applyUnchanged) : EditablePanel(parent)
{
   auto *layout = new QVBoxLayout(this);
   layout->addWidget(new QLabel(QString::fromStdString(object.GetName() + " — " + object.GetTypeName()), this));
   if (object.IsOfType("CoordinateSystem") && object.GetOwnedObject(0)) {
      // Retain a snapshot: applying any resource reconstructs the engine model.
      auto initial=std::shared_ptr<GmatBase>(object.GetOwnedObject(0)->Clone());
      auto *axesButton=new QPushButton("Axes…",this); axesButton->setObjectName("editCoordinateAxes");
      layout->addWidget(axesButton);
      connect(axesButton,&QPushButton::clicked,this,[this,initial,apply] {
         if (hasChanges()) { status->setText("Apply or discard pending property changes before opening Axes."); return; }
         QDialog dialog(this); dialog.setObjectName("coordinateAxesDialog"); dialog.setWindowTitle("Coordinate system axes");
         auto *layout=new QVBoxLayout(&dialog);
         auto *type=new QComboBox(&dialog); type->setObjectName("coordinateAxisType");
         for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::AXIS_SYSTEM)) type->addItem(QString::fromStdString(name));
         type->setCurrentText(QString::fromStdString(initial->GetTypeName())); layout->addWidget(type);
         auto *help=new QLabel("Choose an axis type and set its dependent properties. Changing type resets pending axis edits. Apply validates the complete coordinate system.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         ResourceEditor *panel=nullptr; bool completed=false;
         auto rebuild=[&] {
            if (panel) { delete panel; panel=nullptr; }
            try {
               std::unique_ptr<GmatBase> axes(type->currentText()==QString::fromStdString(initial->GetTypeName()) ? initial->Clone() :
                  Moderator::Instance()->CreateAxisSystem(type->currentText().toStdString(),"",0));
               if (!axes) { help->setText("The engine could not create these axes."); return; }
               panel=new ResourceEditor(*axes,[&,selected=type->currentText()](const QMap<QString,QString> &changes) {
                  auto candidate=changes; candidate.insert("Axes",selected);
                  const auto error=apply(candidate); completed=error.isEmpty(); return error;
               },&dialog,{},true);
               layout->addWidget(panel);
            } catch (BaseException &error) { help->setText(QString::fromStdString(error.GetFullMessage())); }
         };
         connect(type,&QComboBox::currentTextChanged,&dialog,[&] { rebuild(); }); rebuild();
         dialog.resize(700,600); dialog.exec();
         if (completed) { applied=true; parentWidget()->close(); }
      });
   }
   auto *search = new QLineEdit(this);
   search->setObjectName("propertyFilter");
   search->setPlaceholderText("Filter properties…");
   QTabBar *sections=nullptr;
   const bool spacecraft=object.IsOfType("Spacecraft");
   const bool thruster=object.IsOfType("Thruster");
   const bool tank=object.IsOfType("FuelTank");
   const bool solver=object.IsOfType("Solver");
   const bool forces=object.IsOfType("ODEModel");
   const bool power=object.IsOfType("PowerSystem");
   if (spacecraft || thruster || tank || solver || forces || power) {
      sections=new QTabBar(this);
      sections->setObjectName("propertySections");
      sections->setExpanding(false);
      layout->addWidget(sections);
   }
   if (object.IsOfType("ChemicalThruster") || object.IsOfType("ElectricThruster")) {
      const bool electric=object.IsOfType("ElectricThruster");
      QStringList names[2],units[2];
      for (int family=0;family<2;++family) {
         const auto values=object.GetStringArrayParameter(electric ? (family ? "MF_UNITS" : "T_UNITS") : (family ? "K_UNITS" : "C_UNITS"));
         const QString prefix=electric ? (family ? "MassFlowCoeff" : "ThrustCoeff") : (family ? "K" : "C");
         for (size_t i=0;i<values.size();++i) {
            names[family].append(prefix+QString::number(i+1)); units[family].append(QString::fromStdString(values[i]));
         }
      }
      auto *button=new QPushButton("Coefficients…",this); button->setObjectName("thrusterCoefficients"); layout->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,electric,names,units] {
         QDialog dialog(this); dialog.setObjectName("thrusterCoefficientDialog"); dialog.setWindowTitle("Thruster coefficients");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Edit the coefficient sets below. OK keeps changes pending until you Apply the thruster properties.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *tabs=new QTabWidget(&dialog); layout->addWidget(tabs);
         QTableWidget *grids[2];
         QList<QTableWidgetItem *> targets[2];
         for (int family=0;family<2;++family) {
            auto *grid=new QTableWidget(names[family].size(),3,tabs); grids[family]=grid;
            grid->setObjectName(family ? "thrusterSecondaryCoefficients" : "thrusterThrustCoefficients");
            grid->setHorizontalHeaderLabels({"Coefficient","Value","Unit"}); grid->verticalHeader()->hide();
            for (int row=0;row<names[family].size();++row) {
               QTableWidgetItem *target=nullptr;
               for (int r=0;r<table->rowCount();++r) if (table->item(r,0)->text()==names[family][row]) { target=table->item(r,1); break; }
               targets[family].append(target);
               grid->setItem(row,0,new QTableWidgetItem(names[family][row]));
               grid->setItem(row,1,new QTableWidgetItem(target ? target->text() : QString()));
               grid->setItem(row,2,new QTableWidgetItem(units[family][row]));
               for (int column:{0,2}) grid->item(row,column)->setFlags(grid->item(row,column)->flags() & ~Qt::ItemIsEditable);
            }
            configureTableColumns(grid,{18,24,16}); fitTableColumns(grid);
            tabs->addTab(grid,family ? (electric ? "Mass Flow Coefficients" : "Impulse Coefficients") : "Thrust Coefficients");
         }
         auto *error=new QLabel(&dialog); error->setObjectName("thrusterCoefficientError"); error->setWordWrap(true); layout->addWidget(error);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
            for (int family=0;family<2;++family) for (int row=0;row<grids[family]->rowCount();++row) {
               bool valid=false; const double value=grids[family]->item(row,1)->text().trimmed().toDouble(&valid);
               if (!targets[family][row] || !valid || !std::isfinite(value)) {
                  error->setText(names[family][row]+" must be a finite number."); tabs->setCurrentIndex(family);
                  grids[family]->setCurrentCell(row,1); return;
               }
            }
            dialog.accept();
         });
         dialog.resize(680,530);
         if (dialog.exec()!=QDialog::Accepted) return;
         for (int family=0;family<2;++family) for (int row=0;row<grids[family]->rowCount();++row)
            if (targets[family][row]) targets[family][row]->setText(grids[family]->item(row,1)->text().trimmed());
      });
   }
   if (thruster) {
      QStringList available;
      for (const auto &field:resourceProperties(object)) if (field.name=="Tank") available=field.references;
      auto *button=new QPushButton("Tanks and mixtures…",this); button->setObjectName("thrusterTankMixtures"); layout->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,available] {
         QTableWidgetItem *tanks=nullptr,*ratios=nullptr;
         for (int row=0;row<table->rowCount();++row) {
            if (table->item(row,0)->text()=="Tank") tanks=table->item(row,1);
            if (table->item(row,0)->text()=="MixRatio") ratios=table->item(row,1);
         }
         if (!tanks || !ratios) return;
         QDialog dialog(this); dialog.setObjectName("tankMixtureDialog"); dialog.setWindowTitle("Tanks and mixture ratios");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Each ratio belongs to the tank on the same row. Ratios must be positive; they need not sum to one. OK keeps changes pending until Apply.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *grid=new QTableWidget(0,2,&dialog); grid->setObjectName("tankMixtureTable"); grid->setHorizontalHeaderLabels({"Tank","Mixture ratio"});
         grid->setSelectionBehavior(QAbstractItemView::SelectRows); grid->setSelectionMode(QAbstractItemView::SingleSelection);
         grid->verticalHeader()->hide(); configureTableColumns(grid,{24,20}); layout->addWidget(grid);
         auto append=[&](const QString &name,const QString &ratio) {
            const int row=grid->rowCount(); grid->insertRow(row); auto *item=new QTableWidgetItem(name);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); grid->setItem(row,0,item); grid->setItem(row,1,new QTableWidgetItem(ratio)); grid->selectRow(row);
         };
         const auto selected=splitResourceReferences(tanks->text());
         const auto values=ratios->text().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         for (int i=0;i<selected.size();++i) append(selected[i],i<values.size() ? values[i] : "1");
         fitTableColumns(grid);
         auto *controls=new QHBoxLayout; layout->addLayout(controls);
         auto *choices=new QComboBox(&dialog); choices->setObjectName("tankMixtureAvailable"); choices->addItems(available); controls->addWidget(choices,1);
         auto addButton=[&](const QString &text,const QString &name) { auto *control=new QPushButton(text,&dialog); control->setObjectName(name); controls->addWidget(control); return control; };
         auto *add=addButton("Add","tankMixtureAdd"),*remove=addButton("Remove","tankMixtureRemove");
         auto *up=addButton("Up","tankMixtureUp"),*down=addButton("Down","tankMixtureDown");
         auto refresh=[&] {
            bool exists=false;
            for (int row=0;row<grid->rowCount();++row) exists=exists || grid->item(row,0)->text()==choices->currentText();
            add->setEnabled(choices->count()>0 && !exists); remove->setEnabled(grid->currentRow()>=0);
            up->setEnabled(grid->currentRow()>0); down->setEnabled(grid->currentRow()>=0 && grid->currentRow()+1<grid->rowCount());
         };
         connect(add,&QPushButton::clicked,&dialog,[&] { append(choices->currentText(),"1"); refresh(); });
         connect(remove,&QPushButton::clicked,&dialog,[&] { grid->removeRow(grid->currentRow()); refresh(); });
         auto move=[&](int delta) {
            const int row=grid->currentRow(),target=row+delta;
            if (row<0 || target<0 || target>=grid->rowCount()) return;
            const QSignalBlocker block(grid);
            for (int col=0;col<2;++col) { auto *first=grid->takeItem(row,col),*second=grid->takeItem(target,col); grid->setItem(row,col,second); grid->setItem(target,col,first); }
            grid->selectRow(target); refresh();
         };
         connect(up,&QPushButton::clicked,&dialog,[&] { move(-1); }); connect(down,&QPushButton::clicked,&dialog,[&] { move(1); });
         connect(choices,&QComboBox::currentTextChanged,&dialog,[&] { refresh(); });
         connect(grid,&QTableWidget::itemSelectionChanged,&dialog,[&] { refresh(); }); refresh();
         auto *error=new QLabel(&dialog); error->setObjectName("tankMixtureError"); error->setWordWrap(true); layout->addWidget(error);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
            for (int row=0;row<grid->rowCount();++row) {
               bool valid=false; const double value=grid->item(row,1)->text().toDouble(&valid);
               if (!valid || !std::isfinite(value) || value<=0) { error->setText("Enter a finite ratio greater than zero for "+grid->item(row,0)->text()+"."); grid->setCurrentCell(row,1); return; }
            }
            dialog.accept();
         });
         dialog.resize(620,420); if (dialog.exec()!=QDialog::Accepted) return;
         QStringList names,mixes;
         for (int row=0;row<grid->rowCount();++row) { names.append(grid->item(row,0)->text()); mixes.append(grid->item(row,1)->text().trimmed()); }
         tanks->setText(names.join(", ")); ratios->setText(mixes.join(" ")); ratios->setData(Qt::UserRole+1,mixes.size()); pairedTankEdits=true;
      });
   }
   if (object.IsOfType("GmatFunction")) {
      auto *edit=new QPushButton("Edit function file…",this); edit->setObjectName("editFunctionFile"); layout->addWidget(edit);
      connect(edit,&QPushButton::clicked,this,[this] {
         QString path;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="FunctionPath") path=table->item(row,1)->text();
         if (path.trimmed().isEmpty()) { status->setText("Choose a function file first."); return; }
         FunctionFileDialog dialog(path,this); dialog.exec();
      });
   }
   if (object.IsOfType("Array")) {
      auto *button=new QPushButton("Expressions…",this); button->setObjectName("arrayExpressions"); layout->addWidget(button);
      const auto name=QString::fromStdString(object.GetName());
      const int rows=object.GetIntegerParameter("NumRows"),columns=object.GetIntegerParameter("NumCols");
      try { originalExpressions=expressions=arrayExpressions(script,name); setArrayExpressions(script,name,expressions,rows,columns); }
      catch (const std::exception &error) { button->setEnabled(false); button->setToolTip(QString::fromUtf8(error.what())); }
      connect(button,&QPushButton::clicked,this,[this,rows,columns] {
         int pendingRows=rows,pendingColumns=columns;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="RmatValue") {
            const auto lines=table->item(row,1)->text().split(';');
            pendingRows=lines.size(); pendingColumns=0;
            for (const auto &line:lines) pendingColumns=std::max(pendingColumns,static_cast<int>(line.trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).size()));
         }
         int gridRows=pendingRows,gridColumns=pendingColumns;
         for (const auto &value:QJsonDocument::fromJson(expressions.toUtf8()).array()) {
            const auto cell=value.toObject();
            gridRows=std::max(gridRows,cell["row"].toInt()); gridColumns=std::max(gridColumns,cell["column"].toInt());
         }
         QDialog dialog(this); dialog.setObjectName("arrayExpressionDialog"); dialog.setWindowTitle("Array expressions");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Nonempty cells run at mission start, in row order, before existing commands. Blank cells keep their numeric initial values. Apply validates formulas without running them. Numeric values and formulas are applied together.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *grid=new QTableWidget(gridRows,gridColumns,&dialog); grid->setObjectName("arrayExpressionGrid");
         for (int r=0;r<gridRows;++r) for (int c=0;c<gridColumns;++c) {
            auto *item=new QTableWidgetItem; grid->setItem(r,c,item);
            if (r>=pendingRows || c>=pendingColumns) { item->setBackground(QColor(255,235,190)); item->setToolTip("Outside the pending array dimensions. Clear this formula or cancel and enlarge the numeric grid."); }
         }
         for (const auto &value:QJsonDocument::fromJson(expressions.toUtf8()).array()) {
            const auto cell=value.toObject(); const int r=cell["row"].toInt()-1,c=cell["column"].toInt()-1;
            if (r>=0 && r<gridRows && c>=0 && c<gridColumns) grid->item(r,c)->setText(cell["expression"].toString());
         }
         configureTableColumns(grid); fitTableColumns(grid); layout->addWidget(grid);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         auto *error=new QLabel(&dialog); error->setWordWrap(true); layout->addWidget(error);
         const auto validate=[=] {
            bool valid=pendingRows>0 && pendingColumns>0;
            for (int r=0;r<gridRows;++r) for (int c=0;c<gridColumns;++c)
               if ((r>=pendingRows || c>=pendingColumns) && !grid->item(r,c)->text().trimmed().isEmpty()) valid=false;
            buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
            error->setText(valid ? "" : "Clear formulas in highlighted cells, or cancel and enlarge the numeric grid.");
         };
         connect(grid,&QTableWidget::itemChanged,&dialog,[=] { validate(); }); validate();
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
         dialog.resize(650,350);
         if (dialog.exec()!=QDialog::Accepted) return;
         QJsonArray cells;
         for (int r=0;r<gridRows;++r) for (int c=0;c<gridColumns;++c) {
            const auto text=grid->item(r,c)->text().trimmed();
            if (!text.isEmpty()) cells.append(QJsonObject{{"row",r+1},{"column",c+1},{"expression",text}});
         }
         expressions=QString::fromUtf8(QJsonDocument(cells).toJson(QJsonDocument::Compact));
      });
   }
   layout->addWidget(search);
   table = new QTableWidget(this);
   table->setColumnCount(4);
   table->setHorizontalHeaderLabels({"Property", "Value", "Unit", ""});
   configureTableColumns(table,{20,28,5,10});
   table->verticalHeader()->hide();
   for (const auto &field : resourceProperties(object)) {
      int row = table->rowCount(); table->insertRow(row);
      original.insert(field.name, field.value);
      auto *name = new QTableWidgetItem(field.name);
      name->setFlags(name->flags() & ~Qt::ItemIsEditable);
      table->setItem(row, 0, name);
      if (object.GetTypeName()=="ReportFile" && field.name=="Delimiter") {
         auto *choices=new QComboBox(table);
         choices->setObjectName("reportDelimiter");
         choices->setEditable(true);
         choices->setInsertPolicy(QComboBox::NoInsert);
         choices->setProperty("resourceValueData",true);
         choices->addItem("Space"," "); choices->addItem("Tab","\t");
         choices->addItem("Comma",","); choices->addItem("Semicolon",";");
         choices->addItem("Pipe","|");
         if (choices->findData(field.value)<0) choices->addItem(field.value,field.value);
         choices->setCurrentIndex(choices->findData(field.value));
         table->setCellWidget(row,1,choices);
      } else if (field.choices.isEmpty()) {
         auto *value=new QTableWidgetItem(field.value);
         if (field.fileList) {
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
            value->setToolTip("Ordered kernel file paths. Browse to add, remove or reorder files; Apply replaces the complete list.");
         } else if (field.list) value->setToolTip("Comma-separated resource or parameter names. Apply replaces the complete list.");
         if (field.rows>0 && field.columns>0) {
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
            value->setData(Qt::UserRole,field.rows);
            value->setData(Qt::UserRole+1,field.columns);
            value->setData(Qt::UserRole+2,object.IsOfType("Array") && field.name=="RmatValue");
            value->setToolTip("Double-click to edit the numeric cells.");
            auto *edit=new QPushButton("Edit cells…",table);
            edit->setObjectName("editCells_"+field.name);
            table->setCellWidget(row,3,edit);
            connect(edit,&QPushButton::clicked,this,[this,value] { table->itemDoubleClicked(value); });
         }
         table->setItem(row, 1, value);
         if (field.color) {
            value->setToolTip("Enter a GMAT color name or [red green blue], or choose a color. Components range from 0 to 255.");
            auto *choose=new QPushButton("Choose color…",table); choose->setObjectName("chooseProperty_"+field.name); table->setCellWidget(row,3,choose);
            auto currentColor=[value] {
               try { return QColor::fromRgb(RgbColor::ToIntColor(value->text().trimmed().toStdString()) & 0xffffff); }
               catch (BaseException &) { return QColor(); }
            };
            auto preview=[choose,currentColor] {
               const auto color=currentColor(); QPixmap swatch(24,16); swatch.fill(color.isValid() ? color : Qt::transparent);
               choose->setIcon(QIcon(swatch)); choose->setToolTip(color.isValid() ? color.name() : "Invalid color; choose a replacement");
            };
            connect(table,&QTableWidget::itemChanged,choose,[value,preview](QTableWidgetItem *changed) { if (changed==value) preview(); }); preview();
            connect(choose,&QPushButton::clicked,this,[this,value,currentColor] {
               QColorDialog dialog(currentColor(),this); dialog.setObjectName("resourceColorDialog"); dialog.setWindowTitle("Choose color");
               if (dialog.exec()==QDialog::Accepted) {
                  const auto color=dialog.selectedColor();
                  value->setText(QString("[%1 %2 %3]").arg(color.red()).arg(color.green()).arg(color.blue()));
               }
            });
         }
         const bool reportParameters=object.GetTypeName()=="ReportFile" && field.name=="Add";
         if (field.filename || !field.references.isEmpty() || reportParameters) {
            auto *choose=new QPushButton(field.filename ? "Browse…" : "Select…",table);
            choose->setObjectName("chooseProperty_"+field.name);
            table->setCellWidget(row,3,choose);
            connect(choose,&QPushButton::clicked,this,[this,value,field,reportParameters] {
               if (reportParameters) {
                  ReportParameterDialog dialog(splitResourceReferences(value->text()),this);
                  if (dialog.exec()==QDialog::Accepted) value->setText(dialog.selection().join(", "));
               } else if (field.fileList) {
                  QDialog dialog(this); dialog.setObjectName("kernelFileDialog"); dialog.setWindowTitle(field.name); dialog.resize(650,360);
                  auto *layout=new QVBoxLayout(&dialog);
                  layout->addWidget(new QLabel("Add kernel files. Drag rows to change their order.",&dialog));
                  auto *list=new QListWidget(&dialog); list->setObjectName("kernelFileList");
                  list->addItems(value->text().split('\n',Qt::SkipEmptyParts)); list->setSelectionMode(QAbstractItemView::ExtendedSelection);
                  list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
                  auto *actions=new QHBoxLayout; layout->addLayout(actions);
                  auto *add=new QPushButton("Add files…",&dialog); add->setObjectName("kernelFileAdd"); actions->addWidget(add);
                  auto *remove=new QPushButton("Remove selected",&dialog); remove->setObjectName("kernelFileRemove"); actions->addWidget(remove); actions->addStretch();
                  connect(add,&QPushButton::clicked,&dialog,[&] {
                     QFileDialog picker(&dialog,"Select kernel files"); picker.setObjectName("kernelFilePicker"); picker.setFileMode(QFileDialog::ExistingFiles);
                     if (picker.exec()!=QDialog::Accepted) return;
                     for (const auto &path:picker.selectedFiles()) if (list->findItems(path,Qt::MatchExactly).isEmpty()) list->addItem(path);
                  });
                  connect(remove,&QPushButton::clicked,&dialog,[list] { qDeleteAll(list->selectedItems()); });
                  auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
                  connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
                  if (dialog.exec()==QDialog::Accepted) {
                     QStringList paths; for (int i=0;i<list->count();++i) paths.append(list->item(i)->text()); value->setText(paths.join('\n'));
                  }
               } else if (field.filename) {
                  QFileDialog dialog(this,"Choose "+field.name,value->text());
                  dialog.setObjectName("resourceFileDialog");
                  dialog.setAcceptMode(field.fileOutput ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
                  dialog.setFileMode(field.fileInput ? QFileDialog::ExistingFile : QFileDialog::AnyFile);
                  // Selecting an output path does not write it. Apply/run owns
                  // file creation, so do not imply that browsing overwrites it.
                  dialog.setOption(QFileDialog::DontConfirmOverwrite,true);
                  if (dialog.exec()==QDialog::Accepted && !dialog.selectedFiles().isEmpty())
                     value->setText(dialog.selectedFiles().first());
               } else if (!field.list) {
                  auto names=field.references;
                  if (!names.contains(value->text())) names.prepend(value->text());
                  bool accepted=false;
                  const auto selected=QInputDialog::getItem(this,"Select "+field.name,field.name,names,
                     names.indexOf(value->text()),true,&accepted);
                  if (accepted) value->setText(selected);
               } else {
                  QDialog dialog(this); dialog.setObjectName("resourceSelectionDialog");
                  dialog.setWindowTitle("Select "+field.name);
                  auto *layout=new QVBoxLayout(&dialog);
                  layout->addWidget(new QLabel("Select resources. Drag rows to change their order.",&dialog));
                  auto *list=new QListWidget(&dialog); list->setObjectName("resourceSelectionList");
                  list->setDragDropMode(QAbstractItemView::InternalMove);
                  QStringList selected=splitResourceReferences(value->text());
                  for (auto &name:selected) name=name.trimmed();
                  auto names=selected;
                  for (const auto &name:field.references) if (!names.contains(name)) names.append(name);
                  for (const auto &name:names) {
                     auto *item=new QListWidgetItem(name,list);
                     item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
                     item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked);
                  }
                  layout->addWidget(list);
                  auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
                  layout->addWidget(buttons);
                  connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
                  connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
                  if (dialog.exec()==QDialog::Accepted) {
                     QStringList names;
                     for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) names.append(list->item(i)->text());
                     value->setText(names.join(", "));
                  }
               }
            });
         }
      }
      else {
         auto *choices = new QComboBox(table);
         choices->addItems(field.choices);
         if (choices->findText(field.value) < 0) choices->addItem(field.value);
         choices->setCurrentText(field.value); table->setCellWidget(row, 1, choices);
      }
      auto *unit = new QTableWidgetItem(field.unit); unit->setFlags(unit->flags() & ~Qt::ItemIsEditable);
      table->setItem(row, 2, unit);
   }
   if (power || spacecraft) {
      const QString formatProperty=power ? "EpochFormat" : "DateFormat";
      const QString epochProperty=power ? "InitialEpoch" : "Epoch";
      int formatRow=-1; QTableWidgetItem *epoch=nullptr;
      for (int row=0;row<table->rowCount();++row) {
         if (table->item(row,0)->text()==formatProperty) formatRow=row;
         if (table->item(row,0)->text()==epochProperty) epoch=table->item(row,1);
      }
      if (formatRow>=0 && epoch) {
         const auto initialFormat=original.value(formatProperty);
         auto *format=new QComboBox(table); format->setObjectName(power ? "powerEpochFormat" : "spacecraftEpochFormat");
         for (const auto &value:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) format->addItem(QString::fromStdString(value));
         if (format->findText(initialFormat)<0) format->addItem(initialFormat);
         format->setCurrentText(initialFormat); table->setCellWidget(formatRow,1,format);
         connect(format,&QComboBox::currentTextChanged,this,[this,format,epoch,previous=initialFormat](const QString &next) mutable {
            auto text=epoch->text().trimmed();
            if (text.startsWith("'") && text.endsWith("'")) text=text.mid(1,text.size()-2);
            try {
               Real mjd; std::string converted;
               TimeSystemConverter::Instance()->Convert(previous.toStdString(),-999.999,text.toStdString(),next.toStdString(),mjd,converted);
               epoch->setText(QString::fromStdString(converted)); previous=next;
               status->setText("Epoch converted. Apply keeps the date and format together.");
            } catch (BaseException &error) {
               const QSignalBlocker blocker(format); format->setCurrentText(previous);
               status->setText("Epoch conversion failed: "+QString::fromStdString(error.GetFullMessage()));
            }
         });
      }
   }
   if (spacecraft) {
      QComboBox *representation=nullptr; QVector<int> elementRows;
      for (int row=0;row<table->rowCount();++row)
         if (table->item(row,0)->text()=="DisplayStateType") representation=qobject_cast<QComboBox *>(table->cellWidget(row,1));
      for (int element=1;element<=6;++element) {
         const auto label=QString::fromStdString(object.GetParameterText(object.GetParameterID("Element"+std::to_string(element))));
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==label) { elementRows.append(row); break; }
      }
      if (representation && elementRows.size()==6) {
         representation->setObjectName("spacecraftStateRepresentation");
         const auto resourceName=object.GetName();
         connect(representation,&QComboBox::currentTextChanged,this,
            [this,representation,elementRows,resourceName,previous=representation->currentText()](const QString &next) mutable {
            try {
               for (int row=0;row<table->rowCount();++row) {
                  const auto name=table->item(row,0)->text();
                  if (name!="CoordinateSystem" && name!="Epoch" && name!="DateFormat" && name!="AnomalyType") continue;
                  const auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1));
                  const auto value=choice ? comboValue(choice) : table->item(row,1)->text();
                  if (value!=original.value(name)) throw std::runtime_error("Apply the pending coordinate, epoch or anomaly settings before converting the state representation.");
               }
               auto *current=Moderator::Instance()->GetConfiguredObject(resourceName);
               if (!current) throw std::runtime_error("The spacecraft no longer exists. Reopen this panel.");
               std::unique_ptr<GmatBase> preview(current->Clone());
               preview->SetStringParameter("DisplayStateType",previous.toStdString());
               for (const auto row:elementRows) setResourceProperty(*preview,table->item(row,0)->text(),table->item(row,1)->text());
               preview->SetStringParameter("DisplayStateType",next.toStdString());
               QStringList labels,values,units;
               for (int element=1;element<=6;++element) {
                  const auto id=preview->GetParameterID("Element"+std::to_string(element));
                  labels.append(QString::fromStdString(preview->GetParameterText(id)));
                  values.append(QString::number(preview->GetRealParameter(id),'g',17));
                  units.append(QString::fromStdString(preview->GetStringParameter("Element"+std::to_string(element)+"Units")));
               }
               for (int i=0;i<6;++i) {
                  table->item(elementRows[i],0)->setText(labels[i]); table->item(elementRows[i],1)->setText(values[i]); table->item(elementRows[i],2)->setText(units[i]);
               }
               previous=next; status->setText("State values converted. Apply validates and stores the selected representation.");
            } catch (BaseException &error) {
               const QSignalBlocker blocker(representation); representation->setCurrentText(previous); status->setText(QString::fromStdString(error.GetFullMessage()));
            } catch (const std::exception &error) {
               const QSignalBlocker blocker(representation); representation->setCurrentText(previous); status->setText(QString::fromUtf8(error.what()));
            }
         });
      }
   }
   bool hasCellEditor=false;
   for (int row=0;row<table->rowCount();++row) hasCellEditor=hasCellEditor || table->cellWidget(row,3);
   table->setColumnHidden(3,!hasCellEditor);
   fitTableColumns(table);
   QTimer::singleShot(0,table,[this] { fitTableColumns(table,1); });
   connect(table,&QTableWidget::itemDoubleClicked,this,[this](QTableWidgetItem *item) {
      const int rows=item->data(Qt::UserRole).toInt(), columns=item->data(Qt::UserRole+1).toInt();
      if (item->column()!=1 || rows<=0 || columns<=0) return;
      QDialog dialog(this);
      dialog.setObjectName("numericGridDialog");
      dialog.setWindowTitle(table->item(item->row(),0)->text());
      auto *layout=new QVBoxLayout(&dialog);
      auto *grid=new QTableWidget(rows,columns,&dialog);
      grid->setObjectName("numericGrid");
      const auto values=item->text().split(';');
      for (int r=0;r<rows;++r) {
         const auto cells=values.value(r).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         for (int c=0;c<columns;++c) grid->setItem(r,c,new QTableWidgetItem(cells.value(c,"0")));
      }
      if (item->data(Qt::UserRole+2).toBool()) {
         auto *dimensions=new QFormLayout;
         auto *rowCount=new QSpinBox(&dialog),*columnCount=new QSpinBox(&dialog);
         rowCount->setObjectName("arrayRows"); columnCount->setObjectName("arrayColumns");
         rowCount->setRange(1,100); columnCount->setRange(1,100);
         rowCount->setMaximum(std::max(100,rows)); columnCount->setMaximum(std::max(100,columns));
         rowCount->setValue(rows); columnCount->setValue(columns);
         dimensions->addRow("Rows",rowCount); dimensions->addRow("Columns",columnCount);
         layout->addLayout(dimensions);
         layout->addWidget(new QLabel("New cells start at zero. Shrinking removes cells outside the new dimensions.",&dialog));
         auto resize=[=] {
            grid->setRowCount(rowCount->value()); grid->setColumnCount(columnCount->value());
            for (int r=0;r<grid->rowCount();++r) for (int c=0;c<grid->columnCount();++c)
               if (!grid->item(r,c)) grid->setItem(r,c,new QTableWidgetItem("0"));
         };
         connect(rowCount,&QSpinBox::valueChanged,&dialog,resize);
         connect(columnCount,&QSpinBox::valueChanged,&dialog,resize);
      }
      configureTableColumns(grid);
      fitTableColumns(grid);
      layout->addWidget(grid);
      auto *error=new QLabel(&dialog); error->setWordWrap(true); layout->addWidget(error);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
      layout->addWidget(buttons);
      connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
         QStringList output;
         for (int r=0;r<grid->rowCount();++r) {
            QStringList cells;
            for (int c=0;c<grid->columnCount();++c) {
               bool valid=false;
               const double value=grid->item(r,c)->text().toDouble(&valid);
               if (!valid || !std::isfinite(value)) {
                  error->setText(QString("Cell (%1, %2) needs a finite number.").arg(r+1).arg(c+1));
                  grid->setCurrentCell(r,c); return;
               }
               cells.append(QString::number(value,'g',17));
            }
            output.append(cells.join(" "));
         }
         item->setData(Qt::UserRole,grid->rowCount());
         item->setData(Qt::UserRole+1,grid->columnCount());
         item->setText(output.join("; ")); dialog.accept();
      });
      dialog.resize(560,320); dialog.exec();
   });
   if (sections) {
      // Use the engine's current element labels (Cartesian, Keplerian, etc.).
      QSet<QString> orbit={"Epoch","DateFormat","StateType","DisplayStateType","AnomalyType","CoordinateSystem"};
      for (int i=1;i<=6;++i) {
         try { orbit.insert(QString::fromStdString(object.GetParameterText(object.GetParameterID("Element"+std::to_string(i))))); }
         catch (BaseException &) {} // Unrecognized labels remain accessible in All Properties.
      }
      QSet<QString> attitudeFields;
      if (spacecraft) if (auto *attitude=object.GetOwnedObject(0))
         for (const auto &field:resourceProperties(*attitude)) attitudeFields.insert(field.name);
      QSet<QString> present;
      for (int row=0;row<table->rowCount();++row) {
         const auto name=table->item(row,0)->text();
         QString section;
         if (!spacecraft) {
            section="General";
            if (thruster) {
               if (name=="Tank" || name=="MixRatio" || name=="DecrementMass") section="Fuel";
               else if (name.startsWith("ThrustDirection") || name=="CoordinateSystem" || name=="Origin" || name=="Axes") section="Direction";
               else if (name.startsWith("C") || name.startsWith("K") || name.contains("Thrust") || name.contains("Isp") || name=="DutyCycle") section="Performance";
            } else if (power) {
               if (name.startsWith("BusCoeff")) section="Bus coefficients";
               else if (name.startsWith("SolarCoeff")) section="Solar coefficients";
               else if (name.startsWith("Shadow")) section="Shadow";
            } else if (tank) {
               if (name.contains("Mass") || name.contains("Density") || name=="Volume" || name.contains("Pressure") || name.contains("Temperature")) section="Fuel";
            } else if (solver) {
               if (name.contains("Report") || name.contains("Progress") || name.contains("Show")) section="Output";
               else if (name.contains("Tolerance") || name.contains("Iterations") || name.contains("Algorithm") || name.contains("Derivative")) section="Convergence";
            } else if (forces) {
               if (name.startsWith("GravityField.")) section="Gravity field";
               else if (name.startsWith("Drag.")) section="Atmosphere";
               else if (name.startsWith("SRP.")) section="Radiation pressure";
               else if (name.contains("Bodies") || name=="PointMasses" || name=="CentralBody") section="Bodies";
               else if (name.contains("Drag") || name.contains("SRP") || name.contains("Relativistic")) section="Perturbations";
            }
         }
         else if (orbit.contains(name)) section="Orbit";
         else if (name.startsWith("NAIF") || name.contains("Spice")) section="SPICE";
         else if (name=="Attitude" || name.startsWith("Attitude") || attitudeFields.contains(name)) section="Attitude";
         else if (name.startsWith("Model") || name=="OrbitColor" || name=="TargetColor") section="Visualization";
         else if (name.startsWith("Dry") || name.startsWith("System") || name.startsWith("SPAD") ||
                  name.startsWith("AtmosDensity") || name.contains("Mass") || name.contains("Inertia") ||
                  QSet<QString>{"Cd","Cr","CdSigma","CrSigma","DragArea","SRPArea"}.contains(name)) section="Ballistic/Mass";
         else if (name=="PowerSystem") section="Power System";
         else if (name=="Tanks" || name=="Thrusters" || name=="AddHardware" || name=="AddPlates") section="Hardware";
         table->item(row,0)->setData(Qt::UserRole,section);
         if (!section.isEmpty()) present.insert(section);
      }
      for (const auto &section : {"Orbit","Attitude","Ballistic/Mass","Hardware","Power System","SPICE","Visualization",
                                 "General","Bus coefficients","Solar coefficients","Shadow","Bodies","Gravity field","Atmosphere","Radiation pressure","Perturbations","Direction","Fuel","Performance","Convergence","Output"})
         if (present.contains(section)) sections->addTab(section);
      sections->addTab("All Properties");
   }
   layout->addWidget(table, 1);
   status = new QLabel("Apply validates changes and updates the mission script.", this);
   status->setWordWrap(true); layout->addWidget(status);
   auto *buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, this);
   layout->addWidget(buttons);
   const auto filter=[this,search,sections] {
      const auto section=sections ? sections->tabText(sections->currentIndex()) : QString();
      for (int row=0;row<table->rowCount();++row) {
         const auto *name=table->item(row,0);
         const bool inSection=!sections || section=="All Properties" || name->data(Qt::UserRole).toString()==section;
         table->setRowHidden(row,!inSection || !name->text().contains(search->text(),Qt::CaseInsensitive));
      }
   };
   connect(search, &QLineEdit::textChanged, this, filter);
   if (sections) connect(sections,&QTabBar::currentChanged,this,filter);
   filter();
   connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this, apply, applyUnchanged] {
      QMap<QString, QString> changes;
      if (expressions!=originalExpressions) changes.insert("@ArrayExpressions",expressions);
      for (int row = 0; row < table->rowCount(); ++row) {
         const QString name = table->item(row, 0)->text();
         const auto *combo = qobject_cast<QComboBox *>(table->cellWidget(row, 1));
         const QString value = combo ? comboValue(combo) : table->item(row, 1)->text();
         if (value != original.value(name)) changes.insert(name, value);
      }
      if (pairedTankEdits && (changes.contains("Tank") || changes.contains("MixRatio"))) {
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text();
            if (name=="Tank" || name=="MixRatio") changes.insert(name,table->item(row,1)->text());
         }
      }
      if (changes.isEmpty() && !applyUnchanged) { status->setText("No changes to apply."); return; }
      const QString error = apply(changes);
      if (error.isEmpty()) {
         // Model reconstruction can normalize dependent properties. Close this
         // snapshot so a subsequent edit always reads the current model.
         applied = true;
         parentWidget()->close();
      } else status->setText(error);
   });
   connect(buttons, &QDialogButtonBox::rejected, this, [this] { parentWidget()->close(); });
}

bool ResourceEditor::hasChanges() const
{
   if (applied) return false;
   if (expressions!=originalExpressions) return true;
   for (int row = 0; row < table->rowCount(); ++row) {
      const auto *combo = qobject_cast<QComboBox *>(table->cellWidget(row, 1));
      const QString value = combo ? comboValue(combo) : table->item(row, 1)->text();
      if (value != original.value(table->item(row, 0)->text())) return true;
   }
   return false;
}
