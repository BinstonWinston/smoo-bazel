#pragma once

#include "Packet.h"
#include "al/util.hpp"
#include "algorithms/PlayerAnims.h"
#include "server/capturetheflag/FlagTeam.hpp"

struct PACKED CaptureTheFlagPlayerTeamAssignment : Packet {
    CaptureTheFlagPlayerTeamAssignment() : Packet() {mType = PacketType::CAPTURE_THE_FLAG_PLAYER_TEAM_ASSIGNMENT; mPacketSize = sizeof(CaptureTheFlagPlayerTeamAssignment) - sizeof(Packet);};
    FlagTeam team;
    bool1 shouldHoldTeamFlag;

    bool operator==(const CaptureTheFlagPlayerTeamAssignment &rhs) const {
        return team == rhs.team && shouldHoldTeamFlag && rhs.shouldHoldTeamFlag;
    }

    bool operator!=(const CaptureTheFlagPlayerTeamAssignment& rhs) const { return !operator==(rhs); }
};