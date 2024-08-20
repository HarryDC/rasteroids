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
static Vector2 shipVertices[4] = { 0 };

static float shipRotationFactor = 1.0f;
static float shipAccelartionFactor = .1f;
static float shipDecelrationFactor = .99;
static float shipMaxSpeed = 1.0;
static float shipSpeedCutoff = 0.05;

//----------------------------------------------------------------------------------
// Bullet Definition
//---------------------------------------------------------------------------------- 

#define MAX_BULLETS 5
typedef struct Bullet {
    int object_num;
    int life_time;
} Bullet;

static Bullet bullets[MAX_BULLETS] = { 0 };

//----------------------------------------------------------------------------------
// Objects Definition
//----------------------------------------------------------------------------------

static const Vector2 yUp = { 0, -1 };

typedef struct Object {
    bool active;
    int vertexCount;
    Vector2* initialVertices;
    Vector2* vertices;
    Vector2 position;
    float rot;
    Vector2 velocity;
} Object;

#define MAX_GAME_OBJECTS 100
static Object gameObjects[MAX_GAME_OBJECTS] = { 0 };
static int gameObjectCount = 0;

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    Object* ship = &gameObjects[0];
    ship->vertexCount = 4;
    ship->active = true;
    ship->initialVertices = shipData;
    ship->position = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    ship->vertices = (Vector2*)RL_CALLOC(ship->vertexCount, sizeof(Vector2));
    gameObjectCount++;

    for (int i = 0; i < MAX_BULLETS; ++i) {
        Object* obj = &gameObjects[gameObjectCount];
        obj->active = false;
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

    ship->position = Vector2Add(ship->position, ship->velocity);

    if (IsKeyPressed(KEY_SPACE)) {

    }
}

// Gameplay Screen Update logic

void UpdateGameplayScreen(void)
{

    // TODO: Update GAMEPLAY screen variables here!

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }

    UpdateShip(&gameObjects[0]);

    // Update all game objects data

    for (int i = 0; i < gameObjectCount; ++i) {
        Object* obj = &gameObjects[i];
        for (int v = 0; v < obj->vertexCount; ++v) {
            obj->vertices[v] = Vector2Add(Vector2Scale(Vector2Rotate(obj->initialVertices[v], obj->rot * PI
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

    for (int i = 0; i < gameObjectCount; ++i) {
        Object* obj = &gameObjects[i];
        DrawLineStrip(obj->vertices, obj->vertexCount, RAYWHITE);
    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    for (int i = 0; i < gameObjectCount; ++i) {
        Object* obj = &gameObjects[i];
        free(obj->vertices);
    }
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}