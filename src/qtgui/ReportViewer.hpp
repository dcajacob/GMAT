#pragma once
#include <QWidget>
#include <QString>
#include <QByteArray>
class QPlainTextEdit;
class QLineEdit;
class QFile;
class QTimer;
class QLabel;
class QSpinBox;
class QPushButton;
class ReportViewer final : public QWidget
{
public:
   ReportViewer(const QString &path,const QString &name,QWidget *parent=nullptr);
private:
   bool loadPage(qint64 page);
   void searchFile(qint64 start);
   void stopSearch();
   QLineEdit *searchText;
   QPushButton *nextMatch,*stop;
   QFile *searchInput;
   QTimer *searchTimer;
   QByteArray needle,overlap,visibleBytes;
   qint64 nextOffset=0,visibleStart=0;
   QString path;
   qint64 currentPage=0;
   QPlainTextEdit *text;
   QLabel *status;
   QSpinBox *pageNumber;
   QPushButton *first,*previous,*next,*last;
};
