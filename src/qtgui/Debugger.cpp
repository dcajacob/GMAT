#include "Debugger.hpp"
#include "GmatCommand.hpp"
#include "BranchCommand.hpp"
#include "Moderator.hpp"
#include "Parameter.hpp"
#include "BaseException.hpp"
#include "SandboxException.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QEventLoop>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QShortcut>

namespace {
class DebuggerDialog final : public QDialog
{
public:
   DebuggerDialog(QWidget *owner,std::function<void()> resume) : QDialog(owner),resume(std::move(resume)) {}
   void reject() override { resume(); QDialog::reject(); }
protected:
   void closeEvent(QCloseEvent *event) override { resume(); QDialog::closeEvent(event); }
private:
   std::function<void()> resume;
};
}
Debugger::Debugger(QWidget *parent) : QObject(parent),owner(parent) {}
void Debugger::begin(const QVector<GmatCommand *> &commands,const QSet<int> &selected)
{
   indices.clear(); breakpoints.clear();
   for (int index=0;index<commands.size();++index) {
      indices[commands[index]]=index;
      if (selected.contains(index)) breakpoints.insert(commands[index]);
   }
   active=true; waiting=false; stopped=false; stopAtNext=breakpoints.isEmpty();
}
void Debugger::finish()
{
   active=false; waiting=false; waitLoop=nullptr; indices.clear(); breakpoints.clear();
   if (dialog) dialog->hide();
}
void Debugger::step()
{
   if (!active || !waiting) return;
   stopAtNext=true;
   if (resumeRequested) resumeRequested();
}
void Debugger::resume()
{
   waiting=false;
   if (waitLoop) waitLoop->quit();
   if (dialog) dialog->hide();
}
void Debugger::stop()
{
   stopped=true; resume();
}
void Debugger::ensureDialog()
{
   if (dialog) return;
   dialog=new DebuggerDialog(owner,[this] { if (active && waiting) { stopAtNext=false; if (resumeRequested) resumeRequested(); } });
   dialog->setObjectName("debuggerDialog"); dialog->setProperty("helpTopic","Breakpoint"); dialog->setWindowTitle("Debugger — mission paused"); dialog->resize(940,620);
   auto *layout=new QVBoxLayout(dialog);
   state=new QLabel(dialog); state->setObjectName("debuggerState"); state->setWordWrap(true); layout->addWidget(state);
   commandText=new QPlainTextEdit(dialog); commandText->setObjectName("debuggerCommand"); commandText->setReadOnly(true);
   commandText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); commandText->setMaximumHeight(110); layout->addWidget(commandText);
   auto *choices=new QHBoxLayout; choices->addWidget(new QLabel("Runtime objects",dialog));
   filter=new QComboBox(dialog); filter->setObjectName("debuggerFilter"); filter->addItems({"Spacecraft","All objects"}); choices->addWidget(filter); choices->addStretch(); layout->addLayout(choices);
   auto *splitter=new QSplitter(dialog);
   objects=new QListWidget(splitter); objects->setObjectName("debuggerObjects");
   text=new QPlainTextEdit(splitter); text->setObjectName("debuggerObjectText"); text->setReadOnly(true); text->setLineWrapMode(QPlainTextEdit::NoWrap); text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   splitter->setStretchFactor(0,1); splitter->setStretchFactor(1,4); layout->addWidget(splitter,1);
   auto *actions=new QHBoxLayout;
   auto *help=new QPushButton("Help",dialog); help->setObjectName("debuggerHelp"); actions->addWidget(help);
   stepButton=new QPushButton("Step command",dialog); stepButton->setObjectName("debuggerStep"); actions->addWidget(stepButton);
   resumeButton=new QPushButton("Resume",dialog); resumeButton->setObjectName("debuggerResume"); actions->addWidget(resumeButton);
   stopButton=new QPushButton("End mission",dialog); stopButton->setObjectName("debuggerEnd"); actions->addWidget(stopButton); actions->addStretch();
   auto *close=new QPushButton("Close and resume",dialog); close->setObjectName("debuggerClose"); actions->addWidget(close); layout->addLayout(actions);
   connect(filter,&QComboBox::currentIndexChanged,this,[this] { populateObjects(); });
   connect(objects,&QListWidget::currentItemChanged,this,[this] { showObject(); });
   connect(stepButton,&QPushButton::clicked,this,[this] { step(); });
   connect(resumeButton,&QPushButton::clicked,this,[this] { stopAtNext=false; if (resumeRequested) resumeRequested(); });
   connect(stopButton,&QPushButton::clicked,this,[this] { if (stopRequested) stopRequested(); });
   connect(close,&QPushButton::clicked,dialog,&QDialog::reject);
   connect(help,&QPushButton::clicked,this,[this] { if (helpRequested) helpRequested(); });
   auto *resumeKey=new QShortcut(QKeySequence("F5"),dialog);
   connect(resumeKey,&QShortcut::activated,resumeButton,&QPushButton::click);
   auto *endKey=new QShortcut(QKeySequence("Shift+F5"),dialog);
   connect(endKey,&QShortcut::activated,stopButton,&QPushButton::click);
}
void Debugger::populateObjects()
{
   const auto previous=objects->currentItem() ? objects->currentItem()->text() : QString();
   QStringList names;
   for (const auto &name:Moderator::Instance()->GetListOfObjects(filter->currentIndex()==0 ? Gmat::SPACECRAFT : Gmat::UNKNOWN_OBJECT))
      names.append(QString::fromStdString(name));
   names.sort(); names.removeDuplicates(); objects->clear(); objects->addItems(names);
   const auto matches=objects->findItems(previous,Qt::MatchExactly);
   if (!matches.isEmpty()) objects->setCurrentItem(matches.first()); else if (objects->count()>0) objects->setCurrentRow(0); else text->setPlainText("No runtime objects in this view. Select All objects.");
}
void Debugger::showObject()
{
   if (!active || !objects->currentItem()) return;
   const auto name=objects->currentItem()->text();
   try {
      auto *object=Moderator::Instance()->GetInternalObject(name.toStdString());
      if (!object) { text->setPlainText("No runtime object is available for "+name+"."); return; }
      auto contents=QString::fromStdString(object->GetGeneratingString(Gmat::DEBUG_INSPECT));
      if (auto *parameter=dynamic_cast<Parameter *>(object)) contents+="\n\n------- Current Value -------\n\n"+QString::fromStdString(parameter->ToString());
      text->setPlainText(contents);
   } catch (BaseException &error) { text->setPlainText("Cannot inspect "+name+":\n"+QString::fromStdString(error.GetFullMessage())); }
   catch (const std::exception &error) { text->setPlainText("Cannot inspect "+name+":\n"+QString::fromUtf8(error.what())); }
}
void Debugger::BeforeExecution(GmatCommand *command)
{
   if (!active || !command || command->GetCurrentFunction()) return; // Step over function calls.
   if (command->IsOfType("NoOp") || command->IsOfType("BeginMissionSequence")) return;
   // Branch dispatchers can keep executing solver state transitions while no
   // child branch is active. GetNext() returns self for those internal ticks;
   // pause at entry and at child commands, not at dispatcher maintenance.
   if (auto *branch=dynamic_cast<BranchCommand *>(command);branch && (branch->IsExecuting() || branch->GetNext()==branch)) return;
   if (stopped) throw SandboxException("Execution interrupted by debugger");
   if (!stopAtNext && !breakpoints.contains(command)) return;
   stopAtNext=false; ensureDialog(); waiting=true;
   state->setText("Paused before "+QString::fromStdString(command->GetTypeName())+". Step runs this command and pauses before the next one.");
   commandText->setPlainText(QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS)));
   dialog->setProperty("commandIndex",indices.value(command,-1));
   if (filter->currentIndex()==0 && Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT).empty()) filter->setCurrentIndex(1);
   populateObjects(); dialog->show(); dialog->raise(); dialog->activateWindow(); stepButton->setFocus();
   QEventLoop wait; waitLoop=&wait;
   if (paused) paused(QString::fromStdString(command->GetTypeName()));
   if (waiting) wait.exec();
   waitLoop=nullptr; waiting=false;
   if (stopped) throw SandboxException("Execution interrupted by debugger");
}
