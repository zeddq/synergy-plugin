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

#include "synergysettings.h"
#include "synergyconstants.h"
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>

namespace Synergy {
namespace Internal {

static QString defaultCommand()
{
    return QLatin1String("s:\\cm_syn\\V7.1L.wetp055x\\bin\\ccm.exe");
}

SynergySettings::SynergySettings() :
    m_synergyBinaryPath(defaultCommand()),
    m_historyCount(Constants::defaultHistoryCount),
    m_timeOutS(Constants::defaultTimeOutS)
{
}

void SynergySettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(Constants::groupS));

    m_synergyBinaryPath = settings->value(QLatin1String(Constants::binaryPathKeyS), defaultCommand()).toString();
    m_compareToolPath = settings->value(QLatin1String(Constants::compareToolPathKeyS), QLatin1String("")).toString();
    m_timeOutS          = settings->value(QLatin1String(Constants::timeOutKeyS), Constants::defaultTimeOutS).toInt();
    m_historyCount      = settings->value(QLatin1String(Constants::historyCountKeyS), int(Constants::defaultHistoryCount)).toInt();
    m_uid               = settings->value(QLatin1String(Constants::uidKeyS), QLatin1String("")).toString();
    m_password          = settings->value(QLatin1String(Constants::passwordKeyS), QLatin1String("")).toString();

    settings->endGroup();
}

void SynergySettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(Constants::groupS));

    settings->setValue(QLatin1String(Constants::binaryPathKeyS), m_synergyBinaryPath);
    settings->setValue(QLatin1String(Constants::compareToolPathKeyS), m_compareToolPath);
    settings->setValue(QLatin1String(Constants::timeOutKeyS), m_timeOutS);
    settings->setValue(QLatin1String(Constants::historyCountKeyS), m_historyCount);
    settings->setValue(QLatin1String(Constants::uidKeyS), m_uid);
    settings->setValue(QLatin1String(Constants::passwordKeyS), m_password);

    settings->endGroup();
}

bool SynergySettings::equals(const SynergySettings &s) const
{
    return m_synergyBinaryPath  == s.m_synergyBinaryPath
        && m_historyCount       == s.m_historyCount
        && m_timeOutS           == s.m_timeOutS
        && m_uid                == s.m_uid
        && m_password           == s.m_password
        && m_compareToolPath    == s.m_compareToolPath;
}

} // namespace Internal
} // namespace Synergy
