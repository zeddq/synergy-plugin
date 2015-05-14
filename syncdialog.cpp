#include "syncdialog.h"
#include "ui_syncdialog.h"

#include "asyncworker.h"
#include "synergyplugin.h"
#include "asyncworker.h"
#include "tasksdialog.h"

#include <QThread>
#include <QMessageBox>
#include <QMenu>
#include <QDir>
#include <QFileInfo>

using namespace Synergy;
using namespace Synergy::Internal;

SyncDialog::SyncDialog(QString workingDir, QString objectName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SyncDialog),
    m_workingDir(workingDir),
    m_objectName(objectName)
{
    ui->setupUi(this);
    this->setWindowTitle(this->windowTitle() + QLatin1Char(' ') + objectName);
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(accept()));

    connect(this, SIGNAL(setConnectionDialogVisibility(bool)),
            SynergyPlugin::instance(), SLOT(setConnectionDialogVisibilitySlot(bool)));

    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(openContextMenu(QPoint)));

    ui->stackedWidget->setCurrentIndex(0);

    QFont font;
    font.setPointSize(8);
    ui->tableWidget->setFont(font);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << QLatin1String("Name")
                                                      << QLatin1String("Conflict"));
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    emit setConnectionDialogVisibility(true);

    if (!SynergyPlugin::instance()->isSynergyStarted())
    {
        WorkerThreadPair workerThread = SynergyPlugin::instance()->startSynergyAsync(m_workingDir);
        // TO UNCOMMENT
        //connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)),
        //        this, SLOT(startReconcile()));
        startReconcile();
        connect(this, SIGNAL(finished(int)), workerThread.second, SLOT(terminate()));
    }
    else
        startReconcile();
}

SyncDialog::~SyncDialog()
{
    delete ui;
}

void SyncDialog::startReconcile()
{
    emit setConnectionDialogVisibility(false);

    QStringList args(QLatin1String("reconcile"));
    args    << QLatin1String("-consider_uncontrolled")
            << QLatin1String("-nocolumn_header")
            << QLatin1String("-recurse")
            << QLatin1String("-show")
            << m_objectName;

    WorkerThreadPair workerThread = SynergyPlugin::instance()->runSynergyAsync(m_workingDir, args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(reconcileResult(SynergyResponse)));
}

void SyncDialog::reconcileResult(SynergyResponse response)
{
    ui->stackedWidget->setCurrentIndex(1);
    if (response.error)
    {
        error(response.stdErr);
        reject();
    }

    if (response.stdOut.isEmpty())
        return;

    QStringList conflictList = response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    foreach(QString conflict, conflictList)
    {
        int spaceIndex = conflict.indexOf(QLatin1Char(' '));
        QString objectName = conflict.left(spaceIndex).trimmed();
        QString conflictName = conflict.right(conflict.length() - spaceIndex).trimmed();
        if (1 != objectName.indexOf(QLatin1Char(':')))
            continue;

        QTableWidgetItem *object = new QTableWidgetItem(objectName);
        QTableWidgetItem *conf = new QTableWidgetItem(conflictName);

        if (!conflictName.startsWith(QLatin1String("Uncontrolled"), Qt::CaseInsensitive))
        {
            QFont font;
            font.setBold(true);
            font.setPointSize(9);
            object->setFont(font);
            conf->setFont(font);

            object->setData(Qt::UserRole, true);
            conf->setData(Qt::UserRole, true);
        }
        ui->tableWidget->insertRow(ui->tableWidget->rowCount());
        ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 0, object);
        ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 1, conf);
    }
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(15);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    ui->tableWidget->sortByColumn(0);
    ui->tableWidget->sortByColumn(1, Qt::DescendingOrder);
}

void SyncDialog::openContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidget->itemAt(point);
    if (item)
    {
        QMenu myMenu(this);

        QAction *saveAction = myMenu.addAction(QLatin1String("Save"));
        connect(saveAction, SIGNAL(triggered()), this, SLOT(saveToDatabase()));

        QAction *rejectAction = myMenu.addAction(QLatin1String("Reject"));
        connect(rejectAction, SIGNAL(triggered()), this, SLOT(rejectFromWA()));

        if (item->data(Qt::UserRole).toBool() == true)
        {
            QAction *diffAction = myMenu.addAction(QLatin1String("Diff"));
            connect(diffAction, SIGNAL(triggered()), this, SLOT(diffFiles()));
        }
        myMenu.exec(QCursor::pos());
    }
}

void SyncDialog::saveToDatabase()
{
    TasksDialog taskDialog(Indicate);
    if (Accepted == taskDialog.exec())
    {
        QString chosenTask = taskDialog.getChosenTaskNumber();
        if (chosenTask.isEmpty())
            return;

        QStringList objectNames = getObjectNamesFromList();

        objectNames.sort();
        ui->stackedWidget->setEnabled(false);
        foreach(QString objectName, objectNames)
        {
            // TODO: Przejsc do odpowiedniego folderu i tam stworzyc folder/plik
            QFileInfo fileInfo(objectName);
            QDir dir(fileInfo.absoluteFilePath());
            dir.cdUp();
            m_workingDir = dir.absolutePath();
            QStringList args;
            args    << QLatin1String("reconcile")
                    << QLatin1String("-update_db")
                    << QLatin1String("-consider_uncontrolled")
                    << QLatin1String("-missing_wa_file")
                    << QLatin1String("-task") << chosenTask
                    << fileInfo.fileName();
            SynergyResponse response = SynergyPlugin::instance()->runSynergy(m_workingDir, args);
            if (response.error)
            {
                error(response.stdErr);
                reject();
            }

            QList<QTableWidgetItem *> tableItemList = ui->tableWidget->findItems(objectName, Qt::MatchExactly);
            foreach(QTableWidgetItem *item, tableItemList)
            {
                qDebug() << "znalazlo item" << item;
                int row = item->row();
                ui->tableWidget->removeRow(row);
            }
        }
        ui->stackedWidget->setEnabled(true);
    }
}

void SyncDialog::rejectFromWA()
{
    QStringList objectNames = getObjectNamesFromList();

    objectNames.sort();
    ui->stackedWidget->setEnabled(false);
    foreach(QString objectName, objectNames)
    {
        QFileInfo fileInfo(objectName);
        QDir dir(fileInfo.absoluteFilePath());
        dir.cdUp();
        m_workingDir = dir.absolutePath();
        QStringList args;
        args    << QLatin1String("reconcile")
                << QLatin1String("-update_wa")
                << QLatin1String("-consider_uncontrolled")
                << QLatin1String("-missing_wa_file")
                << fileInfo.fileName();
        SynergyResponse response = SynergyPlugin::instance()->runSynergy(m_workingDir, args);
        if (response.error)
        {
            error(response.stdErr);
            reject();
        }

        QList<QTableWidgetItem *> tableItemList = ui->tableWidget->findItems(objectName, Qt::MatchExactly);
        foreach(QTableWidgetItem *item, tableItemList)
        {
            qDebug() << "znalazlo item" << item;
            int row = item->row();
            ui->tableWidget->removeRow(row);
        }
    }
    ui->stackedWidget->setEnabled(true);
}

void SyncDialog::diffFiles()
{
    QStringList objectNames = getObjectNamesFromList();

    foreach(QString objectName, objectNames)
    {
        // TODO: Przejsc do odpowiedniego folderu i tam stworzyc folder/plik
        QFileInfo fileInfo(objectName);
        QDir dir(fileInfo.absoluteFilePath());
        dir.cdUp();
        m_workingDir = dir.absolutePath();
        SynergyPlugin::instance()->diffWithCurrentVersion(fileInfo.fileName(), m_workingDir);
    }
}

void SyncDialog::error(QString errorMessage)
{
    QMessageBox::information(0, tr("Error"), errorMessage);
}

QStringList SyncDialog::getObjectNamesFromList()
{
    QList<QTableWidgetItem *> objects = ui->tableWidget->selectedItems();
    QStringList objectNames;
    foreach(QTableWidgetItem *object, objects)
    {
        if (object->column() == 0)
            objectNames << object->text();
    }
    return objectNames;
}
