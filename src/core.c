#include "math.h"

#include "kvec.h"
#include "deque.h"

#include "core.h"
#include "matrix.h"

typedef struct
{
    SDL_Surface *video;
    SDL_Surface *screen;
    float *depthBuffer;
} PlatformData;

typedef struct RenderState {
    Matrix4 viewMatrix;
    Matrix4 projMatrix;
    Camera3d camera;
    Vector3 light;

    kvec_t(Triangle3d) trianglesToRaster;
    kdq_t(Triangle3d) trinaglesDeque;
} RenderState;

static PlatformData platform = {0};
static RenderState renderState = {0};

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

    platform.depthBuffer = (float*)malloc(sizeof(float) * SCREEN_HEIGHT * SCREEN_WIDTH);
    // for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
    //     platform.depthBuffer[i] = 0.0f;
    // }

    // TODO: move it to separate 3D initialization?
    kv_init(renderState.trianglesToRaster);
    kdq_init(renderState.trinaglesDeque);

    return 0;
}

int CloseWindow()
{
    kdq_destroy(renderState.trinaglesDeque);
    kv_destroy(renderState.trianglesToRaster);
    free(platform.depthBuffer);

    SDL_FreeSurface(platform.screen);
    SDL_FreeSurface(platform.video);

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
    for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
        platform.depthBuffer[i] = MIN_FLOAT;
    }

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

void DrawPixelDepth(int x, int y, float w, SDL_Color color) {
    if (
           x < 0 || x + 1 > SCREEN_WIDTH
        || y < 0 || y + 1 > SCREEN_HEIGHT
    ) {
        return;
    }
    if (w > platform.depthBuffer[y * SCREEN_WIDTH + x]) {
        Uint32 *pixels = (Uint32*)platform.screen->pixels;
        pixels[ (y * platform.screen->w) + x ] = SDL_MapRGB(platform.screen->format, color.r, color.g, color.b);

        platform.depthBuffer[y * SCREEN_WIDTH + x] = w;
    }
}

void DrawPixel(int x, int y, SDL_Color color) {
    DrawPixelDepth(x, y, MAX_FLOAT, color);
}

void PutPixelDepth(int x, int y, float w, Uint32 pixel) {
    if (
           x < 0 || x + 1 > SCREEN_WIDTH
        || y < 0 || y + 1 > SCREEN_HEIGHT
    ) {
        return;
    }

    if (w > platform.depthBuffer[y * SCREEN_WIDTH + x]) {
        Uint32 *pixels = (Uint32*)platform.screen->pixels;
        pixels[ (y * platform.screen->w) + x ] = pixel;

        platform.depthBuffer[y * SCREEN_WIDTH + x] = w;
    }
}

void PutPixel(int x, int y, Uint32 pixel) {
    PutPixelDepth(x, y, MAX_FLOAT, pixel);
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
    Vector4 points1 = triangle.points[0];
    Vector4 points2 = triangle.points[1];
    Vector4 points3 = triangle.points[2];
    
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

/* unfinished */
void DrawTexturedTriangle(
    int x1, int y1, float u1, float v1, float w1,
    int x2, int y2, float u2, float v2, float w2,
    int x3, int y3, float u3, float v3, float w3,
    SDL_Surface *tex
) {
    if (y2 < y1) {
        SWAP(y1, y2, int);
        SWAP(x1, x2, int);
        SWAP(u1, u2, float);
        SWAP(v1, v2, float);
        SWAP(w1, w2, float);
    }
    if (y3 < y1) {
        SWAP(y1, y3, int);
        SWAP(x1, x3, int);
        SWAP(u1, u3, float);
        SWAP(v1, v3, float);
        SWAP(w1, w3, float);
    }
    if (y3 < y2) {
        SWAP(y2, y3, int);
        SWAP(x2, x3, int);
        SWAP(u2, u3, float);
        SWAP(v2, v3, float);
        SWAP(w2, w3, float);
    }

    int dy1   = y2 - y1;
    int dx1   = x2 - x1;
    float dv1 = v2 - v1;
    float du1 = u2 - u1;
    float dw1 = w2 - w1;

    int dy2   = y3 - y1;
    int dx2   = x3 - x1;

    float dv2 = v3 - v1;
    float du2 = u3 - u1;
    float dw2 = w3 - w1;

    float texU, texV, texW;

    float daxStep = 0, dbxStep = 0,
          du1Step = 0, dv1Step = 0,
          du2Step = 0, dv2Step = 0,
          dw1Step = 0, dw2Step = 0;

    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    if (dy1) du1Step = du1 / (float)abs(dy1);
    if (dy1) dv1Step = dv1 / (float)abs(dy1);
    if (dy1) dw1Step = dw1 / (float)abs(dy1);

    if (dy2) du2Step = du2 / (float)abs(dy2);
    if (dy2) dv2Step = dv2 / (float)abs(dy2);
    if (dy2) dw2Step = dw2 / (float)abs(dw2);

    Uint32* pixels = (Uint32*)tex->pixels;

    if (dy1) {
        for (int i = y1; i <= y2; i++) {
            int ax = x1 + (float)(i - y1) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            float texSu = u1  + (float)(i - y1) * du1Step;
            float texSv = v1  + (float)(i - y1) * dv1Step;
            float texSw = w1  + (float)(i - y1) * dw1Step;

            float texEu = u1  + (float)(i - y1) * du2Step;
            float texEv = v1  + (float)(i - y1) * dv2Step;
            float texEw = w1  + (float)(i - y1) * dw2Step;

            if (ax > bx) {
                SWAP(ax, bx, int);
                SWAP(texSu, texEu, float);
                SWAP(texSv, texEv, float);
                SWAP(texSw, texEw, float);
            }

            texU = texSu;
            texV = texSv;
            texW = texSw;

            float tStep = 1.0f / (float)(bx - ax);
            float t = 0.0f;

            for (int j = ax; j < bx; j++) {
                texU = (1.0f - t) * texSu + t * texEu;
                texV = (1.0f - t) * texSv + t * texEv;
                texW = (1.0f - t) * texSv + t * texEv;

                float sx = (texU / texW) * tex->w;
                float sy = (texV / texW) * tex->h;

                int coord = (int)(sy * tex->w + sx);

                if (texW > platform.depthBuffer[i * SCREEN_WIDTH + j])
                {
                    PutPixel(j, i, pixels[coord]);
                    // Draw(j, i, tex->SampleGlyph(tex_u / tex_w, tex_v / tex_w), tex->SampleColour(tex_u / tex_w, tex_v / tex_w));
                    platform.depthBuffer[i * SCREEN_WIDTH + j] = texW;
                }

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
    int x1, int y1, float w1,
    int x2, int y2, float w2,
    int x3, int y3, float w3,
    SDL_Color color
) {
    if (y2 < y1) {
        SWAP(y1, y2, int);
        SWAP(x1, x2, int);
        SWAP(w1, w2, float);
    }
    if (y3 < y1) {
        SWAP(y1, y3, int);
        SWAP(x1, x3, int);
        SWAP(w1, w3, float);
    }
    if (y3 < y2) {
        SWAP(y2, y3, int);
        SWAP(x2, x3, int);
        SWAP(w2, w3, float);
    }

    int dy1 = y2 - y1;
    int dx1 = x2 - x1;

    float dw1 = w2 - w1;

    int dy2 = y3 - y1;
    int dx2 = x3 - x1;

    float dw2 = w3 - w1;

    float texU, texV, texW;

    float daxStep = 0, dbxStep = 0;

    float dw1Step = 0, dw2Step = 0;

    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    if (dy1) dw1Step = dw1 / (float)abs(dy1);
    if (dy2) dw2Step = dw2 / (float)abs(dy2);

    if (dy1) {
        for (int i = y1; i <= y2; i++) {
            int ax = x1 + (float)(i - y1) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            float texSw = w1 + (float)(i - y1) * dw1Step;
            float texEw = w1 + (float)(i - y1) * dw2Step;

            if (ax > bx) {
                SWAP(ax, bx, int);
                SWAP(texSw, texEw, float);
            }

            texW = texSw;

            float tstep = 1.0f / ((float)(bx - ax));
            float t = 0.0f;

            for (int j = ax; j < bx; j++) {
                texW = (1.0f - t) * texSw + t * texEw;
                DrawPixelDepth(j, i, texW, color);
                t += tstep;
            }
        }
    }

    dx1 = x3 - x2;
    dy1 = y3 - y2;
    dw1 = w3 - w2;

    if (dy1) daxStep = dx1 / (float)abs(dy1);
    if (dy2) dbxStep = dx2 / (float)abs(dy2);

    if (dy1) dw1Step = dw1 / (float)abs(dy1);

    if (dy1) {
        for (int i = y2; i <= y3; i ++) {
            int ax = x2 + (float)(i - y2) * daxStep;
            int bx = x1 + (float)(i - y1) * dbxStep;

            float texSw = w2 + (float)(i - y2) * dw1Step;
            float texEw = w1 + (float)(i - y1) * dw2Step;

            if (ax > bx) {
                SWAP(ax, bx, int);
                SWAP(texSw, texEw, float);
            }

            texW = texSw;

            float tstep = 1.0f / ((float)(bx - ax));
            float t = 0.0f;

            for (int j = ax; j < bx; j++) {
                texW = (1.0f - t) * texSw + t * texEw;
                DrawPixelDepth(j, i, texW, color);
                t += tstep;
            }
        }
    }
}

void CameraMoveForward(Camera3d* camera, float distance) {
    Vector3 forward = Vector3Sub(camera->target, camera->position);
    forward = Vector3Mul(forward, distance);
    camera->position = Vector3Add(camera->position, forward);
    camera->target = Vector3Add(camera->target, forward);
}

void CameraMoveRight(Camera3d* camera, float distance) {
    Vector3 right = Vector3CrossProduct(Vector3Sub(camera->target, camera->position), camera->up);
    right = Vector3Mul(right, distance);
    camera->position = Vector3Add(camera->position, right);
    camera->target = Vector3Add(camera->target, right);
}

void CameraMoveUp(Camera3d* camera, float distance) {
    Vector3 up = Vector3Mul(camera->up, distance);
    camera->position = Vector3Add(camera->position, up);
    camera->target = Vector3Add(camera->target, up);
}

void CameraYaw(Camera3d *camera, float angle)
{
    // Rotation axis
    Vector3 up = Vector3Normalize(&camera->up);

    // View vector
    Vector3 targetPosition = Vector3Sub(camera->target, camera->position);

    // Rotate view vector around up axis
    targetPosition = Vector3RotateByAxisAngle(targetPosition, up, angle);

    camera->target = Vector3Add(camera->position, targetPosition);
}

void BeginMode3d(Camera3d *camera) {
    Matrix4 viewMatrix = Matrix_LookAt(&camera->position, &camera->target, &camera->up);
    Matrix4 projMatrix = Matrix_MakeProjection(camera->fovy, (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH, 0.1f, 1000.0f);

    renderState.viewMatrix = viewMatrix;
    renderState.projMatrix = projMatrix;

    renderState.camera = *camera;
}

void EndMode3d() {
    // renderState.viewMatrix = NULL;
    // renderState.projMatrix = NULL;
    // renderState.camera = NULL;
}

void SetupLight(Vector3 light) {
    renderState.light = light;
}

Vector4 Vector_IntersectPlane(Vector4 plane_p, Vector4 plane_n, Vector4 *lineStart, Vector4 *lineEnd)
{
    // VectorNormalize(&plane_n);
    float plane_d = -VectorDot(plane_n, plane_p);
    float ad = VectorDot(*lineStart, plane_n);
    float bd = VectorDot(*lineEnd, plane_n);
    float t = (-plane_d - ad) / (bd - ad);
    Vector4 lineStartToEnd = VectorSub(*lineEnd, *lineStart);
    Vector4 lineToIntersect = VectorMul(lineStartToEnd, t);

    return VectorAdd(*lineStart, lineToIntersect);
}

float Vector_PlaneDistance(Vector4 *plane_p, Vector4 *plane_n, Vector4 *p)
{
    // VectorNormalize(p);
    return (VectorDot(*plane_n, *p) - VectorDot(*plane_n, *plane_p));
};

int Triangle_ClipAgainstPlane(Vector4 plane_p, Vector4 plane_n, Triangle3d *in_tri, Triangle3d *out_tri1, Triangle3d *out_tri2)
{
    // Make sure plane normal is indeed normal
    VectorNormalize(&plane_n);

    // Create two temporary storage arrays to classify points either side of plane
    // If distance sign is positive, point lies on "inside" of plane
    Vector4 inside_points[3]  = { 0 };  int nInsidePointCount = 0;
    Vector4 outside_points[3] = { 0 }; int nOutsidePointCount = 0;

    // Get signed distance of each point in triangle to plane
    float d0 = Vector_PlaneDistance(&plane_p, &plane_n, &in_tri->points[0]);
    float d1 = Vector_PlaneDistance(&plane_p, &plane_n, &in_tri->points[1]);
    float d2 = Vector_PlaneDistance(&plane_p, &plane_n, &in_tri->points[2]);

    if (d0 >= 0) {
        inside_points[nInsidePointCount++] = in_tri->points[0];
    } else {
        outside_points[nOutsidePointCount++] = in_tri->points[0];
    }
    if (d1 >= 0) {
        inside_points[nInsidePointCount++] = in_tri->points[1];
    } else {
        outside_points[nOutsidePointCount++] = in_tri->points[1];
    }
    if (d2 >= 0) {
        inside_points[nInsidePointCount++] = in_tri->points[2];
    } else {
        outside_points[nOutsidePointCount++] = in_tri->points[2];
    }

    // Now classify triangle points, and break the input triangle into 
    // smaller output triangles if required. There are four possible
    // outcomes...

    if (nInsidePointCount == 0)
    {
        // All points lie on the outside of plane, so clip whole triangle
        // It ceases to exist

        return 0; // No returned triangles are valid
    }

    if (nInsidePointCount == 3)
    {
        // All points lie on the inside of plane, so do nothing
        // and allow the triangle to simply pass through
        *out_tri1 = *in_tri;

        return 1; // Just the one returned original triangle is valid
    }

    if (nInsidePointCount == 1 && nOutsidePointCount == 2)
    {
        // Triangle should be clipped. As two points lie outside
        // the plane, the triangle simply becomes a smaller triangle

        out_tri1->color = in_tri->color;

        // The inside point is valid, so keep that...
        out_tri1->points[0] = inside_points[0];

        // but the two new points are at the locations where the 
        // original sides of the triangle (lines) intersect with the plane
        out_tri1->points[1] = Vector_IntersectPlane(plane_p, plane_n, &inside_points[0], &outside_points[0]);
        out_tri1->points[2] = Vector_IntersectPlane(plane_p, plane_n, &inside_points[0], &outside_points[1]);

        return 1; // Return the newly formed single triangle
    }

    if (nInsidePointCount == 2 && nOutsidePointCount == 1)
    {
        // Triangle should be clipped. As two points lie inside the plane,
        // the clipped triangle becomes a "quad". Fortunately, we can
        // represent a quad with two new triangles

        out_tri1->color = in_tri->color;
        out_tri2->color = in_tri->color;

        // The first triangle consists of the two inside points and a new
        // point determined by the location where one side of the triangle
        // intersects with the plane
        out_tri1->points[0] = inside_points[0];
        out_tri1->points[1] = inside_points[1];
        out_tri1->points[2] = Vector_IntersectPlane(plane_p, plane_n, &inside_points[0], &outside_points[0]);

        // The second triangle is composed of one of he inside points, a
        // new point determined by the intersection of the other side of the 
        // triangle and the plane, and the newly created point above
        out_tri2->points[0] = inside_points[1];
        out_tri2->points[1] = out_tri1->points[2];
        out_tri2->points[2] = Vector_IntersectPlane(plane_p, plane_n, &inside_points[1], &outside_points[0]);

        return 2; // Return two newly formed triangles which form a quad
    }

    // actually should not be reachable
    return 0;
}

void DrawModel(Mesh3d *mesh, Vector3 position) {
    // if (!renderState.camera || !renderState.viewMatrix || !renderState.projMatrix) {
    //     printf("Render was not set up!\n");
    //     exit(1);
    // }

    // Matrix4 matRotZ = Matrix_MakeRotationZ(0.0f);
    // Matrix4 matRotX = Matrix_MakeRotationX(0.0f);

    Matrix4 matTrans = Matrix_MakeTranslation(position.x, position.y, position.z);

    Matrix4 matWorld = IdentityMatrix(); // Form World Matrix
    // matWorld = Matrix_MultiplyMatrix(&matRotZ, &matRotX); // Transform by rotation
    matWorld = Matrix_MultiplyMatrix(&matWorld, &matTrans); // Transform by translation

    kdq_empty(renderState.trinaglesDeque);
    kv_empty(renderState.trianglesToRaster);

    for (int i = 0; i < mesh->polygonCount; i++) {
        Triangle3d tri = mesh->polygons[i];
        Triangle3d triProjected = InitTriangle3d(), triTransformed = InitTriangle3d(), triViewed = InitTriangle3d();

        // World Matrix Transform
        triTransformed.points[0] = Matrix_MultiplyVector(matWorld, tri.points[0]);
        triTransformed.points[1] = Matrix_MultiplyVector(matWorld, tri.points[1]);
        triTransformed.points[2] = Matrix_MultiplyVector(matWorld, tri.points[2]);

        Vector3 line1 = Vector3Sub(
            MakeVector3FromVector4(triTransformed.points[1]),
            MakeVector3FromVector4(triTransformed.points[0])
        );
        Vector3 line2 = Vector3Sub(
            MakeVector3FromVector4(triTransformed.points[2]),
            MakeVector3FromVector4(triTransformed.points[0])
        );

        Vector3 normal = Vector3CrossProduct(line1, line2);
        normal = Vector3Normalize(&normal);
        float lightIntensity = MAX(0.1f, Vector3DotProduct(renderState.light, normal));
        tri.color = (SDL_Color){lightIntensity * 255, lightIntensity * 255, lightIntensity * 255};

        Vector3 cameraRay = Vector3Sub(
            MakeVector3FromVector4(triTransformed.points[0]),
            renderState.camera.position
        );

        if (Vector3DotProduct(normal, cameraRay) < 0.0f) {
            // Convert World Space --> View Space
            triViewed.points[0] = Matrix_MultiplyVector(renderState.viewMatrix, triTransformed.points[0]);
            triViewed.points[1] = Matrix_MultiplyVector(renderState.viewMatrix, triTransformed.points[1]);
            triViewed.points[2] = Matrix_MultiplyVector(renderState.viewMatrix, triTransformed.points[2]);

            int clippedTriangles = 0;
            Triangle3d clipped[2] = { 0 };
            clippedTriangles = Triangle_ClipAgainstPlane(
                (Vector4){0.0f, 0.0f, 0.1f, 1.0f}, (Vector4){0.0f, 0.0f, 1.0f, 1.0f},
                &triViewed, &clipped[0], &clipped[1]
            );

            for (int n = 0; n < clippedTriangles; n++) {
                // Project triangles from 3D --> 2D
                triProjected.points[0] = Matrix_MultiplyVector(renderState.projMatrix, clipped[n].points[0]);
                triProjected.points[1] = Matrix_MultiplyVector(renderState.projMatrix, clipped[n].points[1]);
                triProjected.points[2] = Matrix_MultiplyVector(renderState.projMatrix, clipped[n].points[2]);

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
                triProjected.points[0].z *= -1.0f;
                triProjected.points[1].z *= -1.0f;
                triProjected.points[2].z *= -1.0f;

                // Offset verts into visible normalised space
                Vector4 vOffsetView = { 1, 1, 0, 1.0f };
                triProjected.points[0] = VectorAdd(triProjected.points[0], vOffsetView);
                triProjected.points[1] = VectorAdd(triProjected.points[1], vOffsetView);
                triProjected.points[2] = VectorAdd(triProjected.points[2], vOffsetView);
                triProjected.points[0].x *= 0.5f * (float)SCREEN_WIDTH;
                triProjected.points[0].y *= 0.5f * (float)SCREEN_HEIGHT;
                triProjected.points[1].x *= 0.5f * (float)SCREEN_WIDTH;
                triProjected.points[1].y *= 0.5f * (float)SCREEN_HEIGHT;
                triProjected.points[2].x *= 0.5f * (float)SCREEN_WIDTH;
                triProjected.points[2].y *= 0.5f * (float)SCREEN_HEIGHT;

                triProjected.color = tri.color;

                kv_push(Triangle3d, renderState.trianglesToRaster, triProjected);
            }
        }
    }

    for (int i = 0; i < kv_size(renderState.trianglesToRaster); i++) {
        Triangle3d tri = kv_A(renderState.trianglesToRaster, i);

        Triangle3d clipped[2];
        kdq_push(Triangle3d, renderState.trinaglesDeque, tri);
        int newTriangles = 1;

        for (int p = 0; p < 4; p++) {
            int trisToAdd = 0;
            while (newTriangles > 0) {
                Triangle3d test = kdq_A(renderState.trinaglesDeque, 0);
                kdq_unshift(renderState.trinaglesDeque);
                newTriangles--;

                switch (p)
                {
                case 0:	trisToAdd = Triangle_ClipAgainstPlane(
                    (Vector4){ 0.0f, 0.0f, 0.0f },
                    (Vector4){ 0.0f, 1.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                    break;
                case 1:	trisToAdd = Triangle_ClipAgainstPlane(
                    (Vector4){ 0.0f, (float)SCREEN_HEIGHT - 1, 0.0f },
                    (Vector4){ 0.0f, -1.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                    break;
                case 2:	trisToAdd = Triangle_ClipAgainstPlane(
                    (Vector4){ 0.0f, 0.0f, 0.0f },
                    (Vector4){ 1.0f, 0.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                    break;
                case 3:	trisToAdd = Triangle_ClipAgainstPlane(
                    (Vector4){ (float)SCREEN_WIDTH, 0.0f, 0.0f },
                    (Vector4){ -1.0f, 0.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                    break;
                }
                for (int w = 0; w < trisToAdd; w++)
                    kdq_push(Triangle3d, renderState.trinaglesDeque, clipped[w]);
            }
            newTriangles = kdq_size(renderState.trinaglesDeque);
        }

        for (int i = 0 ; i < kdq_size(renderState.trinaglesDeque); i++) {
            Triangle3d tri = kdq_A(renderState.trinaglesDeque, i);
            FillTriangle(
                (int)tri.points[0].x, (int)tri.points[0].y, tri.points[0].z,
                (int)tri.points[1].x, (int)tri.points[1].y, tri.points[1].z,
                (int)tri.points[2].x, (int)tri.points[2].y, tri.points[2].z,
                tri.color
            );
            // if (wireframe) {
                // DrawTriangle(tri, COLOR_GREEN);
            // }
        }
        kdq_empty(renderState.trinaglesDeque);
    }
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
