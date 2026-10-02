#pragma once
#include <QString>
class GmatBase;
bool resourceCanBeRenamed(GmatBase *object);
// Patch only the current document. Literal strings, comments and implicit
// settings are retained; external defining/reference documents are explicit.
QString renameResourceSource(const QString &source,GmatBase &object,const QString &name);
