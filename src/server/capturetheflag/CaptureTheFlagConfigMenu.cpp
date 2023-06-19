#include "server/capturetheflag/CaptureTheFlagConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/capturetheflag/CaptureTheFlagMode.hpp"
#include "server/Client.hpp"

CaptureTheFlagConfigMenu::CaptureTheFlagConfigMenu() : GameModeConfigMenu() {}

void CaptureTheFlagConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {
    
}

const sead::WFixedSafeString<0x200> *CaptureTheFlagConfigMenu::getStringData() {
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* gamemodeConfigOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();

    gamemodeConfigOptions->mBuffer[0].copy(u"Toggle CTF Gravity On");
    gamemodeConfigOptions->mBuffer[1].copy(u"Toggle CTF Gravity Off");

    return gamemodeConfigOptions->mBuffer;
}

bool CaptureTheFlagConfigMenu::updateMenu(int selectIndex) {

    CaptureTheFlagInfo *curMode = GameModeManager::instance()->getInfo<CaptureTheFlagInfo>();

    Logger::log("Setting Gravity Mode.\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: {
            if (GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
                curMode->mIsUseGravity = true;
            }
            return true;
        }
        case 1: {
            if (GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
                curMode->mIsUseGravity = false;
            }
            return true;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
    
}