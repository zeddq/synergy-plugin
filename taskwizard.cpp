#include "taskwizard.h"
#include "ui_taskwizard.h"
#include "synergyplugin.h"
#include "asyncworker.h"

#include "synergyconstants.h"

#include <coreplugin/icore.h>

#include <QLabel>
#include <QGridLayout>
#include <QProgressBar>
#include <QThread>
#include <QAbstractButton>
#include <QSettings>

using namespace Synergy;
using namespace Synergy::Internal;

TaskWizard::TaskWizard(QString workingDir, QWidget *parent) :
    QWizard(parent),
    m_workingDir(workingDir),
    ui(new Ui::TaskWizard)
{
    ui->setupUi(this);

    QStringList args(QLatin1String("query"));
    args    << QLatin1String("-type") << QLatin1String("releasedef")
            << QLatin1String("active=TRUE")
            << QLatin1String("-format") << QLatin1String("%name/%version")
            << QLatin1String("-unnumbered");

    SynergyResponse response =
        SynergyPlugin::instance()->runSynergy(workingDir, args);

    QStringList releaseList = response.stdOut.split(QLatin1String("\n"), QString::SkipEmptyParts);

    addPage(createFirstPage(releaseList));
}

TaskWizard::~TaskWizard()
{
    delete ui;
}

void TaskWizard::accept()
{
    if (m_synopsisLineEdit->text().isEmpty()        ||
        m_descriptionLineEdit->text().isEmpty()     ||
        m_releaseComboBox->currentText().isEmpty()  ||
        m_subsystemComboBox->currentText().isEmpty())
    {
        return;
    }

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::groupS));
    settings->setValue(QLatin1String(Constants::synopsisKeyS), m_synopsisLineEdit->text());
    settings->setValue(QLatin1String(Constants::releaseKeyS), m_releaseComboBox->currentText());
    settings->setValue(QLatin1String(Constants::subsystemKeyS), m_subsystemComboBox->currentText());
    settings->endGroup();

    QStringList args(QLatin1String("task"));
    args    << QLatin1String("-create")
            << QLatin1String("-synopsis") << m_synopsisLineEdit->text()
            << QLatin1String("-current")
            << QLatin1String("-description") << m_descriptionLineEdit->text()
            << QLatin1String("-release") << m_releaseComboBox->currentText()
            << QLatin1String("-subsystem") << m_subsystemComboBox->currentText()
            << QLatin1String("-resolver") << SynergyPlugin::instance()->settings().m_uid;

    WorkerThreadPair workerThread = SynergyPlugin::instance()->runSynergyAsync(m_workingDir, args);
    connect(workerThread.first, SIGNAL(resultDownloaded(SynergyResponse)), this, SLOT(taskCreated()));

    for(int i = 0; i < currentPage()->layout()->count(); ++i)
    {
        QWidget *widget = currentPage()->layout()->itemAt(i)->widget();
        if (widget)
            widget->setVisible(false);

        this->button(NextButton)->setEnabled(false);
        this->button(FinishButton)->setEnabled(false);
    }

    QProgressBar *progressBar = new QProgressBar;
    progressBar->setMaximum(0);
    progressBar->setMinimum(0);
    currentPage()->layout()->addWidget(progressBar);
}

void TaskWizard::taskCreated()
{
    emit taskCreatedSignal();
    QWizard::accept();
}

QWizardPage *TaskWizard::createFirstPage(QStringList releaseList)
{
    QWizardPage *firstPage = new QWizardPage(this);
    firstPage->setTitle(QLatin1String("Task creation"));
    firstPage->setSubTitle(QLatin1String("Please fill all fields."));

    QLabel *synopsisLabel = new QLabel(QLatin1String("Synopsis:"));
    m_synopsisLineEdit = new QLineEdit;

    QLabel *desriptionLabel = new QLabel(QLatin1String("Description:"));
    m_descriptionLineEdit = new QLineEdit;

    QLabel *releaseLabel = new QLabel(QLatin1String("Release:"));
    m_releaseComboBox = new QComboBox;
    m_releaseComboBox->addItems(releaseList);

    QLabel *subsystemLabel = new QLabel(QLatin1String("Subsystem:"));
    m_subsystemComboBox = new QComboBox;
    m_subsystemComboBox->addItems(Synergy::Constants::VP2_SUBSYSTEMS);

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::groupS));
    m_synopsisLineEdit->setText(settings->value(QLatin1String(Constants::synopsisKeyS), QLatin1String("")).toString());
    m_releaseComboBox->setCurrentText(settings->value(QLatin1String(Constants::releaseKeyS), QLatin1String("")).toString());
    m_subsystemComboBox->setCurrentText(settings->value(QLatin1String(Constants::subsystemKeyS), QLatin1String("")).toString());
    settings->endGroup();

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(synopsisLabel, 0, 0);
    layout->addWidget(m_synopsisLineEdit, 0, 1);
    layout->addWidget(desriptionLabel, 1, 0);
    layout->addWidget(m_descriptionLineEdit, 1, 1);
    layout->addWidget(releaseLabel, 2, 0);
    layout->addWidget(m_releaseComboBox, 2, 1);
    layout->addWidget(subsystemLabel, 3, 0);
    layout->addWidget(m_subsystemComboBox, 3, 1);
    firstPage->setLayout(layout);

    return firstPage;
}
