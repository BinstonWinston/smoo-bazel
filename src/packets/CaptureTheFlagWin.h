#pragma once

#include "Packet.h"
#include "al/util.hpp"
#include "algorithms/PlayerAnims.h"
#include "server/capturetheflag/FlagTeam.hpp"

struct PACKED CaptureTheFlagWin : Packet {
    CaptureTheFlagWin() : Packet() {mType = PacketType::CAPTURE_THE_FLAG_WIN; mPacketSize = sizeof(CaptureTheFlagWin) - sizeof(Packet);};
    FlagTeam winningTeam;

    bool operator==(const CaptureTheFlagWin &rhs) const {
        return (
            winningTeam == rhs.winningTeam
        );
    }

    bool operator!=(const CaptureTheFlagWin& rhs) const { return !operator==(rhs); }
};