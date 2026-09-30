#pragma once
#include <QDialog>
#include <QPointer>
#include <QUrl>
class QTextBrowser;
class QLabel;
class QLineEdit;
class HelpDialog final : public QDialog
{
public:
   HelpDialog(const QString &topic,QWidget *parent=nullptr);
   void openTopic(const QString &topic);
private:
   void load(const QUrl &url);
   void find(bool backwards=false);
   QTextBrowser *browser;
   QLabel *status;
   QLineEdit *search;
   QUrl home;
   QUrl requested;
};
// The main window owns one controller so F1 also works in its modal editors.
class HelpController final : public QObject
{
public:
   explicit HelpController(QWidget *owner);
   void attach(QWidget *widget,const QString &topic);
   void show(const QString &topic);
protected:
   bool eventFilter(QObject *object,QEvent *event) override;
private:
   QString topic(QWidget *widget) const;
   QPointer<QWidget> owner;
   QPointer<HelpDialog> dialog;
};
