#include "GuiTest.hpp"

class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      frame->SetAutoExitAfterRun(true);
      frame->Close();
   }
};

extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
