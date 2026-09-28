#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include "ModelObject.hpp"
#include <GL/gl.h>
#include <vector>
#include <cstring>
#include <fstream>
#include <iterator>
template<class T>T* Find(wxWindow*w){if(auto p=dynamic_cast<T*>(w))return p;for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;return nullptr;}
class Access:public OrbitViewCanvas{public:static bool Load(ViewCanvas &c,const std::string &p){return (c.*(&Access::LoadImage))(p,0);}};
static void SetUnpack(int alignment){glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,41);glPixelStorei(GL_UNPACK_SKIP_ROWS,2);glPixelStorei(GL_UNPACK_SKIP_PIXELS,3);}
static void CheckUnpack(int alignment){GLint a,r,y,x;glGetIntegerv(GL_UNPACK_ALIGNMENT,&a);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&r);glGetIntegerv(GL_UNPACK_SKIP_ROWS,&y);glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&x);Check(a==alignment && r==41 && y==2 && x==3,"texture loader preserves all caller row-layout settings");}
class Regression:public wxTimer{
public:void Notify()override{
 auto frame=GmatAppData::Instance()->GetMainFrame();Check(frame->RunCurrentMission()==1,"native mission prepares texture context");auto canvas=Find<OrbitViewCanvas>(frame);Check(canvas && canvas->SetGLContext(),"texture test has current context");
 std::string dir=std::getenv("GMAT_GUI_TEST_IMAGES");
 if(canvas){
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_PACK_ALIGNMENT,1);
  for(int width:{17,32})for(const char *ext:{"png","bmp","jpg"}){
   wxImage image(width,19);for(int y=0;y<19;++y)for(int x=0;x<width;++x)image.SetRGB(x,y,(x*13+y*7)%256,(y*17+x*3)%256,(x*5+y*11)%256);
   std::string path=dir+"/texture."+ext;image.SaveFile(wxString::FromUTF8(path));wxImage decoded(wxString::FromUTF8(path));
   // The fixture model's diffuse maps all name aura_map.jpg. The bytes may be
   // PNG/BMP/JPEG; wxImage autodetection in the view path uses the file extension.
   std::ifstream source(dir+"/source.3ds",std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(source)),{});
   std::string texture=std::string("textureX.")+ext;size_t at=0;while((at=bytes.find("aura_map.jpg",at))!=std::string::npos){bytes.replace(at,12,texture);at+=12;}
   {std::ofstream model(dir+"/model.3ds",std::ios::binary);model.write(bytes.data(),bytes.size());}
   decoded.SaveFile(wxString::FromUTF8(dir+"/"+texture));wxImage modelDecoded(wxString::FromUTF8(dir+"/"+texture));
   for(int alignment:{1,4,8}){
    GLuint id;glGenTextures(1,&id);glBindTexture(GL_TEXTURE_2D,id);SetUnpack(alignment);
    Check(Access::Load(*canvas,path),"native plot uploads decoded image");
    GLint after;glGetIntegerv(GL_UNPACK_ALIGNMENT,&after);CheckUnpack(alignment);
    std::vector<unsigned char> pixels(width*19*3);glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());auto mirror=decoded.Mirror(false);
    std::printf("CASE: %dx19 %s alignment=%d\n",width,ext,alignment);
    Check(std::memcmp(pixels.data(),mirror.GetData(),pixels.size())==0,"plot texture preserves every decoded RGB row");glDeleteTextures(1,&id);
    SetUnpack(alignment);ModelObject model;Check(model.Load(dir+"/model.3ds")!=0,"native model loader accepts textured fixture");
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&after);CheckUnpack(alignment);
    GLint actualWidth=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&actualWidth);Check(actualWidth==width,"model texture has requested width");
    if(actualWidth==width){glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());Check(std::memcmp(pixels.data(),modelDecoded.GetData(),pixels.size())==0,"model texture preserves every decoded RGB row");}
   }
  }
  glPopClientAttrib();
 }
 frame->SetAutoExitAfterRun(true);frame->Close();
}};
extern "C" int __wrap_main(int argc,char**argv){return RunGuiTest<Regression>(argc,argv);}
