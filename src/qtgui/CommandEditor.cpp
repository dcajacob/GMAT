#include "CommandEditor.hpp"
#include "PropagationForm.hpp"
#include "PropagationStopsDialog.hpp"
#include "PropagationGroupsDialog.hpp"
#include "CommandForm.hpp"
#include "ScriptEventDialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
CommandEditor::CommandEditor(const QString &statement,bool adding,const QMap<QString,QString> &templates,
                             Apply apply,const QStringList &propagators,const QStringList &spacecraft,QWidget *parent,const QStringList &formations)
   : EditablePanel(parent),original(statement),inserting(adding)
{
   auto *layout=new QVBoxLayout(this);
   layout->addWidget(new QLabel(adding ? "Insert mission command" : "Edit mission command",this));
   auto *choices=new QComboBox(this);
   choices->addItem("Choose a command template…"); choices->addItems(templates.keys());
   if (adding) layout->addWidget(choices); else choices->hide();
   source=new QPlainTextEdit(statement,this); source->setObjectName("commandSource");
   auto *event=new QPushButton("Script event…",this); event->setObjectName("editScriptEvent"); layout->addWidget(event);
   auto showEvent=[this,event] { event->setVisible(ScriptEventDialog::supports(source->toPlainText())); };
   connect(source,&QPlainTextEdit::textChanged,this,showEvent); showEvent();
   connect(event,&QPushButton::clicked,this,[this] {
      ScriptEventDialog dialog(source->toPlainText(),this); if (dialog.exec()!=QDialog::Accepted) return;
      auto cursor=source->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document); cursor.insertText(dialog.statement()); cursor.endEditBlock();
   });
   auto *propagation=new PropagationForm(propagators,spacecraft,[this](const QString &text) {
      auto cursor=source->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document);
      cursor.insertText(text); cursor.endEditBlock();
   },this);
   layout->addWidget(propagation);
   connect(source,&QPlainTextEdit::textChanged,propagation,[this,propagation] { propagation->setStatement(source->toPlainText()); });
   propagation->setStatement(statement);
   auto *groups=new QPushButton("Propagators and spacecraft…",this); groups->setObjectName("editPropagationGroups"); layout->addWidget(groups);
   auto showGroups=[this,groups] { groups->setVisible(PropagationGroupsDialog::supports(source->toPlainText())); };
   connect(source,&QPlainTextEdit::textChanged,this,showGroups); showGroups();
   connect(groups,&QPushButton::clicked,this,[this,propagators,spacecraft,formations] {
      PropagationGroupsDialog dialog(source->toPlainText(),propagators,spacecraft+formations,this);
      if (dialog.exec()!=QDialog::Accepted) return;
      auto cursor=source->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document); cursor.insertText(dialog.statement()); cursor.endEditBlock();
   });
   auto *stops=new QPushButton("Stopping conditions…",this); stops->setObjectName("editPropagationStops"); layout->addWidget(stops);
   auto showStops=[this,stops] { stops->setVisible(PropagationStopsDialog::supports(source->toPlainText())); };
   connect(source,&QPlainTextEdit::textChanged,this,showStops); showStops();
   connect(stops,&QPushButton::clicked,this,[this] {
      PropagationStopsDialog dialog(source->toPlainText(),this);
      if (dialog.exec()!=QDialog::Accepted) return;
      auto cursor=source->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document); cursor.insertText(dialog.statement()); cursor.endEditBlock();
   });
   auto *command=new CommandForm([this](const QString &text) {
      auto cursor=source->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document);
      cursor.insertText(text); cursor.endEditBlock();
   },this);
   layout->addWidget(command);
   connect(source,&QPlainTextEdit::textChanged,command,[this,command] { command->setStatement(source->toPlainText()); });
   command->setStatement(statement);
   source->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   source->setLineWrapMode(QPlainTextEdit::NoWrap); layout->addWidget(source,1);
   auto *status=new QLabel("Apply validates the complete mission and updates the script. Branches include their enclosed commands.",this);
   status->setWordWrap(true); layout->addWidget(status);
   connect(source,&QPlainTextEdit::textChanged,status,[status] {
      status->setText("Apply validates the complete mission and updates the script. Branches include their enclosed commands.");
   });
   connect(choices,&QComboBox::textActivated,this,[this,templates](const QString &name) {
      if (!templates.contains(name)) return;
      if (source->document()->isModified() && QMessageBox::question(this,"Replace command text",
          "Replace the edited command text with this template?",QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel)!=QMessageBox::Yes) return;
      source->setPlainText(templates.value(name));
   });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close,this);
   layout->addWidget(buttons);
   connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this,apply,status] {
      const auto error=apply(source->toPlainText());
      if (error.isEmpty()) { applied=true; parentWidget()->close(); }
      else status->setText(error);
   });
   connect(buttons,&QDialogButtonBox::rejected,this,[this] { parentWidget()->close(); });
}
bool CommandEditor::hasChanges() const
{
   return !applied && (inserting || source->toPlainText()!=original);
}
