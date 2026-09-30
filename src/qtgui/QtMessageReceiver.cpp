#include "QtMessageReceiver.hpp"
#include "FileManager.hpp"

#include <QMetaObject>
#include <QStringDecoder>
#include <QDir>

#include <cstdarg>
#include <cstdio>
#include <vector>

QtMessageReceiver::QtMessageReceiver(QObject *callbackContext)
   : QObject(callbackContext),
     mCallbackContext(callbackContext),
     mLogEnabled(true),
     mAppendLog(false)
{
}

QtMessageReceiver::~QtMessageReceiver()
{
   if (mLogStream.is_open())
      mLogStream.close();
}

void QtMessageReceiver::SetMessageCallback(QObject *context, MessageCallback callback)
{
   std::lock_guard<std::mutex> lock(mMutex);
   mCallbackContext = context;
   mCallback = callback;
}

void QtMessageReceiver::ShowMessage(const std::string &msg)
{
   LogMessage(msg);
}

void QtMessageReceiver::ShowMessage(const char *msg, ...)
{
   va_list args;
   va_start(args, msg);
   std::string formatted = FormatMessage(msg, args);
   va_end(args);
   LogMessage(formatted);
}

void QtMessageReceiver::PopupMessage(Gmat::MessageType, const std::string &msg)
{
   LogMessage(msg);
}

void QtMessageReceiver::PopupMessage(Gmat::MessageType msgType, const char *msg, ...)
{
   (void)msgType;
   va_list args;
   va_start(args, msg);
   std::string formatted = FormatMessage(msg, args);
   va_end(args);
   LogMessage(formatted);
}

std::string QtMessageReceiver::GetLogFileName()
{
   if (!mLogFileName.empty())
      return mLogFileName;
   if (!mLogPath.empty())
      return mLogPath + "/GmatLog.txt";
   return "GmatLog.txt";
}

bool QtMessageReceiver::GetLogEnable()
{
   return mLogEnabled;
}

void QtMessageReceiver::SetLogEnable(bool flag)
{
   mLogEnabled = flag;
}

void QtMessageReceiver::SetLogPath(const std::string &pathname, bool append)
{
   mLogPath = pathname;
   // A startup LOG_FILE supplies an absolute name. Relocating Output must
   // relocate that name as well, as the wx receiver does.
   mLogFileName = QDir(QString::fromStdString(pathname)).filePath(
      QString::fromStdString(FileManager::Instance()->GetFilename("LOG_FILE"))).toStdString();
   mAppendLog = append;
   if (mLogStream.is_open())
      mLogStream.close();
}

void QtMessageReceiver::SetLogFile(const std::string &filename)
{
   mLogFileName = filename;
   if (mLogStream.is_open())
      mLogStream.close();
}

void QtMessageReceiver::RestoreLog(const std::string &filename, bool enabled)
{
   SetLogFile(filename);
   mLogEnabled = enabled;
   mAppendLog = true; // Resuming an existing destination must preserve its log.
}

void QtMessageReceiver::LogMessage(const std::string &msg)
{
   if (mLogEnabled && !mLogSuspended)
   {
      OpenLogIfNeeded();
      if (mLogStream.is_open())
         mLogStream << msg;
   }

   DeliverMessage(msg);
}

void QtMessageReceiver::LogMessage(const char *msg, ...)
{
   va_list args;
   va_start(args, msg);
   std::string formatted = FormatMessage(msg, args);
   va_end(args);
   LogMessage(formatted);
}

void QtMessageReceiver::ClearMessage()
{
   ClearMessageQueue();
}

std::string QtMessageReceiver::GetMessage()
{
   std::lock_guard<std::mutex> lock(mMutex);
   std::string allMessages;
   while (!mMessages.empty())
   {
      allMessages += mMessages.front();
      mMessages.pop();
   }
   return allMessages;
}

void QtMessageReceiver::PutMessage(const std::string &msg)
{
   std::lock_guard<std::mutex> lock(mMutex);
   mMessages.push(msg);
}

void QtMessageReceiver::ClearMessageQueue()
{
   std::lock_guard<std::mutex> lock(mMutex);
   while (!mMessages.empty())
      mMessages.pop();
}

std::string QtMessageReceiver::FormatMessage(const char *msg, va_list args)
{
   va_list copy;
   va_copy(copy, args);
   int size = vsnprintf(nullptr, 0, msg, copy);
   va_end(copy);

   if (size < 0)
      return std::string(msg);

   std::vector<char> buffer(static_cast<size_t>(size) + 1, '\0');
   vsnprintf(buffer.data(), buffer.size(), msg, args);
   std::string formatted(buffer.data());
   return formatted;
}

void QtMessageReceiver::DeliverMessage(const std::string &msg)
{
   QObject *context = nullptr;
   MessageCallback callback;
   {
      std::lock_guard<std::mutex> lock(mMutex);
      context = mCallbackContext;
      callback = mCallback;
   }

   if (!context || !callback)
      return;

   QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
   QString text = decoder(QByteArrayView(msg.data(), static_cast<qsizetype>(msg.size())));
   if (decoder.hasError())
      text = QString::fromLocal8Bit(msg.data(), static_cast<qsizetype>(msg.size()));
   QMetaObject::invokeMethod(context, [callback, text]() {
      callback(text);
   }, Qt::QueuedConnection);
}

void QtMessageReceiver::OpenLogIfNeeded()
{
   if (mLogStream.is_open())
      return;

   std::ios_base::openmode mode = std::ios::out;
   if (mAppendLog)
      mode |= std::ios::app;
   mLogStream.open(GetLogFileName().c_str(), mode);
}
