#pragma once
#include <QDialog>
#include <QMap>
#include <QString>
class GmatBase;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QLabel;
void validateGroundTrackTexture(GmatBase &plot);
class GroundTrackDialog final : public QDialog
{
public:
   GroundTrackDialog(GmatBase &plot,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QComboBox *body,*solver;
   QCheckBox *show;
   QLineEdit *texture;
   QListWidget *objects;
   QMap<QString,QSpinBox *> data;
   QLabel *error;
};
