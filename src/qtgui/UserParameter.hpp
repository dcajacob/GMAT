#pragma once
#include <QString>
#include <QStringList>
QString userParameterLiteral(const QString &type,const QString &value);
QString setUserParameterValue(const QString &source,const QString &name,const QString &type,const QString &value,const QString &firstMissionStatement={});
// Replace only named configuration assignments, retaining comments and the
// entire mission suffix. Values in the ordered block are already serialized.
QString setConfigurationBlock(const QString &source,const QString &name,const QStringList &properties,const QString &block,const QString &firstMissionStatement={});
// Apply the changed assignments in two engine-generated resource snapshots to
// the original source. Unchanged properties and other resources stay implicit
// or retain their original syntax. Array declarations may change dimensions.
// Gravity body lists, drag, SRP and relativistic creator changes retain original
// selector positions/spelling; removal edits only the affected force family.
// Remaining force selectors reconstruct the model in serializer order.
QString patchResourceConfiguration(const QString &source,const QString &name,const QString &before,const QString &after,const QString &firstMissionStatement={},bool replaceOwnedConfiguration=false);
// Delete only this resource's declaration and configuration assignments. A
// grouped Create keeps its other resources; comments and mission source remain.
QString removeResourceConfiguration(const QString &source,const QString &name,const QString &firstMissionStatement={});
