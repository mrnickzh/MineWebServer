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
};
