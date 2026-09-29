#pragma once
#include <QSettings>
#include <QTemporaryDir>
#include <stdexcept>

// Keep tests out of persistent platform settings, including the Windows registry.
class TestSettings
{
public:
   TestSettings()
   {
      if (!directory.isValid()) throw std::runtime_error("Cannot create isolated test settings");
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
      QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,directory.path()+"/system");
   }
private:
   QTemporaryDir directory;
};
