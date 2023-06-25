#pragma once

#include "proto/packets/Packet.pb.h"

#include <string>
#include <string.h>

#include "algorithms/PlayerAnims.h"
#include "nn/account.h"
#include "sead/math/seadVector.hpp"
#include "sead/math/seadQuat.hpp"
#include "sead/prim/seadSafeString.hpp"

#define PACKBUFSIZE      0x30
#define COSTUMEBUFSIZE   0x20

#define MAXPACKSIZE      0x100

namespace packets {
    util::math::Vector3 convert(sead::Vector3f const& value);
    sead::Vector3f convert(util::math::Vector3 const& value);
    util::math::Quaternion convert(sead::Quatf const& value);
    sead::Quatf convert(util::math::Quaternion const& value);

    packets::Uid convert(nn::account::Uid const& value);
    nn::account::Uid convert(packets::Uid const& value);

    template <s32 L>
    std::string convert(sead::FixedSafeString<L> const& value) {
        return std::string(value.cstr(), value.calcLength());
    }
    template <s32 L>
    sead::FixedSafeString<L> convert(std::string const& value) {
        sead::FixedSafeString<L> s;
        memcpy(s.cstr(), value.c_str(), std::min((s32)value.size(), L)); // Copy as many characters as will fit into the FixedSafeString capacity L
        return s;
    }

    int32_t convert(PlayerAnims::Type value);
    PlayerAnims::Type convert(int32_t value);
}