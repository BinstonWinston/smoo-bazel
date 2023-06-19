#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/capturetheflag/FlagTeam.hpp"

// TODO: kill layout if going through loading zone or paused

class CaptureTheFlagIcon : public al::LayoutActor {
    public:
        CaptureTheFlagIcon(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showTeam(FlagTeam team);
        void showOffsidesWarning();
        void hideOffsidesWarning();
        
        void exeAppear();
        void exeWait();
        void exeEnd();

    private:
        void showBlueTeam();
        void showRedTeam();

        struct CaptureTheFlagInfo *mInfo;
};

namespace {
    NERVE_HEADER(CaptureTheFlagIcon, Appear)
    NERVE_HEADER(CaptureTheFlagIcon, Wait)
    NERVE_HEADER(CaptureTheFlagIcon, End)
}