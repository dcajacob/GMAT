#pragma once
#include "EditablePanel.hpp"
#include <QMap>
#include <QStringList>
#include <functional>
class QPlainTextEdit;
class CommandEditor final : public EditablePanel
{
public:
   using Apply=std::function<QString(const QString &)>;
   CommandEditor(const QString &statement, bool inserting, const QMap<QString,QString> &templates,
                 Apply apply, const QStringList &propagators={}, const QStringList &spacecraft={}, QWidget *parent=nullptr, const QStringList &formations={}, std::function<void()> summary={});
   bool hasChanges() const override;
   void discardChanges() override { applied=true; }
private:
   QPlainTextEdit *source;
   QString original;
   bool inserting=false, applied=false;
};
