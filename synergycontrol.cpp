#include "synergycontrol.h"
#include "synergyplugin.h"
#include "synergyconstants.h"

#include <vcsbase/vcsbaseconstants.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/fileutils.h>

#include <QFileInfo>

using namespace Synergy;
using namespace Synergy::Internal;

SynergyControl::SynergyControl(SynergyPlugin *plugin) :
    m_plugin(plugin)
{
}

QString SynergyControl::displayName() const
{
    return QLatin1String("Synergy");
}

Core::Id SynergyControl::id() const
{
    return Constants::VCS_ID_SYNERGY;
}

bool SynergyControl::isConfigured() const
{
    if (m_plugin->settings().m_synergyBinaryPath.isEmpty()  ||
        m_plugin->settings().m_password.isEmpty()           ||
        m_plugin->settings().m_uid.isEmpty()                ||
        m_plugin->settings().m_compareToolPath.isEmpty())
        return false;

    QFileInfo fi(m_plugin->settings().m_synergyBinaryPath);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool SynergyControl::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case CheckoutOperation:
    case GetRepositoryRootOperation:
        break;
    case MoveOperation:
    case AnnotateOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

Core::IVersionControl::OpenSupportMode SynergyControl::openSupportMode(const QString &fileName) const
{
    return IVersionControl::OpenOptional;
}

bool SynergyControl::vcsOpen(const QString &fileName)
{
    qDebug() << "*** perms" << QFile::permissions(fileName);
    QFileInfo fi(fileName);
    Utils::FileUtils::makeWritable(Utils::FileName(fi));
    Core::EditorManager::instance()->saveDocument();

    return m_plugin->vcsOpen(fileName);
}

Core::IVersionControl::SettingsFlags SynergyControl::settingsFlags() const
{
    SettingsFlags rc;
    rc = 0;
//    if (m_plugin->settings().autoCheckOut)
//        rc|= AutoOpen;
    return rc;
}

bool SynergyControl::vcsAdd(const QString &fileName)
{
    return m_plugin->vcsAdd(fileName);
}

bool SynergyControl::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_plugin->vcsDelete(fi.absolutePath(), fi.fileName(), false);
}

bool SynergyControl::vcsMove(const QString &from, const QString &to)
{
    return false;//m_plugin->vcsMove(ifrom.absolutePath(), ifrom.fileName(), ito.fileName());
}

QString SynergyControl::vcsGetRepositoryURL(const QString &directory)
{
    return QString();//m_plugin->vcsGetRepositoryURL(directory);
}

bool SynergyControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    return m_plugin->managesDirectory(directory, topLevel);
}

bool SynergyControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_plugin->managesFile(workingDirectory, fileName);
}

bool SynergyControl::vcsAnnotate(const QString &file, int line)
{
    //const QFileInfo fi(file);
    //m_plugin->vcsAnnotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return false;
}

QString SynergyControl::vcsOpenText() const
{
    return tr("Check &Out");
}

QString SynergyControl::vcsMakeWritableText() const
{
    return tr("&Make Writable");
}

void SynergyControl::emitRepositoryChanged(const QString &s)
{
    emit repositoryChanged(s);
}

void SynergyControl::emitFilesChanged(const QStringList &l)
{
    emit filesChanged(l);
}

void SynergyControl::emitConfigurationChanged()
{
    emit configurationChanged();
}

bool SynergyControl::vcsCheckout(const QString & /*directory*/, const QByteArray & /*url*/)
{
    return false;
}

bool SynergyControl::vcsCreateRepository(const QString &)
{
    return false;
}
