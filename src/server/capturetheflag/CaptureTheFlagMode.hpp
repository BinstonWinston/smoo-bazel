#pragma once

#include <math.h>
#include "al/camera/CameraTicket.h"
#include "layouts/CaptureTheFlagIcon.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/capturetheflag/CaptureTheFlagConfigMenu.hpp"
#include "FlagTeam.hpp"
#include "server/capturetheflag/StageManager.hpp"


struct CaptureTheFlagInfo : GameModeInfoBase {
    CaptureTheFlagInfo() { mMode = GameMode::CAPTURE_THE_FLAG; }
    FlagTeam mPlayerTeam = FlagTeam::INVALID;
    bool mIsUseGravity = false;
    bool mIsUseGravityCam = false;
    FlagTeam mHeldFlag = FlagTeam::INVALID;
    FlagTeam mWinningTeam = FlagTeam::INVALID;
    GameTime mRoundTime;
};

class CaptureTheFlagMode : public GameModeBase {
    public:

        CaptureTheFlagMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        virtual void begin() override;
        virtual void update() override;
        virtual void end() override;

        void resetGameInfo(); // Resets the info to all default values, useful when starting a new game/round

        FlagTeam getPlayerTeam() const { return mInfo->mPlayerTeam; };

        void setPlayerTeam(FlagTeam team) { mInfo->mPlayerTeam = team; }

        void enableGravityMode() {mInfo->mIsUseGravity = true;}
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }

        void setCameraTicket(al::CameraTicket *ticket) {mTicket = ticket;}

        FlagTeam getTeamSide() const; // Returns the flag team that owns the side of the map the player is on

    private:
        static float constexpr TELEPORT_TIMER_MODE_FREEZE_THRESHOLD_SECONDS = 5.0f;
        float mInvulnTime = 0.0f;
        CaptureTheFlagIcon *mModeLayout = nullptr;
        CaptureTheFlagInfo* mInfo = nullptr;
        al::CameraTicket *mTicket = nullptr;
        float m_teleportTimer = 0;

        PlayerActorBase* getLocalPlayer() const;

        void updateBorderPosition();
        void updateFlags();
        void checkTagged();
        void processPlayerTagged();
        void checkPickUpOrDropFlag(PlayerActorBase* playerBase);
        void sendFlagHoldStateUpdateIfHeld(PlayerActorBase* playerBase);
        void checkWin(PlayerActorBase* playerBase);

        void dropHeldFlag();
        void respawnHeldFlag();

        bool changeStage(StageData const& stageData);
        bool changeStage(StageData const& stageData, const char* changeID);
        bool changeStage(ChangeStageInfo const& info);

        void queueHomeTeleport();

        bool isPlayerOnSelfTeamSide() const; // Returns true iff the player is on their own team's side
        std::optional<FlagTeam> getOpposingTeam(FlagTeam team) const;
        bool isInvulnerable() const;
        StageData getLocalPlayerStageData() const;
        bool isInSubArea() const;

        // Calculate which team's side the given position is on relative to this border
        FlagTeam getTeamSide(sead::Vector3f const& pos, BorderTransform const& borderTransform) const;
};