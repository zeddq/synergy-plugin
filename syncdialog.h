#ifndef SYNCDIALOG_H
#define SYNCDIALOG_H

#include <QDialog>
#include "synergyplugin.h"

namespace Synergy {
namespace Internal {

namespace Ui {
class SyncDialog;
}

class SyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SyncDialog(QString workingDir = QLatin1String("C:"),
                        QString objectName = QLatin1String(""), QWidget *parent = 0);
    ~SyncDialog();

signals:
    void setConnectionDialogVisibility(bool visible);

private slots:
    void startReconcile();
    void reconcileResult(SynergyResponse response);
    void openContextMenu(const QPoint& point);
    void saveToDatabase();
    void rejectFromWA();
    void diffFiles();

private:
    void updateDialogState(bool active);
    void error(QString errorMessage);
    QStringList getObjectNamesFromList();

    Ui::SyncDialog *ui;
    QString m_workingDir;
    QString m_objectName;
};

}
}

#endif // SYNCDIALOG_H
