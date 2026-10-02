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

// A command label is mission syntax, not an object-resource name. Coordinates
// identify only its prefix token; branch bodies and comments are never rebuilt.
struct MissionCommandLabel
{
   QString name;
   qsizetype start=-1,length=0;
   bool afterKeyword=false;
   bool editable() const { return start>=0; }
};
MissionCommandLabel missionCommandLabel(const QString &statement);
QString setMissionCommandLabel(const QString &statement,const QString &name);
