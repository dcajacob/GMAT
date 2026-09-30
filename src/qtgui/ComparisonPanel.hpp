#pragma once
#include <QWidget>
#include <QTemporaryDir>
#include <QStringList>
#include <memory>
#include <atomic>
#include "FileComparison.hpp"
class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QTableWidget;
class QPushButton;
class QLabel;
class QThread;
class ReportViewer;

class ComparisonPanel final : public QWidget
{
public:
   explicit ComparisonPanel(const QString &baseline={},QWidget *parent=nullptr);
   ~ComparisonPanel() override;
   bool isComparing() const;
   QString saveResults(const QString &path,bool replaceExisting=false) const;
   QString resultsPath() const { return completedPath; }
private:
   void addDirectory();
   void start();
   void setBusy(bool busy);
   QComboBox *scope,*mode;
   QLineEdit *base,*candidate,*prefix,*tolerance;
   QCheckBox *skipBlank;
   QSpinBox *limit;
   QTableWidget *directories;
   QWidget *fileRow,*directoryControls;
   QPushButton *compare,*stop,*save,*add,*remove;
   QLabel *status;
   QThread *worker=nullptr;
   std::shared_ptr<std::atomic_bool> cancellation;
   QTemporaryDir scratch;
   QString completedPath;
   QStringList completedInputs;
   int serial=0;
};
