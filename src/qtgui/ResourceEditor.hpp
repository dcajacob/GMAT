#pragma once
#include <QWidget>
#include <QMap>
#include <QString>
#include <functional>
#include <QMdiSubWindow>
class GmatBase;
class QTableWidget;
class QLabel;
class ResourceEditor final : public QWidget
{
public:
   using Apply = std::function<QString(const QMap<QString, QString> &)>;
   ResourceEditor(GmatBase &object, Apply apply, QWidget *parent = nullptr);
   bool hasChanges() const;
   void discardChanges() { applied = true; }
private:
   QTableWidget *table;
   QLabel *status;
   QMap<QString, QString> original;
   bool applied = false;
};

class ResourceSubWindow final : public QMdiSubWindow
{
protected:
   void closeEvent(QCloseEvent *event) override;
};
