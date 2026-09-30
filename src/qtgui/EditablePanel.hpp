#pragma once
#include <QWidget>
#include <QMdiSubWindow>
#include <functional>
class EditablePanel : public QWidget
{
public:
   using QWidget::QWidget;
   virtual bool hasChanges() const = 0;
   virtual void discardChanges() = 0;
   // The workspace refreshes an accepted snapshot in the same MDI window.
   // Standalone callers retain their existing close-on-success contract.
   std::function<void()> onApplied;
protected:
   void appliedSuccessfully() { if (onApplied) onApplied(); else if (parentWidget()) parentWidget()->close(); }
};
class EditorSubWindow final : public QMdiSubWindow
{
protected:
   void closeEvent(QCloseEvent *event) override;
};
