#pragma once
#include <QString>
#include <QStringList>
struct QtScriptConversion {
   QString script,error;
   QStringList notes;
   int plots=0;
};
QtScriptConversion convertOpenFramesViews(const QString &source);
