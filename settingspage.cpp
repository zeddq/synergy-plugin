/**************************************************************************
**
** Copyright (c) 2014 Brian McGillion
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "synergysettings.h"
#include "synergyplugin.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <synergyconstants.h>

#include <QTextStream>

using namespace Synergy::Internal;
using namespace Synergy;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setPromptDialogTitle(tr("Synergy Command"));
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Synergy.Command.History"));

    m_ui.comparePathChooser->setPromptDialogTitle(tr("Compare Tool"));
    m_ui.comparePathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.comparePathChooser->setHistoryCompleter(QLatin1String("Synergy.Command.Compare"));
}

SynergySettings SettingsPageWidget::settings() const
{
    SynergySettings rc;
    rc.m_synergyBinaryPath = m_ui.commandChooser->rawPath();
    rc.m_compareToolPath = m_ui.comparePathChooser->rawPath();
    rc.m_timeOutS = m_ui.timeout->value();
    rc.m_historyCount = m_ui.logEntriesCount->value();
    rc.m_uid = m_ui.uidLineEdit->text();
    rc.m_password = m_ui.passwordLineEdit->text();
    return rc;
}

void SettingsPageWidget::setSettings(const SynergySettings &s)
{
    m_ui.commandChooser->setPath(s.m_synergyBinaryPath);
    m_ui.comparePathChooser->setPath(s.m_compareToolPath);
    m_ui.timeout->setValue(s.m_timeOutS);
    m_ui.logEntriesCount->setValue(s.m_historyCount);
    m_ui.uidLineEdit->setText(s.m_uid);
    m_ui.passwordLineEdit->setText(s.m_password);
}

SettingsPage::SettingsPage() :
    m_optionsPageWidget(0)
{
    setId(Synergy::Constants::VCS_ID_SYNERGY);
    setDisplayName(tr("Synergy"));
}

QWidget *SettingsPage::widget()
{
    if (!m_optionsPageWidget)
        m_optionsPageWidget = new SettingsPageWidget;
    m_optionsPageWidget->setSettings(SynergyPlugin::instance()->settings());
    return m_optionsPageWidget;
}

void SettingsPage::apply()
{
    SynergyPlugin::instance()->setSettings(m_optionsPageWidget->settings());
}

void SettingsPage::finish()
{
    delete m_optionsPageWidget;
}
