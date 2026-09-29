#pragma once
#include "PlotModel.hpp"
#include <QWidget>
#include <memory>
class QAction;
class QSlider;
class QTimer;
class OrbitRenderer;
class PlotCanvas final : public QWidget
{
public:
   explicit PlotCanvas(std::shared_ptr<PlotModel> data, QWidget *parent = nullptr);
   void fit();
   void scriptView();
   void setFrame(quint64 value);
   double zoomFactor() const { return zoom; }
   void zoomBy(double wheelSteps);
   void setViewAngles(double azimuth, double elevation);
   void refresh();
   QImage captureImage();
protected:
   void resizeEvent(QResizeEvent *) override;
   void paintEvent(QPaintEvent *) override;
   void mousePressEvent(QMouseEvent *) override;
   void mouseMoveEvent(QMouseEvent *) override;
   void mouseDoubleClickEvent(QMouseEvent *) override;
   void wheelEvent(QWheelEvent *) override;
private:
   std::shared_ptr<PlotModel> data;
   QPointF lastMouse, pan;
   double zoom = 1, yaw = 0.55, pitch = 0.45;
   quint64 visibleFrame = std::numeric_limits<quint64>::max();
   OrbitRenderer *renderer=nullptr;
};
class PlotWidget final : public QWidget
{
public:
   explicit PlotWidget(std::shared_ptr<PlotModel> data, QWidget *parent = nullptr);
   void refresh();
   PlotCanvas *canvas() const { return drawing; }
private:
   std::shared_ptr<PlotModel> data;
   PlotCanvas *drawing;
   QSlider *timeline;
   QTimer *timer;
   QAction *replay=nullptr;
   quint64 historyGeneration=0;
   void updateReplayFrame();
   void editPlotStyle();
};
