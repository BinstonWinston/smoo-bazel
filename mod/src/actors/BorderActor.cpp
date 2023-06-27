#include "actors/BorderActor.h"

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

BorderActor* BorderActor::singleton = nullptr;

BorderActor::BorderActor(const char* name) : al::LiveActor(name) {
}

void BorderActor::init(al::ActorInitInfo const &initInfo) {
    al::initActorWithArchiveName(this, initInfo, "BorderActor", nullptr);
}

void BorderActor::initAfterPlacement() { 
    al::LiveActor::initAfterPlacement(); 
}

void BorderActor::movement() {
    al::LiveActor::movement();
}

void BorderActor::control() {
    if (!m_info.has_value()) {
        return;
    }

    auto& info = m_info.value();

    // Position & Rotation Handling
    sead::Vector3f* pPos = al::getTransPtr(this);
    sead::Quatf *pQuat = al::getQuatPtr(this);

    if (pPos) {
        *pPos = info.pos;
    }

    if (pQuat) {
        *pQuat = info.rot;
        this->mPoseKeeper->updatePoseQuat(al::getQuat(this));
    }
    // al::setScale(this, BORDER_SCALE);
}

void BorderActor::makeActorAlive() {
    if (al::isDead(this)) {
        al::LiveActor::makeActorAlive();
    }
}

void BorderActor::makeActorDead() {
    if (!al::isDead(this)) {
        al::LiveActor::makeActorDead();
    }
}

void BorderActor::initOnline(BorderInfo const& info) {
    m_info = info;
}

BorderActor *BorderActor::createFromFactory(al::ActorInitInfo const &rootInitInfo, al::PlacementInfo const &rootPlacementInfo, bool isDebug) {
    al::ActorInitInfo actorInitInfo = al::ActorInitInfo();
    actorInitInfo.initViewIdSelf(&rootPlacementInfo, rootInitInfo);

    al::createActor createActor = actorInitInfo.mActorFactory->getCreator("BorderActor");
    
    if(!createActor) {
        return nullptr;
    }

    BorderActor *newActor = (BorderActor*)createActor("BorderActor");

    Logger::log("Creating Border Actor.\n");

    BorderInfo borderInfo{};
    newActor->initOnline(borderInfo); 
    newActor->init(actorInitInfo);

    return newActor;
}