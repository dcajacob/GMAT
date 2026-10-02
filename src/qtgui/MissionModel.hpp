#pragma once
#include <QString>
#include <QVector>
class GmatCommand;
struct MissionNode
{
   QString type, label, statement;
   int parent = -1;
   // Coordinates in the original source, never the regenerated configuration.
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
// Return the selected branch's own mapped closing node, or -1 when it cannot
// safely receive an insertion. Script events use their dedicated body editor.
int missionBranchEnd(const MissionSnapshot &snapshot,int index);
MissionSnapshot snapshotMission(GmatCommand *first, const QString &canonical, const QString &source, QVector<GmatCommand *> *commands=nullptr, bool expandScriptEvents=false);
QString editMission(const MissionSnapshot &snapshot, int node, MissionEdit operation,
                    const QString &replacement);
