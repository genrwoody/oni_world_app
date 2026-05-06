#pragma once

#include <cmath>

template<typename T>
struct Vector2 {
    T x{};
    T y{};

    Vector2() = default;

    template<typename T1, typename T2>
    Vector2(T1 x_, T2 y_)
        : x((T)(x_))
        , y((T)(y_))
    {
    }

    template<typename R>
    Vector2(const Vector2<R> &rhs)
        : Vector2(rhs.x, rhs.y)
    {
    }

    template<typename T1, typename T2>
    void Set(T1 x_, T2 y_)
    {
        x = (T)x_;
        y = (T)y_;
    }

    Vector2 operator+(const Vector2 &rhs) const
    {
        return {x + rhs.x, y + rhs.y};
    }

    Vector2 operator-(const Vector2 &rhs) const
    {
        return {x - rhs.x, y - rhs.y};
    }

    template<typename Ty>
    Vector2 operator*(Ty rhs) const
    {
        return {(T)(x * rhs), (T)(y * rhs)};
    }

    template<typename Ty>
    Vector2 operator/(Ty rhs) const
    {
        return {(T)(x / rhs), (T)(y / rhs)};
    }

    Vector2 &operator+=(const Vector2 &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vector2 &operator-=(const Vector2 &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    template<typename Ty>
    Vector2 &operator*=(Ty rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }

    template<typename Ty>
    Vector2 &operator/=(Ty rhs)
    {
        x /= (double)rhs;
        y /= (double)rhs;
        return *this;
    }

    T Dot(const Vector2 &rhs) const { return x * rhs.x + y * rhs.y; }

    T Cross(const Vector2 &rhs) const { return x * rhs.y - y * rhs.x; }

    T Length() const { return (T)std::sqrt(LengthSquared()); }

    double LengthSquared() const { return (double)x * x + (double)y * y; }

    T Distance(const Vector2 &other) const
    {
        return (T)std::sqrt(DistanceSquared(other));
    }

    double DistanceSquared(const Vector2 &other) const
    {
        return (*this - other).LengthSquared();
    }

    Vector2 Normalized() const
    {
        double length = Length();
        return {x / length, y / length};
    }
};

typedef Vector2<int> Vector2i;
typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;

template<typename T>
struct Vector3 {
    T x{};
    T y{};
    T z{};

    Vector3() = default;

    template<typename T1, typename T2, typename T3>
    Vector3(T1 x_, T2 y_, T3 z_)
        : x((T)(x_))
        , y((T)(y_))
        , z((T)(z_))
    {
    }

    template<typename R>
    Vector3(const Vector3<R> &rhs)
        : Vector3(rhs.x, rhs.y, rhs.z)
    {
    }

    template<typename T1, typename T2, typename T3>
    void Set(T1 x_, T2 y_, T3 z_)
    {
        x = (T)x_;
        y = (T)y_;
        z = (T)z_;
    }

    Vector3 operator+(const Vector3 &rhs) const
    {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    Vector3 operator-(const Vector3 &rhs) const
    {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    template<typename Ty>
    Vector3 operator*(Ty rhs) const
    {
        return {(T)(x * rhs), (T)(y * rhs), (T)(z * rhs)};
    }

    template<typename Ty>
    Vector3 operator/(Ty rhs) const
    {
        return {(T)(x / rhs), (T)(y / rhs), (T)(z / rhs)};
    }

    Vector3 &operator+=(const Vector3 &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vector3 &operator-=(const Vector3 &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    template<typename Ty>
    Vector3 &operator*=(Ty rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    template<typename Ty>
    Vector3 &operator/=(Ty rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }

    T Dot(const Vector3 &rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    Vector3 Cross(const Vector3 &rhs) const
    {
        T nx = y * rhs.z - z * rhs.y;
        T ny = z * rhs.x - x * rhs.z;
        T nz = x * rhs.y - y * rhs.x;
        return {nx, ny, nz};
    }

    T Length() const { return std::sqrt(LengthSquared()); }

    T LengthSquared() const { return x * x + y * y + z * z; }

    T Distance(const Vector3 &other) const
    {
        return std::sqrt(DistanceSquared(other));
    }

    T DistanceSquared(const Vector3 &other) const
    {
        return (*this - other).LengthSquared();
    }

    Vector3 Normalized() const
    {
        T factor = 1 / Length();
        return {x * factor, y * factor, z * factor};
    }
};

typedef Vector3<int> Vector3i;
typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;
