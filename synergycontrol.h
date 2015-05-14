#ifndef SYNERGYCONTROL_H
#define SYNERGYCONTROL_H

#include <coreplugin/iversioncontrol.h>

namespace Synergy {
namespace Internal {

class SynergyPlugin;

class SynergyControl : public Core::IVersionControl
{
    Q_OBJECT
public:
    explicit SynergyControl(SynergyPlugin *plugin);
    QString displayName() const;
    Core::Id id() const;

    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;

    QString vcsOpenText() const;

    bool isConfigured() const;

    bool supportsOperation(Operation operation) const;
    OpenSupportMode openSupportMode(const QString &fileName) const;
    bool vcsOpen(const QString &fileName);
    SettingsFlags settingsFlags() const;
    bool vcsAdd(const QString &fileName);
    bool vcsDelete(const QString &filename);
    bool vcsMove(const QString &from, const QString &to);
    bool vcsCreateRepository(const QString &directory);
    bool vcsCheckout(const QString &directory, const QByteArray &url);
    QString vcsGetRepositoryURL(const QString &directory);

    bool vcsAnnotate(const QString &file, int line);

    QString vcsMakeWritableText() const;

    void emitRepositoryChanged(const QString &);
    void emitFilesChanged(const QStringList &);
    void emitConfigurationChanged();

private:
    SynergyPlugin *m_plugin;
};

}
}

#endif // SYNERGYCONTROL_H
