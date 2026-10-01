#ifndef CommandExecutionObserver_hpp
#define CommandExecutionObserver_hpp

class GmatCommand;

// Optional synchronous observer on the engine owner's thread. A debugger may
// wait here; the normal dispatcher still executes and advances each command.
class CommandExecutionObserver
{
public:
   virtual ~CommandExecutionObserver() = default;
   virtual void BeforeExecution(GmatCommand *command) = 0;
};

#endif
