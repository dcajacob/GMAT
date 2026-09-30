#pragma once
#include <QStringList>
class QtMessageReceiver;
struct PathSettings
{
   QString startupFile, startupText, output;
   QStringList functions;
};
PathSettings capturePathSettings(const QString &startupFile);
PathSettings readPathSettings(const QString &path, QtMessageReceiver &receiver);
QString pathSettingsScript(const PathSettings &settings);
QString validatePathSettings(const PathSettings &settings);
QString applyPathSettings(const PathSettings &settings, QtMessageReceiver &receiver);
