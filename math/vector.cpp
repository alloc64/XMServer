/***********************************************************************
 * Copyright (c) 2009 Milan Jaitner                                   *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "vector.h"

float Absolute(float num) {
    if (num < 0) return (0 - num);
    return num;
}


vec3 Compute2Vectors(vec3 vStart, vec3 vEnd) {
    vec3 vVector;

    vVector.x = vEnd.x - vStart.x;
    vVector.y = vEnd.y - vStart.y;
    vVector.z = vEnd.z - vStart.z;

    Normalize(vVector);
    return vVector;
}

vec3 Cross(vec3 vVector1, vec3 vVector2) {
    vec3 vNormal;

    // Calculate the cross product with the non communitive equation
    vNormal.x = ((vVector1.y * vVector2.z) - (vVector1.z * vVector2.y));
    vNormal.y = ((vVector1.z * vVector2.x) - (vVector1.x * vVector2.z));
    vNormal.z = ((vVector1.x * vVector2.y) - (vVector1.y * vVector2.x));

    return vNormal;
}

float Dot(vec3 vVector1, vec3 vVector2) {
    // The dot product is this equation: V1.V2 = (V1.x * V2.x  +  V1.y * V2.y  +  V1.z * V2.z)
    return (float) ((vVector1.x * vVector2.x) + (vVector1.y * vVector2.y) + (vVector1.z * vVector2.z));
}

float VectorLength(vec3 vNormal) {
    // Here is the equation:  magnitude = sqrt(V.x^2 + V.y^2 + V.z^2) : Where V is the vector
    return (float) sqrt((vNormal.x * vNormal.x) + (vNormal.y * vNormal.y) + (vNormal.z * vNormal.z));
}

float pow(float pow) {
    return pow * pow;
}

vec3 Normalize(vec3 vVector) {
    float magnitude = VectorLength(vVector);
    vVector = vVector / magnitude;

    return vVector;
}

float Distance(vec3 vPoint1, vec3 vPoint2) {
    return fabs((vPoint1 - vPoint2).length());
}

bool isZeroVector(vec3 v) {
    if ((v.x == 0.0f) && (v.y == 0.0f) && (v.z == 0.0f)) return true;
    return false;
}

bool signum(float value) {
    if (value < 0.0f) return 1; else return 0;
}

