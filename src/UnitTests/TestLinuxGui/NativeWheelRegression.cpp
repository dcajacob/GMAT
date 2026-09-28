#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include <GL/gl.h>
#include <vector>
#include <cmath>

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
   size_t initial = 0;
   std::string mode = std::getenv("GMAT_WHEEL_MODE") ? std::getenv("GMAT_WHEEL_MODE") : "centered";
   OrbitViewCanvas *canvas = nullptr;
   size_t EarthPixels()
   {
      canvas->SetGLContext();
      const auto size = canvas->GetClientSize();
      const double scale = canvas->GetContentScaleFactor();
      const int width = wxRound(size.x * scale), height = wxRound(size.y * scale);
      std::vector<unsigned char> pixels(width * height * 3);
      glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      GLint buffer; glGetIntegerv(GL_READ_BUFFER, &buffer);
      glReadBuffer(GL_BACK);
      glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
      glReadBuffer(buffer); glPopClientAttrib();
      size_t count = 0;
      for (size_t i = 0; i < pixels.size(); i += 3)
         if (pixels[i+2] > 40 && pixels[i+1] > 30 && pixels[i+2] > pixels[i] * 1.2)
            ++count;
      std::printf("EARTH_PIXELS: %zu\n", count);
      return count;
   }
   void Wheel(int rotation)
   {
      wxMouseEvent event(wxEVT_MOUSEWHEEL);
      event.m_wheelRotation = rotation;
      event.m_wheelDelta = 120;
      event.SetShiftDown(mode == "shift");
      event.SetPosition(wxPoint(canvas->GetClientSize().x / 2, canvas->GetClientSize().y / 2));
      event.SetEventObject(canvas);
      canvas->GetEventHandler()->ProcessEvent(event);
      canvas->Update();
   }
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      if (step == 0)
      {
         Check(frame->RunCurrentMission() == 1, "default mission completes");
         canvas = FindOrbit(frame);
         Check(canvas != nullptr, "native orbit canvas exists");
         if (!canvas) { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         if (auto child = dynamic_cast<wxMDIChildFrame *>(canvas->GetParent())) child->Activate();
         canvas->Refresh();
      }
      else if (step == 1)
      {
         if (mode == "free" || mode == "astronaut")
         {
            wxKeyEvent key(wxEVT_KEY_DOWN);
            key.m_keyCode = 'Z';
            key.SetShiftDown(mode == "astronaut");
            canvas->GetEventHandler()->ProcessEvent(key);
         }
         initial = EarthPixels();
         Check(initial > 1000, "rendered Earth supplies a measurable zoom target");
         Wheel(120);
      }
      else if (step == 2)
      {
         Check(EarthPixels() > initial * ((mode == "shift" || mode == "astronaut") ? 1.02 : 1.1), "wheel zoom enlarges the rendered Earth");
         Screenshot(frame, "wheel-in");
         Wheel(-120);
      }
      else
      {
         Check(std::abs(double(EarthPixels()) - initial) < initial * 0.03,
               "opposite wheel motion restores the original view");
         frame->SetAutoExitAfterRun(true); frame->Close(); return;
      }
      ++step; StartOnce(500);
   }
};
extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc, argv); }
