#ifndef CHECKOUTDIALOG_H
#define CHECKOUTDIALOG_H

#include "synergyplugin.h"

#include <QDialog>
#include <QtConcurrent>
#include <QTreeWidgetItem>
#include <QMutex>

namespace Synergy {
namespace Internal {

namespace Ui {
class TasksDialog;
}

class TasksDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TasksDialog(DialogAction action, ObjectActionList fileNamesActions = ObjectActionList(), QWidget *parent = 0);
    ~TasksDialog();

    QString getChosenTaskNumber() const;

signals:
    void setConnectionDialogVisibility(bool visible);

public slots:
    void taskListDownloaded(SynergyResponse response);
    void addObjectsToTask();
    void startTaskWizard();
    void refreshTaskList();
    void openContextMenu(const QPoint& point);
    void itemExpandedSlot(QTreeWidgetItem* expandedItem);
    void itemClicked(QTreeWidgetItem* expandedItem);
    void taskObjectsDownloaded(SynergyResponse response);
    void deleteObject();
    void deleteTask();
    void moveObjectToTask();
    void requestCompleted();
    void objectDeleted(SynergyResponse response);
    void restorePermissions();
    virtual void reject();

private:
    void error(QString errorMessage);
    QString obtainTaskNumber(QString task);

    ObjectActionList m_fileNamesActions;
    QStringList      m_taskList;
    QString          m_chosenTask;
    QString          m_workingDir;

    QFuture<bool> m_future;

    int m_pendingActions;
    QMutex m_pendingItemsCounterMutex;

    bool m_taskListReceived;

    DialogAction m_currentAction;

    Ui::TasksDialog *ui;
};

}
}

#endif // CHECKOUTDIALOG_H
