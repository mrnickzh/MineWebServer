//
// Created by onlym on 3/31/2026.
//

#pragma once
#include <cstdint>

enum ConnectionState : uint8_t {
    OFFLINE_PING = 0xFF,
    OFFLINE_PING_PROTOCOL_MISSMATCH = 0xFE,

    HANDSHAKE_EXCHANGE = 0x1,
    KEY_EXCHANGE = 0x2,
    AUTHENTIFICATION = 0x3,
    PLAY = 0x4,
};

enum CompressionType : uint8_t {
    ZLIB = 0x0,

    DUMMY = 0xFF
};

struct NetworkSettings {
    CompressionType compressionType;
    uint16_t compressionThreshold;
};