#pragma once

#include <string>

#include "NetworkSettings.hpp"

class ClientSession {
public:
    std::string clientaddress;
    std::string username;
    std::string uuid;

    ConnectionState connectionState = ConnectionState::HANDSHAKE_EXCHANGE;
#ifdef BUILD_TYPE_DEDICATED
    NetworkSettings networkSettings = {CompressionType::ZLIB, 1024};
#else
     NetworkSettings networkSettings = {CompressionType::DUMMY, 0};
#endif

    ClientSession(std::string addr) { clientaddress = addr; };

    bool operator==(const ClientSession& client) const {
        return clientaddress == client.clientaddress;
    }

    bool operator!=(const ClientSession& client) const {
        return clientaddress != client.clientaddress;
    }

    bool operator<(const ClientSession& client) const {
        return clientaddress < client.clientaddress;
    }
};
