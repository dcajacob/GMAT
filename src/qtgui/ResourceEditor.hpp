#pragma once
#include "EditablePanel.hpp"
#include <QMap>
#include <QString>
#include <QSet>
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
   std::function<QString()> scalarValue;
   QString originalScalarValue;
   QTableWidget *table;
   QLabel *status;
   QMap<QString, QString> original;
   QMap<QString, QString> attitudeEdits;
   QMap<QString, QString> atmosphereEdits;
   QMap<QString, QString> externalEdits,originalExternal;
   QMap<QString, QString> stationEdits;
   QMap<QString, QString> eventEdits;
   QString pendingDynamicData;
   QString pendingTrackingConfigs;
   QSet<QString> attitudeNames;
   std::function<QMap<QString,QString>()> pendingOrbit;
   QString originalExpressions="[]",expressions="[]";
   bool applied = false;
   bool pairedTankEdits = false;
};
