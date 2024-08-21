/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "screens.h"

#include "malloc.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Ship Definitions
//----------------------------------------------------------------------------------

static Vector2 shipData[4] = {
        {-1.0f, 1.0f},
        {0.0f, -3.0f},
        {1.0f, 1.0f},
        {-1.0f, 1.0f},
};

static float shipRotationFactor = 1.0f;
static float shipAccelartionFactor = .1f;
static float shipDecelrationFactor = .99f;
static float shipMaxSpeed = 1.0f;
static float shipSpeedCutoff = 0.05f;

//----------------------------------------------------------------------------------
// Bullet Definition
//---------------------------------------------------------------------------------- 
static Vector2 bullet_data[5] = {
        {-0.1f, -0.1f},
        {0.1f, -0.1f},
        {0.1f, 0.1f},
        {-0.1f, 0.1f},
        {-0.1f, -0.1f}
};
static const int bullet_vertex_count = 5;

#define MAX_BULLETS 5
typedef struct Bullet {
    int objectNum;
    float lifetime;
} Bullet;

static Bullet bullets[MAX_BULLETS] = { 0 };

static float bulletInitialLifetime = 5; // seconds
static float bulletInitialVelocity = 2.0f;

static float lastCallTime = 0.0f;
static float dt = 0.0f;
//----------------------------------------------------------------------------------
// Objects Definition
//----------------------------------------------------------------------------------

static const Vector2 yUp = { 0, -1 };

typedef struct Object {
    bool active;
    int vertex_count;
    Vector2* initial_vertices;
    Vector2* vertices;
    Vector2 position;
    float rot;
    Vector2 velocity;
} Object;

#define MAX_GAME_OBJECTS 100
static Object game_objects[MAX_GAME_OBJECTS] = { 0 };
static int game_object_count = 0;

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    Object* ship = &game_objects[0];
    ship->vertex_count = 4;
    ship->active = true;
    ship->initial_vertices = shipData;
    ship->position = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    ship->vertices = (Vector2*)RL_CALLOC(ship->vertex_count, sizeof(Vector2));
    game_object_count++;

    for (int i = 0; i < MAX_BULLETS; ++i) {
        Object* obj = &game_objects[game_object_count];
        obj->active = false;
        obj->vertex_count = bullet_vertex_count;
        obj->initial_vertices = bullet_data;
        obj->vertices = (Vector2*)RL_CALLOC(obj->vertex_count, sizeof(Vector2));

        Bullet* bullet = &bullets[i];
        bullet->lifetime = 0;
        bullet->objectNum = game_object_count;
        game_object_count++;
    }

    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;


}

void UpdateShip(Object* ship) {
    
    if (IsKeyDown(KEY_A))
    {
        ship->rot -= shipRotationFactor;
    }
    else if (IsKeyDown(KEY_D)) {
        ship->rot += shipRotationFactor;
    }

    ship->rot = Wrap(ship->rot, 0, 360);

    ship->velocity = Vector2Scale(ship->velocity, shipDecelrationFactor);

    Vector2 fwd = Vector2Rotate(yUp, ship->rot * PI / 180.0f);
    Vector2 accell = Vector2Scale(fwd, .1f);
    if (IsKeyDown(KEY_W)) {
        ship->velocity = Vector2Add(ship->velocity, accell);
    }

    float mag = Vector2Length(ship->velocity);
    if (mag < shipSpeedCutoff) {
        ship->velocity = Vector2Zero();
    }
    else {
        ship->velocity = Vector2ClampValue(ship->velocity, -shipMaxSpeed, shipMaxSpeed);
    }

    // Update Bullets and shoot
    bool doShoot = IsKeyPressed(KEY_SPACE);

    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i].lifetime = Clamp(bullets[i].lifetime - dt, -1.0f, 999.0f);

        if (bullets[i].lifetime <= 0.0) {
            Object* obj = &game_objects[bullets[i].objectNum];
            obj->active = false;
            if (doShoot)
            {
                bullets[i].lifetime = bulletInitialLifetime;
                obj->active = true;
                obj->position = ship->position;
                obj->velocity = Vector2Scale(fwd, bulletInitialVelocity);
                doShoot = false;
            }
        }
    }
}

// Gameplay Screen Update logic

void UpdateGameplayScreen(void)
{
    dt = GetFrameTime();
    // TODO: Update GAMEPLAY screen variables here!

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }

    UpdateShip(&game_objects[0]);

    // Update all game objects data

    for (int i = 0; i < game_object_count; ++i) {
        Object* obj = &game_objects[i];
        
        if (!obj->active) continue;

        obj->position = Vector2Add(obj->position, obj->velocity);

        for (int v = 0; v < obj->vertex_count; ++v) {
            obj->vertices[v] = Vector2Add(Vector2Scale(Vector2Rotate(obj->initial_vertices[v], obj->rot * PI
            / 180.0f), 20), obj->position);
        }
    }

}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // TODO: Draw GAMEPLAY screen here!
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), PURPLE);
    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

    for (int i = 0; i < game_object_count; ++i) {
        Object* obj = &game_objects[i];
        DrawLineStrip(obj->vertices, obj->vertex_count, RAYWHITE);
    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    for (int i = 0; i < game_object_count; ++i) {
        Object* obj = &game_objects[i];
        free(obj->vertices);
    }
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}