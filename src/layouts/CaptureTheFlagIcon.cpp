#include "layouts/CaptureTheFlagIcon.h"
#include <cstdio>
#include <cstring>
#include "puppets/PuppetInfo.h"
#include "al/string/StringTmp.h"
#include "prim/seadSafeString.h"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/capturetheflag/CaptureTheFlagMode.hpp"
#include "server/Client.hpp"
#include "al/util.hpp"
#include "logger.hpp"
#include "rs/util.hpp"
#include "main.hpp"

constexpr char* LAYOUT_ACTOR_NAME = "HideAndSeekIcon";
constexpr char* RED_TEAM_ICON = "RedFlagIcon";
constexpr char* BLUE_TEAM_ICON = "BlueFlagIcon";
constexpr char* OFFSIDES_WARNING_PANE = "OffSidesWarning";

CaptureTheFlagIcon::CaptureTheFlagIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {

    al::initLayoutActor(this, initInfo, LAYOUT_ACTOR_NAME, 0);

    mInfo = GameModeManager::instance()->getInfo<CaptureTheFlagInfo>();

    initNerve(&nrvCaptureTheFlagIconEnd, 0);

    al::hidePane(this, RED_TEAM_ICON);
    al::hidePane(this, BLUE_TEAM_ICON);

    
    kill();

}

void CaptureTheFlagIcon::appear() {

    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvCaptureTheFlagIconAppear);

    al::LayoutActor::appear();
}

bool CaptureTheFlagIcon::tryEnd() {
    if (!al::isNerve(this, &nrvCaptureTheFlagIconEnd)) {
        al::setNerve(this, &nrvCaptureTheFlagIconEnd);
        return true;
    }
    return false;
}

bool CaptureTheFlagIcon::tryStart() {

    if (!al::isNerve(this, &nrvCaptureTheFlagIconWait) && !al::isNerve(this, &nrvCaptureTheFlagIconAppear)) {

        appear();

        return true;
    }

    return false;
}

void CaptureTheFlagIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvCaptureTheFlagIconWait);
    }
}

void CaptureTheFlagIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    GameTime &curTime = mInfo->mRoundTime;

    if (curTime.mHours > 0) {
        al::setPaneStringFormat(this, "TxtCounter", "%01d:%02d:%02d", curTime.mHours, curTime.mMinutes,
                            curTime.mSeconds);
    } else {
        al::setPaneStringFormat(this, "TxtCounter", "%02d:%02d", curTime.mMinutes,
                            curTime.mSeconds);
    }

    if (GameModeManager::instance()->getMode<CaptureTheFlagMode>()->getTeamSide() != mInfo->mPlayerTeam && mInfo->mPlayerTeam != FlagTeam::INVALID) {
        showOffsidesWarning();
    }
    else {
        hideOffsidesWarning();
    }

    if (mInfo->mWinningTeam != FlagTeam::INVALID) {
        al::setPaneStringFormat(this, "TxtGameWinLose", "You %s!\n", (mInfo->mPlayerTeam == mInfo->mWinningTeam) ? "won" : "lost");
    }
    else {
        al::setPaneStringFormat(this, "TxtGameWinLose", "");
    }

    int playerCount = Client::getMaxPlayerCount();

    if (playerCount > 0) {

        char playerNameBuf[0x100] = {0}; // max of 16 player names if player name size is 0x10

        sead::BufferedSafeStringBase<char> playerList =
            sead::BufferedSafeStringBase<char>(playerNameBuf, 0x200);
    
        for (size_t i = 0; i < playerCount; i++) {
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (curPuppet && curPuppet->isConnected && (curPuppet->ctfTeam == mInfo->mPlayerTeam)) {
                playerList.appendWithFormat("%s\n", curPuppet->puppetName);
            }
        }
        
        al::setPaneStringFormat(this, "TxtPlayerList", playerList.cstr());
    }
    
}

void CaptureTheFlagIcon::exeEnd() {

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void CaptureTheFlagIcon::showTeam(FlagTeam team) {
    switch (team) {
        case FlagTeam::BLUE:
            showBlueTeam();
            break;
        case FlagTeam::RED:
            showRedTeam();
            break;
        case FlagTeam::INVALID:
        default:
            al::hidePane(this, RED_TEAM_ICON);
            al::hidePane(this, BLUE_TEAM_ICON);
            break;
    }
}

void CaptureTheFlagIcon::showOffsidesWarning() {
    al::showPane(this, OFFSIDES_WARNING_PANE);
}

void CaptureTheFlagIcon::hideOffsidesWarning() {
    al::hidePane(this, OFFSIDES_WARNING_PANE);
}

void CaptureTheFlagIcon::showBlueTeam() {
    al::hidePane(this, RED_TEAM_ICON);
    al::showPane(this, BLUE_TEAM_ICON);
}

void CaptureTheFlagIcon::showRedTeam() {
    al::hidePane(this, BLUE_TEAM_ICON);
    al::showPane(this, RED_TEAM_ICON);
}

namespace {
    NERVE_IMPL(CaptureTheFlagIcon, Appear)
    NERVE_IMPL(CaptureTheFlagIcon, Wait)
    NERVE_IMPL(CaptureTheFlagIcon, End)
}