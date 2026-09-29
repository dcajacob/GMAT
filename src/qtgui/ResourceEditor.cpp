#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
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
#include <QRegularExpression>
#include <cmath>

ResourceEditor::ResourceEditor(GmatBase &object, Apply apply, QWidget *parent) : EditablePanel(parent)
{
   auto *layout = new QVBoxLayout(this);
   layout->addWidget(new QLabel(QString::fromStdString(object.GetName() + " — " + object.GetTypeName()), this));
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
   layout->addWidget(search);
   table = new QTableWidget(this);
   table->setColumnCount(4);
   table->setHorizontalHeaderLabels({"Property", "Value", "Unit", ""});
   table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
   table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
   table->verticalHeader()->hide();
   for (const auto &field : resourceProperties(object)) {
      int row = table->rowCount(); table->insertRow(row);
      original.insert(field.name, field.value);
      auto *name = new QTableWidgetItem(field.name);
      name->setFlags(name->flags() & ~Qt::ItemIsEditable);
      table->setItem(row, 0, name);
      if (field.choices.isEmpty()) {
         auto *value=new QTableWidgetItem(field.value);
         if (field.list) value->setToolTip("Comma-separated resource or parameter names. Apply replaces the complete list.");
         if (field.rows>0 && field.columns>0) {
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
            value->setData(Qt::UserRole,field.rows);
            value->setData(Qt::UserRole+1,field.columns);
            value->setToolTip("Double-click to edit the numeric cells.");
            auto *edit=new QPushButton("Edit cells…",table);
            edit->setObjectName("editCells_"+field.name);
            table->setCellWidget(row,3,edit);
            connect(edit,&QPushButton::clicked,this,[this,value] { table->itemDoubleClicked(value); });
         }
         table->setItem(row, 1, value);
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
      layout->addWidget(grid);
      auto *error=new QLabel(&dialog); error->setWordWrap(true); layout->addWidget(error);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
      layout->addWidget(buttons);
      connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
         QStringList output;
         for (int r=0;r<rows;++r) {
            QStringList cells;
            for (int c=0;c<columns;++c) {
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
   connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this, apply] {
      QMap<QString, QString> changes;
      for (int row = 0; row < table->rowCount(); ++row) {
         const QString name = table->item(row, 0)->text();
         const auto *combo = qobject_cast<QComboBox *>(table->cellWidget(row, 1));
         const QString value = combo ? combo->currentText() : table->item(row, 1)->text();
         if (value != original.value(name)) changes.insert(name, value);
      }
      if (changes.isEmpty()) { status->setText("No changes to apply."); return; }
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
   for (int row = 0; row < table->rowCount(); ++row) {
      const auto *combo = qobject_cast<QComboBox *>(table->cellWidget(row, 1));
      const QString value = combo ? combo->currentText() : table->item(row, 1)->text();
      if (value != original.value(table->item(row, 0)->text())) return true;
   }
   return false;
}
