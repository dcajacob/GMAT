#pragma once
#include <QDialog>
#include <QStringList>
class QTableWidget;
class QComboBox;
class QCheckBox;
class PropagationGroupsDialog final : public QDialog
{
public:
   PropagationGroupsDialog(const QString &source,const QStringList &propagators,const QStringList &objects,QWidget *parent=nullptr);
   static bool supports(const QString &source);
   QString statement() const;
private:
   QString source;
   int start=0,length=0;
   QTableWidget *table;
   QComboBox *mode;
   QCheckBox *backward;
   QCheckBox *stm;
   QCheckBox *aMatrix;
   QCheckBox *covariance;
   bool explicitSTM=false;
};
