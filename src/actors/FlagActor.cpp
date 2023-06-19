#include "actors/FlagActor.h"

#include "main.hpp"
#include "server/Client.hpp"
#include "al/LiveActor/LiveActor.h"
#include "al/layout/BalloonMessage.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/string/StringTmp.h"
#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "algorithms/CaptureTypes.h"
#include "logger.hpp"
#include "math/seadQuat.h"
#include "math/seadVector.h"
#include "server/gamemode/GameModeBase.hpp"

sead::Vector3f FlagActor::latestReceivedPosition{};
FlagTeam FlagActor::latestReceivedFlagTeam{};
char FlagActor::latestReceivedStage[0x40] = {};



FlagActor* FlagActor::teamFlags[static_cast<unsigned char>(FlagTeam::_COUNT)] = {nullptr};
FlagInfo FlagActor::flagInfos[static_cast<unsigned char>(FlagTeam::_COUNT)] = {FlagInfo{}};

void FlagActor::initFlagActors(al::ActorInitInfo const &rootInitInfo, al::PlacementInfo const &rootPlacementInfo, bool isDebug) {
    FlagActor::teamFlags[static_cast<unsigned char>(FlagTeam::RED)] = reinterpret_cast<FlagActor*>(createFromFactory(rootInitInfo, rootPlacementInfo, isDebug));
    FlagActor::teamFlags[static_cast<unsigned char>(FlagTeam::RED)]->setTeam(FlagTeam::RED);
    FlagActor::teamFlags[static_cast<unsigned char>(FlagTeam::BLUE)] = reinterpret_cast<FlagActor*>(createFromFactory(rootInitInfo, rootPlacementInfo, isDebug));
    FlagActor::teamFlags[static_cast<unsigned char>(FlagTeam::BLUE)]->setTeam(FlagTeam::BLUE);
}


std::optional<FlagActor*> FlagActor::getFlagActorForTeam(FlagTeam team) {
    auto* flag = FlagActor::teamFlags[static_cast<unsigned char>(team)];
    if (flag == nullptr) {
        return {};
    }
    return flag;
}

FlagActor::FlagActor(const char* name) : al::LiveActor(name) {
}

void FlagActor::init(al::ActorInitInfo const &initInfo) {
    al::initActorWithArchiveName(this, initInfo, "FlagActor", nullptr);
    startAction("Before");
}

void FlagActor::setTeam(FlagTeam team) {
    initOnline(&FlagActor::flagInfos[static_cast<int>(team)]);
    if (m_info.has_value()) {
        m_info.value()->team = team;
    }

    auto anim = team == FlagTeam::RED ? "After" : "Before";
    if (al::isMtpAnimExist(this, anim)) {
        al::startMtpAnim(this, anim);
    }
}

void FlagActor::startAction(const char* actName) {
    if(!actName) return;
    auto curModel = this;

    if(al::tryStartActionIfNotPlaying(curModel, actName)) {
        const char *curActName = al::getActionName(curModel);
        if(curActName) {
            if(al::isSklAnimExist(curModel, curActName)) {
                al::clearSklAnimInterpole(curModel);
            }
        }
    }
}

void FlagActor::initAfterPlacement() { 
    al::LiveActor::initAfterPlacement(); 
    float const s = 500000.f;
    al::setClippingObb(BorderActor::singleton, sead::BoundBox3f(-s, -s, -s, s, s, s));
}

void FlagActor::movement() {
    al::LiveActor::movement();
}

void FlagActor::control() {
    if (!m_info.has_value()) {
        return;
    }

    auto* info = m_info.value();

    // Position & Rotation Handling
    sead::Vector3f* pPos = al::getTransPtr(this);
    sead::Quatf *pQuat = al::getQuatPtr(this);

    *pPos = info->pos;
    *pQuat = info->rot;
    this->mPoseKeeper->updatePoseQuat(info->rot); // update pose using a quaternion instead of setting quaternion rotation
}

void FlagActor::makeActorAlive() {
    if (al::isDead(this)) {
        al::LiveActor::makeActorAlive();
    }
}

void FlagActor::makeActorDead() {
    if (!al::isDead(this)) {
        this->makeActorDead();
    }
}

void FlagActor::initOnline(FlagInfo* info) {
    m_info = info;
}

al::LiveActor *FlagActor::createFromFactory(al::ActorInitInfo const &rootInitInfo, al::PlacementInfo const &rootPlacementInfo, bool isDebug) {
    al::ActorInitInfo actorInitInfo = al::ActorInitInfo();
    actorInitInfo.initViewIdSelf(&rootPlacementInfo, rootInitInfo);

    al::createActor createActor = actorInitInfo.mActorFactory->getCreator("FlagActor");
    
    if(!createActor) {
        return nullptr;
    }

    FlagActor *newActor = (FlagActor*)createActor("FlagActor");

    Logger::log("Creating Flag Actor.\n");
 
    newActor->init(actorInitInfo);

    return newActor;
}