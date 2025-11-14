#pragma once

#include <vector>
#include <stdexcept>

class ByteBuf {
private:
    std::vector<uint8_t> buffer;
    size_t position = 0;

public:
    ByteBuf(size_t capacity) : buffer(capacity) {}

    void writeInt(int32_t value) {
        if (position + sizeof(int32_t) > buffer.size()) {
            throw std::out_of_range("Buffer overflow");
        }
        std::memcpy(buffer.data() + position, &value, sizeof(int32_t));
        position += sizeof(int32_t);
    }

    int32_t readInt() {
        if (position + sizeof(int32_t) > buffer.size()) {
            throw std::out_of_range("Buffer underflow");
        }
        int32_t value;
        std::memcpy(&value, buffer.data() + position, sizeof(int32_t));
        position += sizeof(int32_t);
        return value;
    }

    void writeFloat(float value) {
        if (position + sizeof(float) > buffer.size()) {
            throw std::out_of_range("Buffer overflow");
        }
        std::memcpy(buffer.data() + position, &value, sizeof(float));
        position += sizeof(float);
    }

    float readFloat() {
        if (position + sizeof(float) > buffer.size()) {
            throw std::out_of_range("Buffer underflow");
        }
        float value;
        std::memcpy(&value, buffer.data() + position, sizeof(float));
        position += sizeof(float);
        return value;
    }

    void writeString(const std::string& value) {
        size_t length = value.size();
        if (position + sizeof(int32_t) + length > buffer.size()) {
            throw std::out_of_range("Buffer overflow");
        }
        writeInt(static_cast<int32_t>(length));
        std::memcpy(buffer.data() + position, value.data(), length);
        position += length;
    }

    std::string readString() {
        int32_t length = readInt();
        if (position + length > buffer.size()) {
            throw std::out_of_range("Buffer underflow");
        }
        std::string value(reinterpret_cast<char*>(buffer.data() + position), length);
        position += length;
        return value;
    }

    std::vector<uint8_t> toByteArray() const {
        return std::vector<uint8_t>(buffer.begin(), buffer.begin() + position);
    }

    void fromByteArray(const std::vector<uint8_t>& data) {
        buffer = data;
        position = 0;
    }

    void fromString(const std::string& str) {
        buffer = std::vector<uint8_t>(str.begin(), str.end());
        position = 0;
    }

    std::string toString() const {
        return std::string(buffer.begin(), buffer.end());
    }

};