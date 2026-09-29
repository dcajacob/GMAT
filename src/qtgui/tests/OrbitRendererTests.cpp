#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QPainter>
#include <QFile>
#include <QScreen>
#include <QEventLoop>
#include <QTimer>
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
      // A real model reader, material and image callback, independent of GMAT assets.
      QFile mesh(directory.filePath("panel.obj")); require(mesh.open(QIODevice::WriteOnly),"Model fixture failed");
      mesh.write("mtllib panel.mtl\nv -1 -1 0\nv 1 -1 0\nv 1 1 0\nv -1 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 1\nusemtl panel\nf 1/1/1 2/2/1 3/3/1 4/4/1\n"); mesh.close();
      QFile material(directory.filePath("panel.mtl")); require(material.open(QIODevice::WriteOnly),"Material fixture failed");
      material.write("newmtl panel\nKa 0.2 0.2 0.2\nKd 1 1 1\nmap_Kd texture.png\n"); material.close();
      model->curves.clear(); auto &spacecraft=model->curves[0];
      spacecraft.modelPath=mesh.fileName(); spacecraft.color=Qt::red;
      model->append(0,0,0,0);
      OrbitRenderer viewer(model); viewer.resize(640,480); viewer.show(); viewer.setView(1,0,0,{},0); app.processEvents();
      const auto modelImage=viewer.captureImage();
      int textured=0;
      for (int y=0;y<modelImage.height();++y) for (int x=0;x<modelImage.width();++x) {
         const auto c=modelImage.pixelColor(x,y); if (c.blue()>c.red()*2 && c.blue()>70) ++textured;
      }
      require(textured>5000,"Textured spacecraft mesh not rendered");
      spacecraft.points.back().hasSun=true; spacecraft.points.back().sunPosition={0,0,1e8};
      const auto lit=viewer.captureImage();
      spacecraft.points.back().sunPosition={0,0,-1e8};
      const auto unlit=viewer.captureImage();
      const auto sample=QPoint(lit.width()/2+30,lit.height()/2);
      require(qGray(lit.pixel(sample))>qGray(unlit.pixel(sample))+40,"Sun position did not control illumination");
      spacecraft.points.front().sunPosition={0,0,1e8};
      auto later=spacecraft.points.front(); later.frame=1; later.sunPosition={0,0,-1e8};
      later.bodyToView={0,-1,0,1,0,0,0,0,1};
      spacecraft.points.push_back(later);
      viewer.setView(1,0,0,{},0); const auto replayStart=viewer.captureImage();
      viewer.setView(1,0,0,{},1); const auto replayEnd=viewer.captureImage();
      require(qGray(replayStart.pixel(sample))>qGray(replayEnd.pixel(sample))+40 && spacecraft.points.size()==2,
              "Replay did not select the recorded lighting/orientation without altering history");
      spacecraft.points.pop_back(); spacecraft.points.front().sunPosition={0,0,-1e8};
      viewer.setView(1,0,0,{},0);
      model->sunlight=false;
      const auto cameraLit=viewer.captureImage();
      require(qGray(cameraLit.pixel(sample))>qGray(unlit.pixel(sample))+40,"Camera lighting option did not restore visibility");
      model->coordinates="EarthMJ2000Eq";
      viewer.update(); app.processEvents();
      const auto exported=viewer.captureImage();
      QEventLoop settle; QTimer::singleShot(50,&settle,&QEventLoop::quit); settle.exec();
      const auto screen=viewer.screen()->grabWindow(viewer.winId()).toImage();
      auto captionPixels=[](const QImage &image) {
         int count=0;
         for (int y=image.height()-qRound(28*image.devicePixelRatio());y<image.height();++y)
            for (int x=0;x<image.width();++x) if (qGray(image.pixel(x,y))>150) ++count;
         return count;
      };
      const int expectedCaption=captionPixels(exported),actualCaption=captionPixels(screen);

      require(expectedCaption>100 && std::abs(actualCaption-expectedCaption)<expectedCaption/4,
              "Qt overlay text differs from exported text after native model rendering");
      spacecraft.modelPath=directory.filePath("missing.3ds"); spacecraft.points.back().hasSun=false;
      const auto missingModel=viewer.captureImage();
      const auto fallback=missingModel.pixelColor(missingModel.width()/2,missingModel.height()/2);
      require(fallback.red()>200 && fallback.green()<60,"Missing model did not fall back to marker");
      std::cout << "Native texture, depth, rotation, resize, model material, Sun illumination, fallback and repeated lifecycle checks passed\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
