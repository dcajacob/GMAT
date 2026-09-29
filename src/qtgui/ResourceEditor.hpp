#pragma once
#include "EditablePanel.hpp"
#include <QMap>
#include <QString>
#include <functional>
#include <QMdiSubWindow>
class GmatBase;
class QTableWidget;
class QLabel;
class ResourceEditor final : public EditablePanel
{
public:
   using Apply = std::function<QString(const QMap<QString, QString> &)>;
   ResourceEditor(GmatBase &object, Apply apply, QWidget *parent = nullptr, const QString &script = {}, bool applyUnchanged = false);
   bool hasChanges() const override;
   void discardChanges() override { applied = true; }
private:
   QTableWidget *table;
   QLabel *status;
   QMap<QString, QString> original;
   QString originalExpressions="[]",expressions="[]";
   bool applied = false;
   bool pairedTankEdits = false;
};
