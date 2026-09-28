// Shared setup for tests linked against the real GMAT GUI objects.
#pragma once
#include <wx/app.h>
#include <wx/display.h>
#include <wx/dcscreen.h>
#include <wx/dcmemory.h>
#include <wx/timer.h>
#include "GmatAppData.hpp"
#include "GmatMainFrame.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

static int failures = 0;
static void Check(bool condition, const char *message)
{
   std::printf("%s: %s\n", condition ? "PASS" : "FAIL", message);
   if (!condition) ++failures;
   std::fflush(stdout);
}

static void Screenshot(GmatMainFrame *frame, const char *name)
{
   const char *directory = std::getenv("GMAT_GUI_TEST_IMAGES");
   if (!directory || std::getenv("GMAT_GUI_TEST_NO_SCREENSHOT")) return;
   wxScreenDC screen;
   const wxSize size = wxDisplay(0u).GetGeometry().GetSize();
   wxBitmap bitmap(size.x,size.y);
   wxMemoryDC dc(bitmap);
   dc.Blit(0,0,size.x,size.y,&screen,0,0);
   dc.SelectObject(wxNullBitmap);
   bitmap.SaveFile(wxString::FromUTF8(directory)+"/"+name+".png",wxBITMAP_TYPE_PNG);
}

template<class Test> int RunGuiTest(int argc, char **argv)
{
   std::setvbuf(stdout, nullptr, _IONBF, 0);
   if (!wxEntryStart(argc,argv) || !wxTheApp->CallOnInit()) return 2;
   Test test;
   test.StartOnce(300);
   wxTheApp->OnRun();
   // GMAT may exit() inside OnExit, so check failures before calling it.
   if (failures) return 1;
   wxTheApp->OnExit();
   wxEntryCleanup();
   return 0;
}
