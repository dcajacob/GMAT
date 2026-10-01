#pragma once
#include "CommandExecutionObserver.hpp"
#include <QObject>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVector>
#include <functional>
class QDialog;
class QEventLoop;
class QComboBox;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QWidget;

class Debugger final : public QObject,public CommandExecutionObserver
{
public:
   explicit Debugger(QWidget *parent);
   void begin(const QVector<GmatCommand *> &commands,const QSet<int> &breakpoints);
   void finish();
   void requestPause() { stopAtNext=true; }
   void step();
   void resume();
   void stop();
   bool isActive() const { return active; }
   bool isWaiting() const { return waiting; }
   void BeforeExecution(GmatCommand *command) override;
   std::function<void(const QString &)> paused;
   std::function<void()> resumeRequested,stopRequested,helpRequested;
private:
   void ensureDialog();
   void populateObjects();
   void showObject();
   QWidget *owner;
   QPointer<QDialog> dialog;
   QComboBox *filter=nullptr;
   QListWidget *objects=nullptr;
   QPlainTextEdit *text=nullptr,*commandText=nullptr;
   QLabel *state=nullptr;
   QPushButton *stepButton=nullptr,*resumeButton=nullptr,*stopButton=nullptr;
   QEventLoop *waitLoop=nullptr;
   QMap<GmatCommand *,int> indices;
   QSet<GmatCommand *> breakpoints;
   bool active=false,waiting=false,stopped=false,stopAtNext=false;
};
