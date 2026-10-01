#pragma once
#include <QDialog>
#include <QStringList>
class QListWidget;
class QComboBox;
// Keeps a local ordered selection. Engine validation happens on command Apply.
class ReportParameterDialog final : public QDialog
{
public:
   enum class Mode { Multiple, Single, Writable, WritableReal, StopParameter, FunctionInputs, FunctionOutputs, PlottableSingle, PlottableMultiple, NumericSingle };
   ReportParameterDialog(const QStringList &selected,QWidget *parent=nullptr,Mode mode=Mode::Multiple);
   QStringList selection() const;
   static bool isPlottableReference(const QString &reference);
private:
   QListWidget *list;
   QComboBox *singleEntry=nullptr;
};
