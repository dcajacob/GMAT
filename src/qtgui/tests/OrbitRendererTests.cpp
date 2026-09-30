#include "OrbitRenderer.hpp"
#include "OrbitCamera.hpp"
#include "VisualModelDialog.hpp"
#include "PlotWidget.hpp"
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
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
      for (const double offset:{0.0,1e7}) for (const bool perspective:{false,true}) {
         auto fitModel=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
         fitModel->perspective=perspective; fitModel->fitCamera=true; fitModel->fieldOfView=50;
         fitModel->labels=false; fitModel->legend=false; fitModel->axes=false; fitModel->grid=false;
         fitModel->curves[0].radius=1; fitModel->curves[0].color=Qt::green; fitModel->append(0,offset,offset,offset);
         fitModel->curves[1].visible=false; fitModel->append(1,-1e9,0,0);
         fitModel->curves[2].lines=false; fitModel->curves[2].showObject=false; fitModel->append(2,1e9,0,0);
         OrbitRenderer viewer(fitModel); viewer.show(); viewer.setView(1,0,0,{},0);
         QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec();
         for (const auto &size:{QSize(240,600),QSize(600,240),QSize(400,400)}) {
            viewer.resize(size); app.processEvents();
            QEventLoop configured; QTimer::singleShot(50,&configured,&QEventLoop::quit); configured.exec();
            const auto image=viewer.captureImage();
            int left=image.width(),right=-1,top=image.height(),bottom=-1,count=0;
            for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
               const auto pixel=image.pixelColor(x,y);
               if (pixel.green()>40 && pixel.green()>pixel.red()*2 && pixel.green()>pixel.blue()*2) {
                  left=std::min(left,x); right=std::max(right,x); top=std::min(top,y); bottom=std::max(bottom,y); ++count;
               }
            }
            require(count>1000,"Perspective Fit body missing");
            if (std::abs((left+right)*.5-image.width()*.5)>=5 || std::abs((top+bottom)*.5-image.height()*.5)>=5)
               std::cerr<<"Fit offset="<<offset<<" perspective="<<perspective<<" requested="<<size.width()<<"x"<<size.height()
                  <<" framebuffer="<<image.width()<<"x"<<image.height()<<" bounds="<<left<<","<<right<<","<<top<<","<<bottom<<std::endl;
            require(std::abs((left+right)*.5-image.width()*.5)<5 && std::abs((top+bottom)*.5-image.height()*.5)<5,
               "Fit did not center the translated visible body");
            require(left>image.width()*.025 && right<image.width()*.975 && top>image.height()*.025 && bottom<image.height()*.975,
               "Perspective Fit clipped the body at a viewport edge");
         }
      }
      {
         PlotModel scripted(PlotModel::Kind::Orbit); scripted.scriptedCamera=true;
         scripted.cameras.push_back({0,{10,20,30},{1,2,3},{0,0,1},false});
         OrbitSceneBounds bounds; bounds.include(1e7,1e7,1e7,1);
         const auto camera=orbitCamera(scripted,0,0,0,2e7,1,&bounds);
         require((camera.target-osg::Vec3d(1,2,3)).length()<1e-12 && std::abs(camera.distance-std::sqrt(9*9+18*18+27*27))<1e-12,
            "Scene bounds changed the stored Script camera without Fit");
      }
      QTemporaryDir directory;
      {
         PlotModel model(PlotModel::Kind::Orbit); model.automaticTrajectory="Path"; model.perspective=true; model.fieldOfView=60;
         model.curves[0].name="Path"; model.append(0,100,200,300); model.append(0,110,220,330);
         model.curves[1].name="Other"; model.append(1,-1e9,1e9,0);
         for (const double aspect:{.4,1.0,2.5}) {
            const auto camera=orbitCamera(model,model.frame,0,0,1,aspect);
            const double halfAngle=std::min(std::atan(aspect/std::sqrt(3.0)),3.14159265358979323846/6);
            require((camera.target-osg::Vec3d(105,210,315)).length()<1e-10 &&
               std::abs(camera.distance-std::sqrt(350.0)/std::sin(halfAngle))<1e-9,"Automatic trajectory center or aspect-aware distance incorrect");
         }
         model.cameraViews.append({"Primary",true,60,{},"Path"});
         model.cameraViews.append({"Other",true,60,{},"Other"}); model.selectedCamera=1;
         require((orbitCamera(model,model.frame,0,0,1).target-osg::Vec3d(-1e9,1e9,0)).length()<1e-6,"Named automatic camera did not select its own trajectory");
      }
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
         QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec();
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
         QEventLoop resizedWindow; QTimer::singleShot(50,&resizedWindow,&QEventLoop::quit); resizedWindow.exec();
         const auto resized=viewer.captureImage();
         if (resized.width()!=qRound(300*viewer.devicePixelRatioF()) || resized.height()!=qRound(500*viewer.devicePixelRatioF()))
            std::cerr<<"Resize widget="<<viewer.width()<<"x"<<viewer.height()<<" ratio="<<viewer.devicePixelRatioF()<<" frame="<<resized.width()<<"x"<<resized.height()<<std::endl;
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
      {
         QMap<QString,QString> values={{"ModelFile",mesh.fileName()},{"ModelScale","1"},{"OrbitColor","Red"},{"TargetColor","Yellow"}};
         for (const auto &axis:QStringList{"X","Y","Z"}) { values["ModelRotation"+axis]="0"; values["ModelOffset"+axis]="0"; }
         VisualModelDialog preview(values,[](const auto &pending) { auto normalized=pending; normalized["@ResolvedModelFile"]=pending.value("ModelFile"); return normalized; });
         preview.show(); app.processEvents();
         QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec();
         auto *canvas=dynamic_cast<PlotCanvas *>(preview.findChild<QWidget *>("visualModelPreview")); require(canvas,"Model preview canvas missing");
         canvas->setViewAngles(0,3.14159265358979323846/2); const auto initial=canvas->captureImage(); int blue=0;
         for (int y=0;y<initial.height();++y) for (int x=0;x<initial.width();++x) { const auto c=initial.pixelColor(x,y); if (c.blue()>c.red()*2 && c.blue()>70) ++blue; }
         require(blue>1000,"Visual model dialog did not render the textured mesh");
         const bool capturePreview=argc==3 && QString::fromLocal8Bit(argv[1])=="--preview-capture";
         if (capturePreview) require(preview.grab().save(QString::fromLocal8Bit(argv[2])),"Native visual preview capture failed");
         preview.findChild<QSlider *>("visualSlider_ModelRotationY")->setValue(90);
         QEventLoop settle; QTimer::singleShot(200,&settle,&QEventLoop::quit); settle.exec();
         const auto rotated=canvas->captureImage();
         require(preview.previewModel()->curves[0].modelRotation[1]==90 && rotated!=initial,"Model rotation preview did not change native pixels");
         preview.findChild<QPushButton *>("visualShowEarth")->click(); app.processEvents();
         require(preview.previewModel()->curves[1].visible && canvas->captureImage()!=rotated,"Native Earth size reference missing");
         preview.close();
         if (capturePreview) { std::cout<<"PASS: textured visual model preview, rotation and wireframe Earth reference"<<std::endl; return 0; }
      }
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
      {
         auto automatic=std::make_shared<PlotModel>(*model); automatic->automaticBody="Ship";
         automatic->perspective=true; automatic->scriptedCamera=true;
         automatic->curves[0].name="Ship"; automatic->curves[0].modelOffset={50,20,10}; automatic->curves[0].modelRotation={90,0,0};
         automatic->cameras.push_back({0,{0,-30000,0},{0,0,0},{0,0,1},false});
         OrbitRenderer autoViewer(automatic); autoViewer.resize(400,600); autoViewer.show(); autoViewer.setView(1,0,0,{},0); app.processEvents();
         const auto framed=autoViewer.captureImage(); int pixels=0;
         for (int y=0;y<framed.height();++y) for (int x=0;x<framed.width();++x) {
            const auto c=framed.pixelColor(x,y); if (c.blue()>c.red()*2 && c.blue()>70) ++pixels;
         }
         require(pixels>5000,"Automatic body camera failed to frame offset/rotated model bounds");
      }
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
      QFile catalogFile(directory.filePath("stars.txt")); require(catalogFile.open(QIODevice::WriteOnly),"Catalog fixture failed");
      catalogFile.write("# RA Dec magnitude\n0 0 5 trailing comment\n90 0 -1\n360 90 2\ninvalid\n5 91 1\n-1 0 2\nnan 0 2\n"); catalogFile.close();
      const auto catalog=StarCatalog::read(catalogFile.fileName());
      require(catalog.error.isEmpty() && catalog.stars.size()==3 && catalog.rejectedLines==4,"Catalog validation failed");
      require(catalog.stars[0].magnitude==-1 && std::abs(catalog.stars[0].direction[1]-1)<1e-12 &&
              std::abs(catalog.stars[1].direction[2]-1)<1e-12,"Catalog sorting or equatorial direction conversion failed");
      require(!StarCatalog::read(directory.filePath("absent.txt")).error.isEmpty(),"Missing catalog not reported");
      auto sky=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      sky->labels=false; sky->legend=false; sky->axes=false; sky->grid=false;
      sky->starCatalog.stars={{{0,0,-1},-1},{{.55,0,-1},0},{{0,0,1},1}};
      sky->starCount=2; sky->curves[0].radius=1; sky->curves[0].color=Qt::green; sky->append(0,0,0,0);
      OrbitRenderer starViewer(sky); starViewer.resize(640,480); starViewer.show(); starViewer.setView(1,0,0,{},0); app.processEvents();
      const auto noStars=starViewer.captureImage();
      sky->starsEnabled=true; sky->starCount=1;
      require(starViewer.captureImage()==noStars,"Star behind planet leaked through its surface");
      sky->starCount=2;
      const auto withStars=starViewer.captureImage(); require(withStars!=noStars,"Visible catalog star was not rendered");
      sky->curves[0].visible=false;
      const auto skyOnly=starViewer.captureImage();
      sky->starCount=3; require(starViewer.captureImage()==skyOnly,"Rear-hemisphere star was rendered");
      sky->perspective=true;
      const auto perspectiveSky=starViewer.captureImage();
      starViewer.setView(1,0,0,{100,-30},0);
      require(starViewer.captureImage()==perspectiveSky,"Perspective stars moved with translation");
      starViewer.setView(2,0,0,{},0);
      require(starViewer.captureImage()!=perspectiveSky,"Perspective zoom did not change celestial angular framing");
      sky->perspective=false;
      starViewer.setView(5,0,0,{100,-30},0);
      require(starViewer.captureImage()==skyOnly,"Stars moved with pan or orthographic zoom");
      starViewer.setView(1,.5,.3,{},0);
      require(starViewer.captureImage()!=skyOnly,"Stars did not rotate with camera");
      sky->curves[0].points[0].inertialToView={0,-1,0,1,0,0,0,0,1};
      starViewer.setView(1,0,0,{},0); const auto rotatedSky=starViewer.captureImage();
      require(rotatedSky!=skyOnly,"Stars ignored the plot coordinate frame");
      auto oldFrame=sky->curves[0].points[0]; oldFrame.frame=1; oldFrame.inertialToView={1,0,0,0,1,0,0,0,1};
      sky->curves[0].points.push_back(oldFrame); starViewer.setView(1,0,0,{},1);
      require(starViewer.captureImage()==skyOnly,"Star replay used the wrong frame orientation");
      sky->starCatalog.stars[1].magnitude=8;
      const auto dimStars=starViewer.captureImage();
      auto starBrightness=[](const QImage &image) {
         int maximum=0; const double scale=image.devicePixelRatio();
         for (int y=qRound(236*scale);y<qRound(244*scale);++y)
            for (int x=qRound(600*scale);x<qRound(608*scale);++x) maximum=std::max(maximum,qGray(image.pixel(x,y)));
         return maximum;
      };
      require(starBrightness(skyOnly)>starBrightness(dimStars)+30,"Magnitude did not reduce star brightness");
      sky->starsEnabled=false; const auto disabledSky=starViewer.captureImage();
      require(disabledSky!=skyOnly,"Disabling stars had no effect");
      sky->starsEnabled=true; sky->starCount=0;
      require(starViewer.captureImage()==disabledSky,"Zero star count still rendered stars");
      QFile constellationFile(directory.filePath("constellations.txt"));
      require(constellationFile.open(QIODevice::WriteOnly),"Constellation fixture failed");
      constellationFile.write("# declination and RA hours\nN Test\n0 0 90 6\n-91 0 0 1\n0 25 0 1\ninvalid\n"); constellationFile.close();
      const auto constellation=ConstellationCatalog::read(constellationFile.fileName());
      require(constellation.error.isEmpty() && constellation.segments.size()==1 && constellation.rejectedLines==3 &&
         constellation.segments[0].name=="Test" && std::abs(constellation.segments[0].first[0]-1)<1e-12 &&
         std::abs(constellation.segments[0].second[2]-1)<1e-12,"Constellation units or validation incorrect");
      sky->constellationCatalog.segments={{"Test",{-.5,.2,-1},{.5,.2,-1}}};
      sky->constellationsEnabled=true;
      const auto outlines=starViewer.captureImage(); require(outlines!=disabledSky,"Constellation line not rendered");
      starViewer.setView(5,0,0,{100,-30},1);
      require(starViewer.captureImage()==outlines,"Constellations moved under pan/zoom");
      starViewer.setView(1,0,0,{},1);
      sky->constellationCatalog.segments={{"Rear",{-.5,.2,1},{.5,.2,1}}};
      require(starViewer.captureImage()==disabledSky,"Rear constellation line leaked into view");
      sky->constellationsEnabled=false; sky->xyPlane=true;
      const auto xyPlane=starViewer.captureImage(); require(xyPlane!=disabledSky,"XY plane not rendered");
      sky->xyPlane=false; sky->eclipticPlane=true;
      require(starViewer.captureImage()!=xyPlane,"Ecliptic plane did not differ from XY plane");
      sky->eclipticPlane=false; sky->sunLine=true;
      sky->curves[0].points.back().hasSun=true; sky->curves[0].points.back().sunPosition={100,0,0};
      require(starViewer.captureImage()!=disabledSky,"Sun direction line not rendered");
      sky->sunLine=false; sky->curves[0].visible=true;
      const auto solidBody=starViewer.captureImage(); sky->wireframe=true;
      require(starViewer.captureImage()!=solidBody,"Wireframe ignored");
      sky->wireframe=false; require(starViewer.captureImage()==solidBody,"Wireframe could not be disabled");
      auto tracking=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      tracking->labels=false; tracking->legend=false; tracking->axes=false; tracking->grid=false; tracking->scriptedCamera=true;
      tracking->curves[0].radius=1; tracking->curves[0].color=Qt::green; tracking->curves[0].lines=false;
      tracking->curves[1].color=Qt::red; tracking->curves[1].lines=false;
      tracking->frame=1; tracking->append(0,10,0,0); tracking->append(1,11.5,0,0);
      tracking->cameras.push_back({1,{10,0,6},{10,0,0},{0,1,0}});
      tracking->frame=2; tracking->append(0,20,0,0); tracking->append(1,21.5,0,0);
      tracking->cameras.push_back({2,{20,0,6},{20,0,0},{0,-1,0}});
      OrbitRenderer trackedViewer(tracking); trackedViewer.resize(640,480); trackedViewer.show();
      trackedViewer.setView(1,0,0,{},1); app.processEvents();
      const auto firstCamera=trackedViewer.captureImage();
      auto redX=[](const QImage &image) {
         double sum=0,count=0;
         for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
            const auto c=image.pixelColor(x,y);
            if (c.red()>200 && c.green()<60) { sum+=x; ++count; }
         }
         require(count>0,"Tracked marker missing"); return sum/count;
      };
      require(redX(firstCamera)>firstCamera.width()/2,"Script camera did not center on tracked target");
      trackedViewer.setView(1,0,0,{},2); const auto secondCamera=trackedViewer.captureImage();
      require(redX(secondCamera)<secondCamera.width()/2,"Script up direction or replay tracking failed");
      auto greenCount=[](const QImage &image) {
         int count=0;
         for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
            const auto c=image.pixelColor(x,y); if (c.green()>80 && c.green()>c.red()*2) ++count;
         }
         return count;
      };
      require(greenCount(secondCamera)>1000,"Tracked body missing");
      tracking->cameras.back().eye[2]=12;
      require(greenCount(trackedViewer.captureImage())<greenCount(secondCamera)/2,"Script camera scale ignored");
      trackedViewer.setView(2,0,0,{},2);
      require(greenCount(trackedViewer.captureImage())>greenCount(secondCamera)*.9,"Manual zoom no longer works with tracking");
      trackedViewer.setView(1,0,0,{},1);
      require(trackedViewer.captureImage()==firstCamera,"Replay did not restore camera snapshot");
      tracking->perspective=true; tracking->fieldOfView=50;
      const auto perspective=trackedViewer.captureImage();
      require(greenCount(perspective)>1000,"Perspective body missing");
      tracking->fieldOfView=90;
      require(greenCount(trackedViewer.captureImage())<greenCount(perspective)/2,"Perspective FOV did not change angular framing");
      tracking->fieldOfView=50;
      tracking->curves[0].points.front().z=2;
      require(greenCount(trackedViewer.captureImage())>greenCount(perspective)*1.5,"Perspective did not enlarge a nearer object");
      tracking->curves[0].points.front().z=8;
      require(greenCount(trackedViewer.captureImage())==0,"Body behind perspective camera was not clipped");
      tracking->curves[0].points.front().z=0;
      tracking->perspective=false;
      require(trackedViewer.captureImage()==firstCamera,"Switching projection failed to restore orthographic view");
      auto bodyMesh=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      bodyMesh->labels=false; bodyMesh->legend=false; bodyMesh->axes=false; bodyMesh->grid=false; bodyMesh->scriptedCamera=true;
      bodyMesh->curves[0].radius=1; bodyMesh->curves[0].modelPath=mesh.fileName(); bodyMesh->curves[0].color=Qt::green;
      bodyMesh->append(0,0,0,0); bodyMesh->cameras.push_back({0,{0,0,6},{0,0,0},{0,1,0}});
      OrbitRenderer bodyViewer(bodyMesh); bodyViewer.resize(640,480); bodyViewer.show(); bodyViewer.setView(1,0,0,{},0); app.processEvents();
      const auto physicalMesh=bodyViewer.captureImage();
      const int cx=physicalMesh.width()/2,cy=physicalMesh.height()/2;
      const auto outside=physicalMesh.pixelColor(cx+qRound(180*bodyViewer.devicePixelRatioF()),cy);
      const auto corner=physicalMesh.pixelColor(cx+qRound(70*bodyViewer.devicePixelRatioF()),cy+qRound(70*bodyViewer.devicePixelRatioF()));
      require(outside.red()<30 && outside.blue()<30,"Celestial model was exaggerated like a spacecraft");
      require(corner.red()>60 || corner.blue()>60,"Celestial model was normalized or replaced by a sphere");
      bodyMesh->curves[0].modelPath=directory.filePath("missing-body.3ds");
      require(greenCount(bodyViewer.captureImage())>1000,"Missing celestial mesh did not fall back to a sphere");
      std::cout << "Native texture, depth, rotation, resize, model material, Sun illumination, star catalog/camera/replay/occlusion, fallback and lifecycle checks passed\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
