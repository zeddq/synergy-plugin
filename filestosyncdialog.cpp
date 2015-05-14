#include "filestosyncdialog.h"
#include "ui_filestosyncdialog.h"

using namespace Synergy;
using namespace Synergy::Internal;

FilesToSyncDialog::FilesToSyncDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilesToSyncDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(sendFilesList()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(restorePermissions()));
}

FilesToSyncDialog::~FilesToSyncDialog()
{
    delete ui;
}

void FilesToSyncDialog::addFileToList(const QString &fileName, DialogAction dialogAction)
{
    ui->listWidget->addItem(fileName);
    ui->listWidget->item(ui->listWidget->count() - 1) ->setData(Qt::UserRole, dialogAction);
}

void FilesToSyncDialog::reject()
{
    restorePermissions();
    QDialog::reject();
}

void FilesToSyncDialog::sendFilesList()
{
    ObjectActionList filesList;
    for (int i = 0; i < ui->listWidget->count(); ++i)
    {
        filesList.append(StringActionPair(ui->listWidget->item(i)->text(), (DialogAction)ui->listWidget->item(i)->data(Qt::UserRole).toInt()));
    }
    SynergyPlugin::instance()->addFilesToTask(filesList);

    ui->listWidget->clear();
    accept();
}

void FilesToSyncDialog::restorePermissions()
{
    for (int i = 0; i < ui->listWidget->count(); ++i)
    {
        if ((DialogAction)ui->listWidget->item(i)->data(Qt::UserRole).toInt() == Checkout)
            QFile::setPermissions(ui->listWidget->item(i)->text(),
                                  QFile::ReadUser | QFile::ReadOther | QFile::ReadOwner | QFile::ReadGroup);
    }

    ui->listWidget->clear();
}
