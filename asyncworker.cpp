#include "asyncworker.h"
#include "synergyplugin.h"

using namespace Synergy;
using namespace Synergy::Internal;

AsyncWorker::AsyncWorker(const QString &workingDir, const QStringList &args, unsigned flags, QObject *parent) :
    QObject(parent),
    m_workingDir(workingDir),
    m_arguments(args),
    m_flags(flags)
{
}

AsyncWorker::~AsyncWorker()
{
}

void AsyncWorker::process()
{
    SynergyResponse response = SynergyPlugin::instance()->runSynergy(m_workingDir, m_arguments, m_flags);
    emit resultDownloaded(response);
}
