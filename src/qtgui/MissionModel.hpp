#pragma once
#include <QString>
#include <QVector>
class GmatCommand;
struct MissionNode
{
   QString type, label, statement;
   int parent = -1;
   qsizetype start = -1, end = -1;
   bool editable = false;
   QVector<int> children;
};
struct MissionSnapshot
{
   QString sourceScript, canonicalScript;
   QVector<MissionNode> nodes;
   QVector<int> roots;
};
enum class MissionEdit { Replace, InsertBefore, InsertAfter, Append, Remove };
MissionSnapshot snapshotMission(GmatCommand *first, const QString &canonical, const QString &source, QVector<GmatCommand *> *commands=nullptr);
QString editMission(const MissionSnapshot &snapshot, int node, MissionEdit operation,
                    const QString &replacement);
