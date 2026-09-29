#include "MissionModel.hpp"
#include "GmatCommand.hpp"
#include <QSet>
#include <stdexcept>

namespace {
struct Line { qsizetype start, end; QString normalized; };
QVector<Line> lines(const QString &text)
{
   QVector<Line> result;
   qsizetype offset = 0;
   for (const auto &line : text.split('\n')) {
      if (!line.trimmed().isEmpty()) result.append({offset, std::min(text.size(), offset+line.size()+1), line.trimmed()});
      offset += line.size()+1;
   }
   return result;
}
QPair<qsizetype, qsizetype> locate(const QVector<Line> &document, const QString &statement,
                                qsizetype begin, qsizetype end)
{
   const auto wanted=lines(statement);
   if (wanted.isEmpty()) return {-1,-1};
   for (qsizetype i=0;i<document.size();++i) {
      if (document[i].start<begin || document[i].end>end) continue;
      qsizetype j=0;
      while (j<wanted.size() && i+j<document.size() && document[i+j].end<=end &&
             document[i+j].normalized==wanted[j].normalized) ++j;
      if (j==wanted.size()) return {document[i].start,document[i+j-1].end};
   }
   return {-1,-1};
}
class Builder
{
public:
   MissionSnapshot result;
   QVector<Line> document;
   QSet<GmatCommand *> visited;
   void collect(GmatCommand *command, GmatCommand *stop, int parent,
                qsizetype begin, qsizetype end, int depth = 0)
   {
      if (depth>100) throw std::runtime_error("Mission nesting is too deep to display");
      int scriptDepth=0;
      while (command && command!=stop) {
         if (visited.contains(command)) throw std::runtime_error("Unexpected cycle in mission sequence");
         visited.insert(command);
         const auto type=QString::fromStdString(command->GetTypeName());
         if (scriptDepth>0) {
            if (type=="BeginScript") ++scriptDepth;
            if (type=="EndScript") --scriptDepth;
            command=command->GetNext(); continue;
         }
         if (type=="NoOp") { command=command->GetNext(); continue; }
         MissionNode node;
         node.type=type;
         const auto name=QString::fromStdString(command->GetName());
         node.label=name.isEmpty() ? type : type+" — "+name;
         node.statement=QString::fromStdString(command->GetGeneratingString(Gmat::SCRIPTING)).trimmed();
         if (name.isEmpty()) {
            for (auto line : node.statement.split('\n')) {
               line=line.trimmed();
               if (line.isEmpty() || line.startsWith('%')) continue;
               if (line.startsWith("GMAT ")) line.remove(0,5);
               if (line.endsWith(';')) line.chop(1);
               node.label=line;
               break;
            }
         }
         const auto range=locate(document,node.statement,begin,end);
         node.start=range.first; node.end=range.second; node.parent=parent;
         const QSet<QString> structural={"BeginMissionSequence","Else","EndIf","EndFor","EndWhile","EndTarget","EndOptimize","EndScript"};
         node.editable=node.start>=0 && !structural.contains(type);
         const int index=result.nodes.size(); result.nodes.append(node);
         if (parent<0) result.roots.append(index); else result.nodes[parent].children.append(index);
         if (node.start>=0) {
            qsizetype childStart=node.start;
            for (int branch=0;auto *child=command->GetChildCommand(branch);++branch) {
               const int before=result.nodes.size();
               collect(child,command,index,childStart,node.end,depth+1);
               for (int n=before;n<result.nodes.size();++n) childStart=std::max(childStart,result.nodes[n].end);
            }
            begin=node.end;
         }
         if (type=="BeginScript") scriptDepth=1;
         command=command->GetNext();
      }
   }
};
}
MissionSnapshot snapshotMission(GmatCommand *first,const QString &canonical,const QString &source)
{
   Builder builder; builder.result.sourceScript=source; builder.result.canonicalScript=canonical;
   builder.document=lines(canonical); builder.collect(first,nullptr,-1,0,canonical.size());
   return builder.result;
}
QString editMission(const MissionSnapshot &snapshot,int index,MissionEdit operation,const QString &replacement)
{
   if (operation!=MissionEdit::Remove && replacement.trimmed().isEmpty()) throw std::runtime_error("Enter a command");
   QString candidate=snapshot.canonicalScript;
   if (operation==MissionEdit::Append) {
      if (!candidate.endsWith('\n')) candidate+='\n';
      return candidate+replacement.trimmed()+"\n";
   }
   if (index<0 || index>=snapshot.nodes.size()) throw std::runtime_error("Select a mission command");
   const auto &node=snapshot.nodes[index];
   if (node.start<0 || node.end<=node.start) throw std::runtime_error("This command cannot be located safely; use the script editor");
   if ((operation==MissionEdit::Replace || operation==MissionEdit::Remove) && !node.editable)
      throw std::runtime_error("Edit the enclosing branch to change this structural command");
   if (operation==MissionEdit::InsertBefore && node.type=="BeginMissionSequence")
      throw std::runtime_error("Insert commands after BeginMissionSequence");
   if (operation==MissionEdit::Remove) candidate.remove(node.start,node.end-node.start);
   else if (operation==MissionEdit::Replace) candidate.replace(node.start,node.end-node.start,replacement.trimmed()+"\n");
   else if (operation==MissionEdit::InsertBefore) candidate.insert(node.start,replacement.trimmed()+"\n");
   else if (operation==MissionEdit::InsertAfter) candidate.insert(node.end,replacement.trimmed()+"\n");
   return candidate;
}
