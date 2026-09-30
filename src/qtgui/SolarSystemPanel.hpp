#pragma once
#include "EditablePanel.hpp"
#include <QMap>
#include <QStringList>
#include <functional>
class SolarSystem;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QLabel;
struct SolarSystemSettings {
   QStringList sources;
   QMap<QString,QString> deFiles;
   QMap<QString,QString> values;
   QString deSource;
};
SolarSystemSettings solarSystemSettings(SolarSystem &system);
QString solarSystemScript(const QString &source,const SolarSystemSettings &initial,const QMap<QString,QString> &values,const QString &firstMissionStatement);
QString solarSystemSettingsError(SolarSystem &system,const QMap<QString,QString> &values);
class SolarSystemPanel final : public EditablePanel {
public:
   using Apply=std::function<QString(const QMap<QString,QString> &)>;
   SolarSystemPanel(SolarSystem &system,Apply apply,QWidget *parent=nullptr);
   bool hasChanges() const override;
   void discardChanges() override { applied=true; }
private:
   QMap<QString,QString> settings() const;
   SolarSystemSettings initial;
   QMap<QString,QString> deFiles;
   QString deSource;
   QComboBox *source;
   QLineEdit *deFile,*spkFile,*pckFile,*interval;
   QCheckBox *useTT;
   QLabel *status;
   bool applied=false;
};
