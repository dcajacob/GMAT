#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QPainter>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      QTemporaryDir directory;
      QImage texture(128,64,QImage::Format_RGB32);
      texture.fill(QColor(20,110,230));
      { QPainter p(&texture); p.fillRect(64,0,64,64,QColor(240,230,170)); }
      const auto path=directory.filePath("texture.png");
      require(texture.save(path),"Texture fixture failed");
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      model->labels=false; model->legend=false; model->axes=false; model->grid=false;
      auto &body=model->curves[0]; body.radius=1; body.texturePath=path; body.color=Qt::green;
      model->append(0,0,0,0);
      auto &marker=model->curves[1]; marker.color=Qt::red; marker.lines=false;
      model->append(1,0,0,1.1);
      for (int iteration=0;iteration<3;++iteration) {
         OrbitRenderer viewer(model);
         viewer.resize(640,480); viewer.show(); viewer.setView(1,0,0,{},0);
         app.processEvents();
         require(viewer.isValid(),"OpenGL context unavailable");
         const auto front=viewer.captureImage();
         require(!front.isNull(),"Framebuffer capture failed");
         auto center=front.pixelColor(front.width()/2,front.height()/2);
         require(center.red()>200 && center.green()<60,"Foreground marker missing");
         marker.points.back().z=-1.1; viewer.update(); app.processEvents();
         const auto behind=viewer.captureImage();
         center=behind.pixelColor(behind.width()/2,behind.height()/2);
         require(!(center.red()>200 && center.green()<60),"Body did not occlude far-side marker");
         int blue=0,sand=0;
         for (int y=0;y<behind.height();++y) for (int x=0;x<behind.width();++x) {
            const auto c=behind.pixelColor(x,y);
            if (c.blue()>c.red()*2 && c.blue()>70) ++blue;
            if (c.red()>100 && c.green()>100 && c.blue()<c.red()*.85) ++sand;
         }
         require(blue>1000 && sand>1000,"Texture colors not rendered");
         body.points.back().bodyToView={0,-1,0,1,0,0,0,0,1};
         const auto rotated=viewer.captureImage();
         require(rotated!=behind,"Body rotation did not rotate texture");
         viewer.resize(300,500); app.processEvents();
         const auto resized=viewer.captureImage();
         require(resized.width()==qRound(300*viewer.devicePixelRatioF()) && resized.height()==qRound(500*viewer.devicePixelRatioF()),"Resized framebuffer dimensions wrong");
         body.texturePath=directory.filePath("missing.png");
         const auto fallback=viewer.captureImage();
         center=fallback.pixelColor(fallback.width()/2,fallback.height()/2);
         require(center.green()>center.red()*2 && center.green()>center.blue()*2,"Missing texture fallback failed");
         body.texturePath=path; body.points.back().bodyToView={1,0,0,0,1,0,0,0,1}; marker.points.back().z=1.1;
      }
      std::cout << "Native texture, depth, rotation, resize, fallback and repeated lifecycle checks passed\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
