#ifndef ASYNCWORKER_H
#define ASYNCWORKER_H

#include "synergyplugin.h"

#include <QObject>

namespace Synergy {
namespace Internal {

class AsyncWorker : public QObject
{
    Q_OBJECT
public:
    explicit AsyncWorker(const QString &workingDir, const QStringList &args, unsigned flags, QObject *parent = 0);
    ~AsyncWorker();

signals:
    void resultDownloaded(SynergyResponse result);
    void error(QString err);

public slots:
    void process();

private:
    QString m_workingDir;
    QStringList m_arguments;
    unsigned m_flags;
};

}
}

#endif // ASYNCWORKER_H
