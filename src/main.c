#include "stdio.h"
#include "stdlib.h"

#include "kvec.h"
#include "deque.h"

#include "matrix.h"
#include "core.h"

// Font formatting
const int FONT_SIZE = 24;

// Resource paths
const char *imagePath = "assets/img/battleback8.png";
const char *fontPath = "assets/font/MMXSNES.ttf";
const char *bgmPath = "assets/bgm/Mars.wav";
const char *sfxPath = "assets/sfx/hop.wav";

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

bool LoadFromObjectFile(Mesh3d *res, char *filename)
{
    FILE* fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return false;

    // Local cache of verts
    kvec_t(Vector4) verts;
    kv_init(verts);

    kvec_t(Triangle3d) tris;
    kv_init(tris);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (line[0] == 'v')
        {
            if (line[1] != 't') {
                Vector4 v = MakeVector4();

                int cur = 2;
                v.x = readFloatFromString(line, &cur);
                v.y = readFloatFromString(line, &cur);
                v.z = readFloatFromString(line, &cur);
                kv_push(Vector4, verts, v);
            }
        }

        if (line[0] == 'f')
        {
            int f[3];
            int cur = 2;
            f[0] = readIntFromString(line, &cur);
            f[1] = readIntFromString(line, &cur);
            f[2] = readIntFromString(line, &cur);

            Triangle3d tri = { kv_A(verts, f[0] - 1), kv_A(verts, f[1] - 1), kv_A(verts, f[2] - 1) };
            kv_push(Triangle3d, tris, tri);
        }
    }

    res->polygons = tris.a;
    res->polygonCount = kv_size(tris);

    kv_destroy(verts);
    free(line);
    fclose(fp);

    return true;
}

int main(int argc, char **argv) {
    InitWindow();

    float elapseds[3] = { 60, 60, 60 };
    int curElapsed = 0;

    Mix_Chunk *sfx = Mix_LoadWAV(sfxPath);
    Mix_Chunk *bgm = Mix_LoadWAV(bgmPath);

    bool done = false;

    // load resources
    TTF_Font *font = TTF_OpenFont(fontPath, FONT_SIZE);
    SDL_Surface *background = IMG_Load(imagePath);

    Vector3 light = Vector3Normalize(&(Vector3){ 0.5f, 0.5f, 1.0f });
    SetupLight(light);

    Mesh3d meshTeapot = { 0 };
    LoadFromObjectFile(&meshTeapot, "assets/obj/teapot.obj");

    Mesh3d meshCube = { 0 };
    LoadFromObjectFile(&meshCube, "assets/obj/cube.obj");

    Mesh3d meshMonkey = { 0 };
    LoadFromObjectFile(&meshMonkey, "assets/obj/monkey.obj");

    float fTheta = 0.0f;
    float prevSecs = (float)SDL_GetTicks() / 1000.0f;


    Camera3d camera = {
        .position = {0.0f, 0.0f, 0.0f},
        .target = {0.0f, 0.0f, 1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 70.0f,
    };
    Matrix4 projMatrix = Matrix_MakeProjection(camera.fovy, (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH, 0.1f, 1000.0f);

    bool printed = false;
    bool wireframe = false;
    bool showdepth = false;
    while (!done && !WindowShouldClose()) {
        float now = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = now - prevSecs;
        prevSecs = now;

        if (IsKeyDown(BUTTON_R1)) {
            fTheta += elapsed;
        }
        if (IsKeyDown(BUTTON_L1)) {
            fTheta -= elapsed;
        }

        if (IsKeyPressed(BUTTON_SELECT)) {
            wireframe = !wireframe;
        }

        if (IsKeyPressed(SDLK_0)) {
            showdepth = !showdepth;
        }

        if (IsKeyDown(BUTTON_X)) {
            CameraMoveUp(&camera, 8.0f * elapsed);
        }
        if (IsKeyDown(BUTTON_B)) {
            CameraMoveUp(&camera, -8.0f * elapsed);
        }

        if (IsKeyDown(BUTTON_UP)) {
            CameraMoveForward(&camera, 8.0f * elapsed);
        }
        if (IsKeyDown(BUTTON_DOWN)) {
            CameraMoveForward(&camera, -8.0f * elapsed);
        }
        if (IsKeyDown(BUTTON_LEFT)) {
            CameraMoveRight(&camera, -8.0f * elapsed);
        }
        if (IsKeyDown(BUTTON_RIGHT)) {
            CameraMoveRight(&camera, 8.0f * elapsed);
        }
        if (IsKeyDown(BUTTON_Y)) {
            CameraYaw(&camera, 2.0f * elapsed);
        }
        if (IsKeyDown(BUTTON_A)) {
            CameraYaw(&camera, -2.0f * elapsed);
        }

        if (IsKeyPressed(BUTTON_MENU)) {
            done = true;
        }

        BeginDrawing();

        DrawRectangle(NULL, COLOR_BLACK);

        Matrix4 viewMatrix = Matrix_LookAt(&camera.position, &camera.target, &camera.up);
        BeginMode3d(&camera);

        DrawModel(&meshTeapot, (Vector3){0.0f, 0.0f, 5.0f});
        DrawModel(&meshCube, (Vector3){5.0f, 0.0f, 5.0f});
        DrawModel(&meshMonkey, (Vector3){-5.0f, 0.0f, 5.0f});
        DrawModel(&meshMonkey, (Vector3){-5.0f, 5.0f, 5.0f});
        DrawModel(&meshMonkey, (Vector3){5.0f, -5.0f, 5.0f});

        if (showdepth) {
            float* depths = Platform_GetDepthBuffer();
            float min = MAX_FLOAT;
            float max = MIN_FLOAT;
            for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
                if (depths[i] != MIN_FLOAT) {
                    if (depths[i] < min) {
                        min = depths[i];
                    }
                    if (depths[i] > max) {
                        max = depths[i];
                    }
                }
            }
            Uint32 *pixels = (Uint32*)Platform_GetScreenSurface()->pixels;

            for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
                float v = depths[i] == MIN_FLOAT ? min : depths[i];
                float c = 255 * (v - min) / (max - min);
                pixels[i] = SDL_MapRGB(Platform_GetScreenSurface()->format,
                    c, c, c);
            }
        }

        EndMode3d();

        char str[100];
        sprintf(str, "%3.2f FPS", 1.0f / elapsed);
        DrawTextEx(font, str, (Vector2){5, 5}, COLOR_WHITE);
        EndDrawing();
    }
    free(meshCube.polygons);

    Mix_HaltChannel(-1);

    Mix_FreeChunk(sfx);
    Mix_FreeChunk(bgm);

    TTF_CloseFont(font);

    SDL_FreeSurface(background);

    return CloseWindow();
}
