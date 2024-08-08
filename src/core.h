#ifndef CORE_H
#define CORE_H

#include "stdbool.h"
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_mixer.h"

#include "utils.h"
#include "buttonmap.h"
#include "colors.h"
#include "vector.h"

#define MIN_FLOAT -340282346638528859811704183484516925440.0f
#define MAX_FLOAT 340282346638528859811704183484516925440.0f

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BITS_PER_PIXEL 32

#define MAX_KEYBOARD_KEYS 512

#define AUDIO_CHUNK_SIZE 512

typedef struct Camera3d {
    Vector3 position;
    Vector3 target;
    Vector3 up;
    float fovy;
} Camera3d;

typedef struct Triangle3d {
    Vector4 points[3];
    SDL_Color color;
} Triangle3d;

typedef struct Mesh3d {
    Triangle3d *polygons;
    int polygonCount;
} Mesh3d;

SDL_Surface* Platform_GetScreenSurface();
float *Platform_GetDepthBuffer();

int InitWindow();
int CloseWindow();
bool WindowShouldClose();

bool IsKeyDown(int);
bool IsKeyPressed(int key);

int BeginDrawing();
int EndDrawing();

void DrawRectangle(SDL_Rect*, SDL_Color);
void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, SDL_Color color);
void DrawTriangle(Triangle3d triangle, SDL_Color color);

void DrawImage(SDL_Surface*);
void DrawTextEx(TTF_Font*, const char*, Vector2, SDL_Color);

void DrawPixel(int x, int y, SDL_Color color);
void PutPixel(int x, int y, Uint32 pixel);

void DrawTexturedTriangle(
    int x1, int y1, float u1, float v1, float w1,
    int x2, int y2, float u2, float v2, float w2,
    int x3, int y3, float u3, float v3, float w3,
    SDL_Surface *tex
);

void FillTriangle(
    int x1, int y1, float w1,
    int x2, int y2, float w2,
    int x3, int y3, float w3,
    SDL_Color color
);

Triangle3d InitTriangle3d();

void CameraMoveForward(Camera3d* camera, float distance);
void CameraMoveRight(Camera3d* camera, float distance);
void CameraMoveUp(Camera3d* camera, float distance);
void CameraYaw(Camera3d *camera, float angle);

void BeginMode3d(Camera3d *camera);
void EndMode3d();

void SetupLight(Vector3 light);
void DrawModel(Mesh3d *mesh, Vector3 position);


Vector4 Vector_IntersectPlane(Vector4 plane_p, Vector4 plane_n, Vector4 *lineStart, Vector4 *lineEnd);
float Vector_PlaneDistance(Vector4 *plane_p, Vector4 *plane_n, Vector4 *p);
int Triangle_ClipAgainstPlane(Vector4 plane_p, Vector4 plane_n, Triangle3d *in_tri, Triangle3d *out_tri1, Triangle3d *out_tri2);

#endif
