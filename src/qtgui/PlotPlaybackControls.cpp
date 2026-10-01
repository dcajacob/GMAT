#include "PlotPlaybackControls.hpp"
#include "QtPlotReceiver.hpp"
#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <algorithm>

PlotPlaybackControls::PlotPlaybackControls(QtPlotReceiver &plots,QWidget *parent)
   : QWidget(parent),receiver(plots)
{
   setObjectName("allPlotPlaybackControls");
   setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Preferred);
   auto *layout=new QHBoxLayout(this); layout->setContentsMargins(4,0,4,0); layout->setSpacing(3);
   layout->setAlignment(Qt::AlignLeft);
   layout->addWidget(new QLabel("Animation",this));
   auto action=[&](const QString &name,const QString &text,QStyle::StandardPixmap icon,const QString &tip) {
      auto *result=new QAction(style()->standardIcon(icon),text,this); result->setObjectName(name); result->setToolTip(tip);
      auto *button=new QToolButton(this); button->setDefaultAction(result); layout->addWidget(button);
      return result;
   };
   auto *start=action("allPlotReplayStart","Start",QStyle::SP_MediaSkipBackward,"Pause all plots and return to the start of retained history");
   play=action("allPlotReplayPlay","Play",QStyle::SP_MediaPlay,"Play or pause retained history in all Orbit, Ground Track and XY displays");
   play->setCheckable(true);
   auto *latest=action("allPlotReplayLatest","Latest",QStyle::SP_MediaSkipForward,"Stop animation and follow the latest data in all plots");
   timeline=new QSlider(Qt::Horizontal,this); timeline->setObjectName("allPlotTimeline");
   timeline->setRange(0,1000); timeline->setValue(1000); timeline->setFixedWidth(120);
   timeline->setAccessibleName("All plots playback position");
   timeline->setToolTip("Shared percentage of each display's retained history; this is not a common epoch"); layout->addWidget(timeline);
   position=new QLabel("Latest",this); position->setObjectName("allPlotReplayPosition");
   position->setMinimumWidth(position->fontMetrics().horizontalAdvance("100.0%")+4); layout->addWidget(position);
   speed=new QComboBox(this); speed->setObjectName("allPlotReplaySpeed"); speed->setAccessibleName("All plots playback speed");
   speed->setToolTip("Animation speed relative to a three-second sweep of retained history");
   for (double rate:{0.25,0.5,1.,2.,4.}) speed->addItem(QString::number(rate)+QString::fromUtf8("×"),rate);
   speed->setCurrentIndex(2); layout->addWidget(speed);
   timer=new QTimer(this); timer->setObjectName("allPlotReplayTimer"); timer->setInterval(30);
   connect(start,&QAction::triggered,this,[this] { stop(); seek(0); });
   connect(latest,&QAction::triggered,this,[this] { stop(); seek(1000); });
   connect(play,&QAction::toggled,this,[this](bool checked) {
      play->setText(checked ? "Pause" : "Play");
      play->setIcon(style()->standardIcon(checked ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
      if (checked) {
         remainder=0;
         seek(timeline->value()==1000 ? 0 : timeline->value());
         timer->start();
      } else timer->stop();
   });
   connect(timeline,&QSlider::sliderPressed,this,[this] { stop(); });
   connect(timeline,&QSlider::valueChanged,this,[this](int value) { seek(value); });
   connect(timer,&QTimer::timeout,this,[this] {
      const double advance=10*speed->currentData().toDouble()+remainder;
      const int steps=static_cast<int>(advance); remainder=advance-steps;
      seek(std::min(1000,timeline->value()+steps));
      if (timeline->value()==1000) stop();
   });
   syncFromReceiver();
}
void PlotPlaybackControls::stop()
{
   timer->stop(); play->setChecked(false);
}
void PlotPlaybackControls::seek(int value)
{
   receiver.setReplayPosition(value);
   syncFromReceiver();
}
void PlotPlaybackControls::syncFromReceiver()
{
   const bool available=receiver.hasReplayDisplays();
   setEnabled(available);
   const auto shared=receiver.replayPosition();
   if (!available || !shared) stop();
   const QSignalBlocker block(timeline);
   if (!available) timeline->setValue(1000);
   else if (shared) timeline->setValue(*shared);
   position->setText(!available || (shared && *shared==1000) ? "Latest" : shared ? QString::number(*shared/10.)+"%" : "Local");
}
