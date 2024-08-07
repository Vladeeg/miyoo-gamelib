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
// const char *texPath = "assets/img/tex.png";
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
    kvec_t(Vector3d) verts;
    kv_init(verts);

    kvec_t(Triangle3d) tris;
    kv_init(tris);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (line[0] == 'v')
        {
            if (line[1] != 't') {
                Vector3d v = MakeVector3d();

                int cur = 2;
                v.x = readFloatFromString(line, &cur);
                v.y = readFloatFromString(line, &cur);
                v.z = readFloatFromString(line, &cur);
                kv_push(Vector3d, verts, v);
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

Vector3d Vector_IntersectPlane(Vector3d plane_p, Vector3d plane_n, Vector3d *lineStart, Vector3d *lineEnd)
{
    // VectorNormalize(&plane_n);
    float plane_d = -VectorDot(plane_n, plane_p);
    float ad = VectorDot(*lineStart, plane_n);
    float bd = VectorDot(*lineEnd, plane_n);
    float t = (-plane_d - ad) / (bd - ad);
    Vector3d lineStartToEnd = VectorSub(*lineEnd, *lineStart);
    Vector3d lineToIntersect = VectorMul(lineStartToEnd, t);

    return VectorAdd(*lineStart, lineToIntersect);
}

float Vector_PlaneDistance(Vector3d *plane_p, Vector3d *plane_n, Vector3d *p)
{
    // VectorNormalize(p);
    return (VectorDot(*plane_n, *p) - VectorDot(*plane_n, *plane_p));
};

int Triangle_ClipAgainstPlane(Vector3d plane_p, Vector3d plane_n, Triangle3d *in_tri, Triangle3d *out_tri1, Triangle3d *out_tri2)
{
    // Make sure plane normal is indeed normal
    VectorNormalize(&plane_n);

    // Create two temporary storage arrays to classify points either side of plane
    // If distance sign is positive, point lies on "inside" of plane
    Vector3d inside_points[3]  = { 0 };  int nInsidePointCount = 0;
    Vector3d outside_points[3] = { 0 }; int nOutsidePointCount = 0;

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
    // SDL_Surface *texture = IMG_Load(texPath);
    // SDL_Surface* tex = SDL_DisplayFormat(texture);

    // play music
    // int bgmChannel = Mix_PlayChannel(-1, bgm, -1);

    Vector3d light = { 0.5f, 0.5f, -1.0f, 1.0f };
    VectorNormalize(&light);

    Mesh3d meshCube = { 0 };

    LoadFromObjectFile(&meshCube, "assets/obj/teapot.obj");

    float fTheta = 0.0f;
    float prevSecs = (float)SDL_GetTicks() / 1000.0f;

    Matrix4 projMatrix = Matrix_MakeProjection(90.0f, (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH, 0.1f, 1000.0f);

    Vector3d camera = {0.0f, 0.0f, 0.0f, 1.0f};
    float cameraYaw = 0.0f;

    Vector3d lookDir = MakeVector3d();
    Vector3d strafeDir = lookDir;

    Vector3d forwardVector = MakeVector3d();
    Vector3d strafeVector = MakeVector3d();

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
            camera.y += 8.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_B)) {
            camera.y -= 8.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_Y)) {
            camera.x += 8.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_A)) {
            camera.x -= 8.0f * elapsed;
        }

        forwardVector = VectorMul(lookDir, 8.0f * elapsed);
        strafeVector = VectorMul(strafeDir, 8.0f * elapsed);
        if (IsKeyDown(BUTTON_UP)) {
            camera = VectorAdd(camera, forwardVector);
        }
        if (IsKeyDown(BUTTON_DOWN)) {
            camera = VectorSub(camera, forwardVector);
        }
        if (IsKeyDown(BUTTON_LEFT)) {
            camera = VectorAdd(camera, strafeVector);
        }
        if (IsKeyDown(BUTTON_RIGHT)) {
            camera = VectorSub(camera, strafeVector);
        }
        if (IsKeyDown(BUTTON_L2)) {
            cameraYaw -= 2.0f * elapsed;
        }
        if (IsKeyDown(BUTTON_R2)) {
            cameraYaw += 2.0f * elapsed;
        }

        // if (IsKeyPressed(BUTTON_A)) {
        //     Mix_PlayChannel(-1, sfx, 0);
        // }
        if (IsKeyPressed(BUTTON_MENU)) {
            done = true;
        }

        BeginDrawing();

        // Set up rotation matrices
        Matrix4 matRotZ = Matrix_MakeRotationZ(fTheta * 0.5f);
        Matrix4 matRotX = Matrix_MakeRotationX(fTheta);

        Matrix4 matTrans = Matrix_MakeTranslation(0.0f, 0.0f, 5.0f);

        Matrix4 matWorld = IdentityMatrix(); // Form World Matrix
        matWorld = Matrix_MultiplyMatrix(&matRotZ, &matRotX); // Transform by rotation
        matWorld = Matrix_MultiplyMatrix(&matWorld, &matTrans); // Transform by translation

        Vector3d upVector = {0.0f, 1.0f, 0.0f, 1.0f};
        Vector3d targetVector = {0.0f, 0.0f, 1.0f, 1.0f};
        Matrix4 cameraRot = Matrix_MakeRotationY(cameraYaw);
        lookDir = Matrix_MultiplyVector(cameraRot, targetVector);
        strafeDir = lookDir;
        strafeDir.y = lookDir.y + 0.1f;
        strafeDir = VectorCross(strafeDir, lookDir);
        VectorNormalize(&strafeDir);
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

        kvec_t(Triangle3d) trianglesToRaster;
        kv_init(trianglesToRaster);
        kdq_t(Triangle3d) trinaglesDeque;
        kdq_init(trinaglesDeque);

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
            float lightIntensity = MAX(0.1f, VectorDot(light, normal));
            tri.color = (SDL_Color){lightIntensity * 255, lightIntensity * 255, lightIntensity * 255};

            Vector3d cameraRay = VectorSub(triTransformed.points[0], camera);

            if (VectorDot(normal, cameraRay) < 0.0f) {
                // Convert World Space --> View Space
                triViewed.points[0] = Matrix_MultiplyVector(viewMatrix, triTransformed.points[0]);
                triViewed.points[1] = Matrix_MultiplyVector(viewMatrix, triTransformed.points[1]);
                triViewed.points[2] = Matrix_MultiplyVector(viewMatrix, triTransformed.points[2]);

                int clippedTriangles = 0;
                Triangle3d clipped[2] = { 0 };
                clippedTriangles = Triangle_ClipAgainstPlane(
                    (Vector3d){0.0f, 0.0f, 0.1f, 1.0f}, (Vector3d){0.0f, 0.0f, 1.0f, 1.0f},
                    &triViewed, &clipped[0], &clipped[1]
                );

                for (int n = 0; n < clippedTriangles; n++) {
                    // Project triangles from 3D --> 2D
                    triProjected.points[0] = Matrix_MultiplyVector(projMatrix, clipped[n].points[0]);
                    triProjected.points[1] = Matrix_MultiplyVector(projMatrix, clipped[n].points[1]);
                    triProjected.points[2] = Matrix_MultiplyVector(projMatrix, clipped[n].points[2]);

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

                    triProjected.color = tri.color;

                    kv_push(Triangle3d, trianglesToRaster, triProjected);
                }
            }
        }

        for (int i = 0; i < kv_size(trianglesToRaster); i++) {
            Triangle3d tri = kv_A(trianglesToRaster, i);

            Triangle3d clipped[2];
            kdq_push(Triangle3d, trinaglesDeque, tri);
            int newTriangles = 1;

            for (int p = 0; p < 4; p++) {
                int trisToAdd = 0;
                while (newTriangles > 0) {
                    Triangle3d test = kdq_A(trinaglesDeque, 0);
                    kdq_unshift(trinaglesDeque);
                    newTriangles--;

                    switch (p)
                    {
                    case 0:	trisToAdd = Triangle_ClipAgainstPlane(
                        (Vector3d){ 0.0f, 0.0f, 0.0f },
                        (Vector3d){ 0.0f, 1.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                        break;
                    case 1:	trisToAdd = Triangle_ClipAgainstPlane(
                        (Vector3d){ 0.0f, (float)SCREEN_HEIGHT - 1, 0.0f },
                        (Vector3d){ 0.0f, -1.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                        break;
                    case 2:	trisToAdd = Triangle_ClipAgainstPlane(
                        (Vector3d){ 0.0f, 0.0f, 0.0f },
                        (Vector3d){ 1.0f, 0.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                        break;
                    case 3:	trisToAdd = Triangle_ClipAgainstPlane(
                        (Vector3d){ (float)SCREEN_WIDTH - 1, 0.0f, 0.0f },
                        (Vector3d){ -1.0f, 0.0f, 0.0f }, &test, &clipped[0], &clipped[1]);
                        break;
                    }
                    for (int w = 0; w < trisToAdd; w++)
                        kdq_push(Triangle3d, trinaglesDeque, clipped[w]);
                }
                newTriangles = kdq_size(trinaglesDeque);
            }

            for (int i = 0 ; i < kdq_size(trinaglesDeque); i++) {
                Triangle3d tri = kdq_A(trinaglesDeque, i);
                FillTriangle(
                    (int)tri.points[0].x, (int)tri.points[0].y, tri.points[0].z,
                    (int)tri.points[1].x, (int)tri.points[1].y, tri.points[1].z,
                    (int)tri.points[2].x, (int)tri.points[2].y, tri.points[2].z,
                    tri.color
                );
                if (wireframe) {
                    DrawTriangle(tri, COLOR_GREEN);
                }
            }
            kdq_empty(trinaglesDeque);
        }

        kdq_empty(trinaglesDeque);
        kv_empty(trianglesToRaster);

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
        kdq_destroy(trinaglesDeque);
        kv_destroy(trianglesToRaster);

        printed = true;

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
