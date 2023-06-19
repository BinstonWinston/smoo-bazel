#include "server/capturetheflag/CaptureTheFlagMode.hpp"
#include <cmath>
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "actors/FlagActor.h"
#include "actors/BorderActor.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "heap/seadHeapMgr.h"
#include "layouts/CaptureTheFlagIcon.h"
#include "logger.hpp"
#include "rs/util.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include <heap/seadHeap.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"

#include "basis/seadNew.h"
#include "sead/math/seadMatrixCalcCommon.hpp"
#include "server/capturetheflag/CaptureTheFlagConfigMenu.hpp"

CaptureTheFlagMode::CaptureTheFlagMode(const char* name) : GameModeBase(name) {}

void CaptureTheFlagMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<CaptureTheFlagInfo>();

    if (curGameInfo) Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (CaptureTheFlagInfo*)curGameInfo;
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mRoundTime.mMinutes, mInfo->mRoundTime.mSeconds);
    } else {
        if (curGameInfo) delete curGameInfo;  // attempt to destory previous info before creating new one
        
        mInfo = GameModeManager::instance()->createModeInfo<CaptureTheFlagInfo>();        
    }

    mModeLayout = new CaptureTheFlagIcon("CaptureTheFlagIcon", *info.mLayoutInitInfo);

    mModeLayout->showTeam(FlagTeam::INVALID);
}

void CaptureTheFlagMode::begin() {
    mModeLayout->appear();

    mIsFirstFrame = true;

    mModeLayout->showTeam(getPlayerTeam());

    CoinCounter *coinCollect = mCurScene->mSceneLayout->mCoinCollectLyt;
    CoinCounter* coinCounter = mCurScene->mSceneLayout->mCoinCountLyt;
    MapMini* compass = mCurScene->mSceneLayout->mMapMiniLyt;
    al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->mSceneLayout->mPlayGuideMenuLyt;

    mInvulnTime = 0;

    if(coinCounter->mIsAlive)
        coinCounter->tryEnd();
    if(coinCollect->mIsAlive)
        coinCollect->tryEnd();
    if (compass->mIsAlive)
        compass->end();
    if (playGuideLyt->mIsAlive)
        playGuideLyt->end();

    GameModeBase::begin();
}

void CaptureTheFlagMode::end() {

    mModeLayout->tryEnd();

    CoinCounter *coinCollect = mCurScene->mSceneLayout->mCoinCollectLyt;
    CoinCounter* coinCounter = mCurScene->mSceneLayout->mCoinCountLyt;
    MapMini* compass = mCurScene->mSceneLayout->mMapMiniLyt;
    al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->mSceneLayout->mPlayGuideMenuLyt;

    mInvulnTime = 0.0f;

    if(!coinCounter->mIsAlive)
        coinCounter->tryStart();
    if(!coinCollect->mIsAlive)
        coinCollect->tryStart();
    if (!compass->mIsAlive)
        compass->appearSlideIn();
    if (!playGuideLyt->mIsAlive)
        playGuideLyt->appear();

    GameModeBase::end();
}

void CaptureTheFlagMode::update() {
    mModeLayout->showTeam(getPlayerTeam());

    if (m_teleportTimer > 0) {
        m_teleportTimer -= Time::deltaTime;
        return;
    }

    if (mIsFirstFrame) {

        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }

        mIsFirstFrame = false;
    }

    auto* playerBase = getLocalPlayer();
    if (!playerBase) {
        return;
    }

    updateBorderPosition();
    updateFlags();
    checkTagged();
    checkPickUpOrDropFlag(playerBase);
    sendFlagHoldStateUpdateIfHeld(playerBase);
    checkWin(playerBase);
}

PlayerActorBase* CaptureTheFlagMode::getLocalPlayer() const {
    if (isFirstFrame()) {
        return nullptr;
    }
    auto* player = rs::getPlayerActor(mCurScene);
    if (!player) {
        return nullptr;
    }
    bool isYukimaru = !player->getPlayerInfo(); // if PlayerInfo is a nullptr, that means we're dealing with the bound bowl racer
    if (isYukimaru) {
        return nullptr;
    }
    return player;
}

void CaptureTheFlagMode::resetGameInfo() {
    CaptureTheFlagInfo defaultValue{};
    *mInfo = defaultValue;
}

void CaptureTheFlagMode::updateBorderPosition() {
    auto const borderTransform = StageManager::getBorderTransform(getLocalPlayerStageData());
    if (!borderTransform.has_value()) {
        // Only display the border in the transform is valid
        if (al::isAlive(BorderActor::singleton)) {
            BorderActor::singleton->makeActorDead();
        }
        return;
    }

    if (al::isDead(BorderActor::singleton)) {
        BorderActor::singleton->makeActorAlive();
    }
    auto scale = al::getScale(BorderActor::singleton);
    scale->z = 0.1f;
    scale->y = 300.f;
    scale->x = 50.f;

    sead::Quatf rot{};
    BorderActor::singleton->setTransform(borderTransform.value().position, rot);
    float const s = 500000.f;
    al::setClippingObb(BorderActor::singleton, sead::BoundBox3f(-s, -s, -s, s, s, s));
}

void CaptureTheFlagMode::updateFlags() {
    auto player = getLocalPlayer();
    if (!player) {
        return;
    }
    auto updateFlag = [this, player](FlagTeam team) {
        auto flagActorMaybe = FlagActor::getFlagActorForTeam(team);
        if (!flagActorMaybe.has_value()) {
            return;
        }
        auto flagActor = flagActorMaybe.value();
        auto heldPlayer = flagActor->getHeldPlayer();
        if (!heldPlayer.has_value()) {
            return;
        }
        auto playerTransform = Client::instance()->getPlayerTransformInSameStage(heldPlayer.value());
        if (!playerTransform.has_value()) {
            return;
        }
        flagActor->makeActorAlive();
        sead::Vector3f pos{};
        sead::Quatf rot{};
        std::tie(pos, rot) = playerTransform.value();
        flagActor->setTransform(pos, rot);
    };

    updateFlag(FlagTeam::RED);
    updateFlag(FlagTeam::BLUE);
}

constexpr float TAG_DISTANCE_THRESHOLD = 200.f;
void CaptureTheFlagMode::checkTagged() {
    auto* playerBase = getLocalPlayer();

    if (isInvulnerable()) {
        mInvulnTime += Time::deltaTime;
        return;
    }
    if (getTeamSide() == mInfo->mPlayerTeam) {
        return;
    }

    for (size_t i = 0; i < mPuppetHolder->getSize(); i++)
    {
        PuppetInfo *curInfo = Client::getPuppetInfo(i);

        if (!curInfo) {
            Logger::log("Checking %d, hit bounds %d-%d\n", i, mPuppetHolder->getSize(), Client::getMaxPlayerCount());
            break;
        }

        if(!curInfo->isConnected || !curInfo->isInSameStage) {
            continue;
        }
        if (curInfo->ctfTeam == getPlayerTeam() || curInfo->ctfTeam == FlagTeam::INVALID) {
            // A player from your own team or without a team assignment can't tag you
            continue;
        }

        float pupDist = al::calcDistance(playerBase, curInfo->playerPos); // TODO: remove distance calculations and use hit sensors to determine this

        if(pupDist < TAG_DISTANCE_THRESHOLD && ((PlayerActorHakoniwa*)playerBase)->mDimKeeper->is2DModel == curInfo->is2D) {
            if(!PlayerFunction::isPlayerDeadStatus(playerBase)) {
                processPlayerTagged();
            }
        } else if (PlayerFunction::isPlayerDeadStatus(playerBase)) {
            // Normal non-tagging deaths are fine, just keep playing
        }
    }
}

constexpr float FLAG_PICKUP_DISTANCE_THRESHOLD = TAG_DISTANCE_THRESHOLD;
void CaptureTheFlagMode::checkPickUpOrDropFlag(PlayerActorBase* playerBase) {
    auto const checkButtonPress = al::isPadHoldZL(-1) && al::isPadTriggerZR(-1);
    if (!checkButtonPress) {
        return;
    }

    if (mInfo->mHeldFlag != FlagTeam::INVALID) { // Drop the flag if holding it
        if (mInfo->mHeldFlag == mInfo->mPlayerTeam && !isPlayerOnSelfTeamSide()) {
            // Can only place your own flag on your own side
            return;
        }
        dropHeldFlag();
        return;
    }


    for (size_t flagTeamIndex = 0; flagTeamIndex < static_cast<size_t>(FlagTeam::_COUNT); flagTeamIndex++) { // Pickup flag if within distance threshold
        auto const flagTeam = static_cast<FlagTeam>(flagTeamIndex);
        if (flagTeam == FlagTeam::INVALID) {
            continue;
        }
        if (flagTeam == mInfo->mPlayerTeam) {
            continue; // Can't pick up your own flag after it's been placed. If another player steals the flag and you tag them, the flag will auto-respawn at it's original placement
        }
        auto const flagActorMaybe = FlagActor::getFlagActorForTeam(flagTeam);
        if (!flagActorMaybe.has_value()) {
            continue;
        }
        auto const* flag = flagActorMaybe.value();
        if (flag->isHeld()) {
            // Not allowed to pick up a flag if another player is holding it
            continue;
        }
        if (al::calcDistance(flag, playerBase) < FLAG_PICKUP_DISTANCE_THRESHOLD) {
            mInfo->mHeldFlag = flagTeam;
            return;
        }
    }
}

void CaptureTheFlagMode::sendFlagHoldStateUpdateIfHeld(PlayerActorBase* playerBase) {
    if (playerBase == nullptr || mInfo->mHeldFlag == FlagTeam::INVALID) {
        return;
    }

    auto const playerPos = al::getTrans(playerBase);
    auto const playerRot = al::getQuat(playerBase);
    FlagHoldState packet{};
    packet.isHeld = true;
    packet.team = mInfo->mHeldFlag;
    Client::sendFlagHoldStatePacket(packet);
    auto flagActorMaybe = FlagActor::getFlagActorForTeam(mInfo->mHeldFlag);
    if (flagActorMaybe.has_value()) {
        auto* flagActor = flagActorMaybe.value();
        flagActor->makeActorAlive();
        flagActor->setTransform(playerPos, playerRot);
    }
}

void CaptureTheFlagMode::checkWin(PlayerActorBase* playerBase) {
    if (mInfo->mWinningTeam != FlagTeam::INVALID) {
        return;
    }

    // reinterpret_cast<PlayerActorHakoniwa*>(getLocalPlayer())->mPlayerConst->mNormalMaxSpeed = 0.5f;

    auto const side = getTeamSide();
    if (side == FlagTeam::INVALID) {
        return;
    }
    auto const opposingTeam = getOpposingTeam(mInfo->mPlayerTeam);
    auto const win = (side == mInfo->mPlayerTeam) && opposingTeam.has_value() && (mInfo->mHeldFlag == opposingTeam);
    if (win) {
        CaptureTheFlagWin winPacket;
        winPacket.winningTeam = mInfo->mPlayerTeam;
        Client::sendCTFWinPacket(winPacket);
    }
}

bool CaptureTheFlagMode::changeStage(StageData const& stageData) {
    GameDataHolderAccessor accessor(mSceneObjHolder);
    const char* changeID = "";
    u8 subScenarioType = 0;
    ChangeStageInfo info(accessor.mData, 
        changeID, 
        stageData.stageName, 
        false, 
        stageData.scenarioNum,
        static_cast<ChangeStageInfo::SubScenarioType>(subScenarioType));
    return changeStage(info);
}

bool CaptureTheFlagMode::changeStage(StageData const& stageData, const char* changeID) {
    GameDataHolderAccessor accessor(mSceneObjHolder);
    u8 subScenarioType = 0;
    ChangeStageInfo info(accessor.mData, 
        changeID, 
        stageData.stageName, 
        false, 
        stageData.scenarioNum,
        static_cast<ChangeStageInfo::SubScenarioType>(subScenarioType));
    return changeStage(info);
}

bool CaptureTheFlagMode::changeStage(ChangeStageInfo const& info) {
    GameDataHolderAccessor accessor(mSceneObjHolder);
    return GameDataFunction::tryChangeNextStage(accessor, &info);
}

void CaptureTheFlagMode::processPlayerTagged() {
    if (mInfo->mHeldFlag == getOpposingTeam(getPlayerTeam())) {
        return; // The team captain died before placing their own team's flag, just let them keep holding the flag
    }
    else if (mInfo->mHeldFlag == getOpposingTeam(getPlayerTeam())) {
        respawnHeldFlag();
    }

    queueHomeTeleport();
}

void CaptureTheFlagMode::queueHomeTeleport() {
    auto* playerBase = getLocalPlayer();
    if (!playerBase) {
        return;
    }
    GameDataFunction::killPlayer(GameDataHolderAccessor(this));
    playerBase->startDemoPuppetable();
    al::setVelocityZero(playerBase);
    rs::faceToCamera(playerBase);
    ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->endSubAnim();
    ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->startAnimDead();
    // auto teleportInfo = StageManager::getHomeTeleport(getLocalPlayerStageData(), getPlayerTeam());
    // if (!teleportInfo.has_value()) {
    //     return;
    // }
    // changeStage(teleportInfo.value().first, teleportInfo.value().second);
    // m_teleportTimer = TELEPORT_TIMER_MODE_FREEZE_THRESHOLD_SECONDS;
}

void CaptureTheFlagMode::dropHeldFlag() {
    if (mInfo->mHeldFlag == FlagTeam::INVALID) {
        return;
    }
    FlagHoldState packet{};
    packet.isHeld = false;
    packet.team = mInfo->mHeldFlag;
    Client::sendFlagHoldStatePacket(packet);
    mInfo->mHeldFlag = FlagTeam::INVALID;
}

void CaptureTheFlagMode::respawnHeldFlag() {
    if (mInfo->mHeldFlag == FlagTeam::INVALID) {
        return;
    }
    dropHeldFlag();
}

bool CaptureTheFlagMode::isPlayerOnSelfTeamSide() const {
    return getTeamSide() == mInfo->mPlayerTeam;
}

FlagTeam CaptureTheFlagMode::getTeamSide() const {
    auto* playerBase = getLocalPlayer();
    if (!playerBase) {
        return FlagTeam::INVALID;
    }
    auto const pos = StageManager::replacePositionWithSubAreaEntranceIfInSubArea(al::getTrans(playerBase), getLocalPlayerStageData());

    auto borderTransform = StageManager::getBorderTransform(getLocalPlayerStageData());
    if (borderTransform.has_value()) {
        return getTeamSide(pos, borderTransform.value());
    }

    auto const parentStageName = StageManager::getParentStageName(getLocalPlayerStageData());
    if (!parentStageName.has_value()) {
        return FlagTeam::INVALID; // No border specified for a top-level stage. This indicates a kingdom is missing from the border transform list.
    }

    borderTransform = StageManager::getBorderTransform(StageData{parentStageName.value(), getLocalPlayerStageData().scenarioNum});
    if (borderTransform.has_value()) {
        return getTeamSide(pos, borderTransform.value());
    }

    return FlagTeam::INVALID;
}

FlagTeam CaptureTheFlagMode::getTeamSide(sead::Vector3f const& pos, BorderTransform const& borderTransform) const {
    auto const borderOrigin = borderTransform.position;
    auto relativePos = pos - borderOrigin;
    relativePos.normalize();

    constexpr float DEGREES_TO_RADIANS = 180.f / M_PI;
    sead::Vector3f borderPlaneNormal(-0.71,0,-0.71); // Default volleyball net rotation, about 45* offset the z-axis

    return borderPlaneNormal.dot(relativePos) > 0 ? FlagTeam::RED : FlagTeam::BLUE;
}

std::optional<FlagTeam> CaptureTheFlagMode::getOpposingTeam(FlagTeam team) const {
    switch (team) {
        case FlagTeam::RED:
            return FlagTeam::BLUE;
        case FlagTeam::BLUE:
            return FlagTeam::RED;
        case FlagTeam::INVALID:
        default:
            return {};
    }
}

bool CaptureTheFlagMode::isInvulnerable() const {
    return mInvulnTime >= 5;
}

StageData CaptureTheFlagMode::getLocalPlayerStageData() const {
    return StageData{
        GameDataFunction::getCurrentStageName(mCurScene->mHolder),
        mCurScene->mHolder.mData->mGameDataFile->getScenarioNo()
    };
}

bool CaptureTheFlagMode::isInSubArea() const {
    return StageManager::isSubArea(getLocalPlayerStageData());
}