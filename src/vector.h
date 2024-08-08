#ifndef _VECTOR_H
#define _VECTOR_H

typedef struct Vector2 {
    union {
        float x;
        float u;
    };
    union {
        float y;
        float v;
    };
} Vector2;

typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;

Vector3 MakeVector3();
Vector4 MakeVector4();

Vector3 MakeVector3FromVector4(Vector4 v);

Vector3 Vector3Add(Vector3 v1, Vector3 v2);
Vector3 Vector3Sub(Vector3 v1, Vector3 v2);
Vector3 Vector3Mul(Vector3 v, float k);
float Vector3DotProduct(Vector3 v1, Vector3 v2);
Vector3 Vector3Normalize(Vector3 *v);
Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2);
Vector3 Vector3RotateByAxisAngle(Vector3 v, Vector3 axis, float angle);

Vector4 VectorAdd(Vector4 v1, Vector4 v2);
Vector4 VectorSub(Vector4 v1, Vector4 v2);
Vector4 VectorMul(Vector4 v, float k);
Vector4 VectorDiv(Vector4 v, float k);
float VectorDot(Vector4 v1, Vector4 v2);
Vector4 VectorCross(Vector4 v1, Vector4 v2);
void VectorNormalize(Vector4 *v);

#endif
