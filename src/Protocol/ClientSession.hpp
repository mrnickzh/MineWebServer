#pragma once

#include <string>

class ClientSession {
public:
    std::string clientaddress;
    std::string username;
    std::string uuid;

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
