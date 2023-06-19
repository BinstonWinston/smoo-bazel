#pragma once

#include "Packet.h"
#include "al/util.hpp"
#include "algorithms/PlayerAnims.h"
#include "server/capturetheflag/FlagTeam.hpp"

struct PACKED CaptureTheFlagGameState : Packet {
    CaptureTheFlagGameState() : Packet() {mType = PacketType::CAPTURE_THE_FLAG_GAME_STATE; mPacketSize = sizeof(CaptureTheFlagGameState) - sizeof(Packet);};
    u8 seconds;
    u16 minutes;
    bool1 resetGame;

    bool operator==(const CaptureTheFlagGameState &rhs) const {
        return seconds == rhs.seconds && minutes == rhs.minutes && resetGame == rhs.resetGame;
    }

    bool operator!=(const CaptureTheFlagGameState& rhs) const { return !operator==(rhs); }
};