#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include <GL/gl.h>

static OrbitViewCanvas *FindOrbit(wxWindow *window)
{
   if (auto canvas = dynamic_cast<OrbitViewCanvas *>(window)) return canvas;
   for (auto child : window->GetChildren())
      if (auto canvas = FindOrbit(child)) return canvas;
   return nullptr;
}

class Regression : public wxTimer
{
   int step = 0;
   OrbitViewCanvas *canvas = nullptr;
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      if (step == 0)
      {
         Check(frame->RunCurrentMission() == 1, "native default mission completes");
         canvas = FindOrbit(frame);
         Check(canvas != nullptr, "native OrbitView canvas exists");
         if (!canvas) { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         if (auto child = dynamic_cast<wxMDIChildFrame *>(canvas->GetParent())) child->Activate();
         canvas->Refresh();
      }
      else
      {
         Check(canvas->SetGLContext(), "native GL context is current");
         GLint viewport[4];
         glGetIntegerv(GL_VIEWPORT,viewport);
         wxSize logical = canvas->GetClientSize();
         const double scale = canvas->GetContentScaleFactor();
         const int width = wxRound(logical.x*scale), height = wxRound(logical.y*scale);
         std::printf("VIEWPORT: logical=%dx%d scale=%.2f actual=%dx%d expected=%dx%d\n",
                     logical.x,logical.y,scale,viewport[2],viewport[3],width,height);
         Check(viewport[0] == 0 && viewport[1] == 0 && viewport[2] == width && viewport[3] == height,
               "OpenGL viewport fills the physical canvas after painting/resizing");
         Screenshot(frame,step == 1 ? "native-initial" : "native-resized");
         if (step == 1) frame->SetSize(850,650);
         else if (step == 2) frame->SetSize(1100,800);
         else { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
      }
      ++step;
      StartOnce(300);
   }
};
extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
