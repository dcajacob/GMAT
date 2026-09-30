#pragma once
#include <QString>
#include <QVector>

struct ScriptStatement
{
   QString code;
   QVector<qsizetype> positions,continuations;
};
// Source coordinates exclude comments and retain quoted literals/continuations.
QVector<ScriptStatement> scriptStatements(const QString &source);
