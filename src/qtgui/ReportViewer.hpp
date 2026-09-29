#pragma once
#include <QWidget>
#include <QString>
class QPlainTextEdit;
class QLabel;
class QSpinBox;
class QPushButton;
class ReportViewer final : public QWidget
{
public:
   ReportViewer(const QString &path,const QString &name,QWidget *parent=nullptr);
private:
   void loadPage(qint64 page);
   QString path;
   qint64 currentPage=0;
   QPlainTextEdit *text;
   QLabel *status;
   QSpinBox *pageNumber;
   QPushButton *first,*previous,*next,*last;
};
