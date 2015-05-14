#ifndef TASKWIZARD_H
#define TASKWIZARD_H

#include <QWizard>
#include <QComboBox>
#include <QLineEdit>

namespace Synergy {
namespace Internal {

namespace Ui {
class TaskWizard;
}

class TaskWizard : public QWizard
{
    Q_OBJECT

public:
    explicit TaskWizard(QString workingDir, QWidget *parent = 0);
    ~TaskWizard();

    void accept();

public slots:
    void taskCreated();

signals:
    void taskCreatedSignal();

private:
    QWizardPage *createFirstPage(QStringList releaseList);

    Ui::TaskWizard *ui;
    QString m_workingDir;
    QLineEdit *m_synopsisLineEdit;
    QLineEdit *m_descriptionLineEdit;
    QComboBox *m_releaseComboBox;
    QComboBox *m_subsystemComboBox;
};

}
}
#endif // TASKWIZARD_H
