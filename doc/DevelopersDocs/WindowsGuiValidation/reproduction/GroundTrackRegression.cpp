#include "GuiTest.hpp"
#include "GroundTrackArea.hpp"
#include "FileManager.hpp"
#include <wx/dcclient.h>
#include <wx/filename.h>
#include <fstream>

// Inspect actual painted pixels: a missing map still has grid lines and red
// tracks, so a successful mission or absence of log errors alone is insufficient.
static wxImage ReadPlot(GroundTrackArea *plot)
{
   wxSize size = plot->GetClientSize();
   wxBitmap bitmap(size.x, size.y);
   wxClientDC source(plot);
   wxMemoryDC copy(bitmap);
   Check(copy.Blit(0,0,size.x,size.y,&source,0,0), "ground-track pixels can be read");
   copy.SelectObject(wxNullBitmap);
   return bitmap.ConvertToImage();
}

static void CheckMap(GroundTrackArea *plot, bool expected)
{
   wxImage image = ReadPlot(plot);
   int blue = 0, count = 0;
   for (int y = 5; y < image.GetHeight(); y += 7)
      for (int x = 5; x < image.GetWidth(); x += 7)
      {
         if (image.GetBlue(x,y) > image.GetRed(x,y) + 20) ++blue;
         ++count;
      }
   // ClearType can introduce a few blue subpixels in otherwise gray labels.
   // A stale map covers a substantial fraction of the canvas.
   Check(count > 0 && (expected ? blue > count/5 : blue * 100 < count),
         expected ? "ground-track map is visibly painted" : "failed map clears the previous bitmap");
}

static GroundTrackArea *FindGroundTrack(wxWindow *window)
{
   if (auto plot = dynamic_cast<GroundTrackArea *>(window)) return plot;
   for (auto child : window->GetChildren())
      if (auto plot = FindGroundTrack(child)) return plot;
   return nullptr;
}

class Regression : public wxTimer
{
   int step = 0;
   GroundTrackArea *plot = nullptr;
   wxString customMap;
   void GroundTrackStep(GmatMainFrame *frame, bool defaultOnly)
   {
      if (step == 0)
      {
         frame->CloseWelcomePanel();
         Check(frame->RunCurrentMission() == 1, "default mission completes");
         plot = FindGroundTrack(frame);
         Check(plot != nullptr, "default mission creates a ground-track plot");
         if (!plot) { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         if (auto child = dynamic_cast<wxMDIChildFrame *>(plot->GetParent()))
         {
            child->Activate();
#ifdef __WXMSW__
            // Windows MDI plots overlap; keep the captured client area visible.
            child->Maximize();
#endif
         }
         plot->Refresh();
      }
      else if (step == 1)
      {
         CheckMap(plot, true);
         Screenshot(frame,"groundtrack-default");
         if (defaultOnly) { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         plot->SetOption("TextureMap", "ModifiedBlueMarble.jpg");
         plot->Refresh();
      }
      else if (step == 2)
      {
         CheckMap(plot, true); // Bare filename resolves through TEXTURE_PATH.
         wxImage image(16,16);
         image.SetRGB(wxRect(0,0,16,16),20,90,210);
         customMap = wxGetCwd()+"/custom-map.png";
         Check(image.SaveFile(customMap,wxBITMAP_TYPE_PNG), "custom PNG map fixture saved");
         plot->SetOption("TextureMap", customMap.ToStdString());
         plot->Refresh();
      }
      else if (step == 3)
      {
         CheckMap(plot, true); // Absolute PNG path, not forced through JPEG decoder.
         FileManager::Instance()->SetGmatWorkingDirectory(wxGetCwd().ToStdString());
         plot->SetOption("TextureMap", "./custom-map.png");
         plot->Refresh();
      }
      else if (step == 4)
      {
         CheckMap(plot, true); // Script-relative PNG path.
         plot->SetOption("TextureMap", "missing-groundtrack-test.jpg");
         plot->Refresh();
      }
      else if (step >= 5 && step <= 8)
      {
         CheckMap(plot, false);
         plot->Refresh(); // A missing map must warn once, not on every repaint.
      }
      else if (step == 9)
      {
         std::ofstream corrupt("corrupt-map.jpg");
         corrupt << "not an image";
         corrupt.close();
         plot->SetOption("TextureMap", "./corrupt-map.jpg");
         plot->Refresh();
      }
      else if (step >= 10 && step <= 12)
      {
         CheckMap(plot, false);
         plot->Refresh();
      }
      else if (step == 13)
      {
         plot->SetOption("TextureMap", customMap.ToStdString());
         plot->TakeAction("ClearData");
         plot->SetColor(0,255,0,0); // Empty curve must not underflow its segment count.
         plot->Refresh();
      }
      else if (step == 14)
      {
         CheckMap(plot, true); // A new setting recovers after failed loads.
         const double point[] = {12.0,20.0};
         plot->AddData(0.0,point,1); // One point has a label but no line segment.
         plot->Refresh();
      }
      else if (step == 15)
      {
         const double point[] = {12.0,20.0};
         plot->AddData(1.0,point,1); // Stationary track also has no drawn segment.
         plot->SetSize(0,0);
         plot->TakeAction("Show");
         plot->Refresh();
      }
      else if (step == 16)
      {
         plot->SetSize(plot->GetParent()->GetClientSize());
         plot->Refresh();
      }
      else
      {
         CheckMap(plot, true);
         Check(true, "empty, single-point and stationary tracks redraw after zero-size resize");
         frame->SetAutoExitAfterRun(true);
         frame->Close();
         return;
      }
      ++step;
      StartOnce(150);
   }
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      const char *mode = std::getenv("GMAT_GUI_TEST_MODE");
      GroundTrackStep(frame, mode && std::string(mode) == "groundtrack-default");
   }
};

extern "C" int __wrap_main(int argc, char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
