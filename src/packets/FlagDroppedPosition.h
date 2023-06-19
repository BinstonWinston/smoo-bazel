#pragma once

#include "Packet.h"
#include "al/util.hpp"
#include "server/capturetheflag/FlagTeam.hpp"

struct PACKED FlagDroppedPosition : Packet {
    FlagDroppedPosition() : Packet() {mType = PacketType::FLAG_DROPPED_POSITION; mPacketSize = sizeof(FlagDroppedPosition) - sizeof(Packet);};
    FlagTeam team;
    sead::Vector3f pos;
    sead::Quatf rot;
    int32_t scenarioNo = 255;
    char stageName[0x40] = {};

    bool operator==(const FlagDroppedPosition &rhs) const {
        return (
            team == rhs.team &&
            pos == rhs.pos &&
            rot == rhs.rot &&
            scenarioNo == rhs.scenarioNo &&
            al::isEqualString(stageName, rhs.stageName)
        );
    }

    bool operator!=(const FlagDroppedPosition& rhs) const { return !operator==(rhs); }
};