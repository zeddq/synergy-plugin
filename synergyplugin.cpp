#include "synergyplugin.h"
#include "synergyconstants.h"
#include "synergycontrol.h"
#include "tasksdialog.h"
#include "asyncworker.h"
#include "settingspage.h"
#include "syncdialog.h"
#include "filestosyncdialog.h"

#include "string"
#include "cstdlib"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/dialogs/settingsdialog.h>

#include <vcsbase/vcsbasesubmiteditor.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QDir>
#include <QThread>
#include <QProcess>
#include <QtConcurrentRun>
#include <QRegExp>
#include <QList>

#include <QtPlugin>

#include <string>
#ifdef WITH_TESTS
#include <QTest>
#endif

namespace Synergy {
namespace Internal {

const int EMPTY_RESULT_ERROR_CODE = 6;

SynergyPlugin *SynergyPlugin::m_SynergyPluginInstance = 0;

using namespace Core;

SynergyPlugin::SynergyPlugin() :
    m_menuAction(0),
    m_diffAction(0),
    m_isSynergyStarted(false),
    m_progress(0),
    m_filesToSyncDialog(0),
    m_waPath(QLatin1String("")),
    m_waPathCandidate(QLatin1String("")),
    m_startPath(QLatin1String("")),
    m_hsProjectName(QLatin1String("")),
    m_privProjectName(QLatin1String("")),
    m_isProjectRootSeekStarted(false),
    m_isSynergyStarting(false),
    m_startingWorkerThread(WorkerThreadPair(0,0)),
    m_isCompareToolSet(false)
{
    m_filesToSyncDialog = new FilesToSyncDialog;
    qRegisterMetaType<SynergyResponse>("SynergyResponse");
}

SynergyPlugin::~SynergyPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool SynergyPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize function, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    Context context(Constants::SYNERGY_CONTEXT);
    m_SynergyPluginInstance = this;
    initializeVcs(new SynergyControl(this), context);

    m_settings.fromSettings(ICore::settings());
    addAutoReleasedObject(new SettingsPage);

    //-------------------
    // Synergy container in Tools

    //Context context(Synergy::Constants::SYNERGY_CONTEXT);

    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *synergyContainer = ActionManager::createMenu(Synergy::Constants::MENU_ID);
    synergyContainer->menu()->setTitle(tr("Synergy"));
    toolsContainer->addMenu(synergyContainer);
    m_menuAction = synergyContainer->menu()->menuAction();

    ActionCommandPair actionCommand =
            createAndBindAction(synergyContainer, tr("Run Synergy shell"), "Synergy.Shell",
                                context, SLOT(runSynergyShellSlot()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+S")));

    actionCommand =
            createAndBindAction(synergyContainer, tr("My Tasks"), "Synergy.Tasks",
                                context, SLOT(showTasks()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+T")));

    actionCommand =
            createAndBindAction(synergyContainer, tr("CITask"), "Synergy.CITask",
                                context, SLOT(startCITask()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+C")));

    actionCommand =
            createAndBindAction(synergyContainer, tr("Full Project Groupings"), "Synergy.FPG",
                                context, SLOT(startFPG()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+F")));

    actionCommand =
            createAndBindAction(synergyContainer, tr("Update Project"), "Synergy.UpdateProject",
                                context, SLOT(updateProject()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+P")));

    actionCommand =
            createAndBindAction(synergyContainer, tr("Update to Newest"), "Synergy.UpdateToNewest",
                                context, SLOT(updateToNewestVersion()));
    actionCommand.second->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+P")));

    // \Synergy container in Tools
    //-------------------

    ActionContainer *mfileContextMenu =
        ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);

    Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);
    const Id treeGroup = ProjectExplorer::Constants::G_PROJECT_TREE;

    mfileContextMenu->addSeparator(context, treeGroup);
    actionCommand =
            createAndBindAction(mfileContextMenu, tr("Diff"), "Synergy.Diff",
                                projecTreeContext, SLOT(startDiff()), treeGroup);
    m_diffAction = actionCommand.first;

    ActionContainer *mdirContextMenu =
        ActionManager::actionContainer(ProjectExplorer::Constants::M_FOLDERCONTEXT);
    mdirContextMenu->addSeparator(context, treeGroup);

    actionCommand =
            createAndBindAction(mdirContextMenu, tr("Update"), "Synergy.UpdateFolder",
                                projecTreeContext, SLOT(updateFolder()), treeGroup);

    actionCommand =
            createAndBindAction(mfileContextMenu, tr("Sync"), "Synergy.StartSyncFile",
                                projecTreeContext, SLOT(startSync()), treeGroup);

    actionCommand =
            createAndBindAction(mdirContextMenu, tr("Sync"), "Synergy.StartSyncDir",
                                projecTreeContext, SLOT(startSync()), treeGroup);

    if (synergyControl()->isConfigured())
    {
        startSynergyAsync(QDir::currentPath());
    }

    return true;
}

void SynergyPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag SynergyPlugin::aboutToShutdown()
{
    stopSynergy();
    delete m_progress;
    delete m_filesToSyncDialog;
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

bool SynergyPlugin::submitEditorAboutToClose()
{
    return true;
}

void SynergyPlugin::updateActions(VcsBase::VcsBasePlugin::ActionState as)
{
    enableMenuAction(as, m_menuAction);

    if (m_isCompareToolSet)
        enableMenuAction(as, m_diffAction);
    else
        m_diffAction->setVisible(false);
}

bool SynergyPlugin::vcsOpen(const QString &fileName)
{    
    qDebug() << "***** vcsOpen():" << fileName;
    addFilesToDialog(reparsePath(fileName), Checkout);
    return true;
}

bool SynergyPlugin::vcsAdd(const QString &fileName)
{
    qDebug() << "***** vcsAdd():" << fileName;
    addFilesToDialog(reparsePath(fileName), Add);
    return true;
}

bool SynergyPlugin::vcsDelete(const QString &workingDir, const QString &fileName, bool replace)
{
    QStringList args = QStringList(QLatin1String("delete"));
    if (replace)
        args << QLatin1String("-replace");

    args << QLatin1String("-force")
         << fileName;
    SynergyResponse response = runSynergy(workingDir, args);
    if (response.error)
        return false;
    else
        return true;
}

WorkerThreadPair SynergyPlugin::vcsDeleteAsync(const QString &workingDir, const QString &fileName, bool replace)
{
    QStringList args = QStringList(QLatin1String("delete"));
    if (replace)
        args << QLatin1String("-replace");

    args << QLatin1String("-force")
         << fileName;
    return runSynergyAsync(workingDir, args);
}

bool SynergyPlugin::managesFile(const QString &workingDirectory, const QString &fileName)
{
    QString absFile = QFileInfo(QDir(workingDirectory), fileName).absolutePath();
    QString fileNameWoPath = QFileInfo(fileName).fileName();
    const FileStatus::Status status = getFileStatus(absFile, fileNameWoPath);
    return status != FileStatus::NotManaged;
}

bool SynergyPlugin::managesDirectory(const QString &dir, QString *topLevel /* = 0 */)
{
    if (m_isProjectRootSeekStarted)
        return false;

    if (!m_waPath.isEmpty())
    {
        qDebug() << "****** wapath not empty";
        if (topLevel)
            *topLevel = m_waPath;

        QString reparsedDir = reparsePath(dir);
        if (reparsedDir.startsWith(m_waPath))
            return true;
        else
            return false;
    }

    m_isProjectRootSeekStarted = true;

    m_waPathCandidate = dir;

    QDir directory(dir);
    QProcess dirProcess;
    QByteArray data;
    QString previousDir;

    do {
        if (directory.absolutePath().contains(QLatin1String("Fiat_VP2/VP2_")) ||
            directory.absolutePath().endsWith(QLatin1String("Fiat_VP2")))
        {
            processRawPath(directory.absolutePath());
            return false;
        }
        else
        {
            dirProcess.setProcessChannelMode(QProcess::MergedChannels);
            dirProcess.setWorkingDirectory(directory.absolutePath());
            dirProcess.start(QLatin1String("cmd.exe /C dir"));
            if (dirProcess.waitForStarted())
            {
                data.clear();
                while (dirProcess.waitForReadyRead())
                {
                    data.append(dirProcess.readAll());
                }
                QStringList filteredDirOutput = QString::fromLatin1(data.data()).
                                                split(QLatin1Char('\n'), QString::SkipEmptyParts).
                                                filter(QLatin1String("JUNCTION")).
                                                filter(previousDir);
                QStringList workAreaPaths = filteredDirOutput.filter(QLatin1String("Fiat_VP2"));
                if (!workAreaPaths.isEmpty())
                {
                    m_waPathCandidate = directory.absolutePath();
                    QString added = addToJunctionMap(workAreaPaths);
                    processRawPath(added);
                    return false;
                }
                else if (!filteredDirOutput.empty())
                {
                    m_waPathCandidate = directory.absolutePath();
                    QString added = addToJunctionMap(filteredDirOutput);
                    directory.setPath(added);
                }
            }
        }
        previousDir = directory.dirName();
    } while (!directory.isRoot() && directory.cdUp());

    m_isProjectRootSeekStarted = false;
    return false;
}

bool SynergyPlugin::isSynergyStarted() const
{
    return m_isSynergyStarted;
}

bool SynergyPlugin::isSynergyStarting() const
{
    return m_isSynergyStarting;
}

SynergyPlugin *SynergyPlugin::instance()
{
    return m_SynergyPluginInstance;
}

SynergyResponse
        SynergyPlugin::runSynergy(const QString &workingDir, const QStringList &arguments,
                                unsigned flags, QTextCodec *outputCodec)
{
    SynergyResponse response;
    if (m_settings.m_synergyBinaryPath.isEmpty())
    {
        response.error = true;
        response.message = tr("No Synergy executable specified.");
        return response;
    }

    const Utils::SynchronousProcessResponse sp_resp =
            VcsBase::VcsBasePlugin::runVcs(workingDir, Utils::FileName::fromUserInput(m_settings.m_synergyBinaryPath),
                                           arguments, m_settings.timeOutMS(),
                                           flags, outputCodec, m_synergyEnviroment);

    response.error = (sp_resp.result != Utils::SynchronousProcessResponse::Finished) &&
                     (sp_resp.exitCode != EMPTY_RESULT_ERROR_CODE);
    if (response.error)
        response.message = sp_resp.exitMessage(m_settings.m_synergyBinaryPath, m_settings.timeOutMS());
    response.stdErr     = sp_resp.stdErr;
    response.stdOut     = sp_resp.stdOut;
    response.exitCode   = sp_resp.exitCode;

    qDebug() << "ErrorOut: " <<response.stdErr;
    qDebug() << "Out: " <<response.stdOut;
    qDebug() << "exitcode: " <<response.exitCode;

    return response;
}

WorkerThreadPair SynergyPlugin::runSynergyAsync(const QString &workingDir, const QStringList &arguments, unsigned flags) const
{
    QThread *thread = new QThread;
    AsyncWorker *worker = new AsyncWorker(workingDir, arguments, flags);
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(resultDownloaded(SynergyResponse)), thread, SLOT(quit()));
    connect(worker, SIGNAL(resultDownloaded(SynergyResponse)), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    return WorkerThreadPair(worker, thread);
}

const SynergySettings SynergyPlugin::settings() const
{
    return m_settings;
}

void SynergyPlugin::setSettings(const SynergySettings &s)
{
    if (s != m_settings)
    {
        QString compareToolPath = m_settings.m_compareToolPath;

        m_settings = s;
        m_settings.toSettings(ICore::settings());
        synergyControl()->emitConfigurationChanged();

        if (compareToolPath != m_settings.m_compareToolPath)
            setCompareTool();

        VcsManager::clearVersionControlCache();
    }
}

void SynergyPlugin::setConnectionDialogVisibilitySlot(bool visible)
{
    if (!m_progress)
        m_progress = new QProgressDialog(QLatin1String("Starting Synergy..."), QLatin1String("Cancel"), 0, 0);

    m_progress->setWindowModality(Qt::WindowModal);
    m_progress->setVisible(visible);
}

void SynergyPlugin::runSynergyShellSlot()
{
    QStringList args(QLatin1String("/k"));
    args << QLatin1String("set CCM_HOME=s:\\cm_syn\\V7.1L.wetp055x") +
            QLatin1String("&& set PATH=s:\\cm_syn\\V7.1L.wetp055x\\bin;s:\\cm_syn\\V7.1L.wetp055x\\bin\\util;s:\\cm_syn\\V7.1L.wetp055x\\jre\\bin;%PATH%") +
            QLatin1String("&& set CCM_ADDR=") +
            m_synergyEnviroment.value(QLatin1String("CCM_ADDR"));

    QProcess::startDetached(QLatin1String("cmd.exe"), args, m_waPath);
}

void SynergyPlugin::showTasks()
{
    TasksDialog tasksDialog(Show);
    tasksDialog.exec();
}

void SynergyPlugin::synergyStartResultDownloaded(SynergyResponse response)
{
    m_isSynergyStarted = !response.error;

    if (response.error)
        return;

    //--- DO PRZERZUCENIA DO synergyStartResultDownloaded
    m_synergyEnviroment.clear();
    m_synergyEnviroment.insert(QLatin1String("CCM_HOME"), QLatin1String("s:\\cm_syn\\V7.1L.wetp055x"));
    m_synergyEnviroment.insert(QLatin1String("PATH"), QLatin1String("s:\\cm_syn\\V7.1L.wetp055x\\bin;"
                                                                    "s:\\cm_syn\\V7.1L.wetp055x\\bin\\util;"
                                                                    "s:\\cm_syn\\V7.1L.wetp055x\\jre\\bin;%PATH%"));

    QString ccmAddress = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts)[0];
    m_synergyEnviroment.insert(QLatin1String("CCM_ADDR"), ccmAddress);

    setCompareTool();

    m_isSynergyStarting = false;

    qDebug() << "ccmAddress: " << ccmAddress;
    qDebug() << response.stdOut;
    qDebug() << "Error: " << response.stdErr;
    qDebug() << "Result: " << response.error;
}

void SynergyPlugin::addFilesToTask(ObjectActionList &filesList)
{
    TasksDialog taskDialog(Checkout, filesList);
    taskDialog.exec();
}

void SynergyPlugin::addFilesToDialog(QString &filePath, DialogAction dialogAction)
{
    m_filesToSyncDialog->setModal(true);
    m_filesToSyncDialog->hide();
    m_filesToSyncDialog->show();
    m_filesToSyncDialog->addFileToList(filePath, dialogAction);
}

void SynergyPlugin::addFilesToDialog(QStringList &filesList, DialogAction dialogAction)
{
    m_filesToSyncDialog->open();
    foreach (QString file, filesList)
    {
        m_filesToSyncDialog->addFileToList(file, dialogAction);
    }
}

FileStatus::Status SynergyPlugin::getFileStatus(const QString &filePath, const QString &fileName)
{
    QString reparsedFilePath = reparsePath(filePath);
    QStringList args(QLatin1String("dir"));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    SynergyResponse response = runSynergy(reparsedFilePath, args);
    QApplication::restoreOverrideCursor();

    if (response.error)
        return FileStatus::NotManaged;

    qDebug() << fileName;
    qDebug() << response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    qDebug() << response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts).filter(fileName);
    if (response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts).filter(fileName).count() > 0)
    {
        qDebug() << "****** jest w dirze";
        return FileStatus::Integrate;
    }

    return FileStatus::NotManaged;
}

WorkerThreadPair SynergyPlugin::startSynergyAsync(QString workingDir)
{
    if (m_isSynergyStarting)
        return m_startingWorkerThread;

    m_isSynergyStarting = true;

    QStringList args = QStringList(QLatin1String("start"));
    args << QLatin1String("-n")  << SynergyPlugin::instance()->settings().m_uid
         << QLatin1String("-pw") << SynergyPlugin::instance()->settings().m_password
         << QLatin1String("-d")  << QLatin1String("/appl/ccm/db/Fiat_WZ")
         << QLatin1String("-s")  << QLatin1String("http://10.215.250.5:8410")
         << QLatin1String("-m")
         << QLatin1String("-q");

    // DEBUG COMMENTS - to uncomment
    m_startingWorkerThread = runSynergyAsync(workingDir, args);
    connect(m_startingWorkerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
            this, SLOT(synergyStartResultDownloaded(SynergyResponse)));

    //--------------------------------------------------

    return m_startingWorkerThread;
}

QString SynergyPlugin::getCurrentFileReparsed()
{
    if (ProjectExplorer::FileNodeType ==
            ProjectExplorer::ProjectExplorerPlugin::currentNode()->nodeType())
    {
        QString reparsedPath = reparsePath(ProjectExplorer::ProjectExplorerPlugin::currentNode()->path());
        reparsedPath = reparsedPath.right(reparsedPath.length() - reparsedPath.indexOf(QLatin1String("/VP2")) - 1);
        return reparsedPath;
    }

    return QLatin1String("");
}

bool SynergyPlugin::stopSynergy()
{
    //if (m_startingWorkerThread.second && m_startingWorkerThread.second->isRunning())
    {
        //m_startingWorkerThread.second->terminate();
        //m_startingWorkerThread.second->deleteLater();
    }
    if (m_isSynergyStarted)
    {
        QStringList args = QStringList(QLatin1String("stop"));

        QFileInfo synergyBin(m_settings.m_synergyBinaryPath);
        if (!synergyBin.exists())
                return false;

        const Utils::SynchronousProcessResponse sp_resp =
                VcsBase::VcsBasePlugin::runVcs(synergyBin.path(), Utils::FileName::fromUserInput(m_settings.m_synergyBinaryPath),
                                               args, m_settings.longTimeOutMS(),
                                               ShowStdOutInLogWindow, 0, m_synergyEnviroment);
        qDebug() << sp_resp.stdOut;
        qDebug() << "Error: " << sp_resp.stdErr;
        qDebug() << "Result: " << sp_resp.result;
    }
}

SynergyControl *SynergyPlugin::synergyControl() const
{
    return static_cast<SynergyControl *>(versionControl());
}

// Create an action to act on the repository

ActionCommandPair SynergyPlugin::createAndBindAction(ActionContainer *ac,
                                            const QString &text, const Id &id, const Core::Context &context,
                                            const char *pluginSlot, const Id &groupId)
{
    QAction  *action = new QAction(text, this);
    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    if (ac)
        ac->addAction(command, groupId);

    connect(action, SIGNAL(triggered()), this, pluginSlot);
    action->setData(id.uniqueIdentifier());
    return ActionCommandPair(action, command);
}

void SynergyPlugin::verifyProjectRoot()
{
    QStringList args(QLatin1String("ls"));

    qDebug() << "verifyProjectRoot()" << m_waPathCandidate;
    const WorkerThreadPair workerThread = runSynergyAsync(m_waPathCandidate, args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
            this, SLOT(lsResultDownloaded(SynergyResponse)));
}

void SynergyPlugin::lsResultDownloaded(SynergyResponse response)
{
    if (!response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts).isEmpty())
    {
        m_waPath = QDir::fromNativeSeparators(m_waPathCandidate);
        m_waPathCandidate = QLatin1String("");
        queryProjectName();
        m_isProjectRootSeekStarted = false;
        VcsManager::clearVersionControlCache();
    }
    else
    {
        m_waPathCandidate = QLatin1String("");
        m_waPath = QLatin1String("");
        m_isProjectRootSeekStarted = false;
    }
}

void SynergyPlugin::startCITask()
{
    QStringList args(QLatin1String("/C"));
    args << QLatin1String("set CCM_HOME=s:\\cm_syn\\V7.1L.wetp055x") +
            QLatin1String("&& set PATH=s:\\cm_syn\\V7.1L.wetp055x\\bin;s:\\cm_syn\\V7.1L.wetp055x\\bin\\util;s:\\cm_syn\\V7.1L.wetp055x\\jre\\bin;%PATH%") +
            QLatin1String("&& set CCM_ADDR=") +
            m_synergyEnviroment.value(QLatin1String("CCM_ADDR")) +
            QLatin1String(" && ") +
            m_waPath + QLatin1String("/tools/es_tools/CITask.bat");

    QProcess::startDetached(QLatin1String("cmd.exe"), args, m_waPath);
}

void SynergyPlugin::startDiff()
{
    if (!QFile(m_settings.m_compareToolPath).exists())
    {
        error(QLatin1String("No Compare Tool specified!\nPlease choose your tool in settings."));
        return;
    }

    ProjectExplorer::Node *currentNode =
            ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode();
    const QFileInfo fi(currentNode->path());
    QString dir = reparsePath(fi.absolutePath());

    if (fi.isWritable())
    {
        QStringList args(QLatin1String("dir"));
        args    << QLatin1String("-format")
                << QLatin1String("%name %status %owner");

        SynergyResponse response = runSynergy(dir, args);
        QStringList parsedResponse = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);
        bool isWorking = !parsedResponse.filter(fi.fileName()).filter(QLatin1String("working")).isEmpty();
        if (isWorking)
            diffWithPredecessor(fi.fileName(), dir);
        else
            diffWithCurrentVersion(fi.fileName(), dir);
    }
    else
    {
        diffWithPredecessor(fi.fileName(), dir);
    }
}

void SynergyPlugin::startFPG()
{
    QProcess *process = new QProcess(this);
    process->setProcessEnvironment(m_synergyEnviroment);

    QStringList args;
    args << QLatin1String("Fiat_WZ") << m_hsProjectName;

    connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
    process->start(QLatin1String("s:/cm_syn/V7.1L.wetp055x/etc/plugins/I_IC_plugins/FPG/SynergyFullProjectGrouping.exe"), args);
}

void SynergyPlugin::hsProjectNameDownloaded(SynergyResponse response)
{
    if (response.error || response.stdOut.isEmpty())
    {
        error(response.stdErr);
        return;
    }
    m_hsProjectName = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts)[0];
}

void SynergyPlugin::privProjectNameDownloaded(Synergy::Internal::SynergyResponse response)
{
    if (response.error || response.stdOut.isEmpty())
    {
        error(response.stdErr);
        return;
    }
    m_privProjectName = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts)[0];
}

void SynergyPlugin::updateProject()
{
    QStringList args(QLatin1String("update"));
    args << QLatin1String("-project")
         << QLatin1String("-recurse")
         << m_hsProjectName;
    runSynergy(m_waPath, args, ShowStdOutInLogWindow);

    args.clear();
    args << QLatin1String("update")
         << QLatin1String("-project")
         << QLatin1String("-recurse")
         << m_privProjectName;
    runSynergy(m_waPath, args, ShowStdOutInLogWindow);
}

void SynergyPlugin::updateFolder()
{
    ProjectExplorer::Node *currentNode =
            ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode();
    QStringList args(QLatin1String("update"));
    args << QLatin1String("-recurse") << reparsePath(currentNode->path());
    runSynergy(m_waPath, args, ShowStdOutInLogWindow);
}

void SynergyPlugin::startSync()
{
    ProjectExplorer::Node *currentNode =
            ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode();
    QDir dir(currentNode->path());
    QFileInfo file(currentNode->path());
    dir.cdUp();
    SyncDialog syncDialog(reparsePath(dir.absolutePath()), file.fileName());
    syncDialog.exec();
}

void SynergyPlugin::updateToNewestVersion()
{
    QStringList args(QLatin1String("query"));
    args << QLatin1String("-format") << QLatin1String("%objectname")
         << QLatin1String("-unnumbered")
         << QLatin1String("is_project_grouping_of('") + m_hsProjectName + QLatin1String("')");

    // project grouping
    SynergyResponse response = runSynergy(m_waPath, args);
    if (response.error)
        return;
    m_projectGrouping = response.stdOut.left(response.stdOut.indexOf(QLatin1Char('\n')));

    QString baselineCreationTime = updateBaselineAndGetCreationTime();
    if (baselineCreationTime.isEmpty())
        return;

    // get baseline version
    QString prefix(QLatin1String("%002f"));
    int startPos = m_projectGrouping.indexOf(prefix) + prefix.length();
    int endPos = m_projectGrouping.indexOf(QLatin1String("%003a"));
    QString baselineNumber = m_projectGrouping.mid(startPos, endPos - startPos);

    // get newest SQA Folders
    QSet<QString> sqaFolders = getNewestSqaFolders(baselineNumber, baselineCreationTime);
    if (!sqaFolders.isEmpty())
    {
        // relate folders
        QFuture<void> task = QtConcurrent::run(&relateFoldersWithProjectGrouping, sqaFolders);
        FutureProgress *progress =
            ProgressManager::addTask(task, tr("Updating Locator Caches"), Core::Constants::TASK_INDEX);
        connect(progress, SIGNAL(finished()), this, SLOT(relateTasksWithProjectGrouping()));
    }
    else
    {
        // relate tasks
        relateTasksWithProjectGrouping();
    }

    // update project
    updateProject();
}

void SynergyPlugin::compareToolSet()
{
    if (QFile(m_settings.m_compareToolPath).exists())
    {
        m_isCompareToolSet = true;
        m_diffAction->setVisible(true);
        m_diffAction->setEnabled(true);
    }
}

void SynergyPlugin::processRawPath(QString path)
{
    if (!synergyControl()->isConfigured())
    {
        Core::ICore::showOptionsDialog("V.Version Control",
                                       synergyControl()->id(),
                                       0);
    }

    if (!synergyControl()->isConfigured())
    {
        m_isProjectRootSeekStarted = false;
        return;
    }

    int fiatVP2Pos = path.indexOf(QLatin1String("Fiat_VP2/VP2_"));
    if (fiatVP2Pos == -1)
        fiatVP2Pos = path.lastIndexOf(QLatin1String("Fiat_VP2"));
    int slashPos = path.indexOf(QLatin1Char('/'), fiatVP2Pos);
    m_waPathCandidate = path.left(slashPos);

    if (!m_isSynergyStarted)
    {
        WorkerThreadPair workerThread = startSynergyAsync(m_waPathCandidate);
        connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
                this, SLOT(verifyProjectRoot()));
    }
    else
    {
        verifyProjectRoot();
    }
}

QString SynergyPlugin::addToJunctionMap(QStringList junctionList)
{
    QString junctionPath = QDir::fromNativeSeparators(m_waPathCandidate);
    QString key;
    foreach (QString junction, junctionList)
    {
        QRegExp rxJunctionKey(QLatin1String("\\>(.*)\\["));
        rxJunctionKey.setMinimal(true);
        rxJunctionKey.indexIn(junction);

        QRegExp rxJunctionValue(QLatin1String("\\[(.*)\\]"));
        rxJunctionValue.setMinimal(true);
        rxJunctionValue.indexIn(junction);

        key = rxJunctionValue.cap(1).trimmed();
        int rootPos = key.indexOf(QLatin1String("D:\\"));
        if (-1 == rootPos)
            rootPos = key.indexOf(QLatin1String("C:\\"));
        key = key.right(key.length() - rootPos);

        if (rxJunctionKey.captureCount() > 0 &&
            rxJunctionValue.captureCount() > 0)
        {
            m_junctionMap[junctionPath + QLatin1Char('/') + rxJunctionKey.cap(1).trimmed()] =
                    QDir::fromNativeSeparators(key);
        }
    }
    return key;
}

void SynergyPlugin::reparseListOfPaths(QStringList &pathList)
{
    for (int i=0; i<pathList.count(); ++i)
    {
        pathList[i] = reparsePath(pathList[i]);
    }
}

QString SynergyPlugin::reparsePath(QString dirPath)
{
    bool stop = false;
    while(!stop)
    {
        stop = true;
        QMapIterator<QString, QString> it(m_junctionMap);
        while(it.hasNext())
        {
            it.next();
            if (dirPath.startsWith(it.key(), Qt::CaseInsensitive))
            {
                dirPath.replace(it.key(), it.value());
                stop = false;
                break;
            }
        }
    }
    return dirPath;;
}

void SynergyPlugin::queryProjectName()
{
    QStringList args(QLatin1String("project"));
    args << QLatin1String("-format") << QLatin1String("%objectname");
    WorkerThreadPair workerThread = runSynergyAsync(m_waPath + QLatin1String("/VP2_HS"), args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
            this, SLOT(hsProjectNameDownloaded(SynergyResponse)));

    args.clear();
    args << QLatin1String("project")
         << QLatin1String("-format") << QLatin1String("%objectname");
    workerThread = runSynergyAsync(m_waPath + QLatin1String("/VP2_PRIV"), args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
            this, SLOT(privProjectNameDownloaded(SynergyResponse)));
}

void SynergyPlugin::diffWithPredecessor(QString fileName, QString workingDir)
{
    QStringList args(QLatin1String("diff"));
    args << fileName;
    runSynergyAsync(workingDir, args);
}

void SynergyPlugin::diffWithCurrentVersion(QString fileName, QString workingDir)
{
    QStringList args(QLatin1String("attribute"));
    args << QLatin1String("-show") << QLatin1String("objectname") << fileName;
    SynergyResponse response = runSynergy(workingDir, args);
    if (response.error)
        return;
    QString objectName = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts)[0];
    args.clear();
    args <<QLatin1String("diff") << fileName << objectName;
    runSynergyAsync(workingDir, args);
}

QString SynergyPlugin::updateBaselineAndGetCreationTime()
{
    // get currently used baseline
    QStringList args(QLatin1String("query"));
    args << QLatin1String("-format") << QLatin1String("%objectname")
         << QLatin1String("-unnumbered")
         << QLatin1String("is_baseline_in_pg_of('") + m_projectGrouping + QLatin1String("')");
    SynergyResponse response = runSynergy(m_waPath, args);
    if (response.error)
        return QString();

    QString currentBaseline = response.stdOut.left(response.stdOut.indexOf(QLatin1Char('\n')));
    // correcting baseline to fit to format
    QString baselineNumber = currentBaseline.left(currentBaseline.indexOf(QLatin1Char('.')));

    // get list of all baselines
    args.clear();
    args << QLatin1String("query")
         << QLatin1String("-format") << QLatin1String("%objectname|%{create_time[dateformat='yyyyMMdd']}")
         << QLatin1String("-unnumbered")
         << QLatin1String("type='baseline' and (status='published_baseline' or status='released')");
    response = runSynergy(m_waPath, args);
    if (response.error)
        return QString();

    QStringList baselineList = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);

    // leave baselines on current line only
    baselineList = baselineList.filter(baselineNumber);

    if (baselineList.isEmpty())
        return QString();

    QString newestBaseline;
    do {
        newestBaseline = baselineList.last();
        baselineList.removeLast();
    } while ( !baselineList.isEmpty() && newestBaseline.contains(QLatin1String("ARC")));

    QStringList baselineObjectnameDate = newestBaseline.split(QLatin1Char('|'), QString::SkipEmptyParts);
    if (baselineObjectnameDate.count() < 2)
        return QString();

    QString createDate = baselineObjectnameDate[1];

    // unrelate from current baseline
    args.clear();
    args << QLatin1String("unrelate")
         << QLatin1String("-name") << QLatin1String("baseline_in_pg")
         << QLatin1String("-from") << m_projectGrouping
         << QLatin1String("-to") << currentBaseline;
    response = runSynergy(m_waPath, args);
    if (response.error)
        return QString();

    // relate to newest baseline
    args.clear();
    args << QLatin1String("relate")
         << QLatin1String("-name") << QLatin1String("baseline_in_pg")
         << QLatin1String("-from") << m_projectGrouping
         << QLatin1String("-to") << baselineObjectnameDate[0];
    response = runSynergy(m_waPath, args);
    if (response.error)
        return QString();

    return createDate;
}

QSet<QString> SynergyPlugin::getNewestSqaFolders(const QString &baseline, const QString &baselineCreationTime)
{
    QStringList args(QLatin1String("query"));
    args << QLatin1String("-unnumbered")
         << QLatin1String("-format") << QLatin1String("%objectname %description")
         << QLatin1String("description match '*sqa_tasks_VP2/") + baseline + QLatin1String("_VP2_HMI_STE_GUI*'");
    SynergyResponse response = runSynergy(m_waPath, args);
    if (response.error)
        return QSet<QString>();

    // select only folders with date in descritpion greater than
    // creation date of the baseline
    int baselineCreationTimeNumber = baselineCreationTime.toInt();
    QStringList sqaFoldersCandidates = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    QStringList sqaFolders;
    QRegExp dateRegex(QLatin1String("_\\d{6}"));
    for (int i = 0; i < sqaFoldersCandidates.count(); ++i)
    {
        int startIndex = sqaFoldersCandidates[i].indexOf(dateRegex);
        QString sqaDateSuffix = sqaFoldersCandidates[i].mid(startIndex + 1, 8);

        int sqaDateNumber = sqaDateSuffix.toInt();
        if (sqaDateNumber > baselineCreationTimeNumber)
        {
            sqaFoldersCandidates[i] = sqaFoldersCandidates[i].left(sqaFoldersCandidates[i].indexOf(QLatin1Char(' ')));
            sqaFolders.append(sqaFoldersCandidates[i]);
        }
    }

    QSet<QString> folderSet = QSet<QString>::fromList(sqaFolders);

    return folderSet;
}

void SynergyPlugin::relateFoldersWithProjectGrouping(QFutureInterface<void> &future, const QSet<QString> &folders)
{
    // get folders in project grouping
    QStringList args(QLatin1String("query"));
    args << QLatin1String("-format") << QLatin1String("%objectname")
         << QLatin1String("-unnumbered")
         << QLatin1String("is_folder_in_pg_of('") + m_projectGrouping + QLatin1String("')");
    SynergyResponse response = runSynergy(m_waPath, args);
    if (response.error)
        return;
    QStringList foldersInProjectGrouping = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);

    future.setProgressRange(0, folders.count());
    // relate folders
    foreach(QString folder, folders)
    {
        if (! future.isCanceled())
        {
            if (!foldersInProjectGrouping.contains(folder))
            {
                args.clear();
                args << QLatin1String("relate")
                     << QLatin1String("-name") << QLatin1String("folder_in_pg")
                     << QLatin1String("-from") << m_projectGrouping
                     << QLatin1String("-to")   << folder;
                response = runSynergy(m_waPath, args);
                if (response.error)
                    return;
            }
            future.setProgressValue(++future.progressValue());
        }
        else
        {
            break;
        }
    }
}

void SynergyPlugin::relateTasksWithProjectGrouping()
{
    // get tasks in project grouping
    qDebug() << "finding tasks in pg...";
    QStringList args(QLatin1String("query"));
    args << QLatin1String("-format") << QLatin1String("%objectname")
         << QLatin1String("-unnumbered")
         << QLatin1String("is_saved_task_in_pg_of('") + m_projectGrouping + QLatin1String("')");
    SynergyResponse response = runSynergy(m_waPath, args);
    if (response.error)
        return;
    QSet<QString> tasksInProjectGrouping = QSet<QString>::fromList(response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts));

    qDebug() << "finding tasks in all added folders...";
    args.clear();
    args << QLatin1String("query")
         << QLatin1String("-format") << QLatin1String("%objectname")
         << QLatin1String("-unnumbered")
         << QLatin1String("is_task_in_folder_of(is_folder_in_pg_of('") + m_projectGrouping + QLatin1String("'))");
    response = runSynergy(m_waPath, args);
    if (response.error)
        return;

    QStringList tasksInFoldersStrings = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    QSet<QString> tasksInFolders = QSet<QString>::fromList(tasksInFoldersStrings);
    QSet<QString> tasksToRelate = tasksInFolders - tasksInProjectGrouping;
    QSet<QString> tasksToUnrelate = tasksInProjectGrouping - tasksInFolders;

    qDebug() << "relating tasks...";
    foreach (QString task, tasksToRelate)
    {
        args.clear();
        args << QLatin1String("relate")
             << QLatin1String("-name") << QLatin1String("saved_task_in_pg")
             << QLatin1String("-from") << m_projectGrouping
             << QLatin1String("-to")   << task;
        response = runSynergy(m_waPath, args);
        if (response.error)
            return;
    }

    qDebug() << "unrelating tasks...";
    foreach (QString task, tasksToUnrelate)
    {
        args.clear();
        args << QLatin1String("project_grouping")
             << QLatin1String("-remove_task")
             << task
             << m_projectGrouping;
        response = runSynergy(m_waPath, args);
        if (response.error)
            return;
    }
}

void SynergyPlugin::setCompareTool()
{
    if (QFile(m_settings.m_compareToolPath).exists())
    {
        QStringList args;
        args << QLatin1String("set")  << QLatin1String("cli_compare_cmd") << m_settings.m_compareToolPath + QLatin1String(" %file1") + QLatin1String(" %file2");
        WorkerThreadPair workerThread = runSynergyAsync(QLatin1String("C:\\"), args);
        connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(compareToolSet()));
    }
}

void SynergyPlugin::error(QString errorMessage)
{
    QMessageBox::information(0, tr("Error"), errorMessage);
}

} //namespace Internal
} //namespace SynergyPlugin
