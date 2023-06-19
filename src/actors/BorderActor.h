#pragma once

#include <optional>
#include <unordered_map>

#include "al/LiveActor/LiveActor.h"
#include "al/async/FunctorV0M.hpp"
#include "al/async/FunctorBase.h"
#include "al/util.hpp"
#include "al/string/StringTmp.h"
#include "al/layout/BalloonMessage.h"

#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerJointControlPartsDynamics.h"
#include "game/Player/PlayerConst.h"
#include "game/Player/PlayerModelHolder.h"

#include "actors/PuppetCapActor.h"
#include "actors/PuppetHackActor.h"
#include "layouts/NameTag.h"
#include "sead/math/seadVector.h"
#include "server/DeltaTime.hpp"

#include "logger.hpp"
#include "puppets/PuppetInfo.h"
#include "puppets/HackModelHolder.hpp"
#include "helpers.hpp"
#include "algorithms/CaptureTypes.h"
#include "server/capturetheflag/FlagTeam.hpp"

struct BorderInfo {
    sead::Vector3f pos = sead::Vector3f(0.f,0.f,0.f);
    sead::Quatf rot = sead::Quatf(0.f,0.f,0.f,0.f);
};

class BorderActor : public al::LiveActor {
    public:
        static BorderActor* singleton;
        static BorderActor* createFromFactory(al::ActorInitInfo const &rootInitInfo, al::PlacementInfo const &rootPlacementInfo, bool isDebug);

        BorderActor(const char* name);
        virtual void init(al::ActorInitInfo const &) override;
        virtual void initAfterPlacement(void) override;
        virtual void control(void) override;
        virtual void movement(void) override;
        virtual void makeActorAlive(void) override;
        virtual void makeActorDead(void) override;

        void initOnline(BorderInfo const& info);

        void setTransform(sead::Vector3f const& pos, sead::Quatf const& rot) {
            if (!m_info.has_value()) {
                return;
            }
            m_info.value().pos = pos;
            m_info.value().rot = rot;
        }

    private:        
        std::optional<BorderInfo> m_info;

        
};
