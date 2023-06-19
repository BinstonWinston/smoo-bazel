#pragma once

#include "Packet.h"
#include "al/util.hpp"
#include "algorithms/PlayerAnims.h"
#include "server/capturetheflag/FlagTeam.hpp"

struct PACKED FlagHoldState : Packet {
    FlagHoldState() : Packet() {mType = PacketType::FLAG_HOLD_STATE; mPacketSize = sizeof(FlagHoldState) - sizeof(Packet);};
    bool isHeld;
    FlagTeam team;

    static_assert(sizeof(bool) == 1, "sizeof(bool) must equal 1 for interop with C# serialization/deserialization");

    bool operator==(const FlagHoldState &rhs) const {
        return (
            mUserID == rhs.mUserID &&
            isHeld == rhs.isHeld &&
            team == rhs.team
        );
    }

    bool operator!=(const FlagHoldState& rhs) const { return !operator==(rhs); }
};

static_assert((sizeof(FlagHoldState) - sizeof(Packet)) == 2, "sizeof(FlagHoldState) must equal 16 for interop with C# serialization/deserialization");