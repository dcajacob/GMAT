#ifndef QtMessageReceiver_hpp
#define QtMessageReceiver_hpp

#include "MessageReceiver.hpp"

#include <QObject>
#include <QString>

#include <cstdarg>
#include <fstream>
#include <functional>
#include <mutex>
#include <queue>
#include <string>

class QtMessageReceiver : public QObject, public MessageReceiver
{
public:
   using MessageCallback = std::function<void(const QString&)>;

   explicit QtMessageReceiver(QObject *callbackContext = nullptr);
   ~QtMessageReceiver() override;

   void SetMessageCallback(QObject *context, MessageCallback callback);

   void ShowMessage(const std::string &msg) override;
   void ShowMessage(const char *msg, ...) override;
   void PopupMessage(Gmat::MessageType msgType, const std::string &msg) override;
   void PopupMessage(Gmat::MessageType msgType, const char *msg, ...) override;

   std::string GetLogFileName() override;
   bool GetLogEnable() override;
   void SetLogEnable(bool flag) override;
   void SetLogPath(const std::string &pathname, bool append = false) override;
   void SetLogFile(const std::string &filename) override;
   void LogMessage(const std::string &msg) override;
   void LogMessage(const char *msg, ...) override;

   void ClearMessage() override;
   std::string GetMessage() override;
   void PutMessage(const std::string &msg) override;
   void ClearMessageQueue() override;

private:
   std::string FormatMessage(const char *msg, va_list args);
   void DeliverMessage(const std::string &msg);
   void OpenLogIfNeeded();

   QObject *mCallbackContext;
   MessageCallback mCallback;
   std::string mLogFileName;
   std::string mLogPath;
   bool mLogEnabled;
   bool mAppendLog;
   std::ofstream mLogStream;
   std::queue<std::string> mMessages;
   std::mutex mMutex;
};

#endif

