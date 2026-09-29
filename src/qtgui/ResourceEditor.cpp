#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
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

ResourceEditor::ResourceEditor(GmatBase &object, Apply apply, QWidget *parent) : EditablePanel(parent)
{
   auto *layout = new QVBoxLayout(this);
   layout->addWidget(new QLabel(QString::fromStdString(object.GetName() + " — " + object.GetTypeName()), this));
   auto *search = new QLineEdit(this);
   search->setPlaceholderText("Filter properties…");
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
      if (field.choices.isEmpty()) table->setItem(row, 1, new QTableWidgetItem(field.value));
      else {
         auto *choices = new QComboBox(table);
         choices->addItems(field.choices);
         if (choices->findText(field.value) < 0) choices->addItem(field.value);
         choices->setCurrentText(field.value); table->setCellWidget(row, 1, choices);
      }
      auto *unit = new QTableWidgetItem(field.unit); unit->setFlags(unit->flags() & ~Qt::ItemIsEditable);
      table->setItem(row, 2, unit);
   }
   layout->addWidget(table, 1);
   status = new QLabel("Apply validates changes and updates the mission script.", this);
   status->setWordWrap(true); layout->addWidget(status);
   auto *buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, this);
   layout->addWidget(buttons);
   connect(search, &QLineEdit::textChanged, this, [this](const QString &text) {
      for (int row = 0; row < table->rowCount(); ++row) table->setRowHidden(row, !table->item(row, 0)->text().contains(text, Qt::CaseInsensitive));
   });
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
