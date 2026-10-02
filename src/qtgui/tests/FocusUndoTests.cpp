#include "MainWindow.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void shortcut(QWidget *target,QAction *action)
{
   const auto sequence=action->shortcut();
   require(sequence.count()==1,"Focus test requires a configured single-chord edit shortcut");
   const auto chord=sequence[0];
   QKeyEvent down(QEvent::KeyPress,chord.key(),chord.keyboardModifiers()),up(QEvent::KeyRelease,chord.key(),chord.keyboardModifiers());
   QApplication::sendEvent(target,&down); QApplication::sendEvent(target,&up);
   QApplication::processEvents();
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtFocusUndo");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Focus test requires offscreen startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Focus fixture directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Focus runtime initialization failed"); window.show();
      auto *area=window.findChild<QMdiArea *>("workspace");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      require(area && editor && tree,"Focus workspace controls missing");
      const auto action=[&](const char *name) {
         auto *value=window.findChild<QAction *>(name); require(value && value->isEnabled(),"Edit action unavailable"); return value;
      };
      const auto focus=[&](QWidget *target) {
         window.activateWindow(); target->setFocus(Qt::OtherFocusReason); app.processEvents();
         require(QApplication::focusWidget()==target || target->isAncestorOf(QApplication::focusWidget()),"Focus target was not selected");
      };
      const QString baseline="% retain focus mission α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\n"
         "FM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nBeginMissionSequence;\n"
         "Propagate 'Keep command label' P(Sat) {Sat.ElapsedSecs = 60}; % keep command comment\n";
      editor->setPlainText(baseline);
      if (!window.buildScript()) throw std::runtime_error(("Focus fixture build failed\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString());
      const auto snapshot=window.missionSnapshot(); int propagation=-1;
      for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.startsWith("Propagate")) propagation=i;
      require(propagation>=0,"Propagation source missing");
      const auto error=window.applyMissionChange(snapshot,propagation,MissionEdit::Replace,
         "Propagate 'Keep command label' P(Sat) {Sat.ElapsedSecs = 120}; % keep command comment");
      require(error.isEmpty(),qPrintable(error)); const auto committed=editor->toPlainText();
      require(committed.contains("ElapsedSecs = 120") && committed!=baseline,"GUI mission edit was not committed");
      // Apply replaces the engine and closes previous plots. Execute once
      // after the edit to obtain an actual current plot without stale pointers.
      require(window.runMission()==MainWindow::RunResult::Completed,"Short focus fixture did not run");
      QMdiSubWindow *plotWindow=nullptr; PlotWidget *plot=nullptr;
      for (auto *child:area->subWindowList()) if (auto *candidate=dynamic_cast<PlotWidget *>(child->widget())) { plotWindow=child; plot=candidate; break; }
      require(plotWindow && plot,"Executed plot window missing");
      auto *timeline=plot->findChild<QSlider *>("plotTimeline"); require(timeline,"Plot timeline missing");
      std::cout<<"Configured shortcuts: Undo "<<action("edit_undo")->shortcut().toString().toStdString()
         <<", Redo "<<action("edit_redo")->shortcut().toString().toStdString()<<'\n';
      // The script is not focused: real shortcut routing must reach its atomic
      // GUI edit from a tree and from an existing plot's non-text controls.
      focus(tree); shortcut(tree,action("edit_undo"));
      require(editor->toPlainText()==baseline,"Tree-focus configured Undo did not restore exact committed source");
      area->setActiveSubWindow(plotWindow); focus(timeline);
      shortcut(timeline,action("edit_redo"));
      require(editor->toPlainText()==committed,"Plot-focus configured Redo did not restore exact source");
      shortcut(timeline,action("edit_undo"));
      require(editor->toPlainText()==baseline,"Plot-focus configured Undo did not undo source");
      focus(tree); action("edit_redo")->trigger(); require(editor->toPlainText()==committed,"Tree-focus menu Redo failed");
      // Clipboard actions remain field-local and have no mission fallback.
      app.clipboard()->setText("focus clipboard sentinel");
      for (const auto *name:{"edit_copy","edit_cut","edit_paste"}) action(name)->trigger();
      require(editor->toPlainText()==committed && app.clipboard()->text()=="focus clipboard sentinel","Non-text clipboard action changed script or clipboard");
      // An independently selected inactive document retains its own history
      // when focus moves outside the workspace to the Resources tree.
      const auto otherPath=files.filePath("inactive Δ.script");
      const QString other="% independent inactive script\nCreate Variable Other;\nBeginMissionSequence;\nOther = 2;\n";
      QFile otherFile(otherPath); require(otherFile.open(QIODevice::WriteOnly) && otherFile.write(other.toUtf8())==other.toUtf8().size(),"Inactive fixture write failed"); otherFile.close();
      require(window.openScriptDocument(otherPath),"Inactive document open failed");
      auto *otherWindow=area->activeSubWindow(); auto *otherEditor=otherWindow->findChild<QPlainTextEdit *>("inactiveScriptEditor");
      require(otherEditor && !otherWindow->property("activeScript").toBool(),"Inactive selection activated the model");
      auto cursor=otherEditor->textCursor(); cursor.movePosition(QTextCursor::End); cursor.beginEditBlock(); cursor.insertText("% selected inactive edit\n"); cursor.endEditBlock();
      const auto otherEdited=otherEditor->toPlainText();
      focus(tree); action("edit_undo")->trigger();
      require(otherEditor->toPlainText()==other && editor->toPlainText()==committed,"Tree-focus Undo selected the active mission instead of inactive script");
      action("edit_redo")->trigger();
      require(otherEditor->toPlainText()==otherEdited && editor->toPlainText()==committed && !otherWindow->property("activeScript").toBool(),"Inactive Redo changed active mission or selection");
      // Local text fields keep local history; no fallback occurs when their
      // local Undo runs out. This uses the same line-edit target as resources.
      auto *field=new QLineEdit("field"); auto *fieldWindow=area->addSubWindow(field); fieldWindow->show(); area->setActiveSubWindow(fieldWindow);
      focus(field); field->setCursorPosition(field->text().size()); field->insert(" edit");
      action("edit_undo")->trigger(); require(field->text()=="field","Field-local Undo was lost");
      action("edit_undo")->trigger(); require(editor->toPlainText()==committed && otherEditor->toPlainText()==otherEdited,"Empty field Undo fell back to a script");
      action("edit_redo")->trigger(); require(field->text()=="field edit" && editor->toPlainText()==committed,"Field-local Redo was lost");
      // Even directly triggered main-menu actions may not reach a script
      // behind a modal editor. Its focused fields may still use local history.
      QDialog modal(&window); auto *layout=new QVBoxLayout(&modal); auto *modalField=new QLineEdit("modal",&modal); auto *button=new QPushButton("Modal control",&modal);
      layout->addWidget(modalField); layout->addWidget(button); std::exception_ptr failure;
      QTimer::singleShot(0,&modal,[&] {
         try {
            require(QApplication::activeModalWidget()==&modal,"Modal context missing");
            modal.activateWindow(); app.processEvents(); modalField->setFocus(Qt::OtherFocusReason); app.processEvents();
            require(QApplication::focusWidget()==modalField,"Offscreen modal field did not receive focus");
            require(modal.parentWidget()==&window && modal.isAncestorOf(modalField),"Modal field ownership is incorrect");
            modalField->setCursorPosition(modalField->text().size()); modalField->insert(" edit");
            action("edit_undo")->trigger(); require(modalField->text()=="modal","Modal field-local Undo failed");
            action("edit_redo")->trigger(); require(modalField->text()=="modal edit","Modal field-local Redo failed");
            button->setFocus(Qt::OtherFocusReason); app.processEvents();
            require(QApplication::focusWidget()==button,"Offscreen modal button did not receive focus");
            action("edit_undo")->trigger(); action("edit_redo")->trigger();
            require(editor->toPlainText()==committed && otherEditor->toPlainText()==otherEdited && modalField->text()=="modal edit","Modal button action reached a hidden history");
         } catch (...) { failure=std::current_exception(); }
         modal.reject();
      });
      modal.exec(); if (failure) std::rethrow_exception(failure);
      std::cout<<"PASS committed mission Undo/Redo from tree/plot focus, exact source, local fields, inactive script selection, modal guard and no clipboard fallback.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
