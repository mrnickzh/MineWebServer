#pragma once

#include <vector>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <iostream>

class ByteBuf {
private:
    std::vector<uint8_t> buffer;
    size_t position = 0;

public:
    ByteBuf(size_t capacity) : buffer(capacity) {}

    void writeByte(uint8_t value) {
        if (position + sizeof(uint8_t) > buffer.size()) {
            assert(0 == 1 && "Buffer overflow");
            // throw std::out_of_range("Buffer overflow");
        }
        buffer[position++] = value;
    }

    uint8_t readByte() {
        if (position + sizeof(uint8_t) > buffer.size()) {
            assert(0 == 1 && "Buffer underflow");
        }
        return buffer[position++];
    }

    void writeInt(int32_t value) {
        if (position + sizeof(int32_t) > buffer.size()) {
            assert(0 == 1 && "Buffer overflow");
            // throw std::out_of_range("Buffer overflow");
        }
        std::memcpy(buffer.data() + position, &value, sizeof(int32_t));
        position += sizeof(int32_t);
    }

    int32_t readInt() {
        if (position + sizeof(int32_t) > buffer.size()) {
            assert(0 == 1 && "Buffer underflow");
            // throw std::out_of_range("Buffer underflow");
        }
        int32_t value;
        std::memcpy(&value, buffer.data() + position, sizeof(int32_t));
        position += sizeof(int32_t);
        return value;
    }

    void writeFloat(float value) {
        if (position + sizeof(float) > buffer.size()) {
            assert(0 == 1 && "Buffer overflow");
            // throw std::out_of_range("Buffer overflow");
        }
        std::memcpy(buffer.data() + position, &value, sizeof(float));
        position += sizeof(float);
    }

    float readFloat() {
        if (position + sizeof(float) > buffer.size()) {
            assert(0 == 1 && "Buffer underflow");
            // throw std::out_of_range("Buffer underflow");
        }
        float value;
        std::memcpy(&value, buffer.data() + position, sizeof(float));
        position += sizeof(float);
        return value;
    }

    void writeString(const std::string& value) {
        size_t length = value.size();
        if (position + sizeof(int32_t) + length > buffer.size()) {
            assert(0 == 1 && "Buffer overflow");
            // throw std::out_of_range("Buffer overflow");
        }
        writeInt(static_cast<int32_t>(length));
        std::memcpy(buffer.data() + position, value.data(), length);
        position += length;
    }

    std::string readString() {
        int32_t length = readInt();
        if (position + length > buffer.size()) {
            assert(0 == 1 && "Buffer underflow");
            // throw std::out_of_range("Buffer underflow");
        }
        std::string value(reinterpret_cast<char*>(buffer.data() + position), length);
        position += length;
        return value;
    }

    std::vector<uint8_t> toByteArray() const {
        return std::vector<uint8_t>(buffer.begin(), buffer.begin() + position);
    }

    std::vector<uint8_t> toByteArray(int size) const {
        return std::vector<uint8_t>(buffer.begin() + position, buffer.begin() + position + size);
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

    void concat(const ByteBuf& other) {
        buffer.resize(position);
        buffer.insert(buffer.end(), other.buffer.begin(), other.buffer.end());
        position += other.buffer.size();
    }

    void resize(int size) {
        buffer.resize(size);
    }
};
