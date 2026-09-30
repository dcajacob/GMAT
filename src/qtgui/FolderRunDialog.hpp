#pragma once
#include "FolderRun.hpp"
#include <QDialog>
#include <memory>
class QtPlotReceiver;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QDoubleSpinBox;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class FolderRunDialog final : public QDialog
{
public:
   using Run=std::function<FolderRunResult(const FolderRunOptions &,QtPlotReceiver &,const std::atomic_bool &,const std::function<void(int,int,const FolderRunItem &)> &)>;
   FolderRunDialog(const QString &output,Run run,std::function<void()> stop,QWidget *parent=nullptr);
   ~FolderRunDialog() override;
   void reject() override;
private:
   FolderRunOptions options() const;
   QLineEdit *directory,*output,*copy,*baseline,*replacement,*comparisonFile,*filter,*secondFilter;
   QSpinBox *first,*count,*repeats;
   QCheckBox *exclude,*secondExclude,*copies,*runFolders,*compare,*saveComparison;
   QDoubleSpinBox *tolerance;
   QPlainTextEdit *results;
   QLabel *status;
   QPushButton *runButton,*stopButton,*closeButton;
   std::unique_ptr<QtPlotReceiver> plots;
   std::atomic_bool cancelled=false;
   std::function<void()> stop;
   bool busy=false;
};
