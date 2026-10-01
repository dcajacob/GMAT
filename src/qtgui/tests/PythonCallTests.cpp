#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "CommandForm.hpp"
#include "ReportParameterDialog.hpp"
#include "HelpDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTextBrowser>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTimer>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Python-call report unavailable"); return file.readAll(); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPythonCalls");
   try {
      TestSettings settings; QTemporaryDir files; const bool preview=argc==4 && QString::fromLocal8Bit(argv[2])=="--preview"; require((argc==2 || preview) && files.isValid(),"Python-call setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Python-call runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("python scalar.txt"),saved=files.filePath("python call Δ.script");
      const QString command="'Compute norm' [Magnitude] = Python.ArrayFunctions.mag(A, Count); % retain Python call";
      const QString prefix="% retain Python source α\nCreate Array A[1,3];\nA(1,1) = 3;\nA(1,2) = 4;\nA(1,3) = 0;\nCreate Variable Count Scale Magnitude;\nCount = 3;\nScale = 3;\nCreate String Message;\nMessage = 'Python GUI string input';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nValues.WriteHeaders = false;\nBeginMissionSequence;\n";
      const QString suffix="\nPython.builtins.print(Message); % no outputs, string input\nReport Values Magnitude;\n";
      const auto source=prefix+command+suffix;
      auto open=[&] {
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'Compute norm'")) index=i; if (index<0 || !snapshot.nodes[index].editable) std::cerr<<"Canonical mission:\n"<<snapshot.canonicalScript.toStdString()<<"\n"; require(index>=0 && snapshot.nodes[index].editable,"Python call is not safely mapped to original source");
         auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree); while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item; require(*item,"Python command tree item missing"); tree->itemDoubleClicked(*item,0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Python actual MDI editor missing"); return child;
      };
      if (preview) {
         editor->setPlainText(source); require(window.buildScript(),"Python native preview fixture failed"); auto *child=open(); auto *panel=child->widget(); require(panel->findChild<QComboBox *>("commandPythonModule") && panel->findChild<QPushButton *>("commandChoose_Inputs"),"Python preview controls missing"); QEventLoop wait; QTimer::singleShot(250,&wait,&QEventLoop::quit); wait.exec(); require(window.windowHandle() && window.windowHandle()->isExposed() && panel->grab().save(QString::fromLocal8Bit(argv[3])),"Python native preview capture unavailable"); std::cout<<"PASS: native actual MDI Python module/function and ordered argument controls preview; no numerical calls repeated.\n"; return 0;
      }
      QString pure; CommandForm form([&](const QString &value) { pure=value; }); const QString syntax="  'Keep label' [One, Two] = Python.ArrayFunctions.cross(A(1, 2), 'literal, comma'); % keep tail\n"; form.setStatement(syntax); require(form.title()=="Python call" && !form.findChild<QPushButton *>("commandChoose_Function"),"Python call mistaken for a GMAT function resource"); form.findChild<QLineEdit *>("commandField_Function")->setText("dot"); require(pure==QString(syntax).replace(".cross(",".dot("),"Python field edit changed arguments/label/comments"); form.setStatement("Python.builtins.print(Message);"); require(form.title()=="Python call" && !form.findChild<QLineEdit *>("commandField_Outputs") && form.findChild<QPushButton *>("commandChoose_Inputs"),"No-output Python call controls missing");
      auto reference=source; reference.replace(".mag(A, Count)",".magtimes(Scale, A, Count)"); editor->setPlainText(reference); run(window); const auto expected=read(report); require(std::abs(QString::fromUtf8(expected).trimmed().toDouble()-15)<1e-12,"Independent Python scalar result incorrect");
      editor->setPlainText(source); require(window.buildScript(),"Python control source build failed"); auto *child=open(); auto *panel=dynamic_cast<CommandEditor *>(child->widget()); auto *module=panel->findChild<QComboBox *>("commandPythonModule"); auto *function=panel->findChild<QLineEdit *>("commandField_Function"); require(module && module->isEditable() && module->findText("ArrayFunctions")>=0 && function,"Configured Python module/function controls missing");
      require(panel->property("helpTopic")=="CallPythonFunction","Labeled Python Help topic incorrect"); panel->findChild<QPushButton *>("contextHelp")->click(); HelpDialog *help=nullptr; for (auto *dialog:window.findChildren<QDialog *>("helpDialog")) if (auto *candidate=dynamic_cast<HelpDialog *>(dialog);candidate && candidate->isVisible()) help=candidate; require(help && help->findChild<QTextBrowser *>("helpBrowser")->source().fileName()=="CallPythonFunction.html" && help->findChild<QLabel *>("helpStatus")->text().isEmpty(),"Python context Help unavailable"); help->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      CommandEditor noOutput("Python.builtins.print(Message);",false,{},[](const QString &) { return QString(); }); require(noOutput.property("helpTopic")=="CallPythonFunction","No-output Python Help topic incorrect");
      auto arguments=[&](const QString &name,const QStringList &values,bool accept) {
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] { ReportParameterDialog *dialog=nullptr; for (auto *item:panel->findChildren<QDialog *>()) if (auto *candidate=dynamic_cast<ReportParameterDialog *>(item)) dialog=candidate;
            try { require(dialog,"Python ordered argument browser unavailable"); auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters"); list->clear(); list->addItems(values); if (accept) dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); else dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         }); panel->findChild<QPushButton *>("commandChoose_"+name)->click(); if (failure) std::rethrow_exception(failure);
      };
      arguments("Inputs",{"Count"},false); require(!panel->hasChanges() && editor->toPlainText()==source,"Python argument Cancel changed source"); arguments("Outputs",{"Magnitude"},true); function->setText("magtimes"); arguments("Inputs",{"Scale","A","Count"},true);
      require(panel->hasChanges() && editor->toPlainText()==source,"Python controls did not remain pending"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Python Apply lost retained clean panel"); const auto changed=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==source,"Python Undo was not exact"); editor->redo(); require(editor->toPlainText()==changed,"Python Redo was not exact"); require(changed.startsWith(prefix) && changed.endsWith(suffix) && changed.contains("% retain Python call"),"Python edit changed unrelated source"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Python Unicode save/reopen failed"); run(window); require(read(report)==expected,"GUI Python call differs from independent scalar report");
      // Runtime lookup failures leave an editable command and permit correction
      // without changing Python's module cache or replacing its implementation.
      for (const auto &failure:QStringList{"module","function"}) {
         child=open(); panel=dynamic_cast<CommandEditor *>(child->widget()); if (failure=="module") panel->findChild<QComboBox *>("commandPythonModule")->setCurrentText("QtMissingPythonModule"); else panel->findChild<QLineEdit *>("commandField_Function")->setText("QtMissingPythonFunction"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto bad=editor->toPlainText(); require(bad.contains(failure=="module" ? "QtMissingPythonModule" : "QtMissingPythonFunction"),"Python missing-name edit not applied"); require(window.runMission()==MainWindow::RunResult::Failed && editor->toPlainText()==bad,"Python missing-name failure lost source");
         child=open(); panel=dynamic_cast<CommandEditor *>(child->widget()); if (failure=="module") panel->findChild<QComboBox *>("commandPythonModule")->setCurrentText("ArrayFunctions"); else panel->findChild<QLineEdit *>("commandField_Function")->setText("magtimes"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(window.saveScriptTo(saved) && window.loadScript(saved),"Corrected Python call did not reopen"); run(window); require(read(report)==expected,"Corrected Python lookup did not recover original result");
      }
      std::cout<<"PASS: actual MDI Python module/function and ordered input/output controls, Cancel, pending/retained Apply, exact Undo/Redo/Unicode save/reopen, independent scalar-from-array and no-output string call; missing module/function run failure and GUI correction/reopen recovery; source spans retain nested/literal commas, labels/comments and unrelated configuration.\n";
   } catch (BaseException &failure) { std::cerr<<"FAIL: "<<failure.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &failure) { std::cerr<<"FAIL: "<<failure.what()<<'\n'; return 1; }
}
