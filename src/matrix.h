#ifndef MATRIX_H
#define MATRIX_H

#include "math.h"
#include "core.h"

typedef struct Matrix4
{
    float m[4][4];
} Matrix4;

Matrix4 IdentityMatrix();

Matrix4 Matrix_MakeRotationX(float angleRad);
Matrix4 Matrix_MakeRotationY(float angleRad);
Matrix4 Matrix_MakeRotationZ(float angleRad);
Matrix4 Matrix_MakeTranslation(float x, float y, float z);
Matrix4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar);

Vector4 Matrix_MultiplyVector(Matrix4 mat, Vector4 in);
Matrix4 Matrix_MultiplyMatrix(Matrix4 *m1, Matrix4 *m2);
Matrix4 Matrix_LookAt(Vector3 *pos, Vector3 *target, Vector3 *up);
Matrix4 Matrix_QuickInverse(Matrix4 *m);

void PrintMatrix(Matrix4 *mat);

#endif
