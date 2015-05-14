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

#ifndef SYNERGYSETTINGS_H
#define SYNERGYSETTINGS_H

#include <vcsbase/vcsbaseclientsettings.h>

namespace Synergy {
namespace Internal {

class SynergySettings
{
public:
    SynergySettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    bool equals(const SynergySettings &s) const;

    inline int timeOutMS() const     { return m_timeOutS * 1000;  }
    inline int longTimeOutMS() const { return m_timeOutS * 10000; }

    QString m_synergyBinaryPath;
    QString m_compareToolPath;
    QString m_uid;
    QString m_password;
    int m_historyCount;
    int m_timeOutS;
};

inline bool operator==(const SynergySettings &p1, const SynergySettings &p2)
    { return p1.equals(p2); }
inline bool operator!=(const SynergySettings &p1, const SynergySettings &p2)
    { return !p1.equals(p2); }

} // namespace Internal
} // namespace Synergy

#endif // SYNERGYSETTINGS_H
