#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include <GL/gl.h>
#include <GL/glu.h>

template<class T> static T *Find(wxWindow *w)
{
   if (auto found = dynamic_cast<T *>(w)) return found;
   for (auto child : w->GetChildren()) if (auto found = Find<T>(child)) return found;
   return nullptr;
}
class Regression : public wxTimer
{
   int step = 0;
   OrbitViewCanvas *canvas = nullptr;
   wxSize initialSize;
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      if (step == 0)
      {
         frame->Maximize(false);
         frame->SetSize(1100,800);
         Check(frame->RunCurrentMission() == 1,"native mission completes");
         canvas = Find<OrbitViewCanvas>(frame);
         Check(canvas != nullptr,"native orbit canvas exists");
         if (!canvas) { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         if (auto child = dynamic_cast<wxMDIChildFrame *>(canvas->GetParent())) child->Activate();
         canvas->Refresh();
      }
      else
      {
         if (step == 1) initialSize = canvas->GetClientSize();
         else Check(canvas->GetClientSize() != initialSize, "canvas dimensions change after resize");
         Check(canvas->SetGLContext(),"font test uses the current canvas context");
         double scale = canvas->GetContentScaleFactor();
         glPushAttrib(GL_ALL_ATTRIB_BITS);
         glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,1000,0,1000);
         glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
         GLfloat before[4],after[4];
         glRasterPos2i(50,50); glGetFloatv(GL_CURRENT_RASTER_POSITION,before);
         const char text[]="MMMM";
         glCallLists(4,GL_UNSIGNED_BYTE,text); glGetFloatv(GL_CURRENT_RASTER_POSITION,after);
         double advance=after[0]-before[0];
         std::printf("TEXT_ADVANCE: pixels=%.2f logical=%.2f scale=%.2f\n",advance,advance/scale,scale);
         Check(advance/scale >= 36 && advance/scale <= 64,"four glyphs have readable, bounded logical width");
         glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
         glPopAttrib();
         Screenshot(frame,step==1?"text-initial":"text-resized");
         if (step==1) { frame->SetMinSize(wxSize(600,450)); frame->SetSize(850,650); }
         else { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
      }
      ++step;StartOnce(500);
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
