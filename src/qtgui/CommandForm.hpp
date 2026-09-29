#pragma once
#include <QGroupBox>
#include <QVector>
#include <functional>
class QFormLayout;
class QLineEdit;

// Edits source spans, preserving labels, formatting, options and branch bodies.
class CommandForm final : public QGroupBox
{
public:
   CommandForm(std::function<void(const QString &)> changed,QWidget *parent=nullptr);
   void setStatement(const QString &statement);
private:
   struct Field { QLineEdit *input; qsizetype start,length; };
   QFormLayout *layout;
   QVector<Field> fields;
   QString original;
   std::function<void(const QString &)> changed;
   bool synchronizing=false;
   void updateSource();
};
