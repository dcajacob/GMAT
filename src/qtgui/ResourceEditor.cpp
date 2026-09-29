#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "AxisSystem.hpp"
#include "BaseException.hpp"
#include <memory>
#include "ResourceProperties.hpp"
#include "ScriptCompatibility.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "TableColumns.hpp"
#include <QTimer>
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
   if (spacecraft || thruster || tank || solver || forces) {
      sections=new QTabBar(this);
      sections->setObjectName("propertySections");
      sections->setExpanding(false);
      layout->addWidget(sections);
   }
   if (object.IsOfType("Array")) {
      auto *button=new QPushButton("Expressions…",this); button->setObjectName("arrayExpressions"); layout->addWidget(button);
      const auto name=QString::fromStdString(object.GetName());
      const int rows=object.GetIntegerParameter("NumRows"),columns=object.GetIntegerParameter("NumCols");
      try { originalExpressions=expressions=arrayExpressions(script,name); setArrayExpressions(script,name,expressions,rows,columns); }
      catch (const std::exception &error) { button->setEnabled(false); button->setToolTip(QString::fromUtf8(error.what())); }
      connect(button,&QPushButton::clicked,this,[this,rows,columns] {
         QDialog dialog(this); dialog.setObjectName("arrayExpressionDialog"); dialog.setWindowTitle("Array expressions");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Nonempty cells run at mission start, in row order, before existing commands. Blank cells keep their numeric initial values. Apply validates formulas without running them. Apply numeric edits separately.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *grid=new QTableWidget(rows,columns,&dialog); grid->setObjectName("arrayExpressionGrid");
         for (int r=0;r<rows;++r) for (int c=0;c<columns;++c) grid->setItem(r,c,new QTableWidgetItem);
         for (const auto &value:QJsonDocument::fromJson(expressions.toUtf8()).array()) {
            const auto cell=value.toObject(); const int r=cell["row"].toInt()-1,c=cell["column"].toInt()-1;
            if (r>=0 && r<rows && c>=0 && c<columns) grid->item(r,c)->setText(cell["expression"].toString());
         }
         configureTableColumns(grid); fitTableColumns(grid); layout->addWidget(grid);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
         dialog.resize(650,350);
         if (dialog.exec()!=QDialog::Accepted) return;
         QJsonArray cells;
         for (int r=0;r<rows;++r) for (int c=0;c<columns;++c) {
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
         if (field.list) value->setToolTip("Comma-separated resource or parameter names. Apply replaces the complete list.");
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
         if (field.filename || !field.references.isEmpty()) {
            auto *choose=new QPushButton(field.filename ? "Browse…" : "Select…",table);
            choose->setObjectName("chooseProperty_"+field.name);
            table->setCellWidget(row,3,choose);
            connect(choose,&QPushButton::clicked,this,[this,value,field] {
               if (field.filename) {
                  QFileDialog dialog(this,"Choose "+field.name,value->text());
                  // Output files need not exist yet; validation belongs to Apply.
                  dialog.setFileMode(QFileDialog::AnyFile);
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
         else if (name.startsWith("NAIF") || name.startsWith("Spice")) section="SPICE";
         else if (name=="Attitude" || name.startsWith("Attitude") || attitudeFields.contains(name)) section="Attitude";
         else if (name.startsWith("Model")) section="Visualization";
         else if (name.startsWith("Dry") || name.startsWith("System") || name.startsWith("SPAD") ||
                  name.startsWith("AtmosDensity") || name.contains("Mass") || name.contains("Inertia") ||
                  QSet<QString>{"Cd","Cr","CdSigma","CrSigma","DragArea","SRPArea"}.contains(name)) section="Ballistic/Mass";
         else if (name=="PowerSystem") section="Power System";
         else if (name=="Tanks" || name=="Thrusters" || name=="AddHardware" || name=="AddPlates") section="Hardware";
         table->item(row,0)->setData(Qt::UserRole,section);
         if (!section.isEmpty()) present.insert(section);
      }
      for (const auto &section : {"Orbit","Attitude","Ballistic/Mass","Hardware","Power System","SPICE","Visualization",
                                 "General","Bodies","Gravity field","Atmosphere","Radiation pressure","Perturbations","Direction","Fuel","Performance","Convergence","Output"})
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
