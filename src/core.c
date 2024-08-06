#include "math.h"

#include "core.h"

typedef struct
{
    SDL_Surface *video;
    SDL_Surface *screen;
    float *depthBuffer;
} PlatformData;

static PlatformData platform = {0};

static int currentKeyState[MAX_KEYBOARD_KEYS];
static int previousKeyState[MAX_KEYBOARD_KEYS];
static bool windowShouldClose = false;

SDL_Surface* Platform_GetScreenSurface() {
    return platform.screen;
}

float *Platform_GetDepthBuffer() {
    return platform.depthBuffer;
}

int InitWindow()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Mix_Init(MIX_INIT_OGG);
    Mix_OpenAudio(
            MIX_DEFAULT_FREQUENCY,
            MIX_DEFAULT_FORMAT,
            MIX_DEFAULT_CHANNELS,
            AUDIO_CHUNK_SIZE
    );

    platform.video = SDL_SetVideoMode(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        BITS_PER_PIXEL,
        SDL_HWSURFACE | SDL_DOUBLEBUF);

    platform.screen = SDL_CreateRGBSurface(
        SDL_HWSURFACE,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        BITS_PER_PIXEL,
        0, 0, 0, 0);

    // platform.depthBuffer = (float*)malloc(sizeof(float) * SCREEN_HEIGHT * SCREEN_WIDTH);
    // for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
    //     platform.depthBuffer[i] = 0.0f;
    // }

    return 0;
}

int CloseWindow()
{
    SDL_FreeSurface(platform.screen);
    SDL_FreeSurface(platform.video);
    // free(platform.depthBuffer);

    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}

bool WindowShouldClose() {
    return windowShouldClose;
}

bool IsKeyDown(int key)
{
    bool down = false;

    if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
    {
        if (currentKeyState[key] == 1)
            down = true;
    }

    return down;
}

bool IsKeyPressed(int key)
{
    bool pressed = false;

    if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
    {
        if ((previousKeyState[key] == 0) && (currentKeyState[key] == 1)) pressed = true;
    }

    return pressed;
}

void PollInputEvents()
{
    SDL_Event event;

    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++)
    {
        previousKeyState[i] = currentKeyState[i];
        // keyRepeatInFrame[i] = 0;
    }

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT: windowShouldClose = true; break;
        case SDL_KEYDOWN:
        {
            int key = event.key.keysym.sym;

            if (key != BUTTON_NA)
            {
                // If key was up, add it to the key pressed queue
                // if ((currentKeyState[key] == 0) && (CORE.Input.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE))
                // {
                //     CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
                //     CORE.Input.Keyboard.keyPressedQueueCount++;
                // }

                currentKeyState[key] = 1;
            }
        } break;
        case SDL_KEYUP:
        {
            int key = event.key.keysym.sym;

            if (key != BUTTON_NA) currentKeyState[key] = 0;
        } break;
        }
    }
}

int BeginDrawing()
{
    // for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
    //     platform.depthBuffer[i] = 0.0f;
    // }

    return 0;
}

int EndDrawing()
{
    SDL_BlitSurface(platform.screen, NULL, platform.video, NULL);
    SDL_Flip(platform.video);

    PollInputEvents();

    return 0;
}

void DrawRectangle(SDL_Rect *rect, SDL_Color color)
{
    SDL_FillRect(platform.screen, rect, SDL_MapRGB(platform.screen->format, color.r, color.g, color.b));
}

void DrawImage(SDL_Surface *image)
{
    SDL_BlitSurface(image, NULL, platform.screen, NULL);
}

void DrawTextEx(TTF_Font *font, const char *text, Vector2 position, SDL_Color color)
{
    SDL_Surface *textSurface = TTF_RenderUTF8_Solid(font, text, color);
    SDL_Rect target = {
        (Sint16)position.x,
        (Sint16)position.y,
        0,
        0,
    };
    SDL_BlitSurface(textSurface, NULL, platform.screen, &target);
    SDL_FreeSurface(textSurface);
}

void DrawPixel(int x, int y, SDL_Color color) {
    if (
           x < 0 || x + 1 > SCREEN_WIDTH
        || y < 0 || y + 1 > SCREEN_HEIGHT
    ) {
        return;
    }
    Uint32 *pixels = (Uint32*)platform.screen->pixels;
    pixels[ (y * platform.screen->w) + x ] = SDL_MapRGB(platform.screen->format, color.r, color.g, color.b);
}

void PutPixel(int x, int y, Uint32 pixel) {
    if (
           x < 0 || x + 1 > SCREEN_WIDTH
        || y < 0 || y + 1 > SCREEN_HEIGHT
    ) {
        return;
    }
    Uint32 *pixels = (Uint32*)platform.screen->pixels;
    pixels[ (y * platform.screen->w) + x ] = pixel;
}

void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, SDL_Color color)
{
    double x = endPosX - startPosX; 
    double y = endPosY - startPosY; 
    double rLength = Q_rsqrt(x*x + y*y);
    double addx = x * rLength;
    double addy = y * rLength;
    x = startPosX; 
    y = startPosY; 

    for (int i = 0; i < 1.0f / rLength; i++) { 
        DrawPixel(x, y, color); 
        x += addx; 
        y += addy; 
    } 
}

void DrawTriangle(Triangle3d triangle, SDL_Color color) {
    Vector3d points1 = triangle.points[0];
    Vector3d points2 = triangle.points[1];
    Vector3d points3 = triangle.points[2];
    
    DrawLine(
        points1.x, points1.y,
        points2.x, points2.y, color);
    DrawLine(
        points2.x, points2.y,
        points3.x, points3.y, color);
    DrawLine(
        points3.x, points3.y,
        points1.x, points1.y, color);
}

void DrawTexturedTriangle(
    int x1, int y1, float u1, float v1,
    int x2, int y2, float u2, float v2,
    int x3, int y3, float u3, float v3,
    SDL_Surface *tex
) {
    if (y2 < y1) {
        SWAP(y1, y2, int);
        SWAP(x1, x2, int);
        SWAP(u1, u2, float);
        SWAP(v1, v2, float);
    }
    if (y3 < y1) {
        SWAP(y1, y3, int);
        SWAP(x1, x3, int);
        SWAP(u1, u3, float);
        SWAP(v1, v3, float);
    }
    if (y3 < y2) {
        SWAP(y2, y3, int);
        SWAP(x2, x3, int);
        SWAP(u2, u3, float);
        SWAP(v2, v3, float);
    }

    int dy1   = y2 - y1;
    int dx1   = x2 - x1;
    float dv1 = v2 - v1;
    float du1 = u2 - u1;

    int dy2   = y3 - y1;
    int dx2   = x3 - x1;
    float dv2 = v3 - v1;
    float du2 = u3 - u1;

    float texU, texV;

    float daxStep = 0, dbxStep = 0,
          du1Step = 0, dv1Step = 0,
          du2Step = 0, dv2Step = 0;

    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    if (dy1) du1Step = du1 / (float)abs(dy1);
    if (dy1) dv1Step = dv1 / (float)abs(dy1);

    if (dy2) du2Step = du2 / (float)abs(dy2);
    if (dy2) dv2Step = dv2 / (float)abs(dy2);

    Uint32* pixels = (Uint32*)tex->pixels;

    if (dy1) {
        for (int i = y1; i <= y2; i++) {
            int ax = x1 + (float)(i - y1) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            float texSu = u1  + (float)(i - y1) * du1Step;
            float texSv = v1  + (float)(i - y1) * dv1Step;

            float texEu = u1  + (float)(i - y1) * du2Step;
            float texEv = v1  + (float)(i - y1) * dv2Step;

            if (ax > bx) {
                SWAP(ax, bx, int);
                SWAP(texSu, texEu, float);
                SWAP(texSv, texEv, float);
            }

            texU = texSu;
            texV = texSv;

            float tStep = 1.0f / (float)(bx - ax);
            float t = 0.0f;

            for (int j = ax; j < bx; j++) {
                texU = (1.0f - t) * texSu + t * texEu;
                texV = (1.0f - t) * texSv + t * texEv;

                float sx = texU * tex->w;
                float sy = texV * tex->h;

                int coord = (int)(sy * tex->w + sx);

                PutPixel(j, i, pixels[coord]);

                t += tStep;
            }
        }
    }

    dy1 = y3 - y2;
    dx1 = x3 - x2;
    dv1 = v3 - v2;
    du1 = u3 - u2;
    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    du1Step = 0; dv1Step = 0;
    if (dy1) du1Step = du1 / (float)abs(dy1);
    if (dy1) dv1Step = dv1 / (float)abs(dy1);

    if (dy1) {
        for (int i = y2; i <= y3; i ++) {
            int ax = x2 + (float)(i - y2) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            float texSu = u2 + (float)(i - y2) * du1Step;
            float texSv = v2 + (float)(i - y2) * dv1Step;

            float texEu = u1 + (float)(i - y1) * du2Step;
            float texEv = v1 + (float)(i - y1) * dv2Step;

            if (ax > bx) {
                SWAP(ax, bx, int);
                SWAP(texSu, texEu, float);
                SWAP(texSv, texEv, float);
            }

            texU = texSu;
            texV = texSv;

            float tStep = 1.0f / (float)(bx - ax);
            float t = 0.0f;

            for (int j = ax; j < bx; j++) {
                texU = (1.0f - t) * texSu + t * texEu;
                texV = (1.0f - t) * texSv + t * texEv;

                float sx = texU * tex->w;
                float sy = texV * tex->h;

                int coord = (int)(sy * tex->w + sx);

                PutPixel(j, i, pixels[coord]);

                t += tStep;
            }
        }
    }
}

void FillTriangle(
    int x1, int y1,
    int x2, int y2,
    int x3, int y3,
    SDL_Color color
) {
    if (y2 < y1) {
        SWAP(y1, y2, int);
        SWAP(x1, x2, int);
    }
    if (y3 < y1) {
        SWAP(y1, y3, int);
        SWAP(x1, x3, int);
    }
    if (y3 < y2) {
        SWAP(y2, y3, int);
        SWAP(x2, x3, int);
    }

    int dy1   = y2 - y1;
    int dx1   = x2 - x1;

    int dy2   = y3 - y1;
    int dx2   = x3 - x1;

    float texU, texV;

    float daxStep = 0, dbxStep = 0;

    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    if (dy1) {
        for (int i = y1; i <= y2; i++) {
            int ax = x1 + (float)(i - y1) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            if (ax > bx) {
                SWAP(ax, bx, int);
            }

            for (int j = ax; j < bx; j++) {
                DrawPixel(j, i, color);
            }
        }
    }

    dy1 = y3 - y2;
    dx1 = x3 - x2;
    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    if (dy1) {
        for (int i = y2; i <= y3; i ++) {
            int ax = x2 + (float)(i - y2) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            if (ax > bx) {
                SWAP(ax, bx, int);
            }

            for (int j = ax; j < bx; j++) {
                DrawPixel(j, i, color);
            }
        }
    }
}

Vector3d MakeVector3d() {
    return (Vector3d){
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
        .w = 1.0f,
    };
}

Vector3d VectorAdd(Vector3d v1, Vector3d v2) {
    return (Vector3d){
        .x = v1.x + v2.x,
        .y = v1.y + v2.y,
        .z = v1.z + v2.z,
        .w = 1.0f,
    };
}

Vector3d VectorSub(Vector3d v1, Vector3d v2) {
    return (Vector3d){
        .x = v1.x - v2.x,
        .y = v1.y - v2.y,
        .z = v1.z - v2.z,
        .w = 1.0f,
    };
}

Vector3d VectorMul(Vector3d v, float k) {
    return (Vector3d){ v.x * k, v.y * k, v.z * k, 1.0f };
}

Vector3d VectorDiv(Vector3d v, float k) {
    return (Vector3d){
        .x = v.x / k,
        .y = v.y / k,
        .z = v.z / k,
        .w = 1.0f
    };
}

float VectorDot(Vector3d v1, Vector3d v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vector3d VectorCross(Vector3d v1, Vector3d v2) {
    return (Vector3d){
        .x = v1.y * v2.z - v1.z * v2.y,
        .y = v1.z * v2.x - v1.x * v2.z,
        .z = v1.x * v2.y - v1.y * v2.x,
        .w = 1.0f,
    };
}

void VectorNormalize(Vector3d *v) {
    float rl = Q_rsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x *= rl; v->y *= rl; v->z *= rl;
}

Triangle3d InitTriangle3d() {
    Triangle3d tri = {
        .points = {
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        },
        .color = COLOR_WHITE
    };

    return tri;
}
