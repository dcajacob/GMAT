#include "QtInterpreter.hpp"
#include "Moderator.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>

QtInterpreter::QtInterpreter()
{
   Initialize();
   eventClock.start();
}

void QtInterpreter::SetInputFocus()
{
   if (eventClock.elapsed() >= 16)
   {
      eventClock.restart();
      QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
   }
   // Sandbox polls even when paused; avoid burning a CPU while still servicing
   // Resume/Stop events at the same cadence.
   if (Moderator::Instance()->GetRunState() == Gmat::PAUSED)
      QThread::msleep(1);
}
