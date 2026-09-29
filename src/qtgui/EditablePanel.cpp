#include "EditablePanel.hpp"
#include <QCloseEvent>
#include <QMessageBox>
void EditorSubWindow::closeEvent(QCloseEvent *event)
{
   const auto *panel=dynamic_cast<EditablePanel *>(widget());
   if (panel && panel->hasChanges() && QMessageBox::question(this,"Unapplied changes",
       "Discard the unapplied changes in this panel?",QMessageBox::Discard|QMessageBox::Cancel,
       QMessageBox::Cancel)!=QMessageBox::Discard) {
      event->ignore(); return;
   }
   QMdiSubWindow::closeEvent(event);
}
