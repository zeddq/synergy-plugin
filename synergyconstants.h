#ifndef SYNERGYCONSTANTS_H
#define SYNERGYCONSTANTS_H

#include <QString>

namespace Synergy {
namespace Constants {

const char ACTION_ID[] = "Synergy.Action";
const char MENU_ID[] = "Synergy.Menu";

const char PROJECTTREE_DIFF[] = "ProjectExplorer.Diff";

const char VCS_ID_SYNERGY[] = "S.Synergy";
const char SYNERGY_CONTEXT[] = "Synergy Context";

const QStringList VP2_SUBSYSTEMS = {QLatin1String("VP2_3RD"), QLatin1String("VP2_BSP"), QLatin1String("VP2_CONN"),
                                    QLatin1String("VP2_CONN_ARC"), QLatin1String("VP2_CVTO"), QLatin1String("VP2_CVTO_ARC"),
                                    QLatin1String("VP2_DAB"), QLatin1String("VP2_DAB_ARC"), QLatin1String("VP2_DSP"),
                                    QLatin1String("VP2_DSP_ARC"), QLatin1String("VP2_ENT"), QLatin1String("VP2_ENT_ARC"),
                                    QLatin1String("VP2_HS"), QLatin1String("VP2_HS_ARC"), QLatin1String("VP2_NAV"),
                                    QLatin1String("VP2_NAV_ARC"), QLatin1String("VP2_PRIV"), QLatin1String("VP2_PUB"),
                                    QLatin1String("VP2_RAD"), QLatin1String("VP2_RAD_ARC"), QLatin1String("VP2_SDARS"),
                                    QLatin1String("VP2_SDARS_ARC"), QLatin1String("VP2_SSW"), QLatin1String("VP2_SSW_ARC"),
                                    QLatin1String("VP2_SYS_ARC"), QLatin1String("VP2_TOP")};

const char groupS[] = "Synergy";
const char binaryPathKeyS[] = "BinaryPath";
const char compareToolPathKeyS[] = "CompareToolPath";
const char historyCountKeyS[] = "HistoryCount";
const char timeOutKeyS[] = "TimeOut";
const char uidKeyS[] = "Uid";
const char passwordKeyS[] = "Password";
const char synopsisKeyS[] = "Synopsis";
const char releaseKeyS[] = "Release";
const char subsystemKeyS[] = "Subsystem";

enum { defaultTimeOutS = 60, defaultHistoryCount = 50 };

} // namespace Synergy
} // namespace Constants

#endif // SYNERGYCONSTANTS_H

