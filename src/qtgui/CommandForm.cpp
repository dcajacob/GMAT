#include "CommandForm.hpp"
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpression>
#include <algorithm>

CommandForm::CommandForm(std::function<void(const QString &)> callback,QWidget *parent)
   : QGroupBox("Command settings",parent),layout(new QFormLayout(this)),changed(std::move(callback))
{
   setObjectName("commandForm"); hide();
}

void CommandForm::updateSource()
{
   if (synchronizing) return;
   QString result=original;
   // Spans refer to the original snapshot. Replace from right to left so a
   // changed field length cannot displace another field, or a branch body.
   auto ordered=fields;
   std::sort(ordered.begin(),ordered.end(),[](const Field &a,const Field &b) { return a.start>b.start; });
   for (const auto &field:ordered) result.replace(field.start,field.length,field.input->text());
   synchronizing=true; changed(result); synchronizing=false;
}

void CommandForm::setStatement(const QString &statement)
{
   if (synchronizing) return;
   while (layout->rowCount()) layout->removeRow(0);
   fields.clear(); original=statement;
   const QString label="(?:'[^'\\n]*'\\s+)?";
   const QString name="([A-Za-z][A-Za-z0-9_]*)";
   const QString expression="([^,;{}\\n]+?)";
   const QString end="\\s*;[ \\t]*(?:%[^\\n]*)?\\s*$";
   struct Spec { QString type,pattern; QStringList labels; };
   const QVector<Spec> specs={
      {"Maneuver","Maneuver\\s+"+label+"(?:BackProp\\s+)?"+name+"\\s*\\(\\s*"+name+"\\s*\\)"+end,{"Burn","Spacecraft"}},
      {"Finite burn","(?:BeginFiniteBurn|EndFiniteBurn)\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*\\)"+end,{"Burn","Spacecraft"}},
      {"Vary","Vary\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*=\\s*"+expression+"\\s*(?:,\\s*\\{([^{};]*)\\})?\\s*\\)"+end,{"Solver","Variable","Initial value"}},
      {"Achieve","Achieve\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*=\\s*"+expression+"\\s*(?:,\\s*\\{([^{};]*)\\})?\\s*\\)"+end,{"Solver","Goal","Value"}},
      {"Minimize","Minimize\\s+"+label+name+"\\s*\\(\\s*"+expression+"\\s*\\)"+end,{"Solver","Objective"}},
      {"Constraint","NonlinearConstraint\\s+"+label+name+"\\s*\\(\\s*([^,;{}\\n]+?)\\s*(<=|>=|=)\\s*"+expression+"\\s*\\)"+end,{"Solver","Left side","Relation","Right side"}},
      {"Report","Report\\s+"+label+name+"\\s+([^;%\\n]+?)"+end,{"Report file","Parameters"}},
      {"Event search","FindEvents\\s+"+label+name+"(?:\\s*\\{([^{};]*)\\})?"+end,{"Locator"}},
      {"Function call",label+"(\\[[^\\];\\n]*\\]|[A-Za-z][A-Za-z0-9_]*)\\s*=\\s*"+name+"\\s*\\(([^;\\n]*?)\\)"+end,{"Outputs","Function","Inputs"}},
      {"Function call",label+name+"\\s*\\(([^;\\n]*?)\\)"+end,{"Function","Inputs"}},
      {"Toggle","Toggle\\s+"+label+"([A-Za-z][A-Za-z0-9_]*(?:\\s+[A-Za-z][A-Za-z0-9_]*)*)\\s+(On|Off)"+end,{"Subscribers","State"}},
      {"Objects","(?:Global|Clear|Save)\\s+"+label+"([^;%\\n]+?)"+end,{"Objects"}},
      {"Assignment","(?:GMAT\\s+)?"+label+"([A-Za-z][A-Za-z0-9_.]*(?:\\([^;\\n]*?\\))?)\\s*=\\s*([^;\\n]+?)"+end,{"Destination","Expression"}},
      // Branch forms replace only header spans, never their nested commands.
      {"For loop","For\\s+"+label+name+"\\s*=\\s*([^:;\\n]+?)\\s*:\\s*([^:;\\n]+?)\\s*:\\s*([^;\\n]+?)\\s*;[\\s\\S]*$",{"Index","Start","Step","End"}},
      {"For loop","For\\s+"+label+name+"\\s*=\\s*([^:;\\n]+?)\\s*:\\s*([^:;\\n]+?)\\s*;[\\s\\S]*$",{"Index","Start","End"}},
      {"Condition","(?:If|While)\\s+"+label+"([^;%\\n]+?)[ \\t]*(?:;[^\\n]*|%[^\\n]*)?(?:\\n[\\s\\S]*)?$",{"Condition"}},
      // Only the first line is captured; the entire branch remains untouched.
      {"Solver branch","(?:Target|Optimize)\\s+"+label+name+"(?:\\s*\\{([^{};]*)\\})?\\s*;[\\s\\S]*$",{"Solver"}}
   };
   auto add=[&](const QString &name,qsizetype start,qsizetype length) {
      if (start<0) return;
      auto *input=new QLineEdit(statement.mid(start,length),this);
      input->setObjectName("commandField_"+name);
      layout->addRow(name,input); fields.append({input,start,length});
      connect(input,&QLineEdit::textChanged,this,[this] { updateSource(); });
   };
   for (const auto &spec:specs) {
      const auto match=QRegularExpression("^\\s*"+spec.pattern).match(statement);
      if (!match.hasMatch()) continue;
      setTitle(spec.type);
      for (int i=0;i<spec.labels.size();++i) add(spec.labels[i],match.capturedStart(i+1),match.capturedLength(i+1));
      // Optional keyword values can occur in any order. Only recognized keys
      // become controls; all other option text is retained verbatim.
      const int options=spec.labels.size()+1;
      if (match.lastCapturedIndex()>=options && match.capturedStart(options)>=0) {
         const QRegularExpression option("(?:^|,)\\s*(Perturbation|Lower|Upper|MaxStep|AdditiveScaleFactor|MultiplicativeScaleFactor|Tolerance|SolveMode|ExitMode|ShowProgressWindow|Append)\\s*=\\s*([^,]+?)\\s*(?=,|$)");
         auto matches=option.globalMatch(match.captured(options));
         while (matches.hasNext()) {
            const auto setting=matches.next();
            add(setting.captured(1),match.capturedStart(options)+setting.capturedStart(2),setting.capturedLength(2));
         }
      }
      show(); return;
   }
   hide();
}
