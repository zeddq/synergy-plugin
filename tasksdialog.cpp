#include "tasksdialog.h"
#include "ui_tasksdialog.h"
#include "asyncworker.h"
#include "synergyplugin.h"
#include "taskwizard.h"

#include <QtConcurrent>
#include <QMessageBox>
#include <QMenu>
#include <QIcon>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFile>

using namespace Synergy;
using namespace Synergy::Internal;

TasksDialog::TasksDialog(DialogAction action, ObjectActionList fileNamesActions, QWidget *parent) :
    QDialog(parent),
    m_fileNamesActions(fileNamesActions),
    m_taskListReceived(false),
    m_currentAction(action),
    m_chosenTask(QLatin1String("")),
    ui(new Ui::TasksDialog),
    m_pendingActions(0)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(1);

    connect(this, SIGNAL(setConnectionDialogVisibility(bool)),
            SynergyPlugin::instance(), SLOT(setConnectionDialogVisibilitySlot(bool)));

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    if (m_currentAction == Show || m_currentAction == Indicate)
        connect(ui->okButton, SIGNAL(clicked()), this, SLOT(accept()));
    else
        connect(ui->okButton, SIGNAL(clicked()), this, SLOT(addObjectsToTask()));

    connect(ui->newTaskButton, SIGNAL(clicked()), this, SLOT(startTaskWizard()));

    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(openContextMenu(QPoint)));
    connect(ui->treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(itemExpandedSlot(QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(itemClicked(QTreeWidgetItem*)));

    // Disable OK button if Action == Indicate
    if (Indicate == m_currentAction)
        ui->okButton->setEnabled(false);

    if (! fileNamesActions.isEmpty())
    {
        const QFileInfo fi(fileNamesActions.first().first);
        m_workingDir = fi.absolutePath();
        setWindowTitle(fi.fileName());
    }
    else
    {
        m_workingDir = QLatin1String("C:\\");
    }

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
//    ui->treeWidget->setDragEnabled(true);

    emit setConnectionDialogVisibility(true);

    if (!SynergyPlugin::instance()->isSynergyStarted())
    {
        WorkerThreadPair workerThread = SynergyPlugin::instance()->startSynergyAsync(m_workingDir);
        connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
                this, SLOT(refreshTaskList()));
    }
    else
    {
        refreshTaskList();
    }
}

TasksDialog::~TasksDialog()
{
    delete ui;
}

QString TasksDialog::getChosenTaskNumber() const
{
    return m_chosenTask;
}

void TasksDialog::taskListDownloaded(SynergyResponse response)
{
    if (response.error)
    {
        error(response.stdErr);
        return;
    }

    ui->treeWidget->clear();

    if (response.stdOut.isEmpty())
    {
        m_taskListReceived = true;
        ui->stackedWidget->setCurrentIndex(0);
        return;
    }

    QStringList taskList = response.stdOut.split(QLatin1String("\n"), QString::SkipEmptyParts);
    foreach(QString task, taskList)
    {
        int spaceIndex = task.indexOf(QLatin1Char(' '));
        QString objectName = task.left(spaceIndex);
        QString displayName = task.right(task.length() - spaceIndex);

        QTreeWidgetItem *treeItem = new QTreeWidgetItem(ui->treeWidget);
        treeItem->setText(0, displayName);
        treeItem->setData(0, Qt::UserRole, objectName);
        treeItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
    m_taskListReceived = true;
    ui->stackedWidget->setCurrentIndex(0);
}

void TasksDialog::error(QString errorMessage)
{
    QMessageBox::information(0, tr("Error"), errorMessage);
    reject();
}

QString TasksDialog::obtainTaskNumber(QString task)
{
    QString chosenTask = task;
    int hashPos = chosenTask.indexOf(QLatin1Char('#')) + 1;
    int colonPos = chosenTask.indexOf(QLatin1Char(':'));
    chosenTask = chosenTask.mid(hashPos, colonPos - hashPos);

    return chosenTask;
}

void TasksDialog::addObjectsToTask()
{
    if (ui->treeWidget->currentItem() &&
        !ui->treeWidget->currentItem()->text(0).isEmpty())
    {
        ui->stackedWidget->setCurrentIndex(1);
        ui->progressBar->setMaximum(m_fileNamesActions.count());
        m_pendingActions = m_fileNamesActions.count();
        QStringList args;
        foreach (StringActionPair fileNameAction, m_fileNamesActions)
        {
            args.clear();

            const QFileInfo fi(fileNameAction.first);

            QString workingDir = fi.absolutePath();
            QString file = fi.fileName();

            if (Checkout == fileNameAction.second)
            {
                args << QLatin1String("checkout") << file;
            }
            else if (Add == fileNameAction.second)
            {
                args << QLatin1String("create")
                     << workingDir + QLatin1Char('/') + file;
            }

            args << QLatin1String("-task") << obtainTaskNumber(ui->treeWidget->currentItem()->text(0));

            SynergyPlugin::instance()->runSynergy(workingDir, args);

            ui->progressBar->setValue(ui->progressBar->value() + 1);
        }

        accept();
    }
}

void TasksDialog::startTaskWizard()
{   
    TaskWizard *taskWizard = new TaskWizard(m_workingDir);
    connect(taskWizard, SIGNAL(finished(int)), taskWizard, SLOT(deleteLater()));
    connect(taskWizard, SIGNAL(taskCreatedSignal()), this, SLOT(refreshTaskList()));
    taskWizard->exec();
}

void TasksDialog::refreshTaskList()
{
    emit setConnectionDialogVisibility(false);
    m_taskListReceived = false;
    ui->stackedWidget->setCurrentIndex(1);
    ui->progressBar->setMaximum(0);

    QStringList args(QLatin1String("task"));
    args << QLatin1String("-query")
         << QLatin1String("-scope") << QLatin1String("all_my_assigned")
         << QLatin1String("-unnumbered")
         << QLatin1String("-format") << QLatin1String("%objectname %created_in#%task_number: %task_synopsis");

    WorkerThreadPair workerThread = SynergyPlugin::instance()->runSynergyAsync(m_workingDir, args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(taskListDownloaded(SynergyResponse)));
}

void TasksDialog::openContextMenu(const QPoint &point)
{
    QPoint globalPos = ui->treeWidget->mapToGlobal(point);

    QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
    if (item && item->parent())
    {
        QMenu myMenu(this);

        QAction *delAction = myMenu.addAction(QLatin1String("Delete"));
        connect(delAction, SIGNAL(triggered()), this, SLOT(deleteObject()));

        QMenu *moveMenu = myMenu.addMenu(QLatin1String("Move to task:"));
        for( int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i )
        {
            QTreeWidgetItem *treeItem = ui->treeWidget->topLevelItem(i);

            if (treeItem->text(0) == item->parent()->text(0))
                continue;

            QAction *taskAction = moveMenu->addAction(treeItem->text(0));
            taskAction->setData(treeItem->data(0, Qt::UserRole));
            connect(taskAction, SIGNAL(triggered()), this, SLOT(moveObjectToTask()));
        }

        myMenu.exec(globalPos);
    }
    else if (item)
    {
        QMenu myMenu(this);

        QAction *delAction = myMenu.addAction(QLatin1String("Delete task"));
        connect(delAction, SIGNAL(triggered()), this, SLOT(deleteTask()));

//        QAction *propertiesAction = myMenu.addAction(QLatin1String("Properties"));
//        connect(propertiesAction, SIGNAL(triggered()), this, SLOT(showProperties()));

        myMenu.exec(globalPos);
    }
}

void TasksDialog::itemExpandedSlot(QTreeWidgetItem *expandedItem)
{
    QStringList args(QLatin1String("task"));
    args << QLatin1String("-show")
         << QLatin1String("objects")
         << QLatin1String("-unnumbered")
         << QLatin1String("-format") << QLatin1String("%name %objectname %version %cvtype")
         << obtainTaskNumber(expandedItem->text(0));

    WorkerThreadPair workerThread = SynergyPlugin::instance()->runSynergyAsync(m_workingDir, args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(taskObjectsDownloaded(SynergyResponse)));

    ui->treeWidget->setCursor(Qt::WaitCursor);
}

void TasksDialog::itemClicked(QTreeWidgetItem *expandedItem)
{
    if (Indicate == m_currentAction)
    {
        if (!expandedItem->parent())
        {
            m_chosenTask = obtainTaskNumber(ui->treeWidget->currentItem()->text(0));
            ui->okButton->setEnabled(true);
        }
        else
        {
            m_chosenTask.clear();
            ui->okButton->setEnabled(false);
        }
    }
}

void TasksDialog::taskObjectsDownloaded(SynergyResponse response)
{
    QStringList objectList = response.stdOut.split(QLatin1String("\n"), QString::SkipEmptyParts);
    QString taskName = objectList.first();
    QString taskNumber = obtainTaskNumber(taskName);
    objectList.removeAt(0);

    QTreeWidgetItem *parent = 0;
    for( int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i )
    {
        QTreeWidgetItem *treeItem = ui->treeWidget->topLevelItem(i);
        if (treeItem->data(0, Qt::UserRole).toString().contains(taskNumber))
        {
            parent = treeItem;
            break;
        }
    }
    if (parent == 0)
        return;

    int childCount = parent->childCount();
    for( int i = 0; i < childCount; ++i )
    {
        parent->removeChild(parent->child(0));
    }

    foreach(QString object, objectList)
    {
        QStringList nameObjectnameVersion = object.split(QLatin1Char(' '), QString::SkipEmptyParts);
        if (nameObjectnameVersion.size() < 3)
                continue;
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(parent);
        treeItem->setText(0, nameObjectnameVersion[0]);
        treeItem->setData(0, Qt::UserRole, nameObjectnameVersion[1]);
        treeItem->setData(0, Qt::UserRole + 1, nameObjectnameVersion[2]);

        if (nameObjectnameVersion[3] == QLatin1String("dir"))
            treeItem->setIcon(0, QIcon(QLatin1String(":/icons/folder.png")));
        else
            treeItem->setIcon(0, QIcon(QLatin1String(":/icons/file.png")));

        treeItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
    }

    parent->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    ui->treeWidget->unsetCursor();
}

void TasksDialog::deleteObject()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();

    m_pendingActions = selectedItems.count();
    ui->stackedWidget->setCurrentIndex(1);
    foreach (QTreeWidgetItem *selectedItem, selectedItems)
    {
        QTreeWidgetItem *task = selectedItem->parent();
        if (!task)
        {
            m_pendingItemsCounterMutex.lock();
            --m_pendingActions;
            m_pendingItemsCounterMutex.unlock();

            continue;
        }

        QString objectName = selectedItem->data(0, Qt::UserRole).toString();
        QString version = selectedItem->data(0, Qt::UserRole + 1).toString();
        bool replace = (QLatin1String("1") == version) ? false : true;
        SynergyPlugin::instance()->vcsDelete(m_workingDir, objectName, replace);
        delete selectedItem;
    }
    ui->stackedWidget->setCurrentIndex(0);
}

void TasksDialog::deleteTask()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();

    m_pendingActions = selectedItems.count();
    ui->stackedWidget->setCurrentIndex(1);
    foreach (QTreeWidgetItem *selectedItem, selectedItems)
    {
        if (selectedItem->parent())
        {
            requestCompleted();
            continue;
        }

        QString objectName = selectedItem->data(0, Qt::UserRole).toString();
        WorkerThreadPair workerThread = SynergyPlugin::instance()->vcsDeleteAsync(m_workingDir, objectName, false);
        connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(requestCompleted()));
    }
}

void TasksDialog::moveObjectToTask()
{
    QString taskToRelateObject = qobject_cast<QAction *>(sender())->data().toString();

    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();

    m_pendingActions = 2 * selectedItems.count();
    ui->stackedWidget->setCurrentIndex(1);
    foreach (QTreeWidgetItem *selectedItem, selectedItems)
    {
        QTreeWidgetItem *task = selectedItem->parent();
        if (!task)
        {
            m_pendingItemsCounterMutex.lock();
            --m_pendingActions;
            m_pendingItemsCounterMutex.unlock();

            continue;
        }

        QString objectName = selectedItem->data(0, Qt::UserRole).toString();

        QStringList relateArgs(QLatin1String("task"));
        relateArgs << QLatin1String("-relate")
                   << taskToRelateObject
                   << QLatin1String("-object")
                   << objectName;

        WorkerThreadPair workerThread = SynergyPlugin::instance()->runSynergyAsync(m_workingDir, relateArgs);
        connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(requestCompleted()));

        QString tasktToUnrelate = task->data(0, Qt::UserRole).toString();

        QStringList unRelateArgs(QLatin1String("task"));
        unRelateArgs << QLatin1String("-unrelate")
                     << tasktToUnrelate
                     << QLatin1String("-object")
                     << objectName;

        workerThread = SynergyPlugin::instance()->runSynergyAsync(m_workingDir, unRelateArgs);
        connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(requestCompleted()));
    }
}

void TasksDialog::requestCompleted()
{
    m_pendingItemsCounterMutex.lock();
    if (--m_pendingActions == 0)
    {
        refreshTaskList();
    }
    m_pendingItemsCounterMutex.unlock();
}

void TasksDialog::objectDeleted(SynergyResponse response)
{
    m_pendingItemsCounterMutex.lock();
    if (--m_pendingActions == 0)
    {
        ui->stackedWidget->setCurrentIndex(0);
    }
    m_pendingItemsCounterMutex.unlock();

    if (response.error)
        return;
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();
    foreach (QTreeWidgetItem *selectedItem, selectedItems)
    {
        if (response.stdOut.contains(selectedItem->data(0, Qt::UserRole).toString()))
        {
            delete selectedItem;
            return;
        }
    }
}

void TasksDialog::restorePermissions()
{
    foreach (StringActionPair fileNameAction, m_fileNamesActions)
    {
        if (fileNameAction.second == Checkout)
            QFile::setPermissions(fileNameAction.first, QFile::ReadUser | QFile::ReadOther | QFile::ReadOwner | QFile::ReadGroup);
    }
}

void TasksDialog::reject()
{
    restorePermissions();
    QDialog::reject();
}
