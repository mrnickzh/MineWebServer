#pragma once

#include "../../MineWebClient/src/Protocol/Packet.hpp"
#include "../../Utils/Vec.hpp"

class AddMapObjectServer : public Packet {
public:
    int id;
    Vec3<float> position;
    Vec3<float> rotation;

    void receive(ByteBuf &buffer) override {}

    void send(ByteBuf &buffer) override {
        buffer.writeInt(id);
        buffer.writeFloat(position.x);
        buffer.writeFloat(position.y);
        buffer.writeFloat(position.z);
        buffer.writeFloat(rotation.x);
        buffer.writeFloat(rotation.y);
        buffer.writeFloat(rotation.z);
    }
};