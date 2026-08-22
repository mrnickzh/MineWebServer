#pragma once

class ServerModEvent {
public:
    virtual ~ServerModEvent() = default;

    std::string eventid;
    bool canceled = false;

    void cancel() {
        canceled = true;
    }
};