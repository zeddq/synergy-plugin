#ifndef FILESTOSYNCDIALOG_H
#define FILESTOSYNCDIALOG_H

#include "synergyplugin.h"

#include <QDialog>

namespace Synergy {
namespace Internal {


namespace Ui {
class FilesToSyncDialog;
}

class FilesToSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilesToSyncDialog(QWidget *parent = 0);
    ~FilesToSyncDialog();

    void addFileToList(const QString &fileName, DialogAction dialogAction);

public slots:
    virtual void reject();

private slots:
    void sendFilesList();
    void restorePermissions();

private:
    Ui::FilesToSyncDialog *ui;
};

}
}

#endif // FILESTOSYNCDIALOG_H
