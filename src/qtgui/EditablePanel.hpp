#pragma once
#include <QWidget>
#include <QMdiSubWindow>
class EditablePanel : public QWidget
{
public:
   using QWidget::QWidget;
   virtual bool hasChanges() const = 0;
   virtual void discardChanges() = 0;
};
class EditorSubWindow final : public QMdiSubWindow
{
protected:
   void closeEvent(QCloseEvent *event) override;
};
