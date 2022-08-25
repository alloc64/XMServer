/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

#define PI    3.1415926535897932384626433832795
#define DEG    PI/180

class vec3 {
public:
    float x, y, z;

public:
    inline vec3(void) {}

    vec3(float X, float Y, float Z) {
        x = X;
        y = Y;
        z = Z;
    }

    vec3(float num) {
        x = y = z = num;
    }

    friend vec3 operator*(float k, const vec3 V) {
        return vec3(V.x * k, V.y * k, V.z * k);
    }

    inline float operator*(vec3 V) const {
        return (x * V.x) + (y * V.y) + (z * V.z);
    }

    inline vec3 operator*(float Scalar) const {
        return vec3(x * Scalar, y * Scalar, z * Scalar);
    }

    vec3 operator-() {
        return vec3(-x, -y, -z);
    }

    inline vec3 operator+=(vec3 Other) {
        x += Other.x;
        y += Other.y;
        z += Other.z;

        return *this;
    }

    inline vec3 operator+=(float Other) {
        x += Other;
        y += Other;
        z += Other;

        return *this;
    }

    inline vec3 operator-=(vec3 Other) {
        x -= Other.x;
        y -= Other.y;
        z -= Other.z;

        return *this;
    }

    inline vec3 operator-=(float Other) {
        x -= Other;
        y -= Other;
        z -= Other;

        return *this;
    }

    inline vec3 operator*=(float Scalar) {
        x *= Scalar;
        y *= Scalar;
        z *= Scalar;

        return *this;
    }

    inline float GetLengthSquared(void) {
        return (*this) * (*this);
    }

    inline float length(void) {
        return sqrt(GetLengthSquared());
    }


    bool Normalise(bool AllowFatal = false) {
        float Length = length();
        if (Length == 0.0f) return false;
        (*this) *= (1.0f / Length);

        return true;
    }


    vec3 Interpolate(vec3 Vec1, vec3 Vec2, float Fraction) {
        x = Vec1.x * Fraction;
        y = Vec1.y * Fraction;
        z = Vec1.z * Fraction;

        float OneMFrac = 1.0f - Fraction;

        x += Vec2.x * OneMFrac;
        y += Vec2.y * OneMFrac;
        z += Vec2.z * OneMFrac;

        return *this;
    }

    inline vec3 operator/=(float Scalar) {
        *this *= (1.0f / Scalar);

        return *this;
    }


    inline vec3 Scale(vec3 Scalar) {
        x *= Scalar.x;
        y *= Scalar.y;
        z *= Scalar.z;

        return *this;
    }


    inline bool operator!=(vec3 Other) const {
        float Epsilon = 0.0001f;

        if ((fabs(Other.x - x) > Epsilon) ||
            (fabs(Other.y - y) > Epsilon) ||
            (fabs(Other.z - z) > Epsilon)) {
            return true;
        }

        return false;
    }

    inline bool operator==(vec3 Other) const {
        return !((*this) != Other);
    }


    inline vec3 operator+(vec3 fromV) {
        return vec3(x + fromV.x, y + fromV.y, z + fromV.z);
    }

    inline vec3 operator-(vec3 fromV) const {
        return vec3(x - fromV.x, y - fromV.y, z - fromV.z);
    }

    inline vec3 operator/(float Scalar) const {
        return (*this) * (1.0f / Scalar);
    }


    inline vec3 SetAndScale(float Scalar, vec3 Vec) {
        x = Scalar * Vec.x;
        y = Scalar * Vec.y;
        z = Scalar * Vec.z;

        return *this;
    }

};


inline float Pi(void) {
    static float gPi = (float) atan(1.0f) * 4.0f;
    return gPi;
}

inline float TwoPi(void) {
    static float gTwoPi = (float) atan(1.0f) * 8.0f;
    return gTwoPi;
}

inline float Modulo(float x, float div) { return (float) fmod((double) x, (double) div); }

inline float DegreesToRadians(float Degrees) { return Degrees * (Pi() / 180.0f); }

inline float RadiansToDegrees(float Radians) { return Radians * (180.0f / Pi()); }

inline float Sine(float Radians) { return (float) sin(Radians); }

inline float Cosine(float Radians) { return (float) cos(Radians); }

inline float Tangent(float Radians) { return (float) tan((double) Radians); }

inline float ArcTangent(float X) { return (float) atan((double) X); }

inline float ArcTangent(float X, float Y) { return (float) atan((double) X); }

inline float ArcSine(float X) { return (float) asin((double) X); }

inline float ArcCosine(float X) { return (float) acos((double) X); }

float Absolute(float num);

float pow(float pow);

vec3 Compute2Vectors(vec3 vStart, vec3 vEnd);

vec3 Cross(vec3 vec1, vec3 vec2);

float VectorLength(vec3 vNormal);

vec3 Normalize(vec3 vec);

vec3 Normal(vec3 vPolygon[]);

float Distance(vec3 vPoint1, vec3 vPoint2);

float Dot(vec3 vec1, vec3 vec2);

bool isZeroVector(vec3 v);

bool signum(float value);


inline bool SphereInSphere(vec3 spherePos1, float sphereRadius1, vec3 spherePos2, float sphereRadius2) {
    if (VectorLength(spherePos1 + spherePos2) < sphereRadius1 + sphereRadius2) return true;
    else return false;
}

inline bool
SphereInSphere(vec3 spherePos1, vec3 spherePos2, float sphereRadius1, float sphereRadius2, float &distance) {
    distance = VectorLength(spherePos1 + spherePos2);
    if (distance < sphereRadius1 + sphereRadius2) return true;
    else return false;
}

#endif
