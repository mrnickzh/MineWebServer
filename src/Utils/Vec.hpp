#pragma once
#include <cmath>
#include <type_traits>

template<typename T>
class Vec2 {
public:
    T x = 0, y = 0;

    Vec2() = default;
    explicit Vec2(T x, T y) : x(x), y(y) {}

    Vec2<T> operator+(const Vec2<T>& v) const {
        return Vec2<T>(x + v.x, y + v.y);
    }

    Vec2<T> operator-(const Vec2<T>& v) const {
        return Vec2<T>(x - v.x, y - v.y);
    }

    Vec2<T> operator*(T scalar) const {
        return Vec2<T>(x * scalar, y * scalar);
    }

    Vec2<T> operator/(T scalar) const {
        return Vec2<T>(x / scalar, y / scalar);
    }

    T dot(const Vec2<T>& v) const {
        return x * v.x + y * v.y;
    }

    T length() const {
        return std::sqrt(x * x + y * y);
    }

    void normalize() {
        T len = length();
        if (len > static_cast<T>(0)) {
            x /= len;
            y /= len;
        }
    }

    Vec2<float> ToFloat() const {
        return Vec2<float>(static_cast<float>(x), static_cast<float>(y));
    }
};

template<typename T>
class Vec3 {
public:
    T x = 0, y = 0, z = 0;

    Vec3() = default;
    explicit Vec3(T x, T y, T z) : x(x), y(y), z(z) {}

    Vec3<T> operator+(const Vec3<T>& v) const {
        return Vec3<T>(x + v.x, y + v.y, z + v.z);
    }

    Vec3<T> operator-(const Vec3<T>& v) const {
        return Vec3<T>(x - v.x, y - v.y, z - v.z);
    }

    Vec3<T> operator*(T scalar) const {
        return Vec3<T>(x * scalar, y * scalar, z * scalar);
    }

    Vec3<T> operator/(T scalar) const {
        return Vec3<T>(x / scalar, y / scalar, z / scalar);
    }

    T dot(const Vec3<T>& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vec3<T> cross(const Vec3<T>& v) const {
        return Vec3<T>(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    T length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    void normalize() {
        T len = length();
        if (len > static_cast<T>(0)) {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    Vec3<float> ToFloat() const {
        return Vec3<float>(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        );
    }
};
