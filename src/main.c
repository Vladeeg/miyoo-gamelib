#include "stdio.h"
#include "stdlib.h"

#include "kvec.h"

#include "math.h"
#include "core.h"

// Font formatting
const int FONT_SIZE = 64;

// Resource paths
const char *imagePath = "assets/img/battleback8.png";
// const char *texPath = "assets/img/tex.png";
const char *fontPath = "assets/font/MMXSNES.ttf";
const char *bgmPath = "assets/bgm/Mars.wav";
const char *sfxPath = "assets/sfx/hop.wav";

const SDL_Color COLOR_WHITE = {255, 255, 255};
const SDL_Color COLOR_GREEN = {0, 255, 0};
const SDL_Color COLOR_GRAY = {128, 128, 128};
const SDL_Color COLOR_BLACK = {0, 0, 0};

const int LOOP_MUSIC = 1;

float readFloatFromString(char* str, int *cur) {
    char buf[128];
    char c;

    for (int i = 0; i < 128 && c != ' '; i++, (*cur)++) {
        c = str[*cur];
        buf[i] = c;
    }

    return atof(buf);
}

int readIntFromString(char* str, int *cur) {
    char buf[128];
    char c;

    for (int i = 0; i < 128 && c != ' '; i++, (*cur)++) {
        c = str[*cur];
        buf[i] = c;
    }

    return atoi(buf);
}

bool LoadFromObjectFile(Mesh3d* res, char* filename)
{
    FILE* fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return false;

    // Local cache of verts
    kvec_t(Vector3d) verts;
    kv_init(verts);

    kvec_t(Triangle3d) tris;
    kv_init(tris);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        char buf[128];

        if (line[0] == 'v')
        {
            if (line[1] != 't') {
                Vector3d v = MakeVector3d();

                // s >> junk >> v.x >> v.y >> v.z;
                int cur = 2;
                float coord;
                coord = readFloatFromString(line, &cur);
                v.x = coord;
                coord = readFloatFromString(line, &cur);
                v.y = coord;
                coord = readFloatFromString(line, &cur);
                v.z = coord;
                kv_push(Vector3d, verts, v);
            }
        }

        if (line[0] == 'f')
        {
            int f[3];
            // s >> junk >> f[0] >> f[1] >> f[2];
            int cur = 2;
            float coord;
            coord = readIntFromString(line, &cur);
            f[0] = coord;
            coord = readIntFromString(line, &cur);
            f[1] = coord;
            coord = readIntFromString(line, &cur);
            f[2] = coord;

            Triangle3d tri = { kv_A(verts, f[0] - 1), kv_A(verts, f[1] - 1), kv_A(verts, f[2] - 1) };
            kv_push(Triangle3d, tris, tri);
        }
    }

    res->polygons = tris.a;
    res->polygonCount = kv_size(tris);

    fclose(fp);

    return true;
}


typedef struct Matrix4
{
    float m[4][4];
} Matrix4;

Vector3d Matrix_MultiplyVector(Matrix4 mat, Vector3d in) {
    Vector3d out = MakeVector3d();

    out.x = in.x * mat.m[0][0] + in.y * mat.m[1][0] + in.z * mat.m[2][0] + in.w * mat.m[3][0];
    out.y = in.x * mat.m[0][1] + in.y * mat.m[1][1] + in.z * mat.m[2][1] + in.w * mat.m[3][1];
    out.z = in.x * mat.m[0][2] + in.y * mat.m[1][2] + in.z * mat.m[2][2] + in.w * mat.m[3][2];
    out.w = in.x * mat.m[0][3] + in.y * mat.m[1][3] + in.z * mat.m[2][3] + in.w * mat.m[3][3];

    return out;
}

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

Matrix4 Matrix_MakeRotationY(float fAngleRad)
{
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = cosf(fAngleRad);
    matrix.m[0][2] = sinf(fAngleRad);
    matrix.m[2][0] = -sinf(fAngleRad);
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = cosf(fAngleRad);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

Matrix4 Matrix_MakeRotationZ(float fAngleRad)
{
    Matrix4 matrix = { 0 };
    matrix.m[0][0] = cosf(fAngleRad);
    matrix.m[0][1] = sinf(fAngleRad);
    matrix.m[1][0] = -sinf(fAngleRad);
    matrix.m[1][1] = cosf(fAngleRad);
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

int main(int argc, char **argv) {
    InitWindow();

    Mix_Chunk *sfx = Mix_LoadWAV(sfxPath);
    Mix_Chunk *bgm = Mix_LoadWAV(bgmPath);

    bool done = false;

    // load resources
    TTF_Font *font = TTF_OpenFont(fontPath, FONT_SIZE);
    SDL_Surface *background = IMG_Load(imagePath);
    // SDL_Surface *texture = IMG_Load(texPath);
    // SDL_Surface* tex = SDL_DisplayFormat(texture);

    // play music
    // int bgmChannel = Mix_PlayChannel(-1, bgm, -1);

    Vector3d light = { 0.5f, 0.5f, -1.0f, 1.0f };
    VectorNormalize(&light);

    Mesh3d meshCube = { 0 };
    
    // printf("xxxxxxxxxxxxxxxx\n");
    LoadFromObjectFile(&meshCube, "assets/obj/teapot.obj");
    
    // printf("zzzzzzzzzzzzzzzzz\n");

    // Triangle3d polygons[] = {

    //     // SOUTH
    //     { 0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f, },
    //     { 0.0f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f, },

    //     // EAST                                                      
    //     { 1.0f, 0.0f, 0.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f, },
    //     { 1.0f, 0.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f, },

    //     // NORTH                                                     
    //     { 1.0f, 0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f, },
    //     { 1.0f, 0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f, },

    //     // WEST
    //     { 0.0f, 0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f, },
    //     { 0.0f, 0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f, 1.0f, },

    //     // TOP
    //     { 0.0f, 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f, },
    //     { 0.0f, 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f, },

    //     // BOTTOM
    //     { 1.0f, 0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 0.0f, 1.0f, },
    //     { 1.0f, 0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f, },

    // };

    // meshCube.polygons = polygons;
    // meshCube.polygonCount = sizeof(polygons) / sizeof(Triangle3d);

    float fTheta = 0.0f;
    float prevSecs = (float)SDL_GetTicks() / 1000.0f;

    Matrix4 projMatrix = Matrix_MakeProjection(90.0f, (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH, 0.1f, 1000.0f);
    PrintMatrix(&projMatrix);

    Vector3d camera = {0.0f, 0.0f, 0.0f, 1.0f};

    bool printed = false;
    bool wireframe = false;
    while (!done && !WindowShouldClose()) {
        float now = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = now - prevSecs;
        prevSecs = now;

        BeginDrawing();

        if (IsKeyDown(BUTTON_A)) {
            fTheta += elapsed;
        }
        if (IsKeyDown(BUTTON_B)) {
            fTheta -= elapsed;
        }
        if (IsKeyPressed(BUTTON_X)) {
            wireframe = !wireframe;
        }
        if (IsKeyDown(BUTTON_UP)) {
            camera.y += 8.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_DOWN)) {
            camera.y -= 8.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_LEFT)) {
            camera.x += 8.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_RIGHT)) {
            camera.x -= 8.0f * elapsed;
        }
        // if (IsKeyPressed(BUTTON_A)) {
        //     Mix_PlayChannel(-1, sfx, 0);
        // }
        if (IsKeyPressed(BUTTON_MENU)) {
            done = true;
        }

        // Set up rotation matrices
        Matrix4 matRotZ = Matrix_MakeRotationZ(fTheta * 0.5f);
        Matrix4 matRotX = Matrix_MakeRotationX(fTheta);

        Matrix4 matTrans = Matrix_MakeTranslation(0.0f, 0.0f, 5.0f);

        Matrix4 matWorld = IdentityMatrix(); // Form World Matrix
        matWorld = Matrix_MultiplyMatrix(&matRotZ, &matRotX); // Transform by rotation
        matWorld = Matrix_MultiplyMatrix(&matWorld, &matTrans); // Transform by translation

        Vector3d upVector = {0.0f, 1.0f, 0.0f, 1.0f};
        Vector3d targetVector = {0.0f, 0.0f, 1.0f, 1.0f};
        Matrix4 cameraRot = Matrix_MakeRotationY(0.0f);
        Vector3d lookDir = Matrix_MultiplyVector(cameraRot, targetVector);
        targetVector = VectorAdd(camera, lookDir);
        Matrix4 cameraMatrix = Matrix_PointAt(&camera, &targetVector, &upVector);

        Matrix4 viewMatrix = Matrix_QuickInverse(&cameraMatrix);

        // clear screen
        DrawRectangle(NULL, COLOR_BLACK);
        // draw image to screen
        // DrawImage(background);
        // DrawImage(tex);

        // Vector2 pos = {10, 10};
        // DrawTextEx(font, "Hello world", pos, COLOR_WHITE);

        for (int i = 0; i < meshCube.polygonCount; i++) {
            Triangle3d tri = meshCube.polygons[i];
            Triangle3d triProjected = InitTriangle3d(), triTransformed = InitTriangle3d(), triViewed = InitTriangle3d();

            // World Matrix Transform
            triTransformed.points[0] = Matrix_MultiplyVector(matWorld, tri.points[0]);
            triTransformed.points[1] = Matrix_MultiplyVector(matWorld, tri.points[1]);
            triTransformed.points[2] = Matrix_MultiplyVector(matWorld, tri.points[2]);

            Vector3d line1 = VectorSub(triTransformed.points[1], triTransformed.points[0]);
            Vector3d line2 = VectorSub(triTransformed.points[2], triTransformed.points[0]);

            Vector3d normal = VectorCross(line1, line2);
            VectorNormalize(&normal);

            Vector3d cameraRay = VectorSub(triTransformed.points[0], camera);

            if (VectorDot(normal, cameraRay) < 0.0f) {
                // Convert World Space --> View Space
                triViewed.points[0] = Matrix_MultiplyVector(viewMatrix, triTransformed.points[0]);
                triViewed.points[1] = Matrix_MultiplyVector(viewMatrix, triTransformed.points[1]);
                triViewed.points[2] = Matrix_MultiplyVector(viewMatrix, triTransformed.points[2]);

                // Project triangles from 3D --> 2D
                triProjected.points[0] = Matrix_MultiplyVector(projMatrix, triViewed.points[0]);
                triProjected.points[1] = Matrix_MultiplyVector(projMatrix, triViewed.points[1]);
                triProjected.points[2] = Matrix_MultiplyVector(projMatrix, triViewed.points[2]);

                // Scale into view
                triProjected.points[0] = VectorDiv(triProjected.points[0], triProjected.points[0].w);
                triProjected.points[1] = VectorDiv(triProjected.points[1], triProjected.points[1].w);
                triProjected.points[2] = VectorDiv(triProjected.points[2], triProjected.points[2].w);

                // X/Y are inverted so put them back
                triProjected.points[0].x *= -1.0f;
                triProjected.points[1].x *= -1.0f;
                triProjected.points[2].x *= -1.0f;
                triProjected.points[0].y *= -1.0f;
                triProjected.points[1].y *= -1.0f;
                triProjected.points[2].y *= -1.0f;

                // Offset verts into visible normalised space
                Vector3d vOffsetView = { 1, 1, 0, 1.0f };
                triProjected.points[0] = VectorAdd(triProjected.points[0], vOffsetView);
                triProjected.points[1] = VectorAdd(triProjected.points[1], vOffsetView);
                triProjected.points[2] = VectorAdd(triProjected.points[2], vOffsetView);
                triProjected.points[0].x *= 0.5f * (float)SCREEN_WIDTH;
                triProjected.points[0].y *= 0.5f * (float)SCREEN_HEIGHT;
                triProjected.points[1].x *= 0.5f * (float)SCREEN_WIDTH;
                triProjected.points[1].y *= 0.5f * (float)SCREEN_HEIGHT;
                triProjected.points[2].x *= 0.5f * (float)SCREEN_WIDTH;
                triProjected.points[2].y *= 0.5f * (float)SCREEN_HEIGHT;

                float lightIntensity = MAX(0.1f, VectorDot(light, normal));

                FillTriangle(
                    (int)triProjected.points[0].x, (int)triProjected.points[0].y,
                    (int)triProjected.points[1].x, (int)triProjected.points[1].y,
                    (int)triProjected.points[2].x, (int)triProjected.points[2].y,
                    (SDL_Color){lightIntensity * 255, lightIntensity * 255, lightIntensity * 255}
                    // COLOR_GRAY
                );
                if (wireframe) {
                    DrawTriangle(triProjected, COLOR_GREEN);
                }
            }
        }

        EndDrawing();
    }
    Mix_HaltChannel(-1);

    Mix_FreeChunk(sfx);
    Mix_FreeChunk(bgm);

    TTF_CloseFont(font);

    SDL_FreeSurface(background);

    return CloseWindow();
}
