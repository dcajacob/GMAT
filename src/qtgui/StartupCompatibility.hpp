#pragma once
#include <QString>

// Diagnose known wx window providers before the engine loads any plugins.
QString qtStartupCompatibilityError(const QString &path);
