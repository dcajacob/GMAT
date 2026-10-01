#pragma once
#include "PlotModel.hpp"
#include <QPainter>
#include <QFontMetricsF>

// Both native and fallback overlays use logical pixels. QPainter applies the
// device scale exactly once, including pixel-sized fonts.
inline void drawOrbitLabel(QPainter &painter,const QPointF &anchor,const PlotCurve &curve)
{
   if (curve.importedFontSize && *curve.importedFontSize==0) return;
   painter.save();
   if (curve.importedFontSize) { auto font=painter.font(); font.setPixelSize(static_cast<int>(*curve.importedFontSize)); painter.setFont(font); }
   if (curve.importedFontPosition.isEmpty()) painter.drawText(anchor+QPointF(7,-7),curve.name);
   else {
      const auto bounds=QFontMetricsF(painter.font()).boundingRect(curve.name);
      const bool left=curve.importedFontPosition.endsWith("Left"),bottom=curve.importedFontPosition.startsWith("Bottom");
      const QPointF topLeft(anchor.x()+(left ? -7-bounds.width() : 7),anchor.y()+(bottom ? 7 : -7-bounds.height()));
      painter.drawText(topLeft-bounds.topLeft(),curve.name);
   }
   painter.restore();
}
