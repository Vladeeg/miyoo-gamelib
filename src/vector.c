#include "math.h"

#include "utils.h"

#include "vector.h"

Vector3 MakeVector3() {
    return (Vector3){
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
    };
}

Vector4 MakeVector4() {
    return (Vector4){
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
        .w = 1.0f,
    };
}

Vector3 MakeVector3FromVector4(Vector4 v) {
    return (Vector3){
        .x = v.x,
        .y = v.y,
        .z = v.z,
    };
}

Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
    return (Vector3){
        .x = v1.x + v2.x,
        .y = v1.y + v2.y,
        .z = v1.z + v2.z,
    };
}

Vector3 Vector3Sub(Vector3 v1, Vector3 v2) {
    return (Vector3){
        .x = v1.x - v2.x,
        .y = v1.y - v2.y,
        .z = v1.z - v2.z,
    };
}

Vector3 Vector3Mul(Vector3 v, float k) {
    return (Vector3){ v.x * k, v.y * k, v.z * k };
}

float Vector3DotProduct(Vector3 v1, Vector3 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2) {
    return (Vector3){
        .x = v1.y * v2.z - v1.z * v2.y,
        .y = v1.z * v2.x - v1.x * v2.z,
        .z = v1.x * v2.y - v1.y * v2.x,
    };
}

Vector3 Vector3RotateByAxisAngle(Vector3 v, Vector3 axis, float angle)
{
    // Using Euler-Rodrigues Formula
    // Ref.: https://en.wikipedia.org/w/index.php?title=Euler%E2%80%93Rodrigues_formula

    Vector3 result = v;

    // Vector3Normalize(axis);
    float ilength = Q_rsqrt(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
    axis.x *= ilength;
    axis.y *= ilength;
    axis.z *= ilength;

    angle /= 2.0f;
    float a = sinf(angle);
    float b = axis.x*a;
    float c = axis.y*a;
    float d = axis.z*a;
    a = cosf(angle);
    Vector3 w = { b, c, d };

    // Vector3CrossProduct(w, v)
    Vector3 wv = { w.y*v.z - w.z*v.y, w.z*v.x - w.x*v.z, w.x*v.y - w.y*v.x };

    // Vector3CrossProduct(w, wv)
    Vector3 wwv = { w.y*wv.z - w.z*wv.y, w.z*wv.x - w.x*wv.z, w.x*wv.y - w.y*wv.x };

    // Vector3Scale(wv, 2*a)
    a *= 2;
    wv.x *= a;
    wv.y *= a;
    wv.z *= a;

    // Vector3Scale(wwv, 2)
    wwv.x *= 2;
    wwv.y *= 2;
    wwv.z *= 2;

    result.x += wv.x;
    result.y += wv.y;
    result.z += wv.z;

    result.x += wwv.x;
    result.y += wwv.y;
    result.z += wwv.z;

    return result;
}

Vector4 VectorAdd(Vector4 v1, Vector4 v2) {
    return (Vector4){
        .x = v1.x + v2.x,
        .y = v1.y + v2.y,
        .z = v1.z + v2.z,
        .w = v1.w,
    };
}

Vector4 VectorSub(Vector4 v1, Vector4 v2) {
    return (Vector4){
        .x = v1.x - v2.x,
        .y = v1.y - v2.y,
        .z = v1.z - v2.z,
        .w = v1.w,
    };
}

Vector4 VectorMul(Vector4 v, float k) {
    return (Vector4){ v.x * k, v.y * k, v.z * k, 1.0f };
}

Vector4 VectorDiv(Vector4 v, float k) {
    return (Vector4){
        .x = v.x / k,
        .y = v.y / k,
        .z = v.z / k,
        .w = v.w
    };
}

float VectorDot(Vector4 v1, Vector4 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vector4 VectorCross(Vector4 v1, Vector4 v2) {
    return (Vector4){
        .x = v1.y * v2.z - v1.z * v2.y,
        .y = v1.z * v2.x - v1.x * v2.z,
        .z = v1.x * v2.y - v1.y * v2.x,
        .w = 1.0f,
    };
}

Vector3 Vector3Normalize(Vector3 *v) {
    float rl = Q_rsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    return (Vector3){
        v->x * rl,
        v->y * rl,
        v->z * rl,
    };
}

void VectorNormalize(Vector4 *v) {
    float rl = Q_rsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x *= rl; v->y *= rl; v->z *= rl;
}
