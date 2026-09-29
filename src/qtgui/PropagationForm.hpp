#pragma once
#include <QGroupBox>
#include <QStringList>
#include <functional>
class QComboBox;
class QLineEdit;
class PropagationForm final : public QGroupBox
{
public:
   PropagationForm(const QStringList &propagators, const QStringList &spacecraft,
                   std::function<void(const QString &)> changed, QWidget *parent=nullptr);
   void setStatement(const QString &statement);
private:
   QComboBox *propagator, *spacecraft, *units;
   QLineEdit *duration,*stopParameter;
   QString original;
   bool synchronizing=false;
};
