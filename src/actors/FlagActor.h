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

struct FlagInfo {
    FlagTeam team;
    bool isHeld;
    nn::account::Uid holderUserId;
    // Flag transform
    sead::Vector3f pos = sead::Vector3f(0.f,0.f,0.f);
    sead::Quatf rot = sead::Quatf(0.f,0.f,0.f,0.f);
};

class FlagActor : public al::LiveActor {
    public:
        // For debugging
        static sead::Vector3f latestReceivedPosition;
        static FlagTeam latestReceivedFlagTeam;
        static char latestReceivedStage[0x40];

        static void initFlagActors(al::ActorInitInfo const &rootInitInfo, al::PlacementInfo const &rootPlacementInfo, bool isDebug);
        static std::optional<FlagActor*> getFlagActorForTeam(FlagTeam team);
        FlagActor(const char* name);
        virtual void init(al::ActorInitInfo const &) override;
        virtual void initAfterPlacement(void) override;
        virtual void control(void) override;
        virtual void movement(void) override;
        virtual void makeActorAlive(void) override;
        virtual void makeActorDead(void) override;

        void initOnline(FlagInfo* info);

        FlagTeam getTeam() const { return m_info.has_value() ? m_info.value()->team : FlagTeam::INVALID; }
        bool isHeld() const { return m_info.has_value() ? m_info.value()->isHeld : false; }
        void setIsHeld(bool isHeld) { 
            if (m_info.has_value()) {
                m_info.value()->isHeld = isHeld;
            } 
        }
        std::optional<nn::account::Uid> getHeldPlayer() {
            if (!m_info.has_value() || !isHeld()) {
                return {};
            }
            return m_info.value()->holderUserId;
        }
        void setHeldPlayer(nn::account::Uid const& userID) {
            if (m_info.has_value()) {
                m_info.value()->holderUserId = userID;
            }
        }
        void setTransform(sead::Vector3f const& pos, sead::Quatf const& rot) {
            sead::Vector3f* pPos = al::getTransPtr(this);
            *pPos = pos;

            if (!m_info.has_value()) {
                return;
            }
            m_info.value()->pos = pos;
            m_info.value()->rot = rot;
        }

        float m_closingSpeed = 0;
    private:
        static al::LiveActor *createFromFactory(al::ActorInitInfo const &rootInitInfo, al::PlacementInfo const &rootPlacementInfo, bool isDebug);

        void changeModel(const char* newModel);
        void startAction(const char* action);
        void setTeam(FlagTeam team);
        
        static FlagActor* teamFlags[static_cast<int>(FlagTeam::_COUNT)];
        static FlagInfo flagInfos[static_cast<int>(FlagTeam::_COUNT)];
        std::optional<FlagInfo*> m_info;
        
};
