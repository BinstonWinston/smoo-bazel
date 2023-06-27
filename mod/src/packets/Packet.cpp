#include "Packet.hpp"

namespace packets {

util::math::Vector3 convert(sead::Vector3f const& value) {
    util::math::Vector3 v;
    v.set_x(value.x);
    v.set_y(value.y);
    v.set_z(value.z);
    return v;
}
    
sead::Vector3f convert(util::math::Vector3 const& value) {
    sead::Vector3f v;
    v.x = value.x();
    v.y = value.y();
    v.z = value.z();
    return v;
}

util::math::Quaternion convert(sead::Quatf const& value) {
    util::math::Quaternion q;
    q.set_x(value.x);
    q.set_y(value.y);
    q.set_z(value.z);
    q.set_w(value.w);
    return q;
}

sead::Quatf convert(util::math::Quaternion const& value) {
    sead::Quatf q;
    q.x = value.x();
    q.y = value.y();
    q.z = value.z();
    q.w = value.w();
    return q;
}

packets::Uid convert(nn::account::Uid const& value) {
    packets::Uid uid;
    uid.set_uid(value.data, sizeof(value.data));
    return uid;
}

nn::account::Uid convert(packets::Uid const& value) {
    nn::account::Uid uid;
    memset(uid.data, 0, sizeof(uid.data)); // Clear uid
    memcpy(uid.data, value.uid().c_str(), std::min(value.uid().size(), sizeof(uid.data)));
    return uid;
}

int32_t convert(PlayerAnims::Type value) {
    return static_cast<int32_t>(value);
}

PlayerAnims::Type convert(int32_t value) {
    return static_cast<PlayerAnims::Type>(value);
}

}


// mutable_user_id().set_uid(Client::getClientId().data, sizeof(Client::getClientId().data))