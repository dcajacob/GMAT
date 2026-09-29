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

ResourceEditor::ResourceEditor(GmatBase &object, Apply apply, QWidget *parent) : EditablePanel(parent)
{
   auto *layout = new QVBoxLayout(this);
   layout->addWidget(new QLabel(QString::fromStdString(object.GetName() + " — " + object.GetTypeName()), this));
   auto *search = new QLineEdit(this);
   search->setObjectName("propertyFilter");
   search->setPlaceholderText("Filter properties…");
   QTabBar *sections=nullptr;
   if (object.IsOfType("Spacecraft")) {
      sections=new QTabBar(this);
      sections->setObjectName("propertySections");
      sections->setExpanding(false);
      layout->addWidget(sections);
   }
   layout->addWidget(search);
   table = new QTableWidget(this);
   table->setColumnCount(3);
   table->setHorizontalHeaderLabels({"Property", "Value", "Unit"});
   table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
   table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
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
   if (sections) {
      // Use the engine's current element labels (Cartesian, Keplerian, etc.).
      QSet<QString> orbit={"Epoch","DateFormat","StateType","DisplayStateType","AnomalyType","CoordinateSystem"};
      for (int i=1;i<=6;++i) {
         try { orbit.insert(QString::fromStdString(object.GetParameterText(object.GetParameterID("Element"+std::to_string(i))))); }
         catch (BaseException &) {} // Unrecognized labels remain accessible in All Properties.
      }
      QSet<QString> present;
      for (int row=0;row<table->rowCount();++row) {
         const auto name=table->item(row,0)->text();
         QString section;
         if (orbit.contains(name)) section="Orbit";
         else if (name.startsWith("NAIF") || name.startsWith("Spice")) section="SPICE";
         else if (name=="Attitude" || name.startsWith("Attitude")) section="Attitude";
         else if (name.startsWith("Model")) section="Visualization";
         else if (name.startsWith("Dry") || name.startsWith("System") || name.startsWith("SPAD") ||
                  name.startsWith("AtmosDensity") || name.contains("Mass") || name.contains("Inertia") ||
                  QSet<QString>{"Cd","Cr","CdSigma","CrSigma","DragArea","SRPArea"}.contains(name)) section="Ballistic/Mass";
         else if (name=="PowerSystem") section="Power System";
         table->item(row,0)->setData(Qt::UserRole,section);
         if (!section.isEmpty()) present.insert(section);
      }
      for (const auto &section : {"Orbit","Attitude","Ballistic/Mass","Power System","SPICE","Visualization"})
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
