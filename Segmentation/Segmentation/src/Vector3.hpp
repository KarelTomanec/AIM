#pragma once

#include <cmath>
#include <stdlib.h>
#include <iostream>

class Vector3 {
public:

	float x, y, z;

public:

	Vector3() = default;

	Vector3(float x)
		: x(x), y(x), z(x)
	{}

	Vector3(float x, float y, float z)
		: x(x), y(y), z(z)

	{}


	float& operator [](int i)
	{
		return (&x)[i];
	}

	const float& operator [](int i) const
	{
		return (&x)[i];
	}


	Vector3 operator -() const { return Vector3(-x, -y, -z); }

	Vector3& operator *=(const float a) {
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}

	Vector3& operator *=(const Vector3& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	Vector3& operator /=(const float a) {
		return *this *= 1 / a;
	}

	Vector3& operator +=(const Vector3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vector3& operator -=(const Vector3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	float Y() {
		return 0.2126f * x + 0.7152f * y + 0.0722f * z;
	}

	inline float Average()
	{
		return (x + y + z) / 3.0f;
	}


};

inline Vector3 operator *(const Vector3& v, float a) {
	return Vector3(v.x * a, v.y * a, v.z * a);
}

inline Vector3 operator *(float a, const Vector3& v) {
	return Vector3(v.x * a, v.y * a, v.z * a);
}

inline Vector3 operator /(const Vector3& v, float a) {
	a = 1.0 / a;
	return Vector3(v.x * a, v.y * a, v.z * a);
}

inline Vector3 operator +(const Vector3& v, const Vector3& u) {
	return Vector3(v.x + u.x, v.y + u.y, v.z + u.z);
}

inline Vector3 operator -(const Vector3& v, const Vector3& u) {
	return Vector3(v.x - u.x, v.y - u.y, v.z - u.z);
}

inline Vector3 operator *(const Vector3& v, const Vector3& u) {
	return Vector3(v.x * u.x, v.y * u.y, v.z * u.z);
}

inline float Average(const Vector3& v) {
	return (v.x + v.y + v.z) / 3.0f;
}

inline float SquaredLength(const Vector3& v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float Length(const Vector3& v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vector3 Normalize(const Vector3& v) {
	return v / Length(v);
}

inline float Dot(const Vector3& v, const Vector3& u) {
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

inline Vector3 Cross(const Vector3& u, const Vector3& v)
{
	return Vector3(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
}

inline Vector3 Reflect(const Vector3& v, const Vector3& n) {
	return 2.0f * Dot(v, n) * n - v;
}

inline std::ostream& operator<<(std::ostream& out, const Vector3& v) {
	return out << v.x << ' ' << v.y << ' ' << v.z;
}

using Point3 = Vector3;
using Color3 = Vector3;
