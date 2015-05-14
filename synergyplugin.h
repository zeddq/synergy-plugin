#ifndef SYNERGYPLUGIN_H
#define SYNERGYPLUGIN_H

#include "synergy_global.h"
#include "synergysettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <extensionsystem/iplugin.h>
#include <vcsbase/vcsbaseplugin.h>
#include <utils/synchronousprocess.h>
#include <utils/macroexpander.h>

#include <QFile>
#include <QMutexLocker>
#include <QProgressDialog>
#include <QMap>
#include <QFutureInterface>
#include <QtConcurrent>

namespace Synergy {
namespace Internal {

class AsyncWorker;
class SynergyControl;
class FilesToSyncDialog;

enum DialogAction {Checkout, Add, Show, Indicate};

typedef QPair<QAction *, Core::Command* > ActionCommandPair;
typedef QPair<AsyncWorker *, QThread* > WorkerThreadPair;
typedef QPair<QString, DialogAction > StringActionPair;
typedef QList<StringActionPair > ObjectActionList;
typedef QMap<QString, QString> JunctionMap;

class SynergyResponse
{
public:
    SynergyResponse() : error(false) {}
    bool error;
    QString stdOut;
    QString stdErr;
    QString message;
        int exitCode;
};

class FileStatus
{
public:
    enum Status
    {
        Unknown    = 0x0f,
        Integrate  = 0x01,
        CheckedOut = 0x02,
        Working    = 0x04,
        NotManaged = 0x08,
        Missing    = 0x10
    } status;

    QFile::Permissions permissions;

    FileStatus(Status _status = Unknown, QFile::Permissions perm = 0)
        : status(_status), permissions(perm)
    {}
};

class SynergyPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SynergyPlugin.json")

public:
    SynergyPlugin();
    ~SynergyPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
    bool submitEditorAboutToClose();
    void updateActions(VcsBase::VcsBasePlugin::ActionState as);

    bool vcsOpen(const QString &fileName);
    bool vcsAdd(const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName, bool replace);
    bool managesFile(const QString &workingDirectory, const QString &fileName);
    bool managesDirectory(const QString &dir, QString *topLevel /* = 0 */);

    bool isSynergyStarted() const;
    bool isSynergyStarting() const;
    static SynergyPlugin *instance();
    SynergyResponse runSynergy(const QString &workingDir, const QStringList &arguments,
                                             unsigned flags = 0, QTextCodec *outputCodec = 0);
    WorkerThreadPair runSynergyAsync(const QString &workingDir, const QStringList &arguments, unsigned flags = 0) const;
    const SynergySettings settings() const;
    void setSettings(const SynergySettings &s);
    bool checkAndStartSynergy();
    WorkerThreadPair startSynergyAsync(QString workingDir);
    QString getCurrentFileReparsed();

    WorkerThreadPair vcsDeleteAsync(const QString &workingDir, const QString &fileName, bool replace);
    void diffWithPredecessor(QString fileName, QString workingDir);
    void diffWithCurrentVersion(QString fileName, QString workingDir);

signals:
    void taskListDownloaded(QStringList list);

public slots:
    void setConnectionDialogVisibilitySlot(bool visible);
    void runSynergyShellSlot();
    void showTasks();
    void synergyStartResultDownloaded(SynergyResponse response);
    void addFilesToTask(ObjectActionList &filesList);
    void addFilesToDialog(QString &filePath, DialogAction dialogAction);
    void addFilesToDialog(QStringList &filesList, DialogAction dialogAction);

private slots:
    void verifyProjectRoot();
    void lsResultDownloaded(SynergyResponse response);
    void startCITask();
    void startDiff();
    void startFPG();
    void hsProjectNameDownloaded(SynergyResponse response);
    void privProjectNameDownloaded(SynergyResponse response);
    void updateProject();
    void updateFolder();
    void startSync();
    void updateToNewestVersion();
    void compareToolSet();
    void relateTasksWithProjectGrouping();

private:
    FileStatus::Status getFileStatus(const QString &filePath, const QString &fileName);
    bool stopSynergy();
    inline SynergyControl *synergyControl() const;
    ActionCommandPair createAndBindAction(Core::ActionContainer *ac, const QString &text, const Core::Id &id,
                                const Core::Context &context, const char *pluginSlot, const Core::Id &groupId = Core::Id());
    void processRawPath(QString path);
    QString addToJunctionMap(QStringList junctionList);
    void reparseListOfPaths(QStringList &pathList);
    QString reparsePath(QString dirPath);
    void queryProjectName();
    QString updateBaselineAndGetCreationTime();
    QSet<QString> getNewestSqaFolders(const QString &baseline, const QString &baselineCreationTime);
    void relateFoldersWithProjectGrouping(QFutureInterface<void> &future, const QSet<QString> &folders);
    void setCompareTool();

    void error(QString errorMessage);

    QString m_waPath;
    QString m_waPathCandidate;
    QString m_startPath;
    QString m_hsProjectName;
    QString m_privProjectName;
    QString m_projectGrouping;

    QAction *m_menuAction;
    QAction *m_diffAction;
    SynergySettings m_settings;
    QProgressDialog *m_progress;
    FilesToSyncDialog *m_filesToSyncDialog;

    WorkerThreadPair m_startingWorkerThread;
    QProcessEnvironment m_synergyEnviroment;

    JunctionMap m_junctionMap;

    bool m_isSynergyStarted;
    bool m_isCompareToolSet;
    mutable bool m_isProjectRootSeekStarted;
    mutable bool m_isSynergyStarting;

    static SynergyPlugin *m_SynergyPluginInstance;
};

} // namespace Internal
} // namespace Synergy

#endif // SYNERGYPLUGIN_H

