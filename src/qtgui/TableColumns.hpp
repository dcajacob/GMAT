#pragma once
#include <QHeaderView>
#include <QTableWidget>
#include <QVariant>
#include <algorithm>

// Content-sized defaults, without locking header handles or overwriting a
// user's widths when a live table publishes another value.
inline void configureTableColumns(QTableWidget *table, const QList<int> &minimumCharacters = {})
{
   auto *header=table->horizontalHeader();
   header->setSectionResizeMode(QHeaderView::Interactive);
   header->setStretchLastSection(false);
   header->setMinimumSectionSize(40);
   header->setToolTip("Drag column borders to resize. Double-click a border to fit its contents.");
   QVariantList minimums;
   for (int characters:minimumCharacters) minimums.append(characters);
   table->setProperty("columnMinimumCharacters",minimums);
   QObject::connect(header,&QHeaderView::sectionResized,table,[table](int column,int,int) {
      if (table->property("sizingColumns").toBool()) return;
      auto manual=table->property("manualColumnWidths").toMap();
      manual.insert(QString::number(column),true);
      table->setProperty("manualColumnWidths",manual);
   });
}

inline void fitTableColumns(QTableWidget *table, int fillColumn = -1)
{
   const auto manual=table->property("manualColumnWidths").toMap();
   auto automatic=table->property("automaticColumnWidths").toMap();
   const auto minimums=table->property("columnMinimumCharacters").toList();
   const int character=table->fontMetrics().horizontalAdvance(QLatin1Char('0'));
   table->setProperty("sizingColumns",true);
   for (int column=0;column<table->columnCount();++column) {
      const auto key=QString::number(column);
      if (manual.contains(key)) continue;
      const int minimum=std::max(48,character*(column<minimums.size() ? minimums[column].toInt() : 12)+16);
      table->resizeColumnToContents(column);
      const int desired=std::clamp(table->columnWidth(column)+12,minimum,std::max(minimum,character*48));
      // Grow with new labels/data, but do not jitter as live numbers shorten.
      const int width=std::max(desired,automatic.value(key).toInt());
      table->setColumnWidth(column,width);
      automatic.insert(key,width);
   }
   if (fillColumn>=0 && fillColumn<table->columnCount() && !manual.contains(QString::number(fillColumn))) {
      const int spare=table->viewport()->width()-table->horizontalHeader()->length();
      if (spare>0) table->setColumnWidth(fillColumn,table->columnWidth(fillColumn)+spare);
   }
   table->setProperty("automaticColumnWidths",automatic);
   table->setProperty("sizingColumns",false);
}
