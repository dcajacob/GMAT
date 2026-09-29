#pragma once

#include "ScriptInterpreter.hpp"
#include <QElapsedTimer>

// GMAT calls SetInputFocus at its existing interruption checkpoints. Keeping
// execution and event delivery on this thread avoids concurrent engine access.
class QtInterpreter final : public ScriptInterpreter
{
public:
   QtInterpreter();
   ~QtInterpreter() override = default;
   void SetInputFocus() override;
private:
   QElapsedTimer eventClock;
};
