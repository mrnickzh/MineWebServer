#pragma once

#include <vector>
#include <cmath>
#include <cstdint>

class PerlinNoise {
private:
    uint64_t seed;

    float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    std::pair<float, float> gradient(int gx, int gy) const {
        uint64_t h = gx * 49632 + gy * 325176 + seed;
        h = (h ^ (h >> 13)) * 1274126177;
        float angle = (h % 36000) / 100.0f * 3.14159265f / 180.0f;
        return { std::cos(angle), std::sin(angle) };
    }

    float dot(std::pair<float, float>& g, float dx, float dy) {
        return g.first * dx + g.second * dy;
    }

    void gradient3(int x, int y, int z, float& gx, float& gy, float& gz) {
        uint64_t h =
                (uint64_t)x * 374761393ULL +
                (uint64_t)y * 668265263ULL +
                (uint64_t)z * 2147483647ULL +
                seed;

        h ^= h >> 13;
        h *= 1274126177ULL;

        float theta = (h & 0xFFFF) * (2.0f * 3.14159265f / 65536.0f);
        float phi   = ((h >> 16) & 0xFFFF) * (3.14159265f / 65536.0f);

        gx = std::cos(theta) * std::sin(phi);
        gy = std::sin(theta) * std::sin(phi);
        gz = std::cos(phi);
    }

    float dot3(float gx, float gy, float gz, float dx, float dy, float dz) {
        return gx * dx + gy * dy + gz * dz;
    }

public:
    explicit PerlinNoise(uint64_t s = 0) : seed(s) {}

    float generateOctaves(float x, float y, int octaves, float frequency, float amplification){
        float total = 0.0f;
        float maxAmp = 0.0f;

        float freq = frequency;
        float amp = 1.0f;

        for (int i = 0; i < octaves; i++) {
            total += generate(x, y, freq) * amp;
            maxAmp += amp;

            freq *= 2.f;
            amp *= amplification;
        }

        return total / maxAmp;
    }


    float generate(float x, float y, float frequency) {
        float fx = x * frequency;
        float fy = y * frequency;

        int x0 = static_cast<int>(std::floor(fx));
        int x1 = x0 + 1;
        int y0 = static_cast<int>(std::floor(fy));
        int y1 = y0 + 1;

        float sx = fx - x0;
        float sy = fy - y0;

        float u = fade(sx);
        float v = fade(sy);

        std::pair<float, float> g00 = gradient(x0, y0);
        std::pair<float, float> g10 = gradient(x1, y0);
        std::pair<float, float> g01 = gradient(x0, y1);
        std::pair<float, float> g11 = gradient(x1, y1);

        float dx0 = sx;
        float dy0 = sy;
        float dx1 = sx - 1.0f;
        float dy1 = sy - 1.0f;

        float n00 = dot(g00, dx0, dy0);
        float n10 = dot(g10, dx1, dy0);
        float n01 = dot(g01, dx0, dy1);
        float n11 = dot(g11, dx1, dy1);

        float nx0 = std::lerp(n00, n10, u);
        float nx1 = std::lerp(n01, n11, u);
        float nxy = std::lerp(nx0, nx1, v);

        return nxy;
    }
    
    float generate3D(float x, float y, float z, float frequency) {
        x *= frequency;
        y *= frequency;
        z *= frequency;

        int x0 = (int)std::floor(x);
        int y0 = (int)std::floor(y);
        int z0 = (int)std::floor(z);

        int x1 = x0 + 1;
        int y1 = y0 + 1;
        int z1 = z0 + 1;

        float fx = x - x0;
        float fy = y - y0;
        float fz = z - z0;

        float u = fade(fx);
        float v = fade(fy);
        float w = fade(fz);

        float g000x, g000y, g000z;
        float g100x, g100y, g100z;
        float g010x, g010y, g010z;
        float g110x, g110y, g110z;
        float g001x, g001y, g001z;
        float g101x, g101y, g101z;
        float g011x, g011y, g011z;
        float g111x, g111y, g111z;

        gradient3(x0, y0, z0, g000x, g000y, g000z);
        gradient3(x1, y0, z0, g100x, g100y, g100z);
        gradient3(x0, y1, z0, g010x, g010y, g010z);
        gradient3(x1, y1, z0, g110x, g110y, g110z);
        gradient3(x0, y0, z1, g001x, g001y, g001z);
        gradient3(x1, y0, z1, g101x, g101y, g101z);
        gradient3(x0, y1, z1, g011x, g011y, g011z);
        gradient3(x1, y1, z1, g111x, g111y, g111z);

        float n000 = dot3(g000x, g000y, g000z, fx,     fy,     fz);
        float n100 = dot3(g100x, g100y, g100z, fx - 1, fy,     fz);
        float n010 = dot3(g010x, g010y, g010z, fx,     fy - 1, fz);
        float n110 = dot3(g110x, g110y, g110z, fx - 1, fy - 1, fz);
        float n001 = dot3(g001x, g001y, g001z, fx,     fy,     fz - 1);
        float n101 = dot3(g101x, g101y, g101z, fx - 1, fy,     fz - 1);
        float n011 = dot3(g011x, g011y, g011z, fx,     fy - 1, fz - 1);
        float n111 = dot3(g111x, g111y, g111z, fx - 1, fy - 1, fz - 1);

        float nx00 = std::lerp(n000, n100, u);
        float nx10 = std::lerp(n010, n110, u);
        float nx01 = std::lerp(n001, n101, u);
        float nx11 = std::lerp(n011, n111, u);

        float nxy0 = std::lerp(nx00, nx10, v);
        float nxy1 = std::lerp(nx01, nx11, v);

        return std::lerp(nxy0, nxy1, w);
    }
};
