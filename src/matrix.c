#include "matrix.h"

Matrix4 IdentityMatrix() {
    Matrix4 m = { 0 };
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;

    return m;
}

Matrix4 Matrix_MakeRotationX(float angleRad) {
    Matrix4 matrix = { 0 };
    matrix.m[0][0] =  1.0f;
    matrix.m[1][1] =  cosf(angleRad);
    matrix.m[1][2] =  sinf(angleRad);
    matrix.m[2][1] = -sinf(angleRad);
    matrix.m[2][2] =  cosf(angleRad);
    matrix.m[3][3] =  1.0f;
    return matrix;
}

Matrix4 Matrix_MakeRotationY(float angleRad)
{
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = cosf(angleRad);
    matrix.m[0][2] = sinf(angleRad);
    matrix.m[2][0] = -sinf(angleRad);
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = cosf(angleRad);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

Matrix4 Matrix_MakeRotationZ(float angleRad)
{
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = cosf(angleRad);
    matrix.m[0][1] = sinf(angleRad);
    matrix.m[1][0] = -sinf(angleRad);
    matrix.m[1][1] = cosf(angleRad);
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

Matrix4 Matrix_MakeTranslation(float x, float y, float z)
{
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    matrix.m[3][0] = x;
    matrix.m[3][1] = y;
    matrix.m[3][2] = z;
    return matrix;
}

Matrix4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
{
    float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = fAspectRatio * fFovRad;
    matrix.m[1][1] = fFovRad;
    matrix.m[2][2] = fFar / (fFar - fNear);
    matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
    matrix.m[2][3] = 1.0f;
    matrix.m[3][3] = 0.0f;
    return matrix;
}

Vector3d Matrix_MultiplyVector(Matrix4 mat, Vector3d in) {
    Vector3d out = MakeVector3d();

    out.x = in.x * mat.m[0][0] + in.y * mat.m[1][0] + in.z * mat.m[2][0] + in.w * mat.m[3][0];
    out.y = in.x * mat.m[0][1] + in.y * mat.m[1][1] + in.z * mat.m[2][1] + in.w * mat.m[3][1];
    out.z = in.x * mat.m[0][2] + in.y * mat.m[1][2] + in.z * mat.m[2][2] + in.w * mat.m[3][2];
    out.w = in.x * mat.m[0][3] + in.y * mat.m[1][3] + in.z * mat.m[2][3] + in.w * mat.m[3][3];

    return out;
}

Matrix4 Matrix_MultiplyMatrix(Matrix4 *m1, Matrix4 *m2) {
    Matrix4 res = { 0 };

    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            res.m[r][c] = m1->m[r][0] * m2->m[0][c] +
                          m1->m[r][1] * m2->m[1][c] +
                          m1->m[r][2] * m2->m[2][c] +
                          m1->m[r][3] * m2->m[3][c];
        }
    }

    return res;
}

Matrix4 Matrix_PointAt(Vector3d *pos, Vector3d *target, Vector3d *up)
{
    // Calculate new forward direction
    Vector3d newForward = VectorSub(*target, *pos);
    VectorNormalize(&newForward);

    // Calculate new Up direction
    Vector3d a = VectorMul(newForward, VectorDot(*up, newForward));
    Vector3d newUp = VectorSub(*up, a);
    VectorNormalize(&newUp);

    // New Right direction is easy, its just cross product
    Vector3d newRight = VectorCross(newUp, newForward);

    // Construct Dimensioning and Translation Matrix
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = newRight.x;   matrix.m[0][1] = newRight.y;   matrix.m[0][2] = newRight.z;   matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = newUp.x;      matrix.m[1][1] = newUp.y;      matrix.m[1][2] = newUp.z;      matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = newForward.x; matrix.m[2][1] = newForward.y; matrix.m[2][2] = newForward.z; matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = pos->x;       matrix.m[3][1] = pos->y;       matrix.m[3][2] = pos->z;       matrix.m[3][3] = 1.0f;
    return matrix;

}

Matrix4 Matrix_QuickInverse(Matrix4 *m) // Only for Rotation/Translation Matrices
{
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = m->m[0][0]; matrix.m[0][1] = m->m[1][0]; matrix.m[0][2] = m->m[2][0]; matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = m->m[0][1]; matrix.m[1][1] = m->m[1][1]; matrix.m[1][2] = m->m[2][1]; matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = m->m[0][2]; matrix.m[2][1] = m->m[1][2]; matrix.m[2][2] = m->m[2][2]; matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = -(m->m[3][0] * matrix.m[0][0] + m->m[3][1] * matrix.m[1][0] + m->m[3][2] * matrix.m[2][0]);
    matrix.m[3][1] = -(m->m[3][0] * matrix.m[0][1] + m->m[3][1] * matrix.m[1][1] + m->m[3][2] * matrix.m[2][1]);
    matrix.m[3][2] = -(m->m[3][0] * matrix.m[0][2] + m->m[3][1] * matrix.m[1][2] + m->m[3][2] * matrix.m[2][2]);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

void PrintMatrix(Matrix4 *mat) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%f ", mat->m[i][j]);
        }
        printf("\n");
    }
}
