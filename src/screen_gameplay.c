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
#include <xkeycheck.h>

//----------------------------------------------------------------------------------
// Data: see https://www.retrogamedeconstructionzone.com/2019/10/asteroids-by-numbers.html
//----------------------------------------------------------------------------------
/*
Screen Res: 1024x768

Object	Length (in player ship lengths)
---------------------------------------
Screen              25 x 36
Large Asteroids	    2.4
Medium Asteroid	    1.2
Small Asteroid	    0.6
Alien Ship (large)  1.5
Alien Ship (small)  0.75

*/

/* Notes: 
    Concept of "alive" differs, for the bullet its, gameobject->active, for the asteroid it's asteroid->gameobject != -1 

*/

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Ship Definitions
//----------------------------------------------------------------------------------

// Nominally 1 long
static Vector2 shipData[4] = {
    {-0.25f, 0.5f},
    {0.0f, -0.5f},
    {0.25f, 0.5f},
    {-0.25f, 0.5f},
};

static float shipRotationFactor = 2.0f;
static float shipAccelartionFactor = .2f;
static float shipDecelrationFactor = .995f;
static float shipMaxSpeed = 14.0f;
static float shipSpeedCutoff = 0.05f;
static float gameScale = 28;

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
    int objectId;
    float lifetime;
} Bullet;

static Bullet bullets[MAX_BULLETS] = { 0 };

static float bulletInitialLifetime = .5f; // seconds
static float bulletInitialVelocity = 14.0f;

static float lastCallTime = 0.0f;
static float dt = 0.0f;

typedef struct Asteroid {
    int objectId;
    int size;
} Asteroid;

#define MAX_ASTEROIDS 100
static Asteroid asteroids[MAX_ASTEROIDS] = { 0 };

static Vector2 asteroidDataLarge[11] = {
    {-0.5f, 1.2f},
    {-1.2f, 0.6f},
    {-1.2f, -0.9f},
    {-0.5f, -1.2f},
    {0.0f, -0.9f},
    {0.5f, -1.2f},
    {1.2f, -0.9f},
    {1.0f, 0.3f},
    {1.2f, 0.6f},
    {0.5f, 1.2f},
    {-0.5f, 1.2f}
};

static Vector2 asteroidDataMedium[11] = { 0 };
static Vector2 asteroidDataSmall[11] = { 0 };
 
static int asteroidVertexCount = 11;

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
    float rotVel;
    Vector2 velocity;
} Object;

#define MAX_GAME_OBJECTS 100
static Object gameobjects[MAX_GAME_OBJECTS] = { 0 };
static int game_object_count = 0;

//----------------------------------------------------------------------------------
// Gameobject Stack
//----------------------------------------------------------------------------------


typedef struct Stack {
    int current;
    int size;
    int content[MAX_GAME_OBJECTS];
} Stack;

Stack stack;

void StackPush(Stack *stack, int index) {
    if (stack->current < stack->size) {
        stack->content[stack->current++] = index;
    }
    else {
        TraceLog(LOG_FATAL, "Push onto full stack");
    }
}

int StackPop(Stack* stack) {
    if (stack->current > 0) {
        return stack->content[--stack->current];
    }
    else {
        TraceLog(LOG_FATAL, "Pop for empty stack");
        return 0xffffffff;
    }
}

void InitGameObjectStack() {
    stack.size = MAX_GAME_OBJECTS;
    // 0, MAX_BULLETS are used for the ship and bullets respectively
    for (int i = MAX_BULLETS + 1; i < stack.size; ++i) {
        StackPush(&stack, i);
    }
}

// Resets Ship to original position and orientation
void ResetShip() {
    Object* ship = &gameobjects[0];
    ship->position = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    ship->velocity = Vector2Zero();
    ship->rot = 0;
    ship->rotVel = 0;
}

void ResetBullets() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        Bullet* bullet = &bullets[i];
        if (bullet->objectId < 0) continue;
        gameobjects[bullet->objectId].active = false;
        bullet->lifetime = -1;
    }
}

// Adds asteroid on the border, size is always large
void AddAsteroid() {
    int asteroidId = -1;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].objectId <= 0) {
            asteroidId = i;
            break;
        }
    }

    if (asteroidId < 0) {
        TraceLog(LOG_FATAL, "Out of Asteroids");
        return;
    }

    TraceLog(LOG_INFO, "Adding Asteroid");

    int objId = StackPop(&stack);
    Object* obj = &gameobjects[objId];

    asteroids[asteroidId].objectId = objId;
    asteroids[asteroidId].size = 4; // Sizes 1,2,4

    obj->active = true;
    if (obj->vertexCount < asteroidVertexCount || obj->vertexCount == 0) {
        free(obj->vertices);
        obj->vertices = (Vector2*)RL_CALLOC(asteroidVertexCount, sizeof(Vector2));
    }
    obj->vertexCount = asteroidVertexCount;
    obj->initialVertices = asteroidDataLarge;

    int sector = GetRandomValue(0, 3);
    switch (sector) {
        case 0:
            obj->position = (Vector2){ 0,(float)GetRandomValue(0, GetScreenHeight()) };
            break;
        case 1:
            obj->position = (Vector2){(float)GetScreenWidth() , (float)GetRandomValue(0, GetScreenHeight())};
            break;
        case 2:
            obj->position = (Vector2){ 0,(float)GetRandomValue(0, GetScreenHeight()) };
            break;
        case 3:
            obj->position = (Vector2){ (float)GetScreenWidth() ,(float)GetRandomValue(0, GetScreenHeight()) };
            break;
        default:
            TraceLog(LOG_WARNING, "Sector switch received invalid sector");
    }

    float rot = (float)GetRandomValue(0, 359) * PI / 180.0f;
    float vel = (float)GetRandomValue(8, 13) / 2.0f;
    obj->velocity = Vector2Scale(Vector2Rotate(yUp, rot), vel);
    obj->rotVel = (float)GetRandomValue(-100, 100) / 1000.0f;
}

void BreakAsteroid(Asteroid* asteroid, Object* obj) {
    if (asteroid->size == 1) {
        StackPush(&stack, asteroid->objectId);
        *asteroid = (Asteroid){ -1, -1 };
        obj->active = false;
        // TODO Add Score
        return;
    }
    int newSize = (asteroid->size == 4) ? 2 : 1;
    Vector2* initialVertexData = (newSize == 2) ? asteroidDataMedium : asteroidDataSmall;
    asteroid->size = newSize;

    // Reuse the Original GameObject and add a new one
    // Vertex Count stays the same
    obj->initialVertices = initialVertexData;
    obj->velocity = Vector2Rotate(obj->velocity, PI / 2.0f);
    obj->rotVel = (float)GetRandomValue(-100, 100) / 1000.0f;

    int newAsteroid = -1;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].objectId == -1) {
            newAsteroid = i;
            break;
        }
    }

    if (newAsteroid < 0) {
        TraceLog(LOG_FATAL, "Out of asteroids");
        return;
    }
    
    int newId = StackPop(&stack); 
    asteroids[newAsteroid] = (Asteroid){ newId, newSize };

    Object* newObj = &gameobjects[newId];

    newObj->active = true;
    if (newObj->vertexCount < asteroidVertexCount || newObj->vertexCount == 0) {
        free(newObj->vertices);
        newObj->vertices = (Vector2*)RL_CALLOC(asteroidVertexCount, sizeof(Vector2));
    }
    newObj->vertexCount = asteroidVertexCount;
    newObj->initialVertices = initialVertexData;
    newObj->position = obj->position;
    newObj->velocity = Vector2Negate(obj->velocity);
    newObj->rotVel = (float)GetRandomValue(-100, 100) / 1000.0f;
}


// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    InitGameObjectStack();

    Object* ship = &gameobjects[0];
    ship->vertexCount = 4;
    ship->active = true;
    ship->initialVertices = shipData;
    ship->vertices = (Vector2*)RL_CALLOC(ship->vertexCount, sizeof(Vector2));
    ResetShip();
    game_object_count++;

    for (int i = 0; i < MAX_BULLETS; ++i) {
        Object* obj = &gameobjects[game_object_count];
        obj->active = false;
        obj->vertexCount = bullet_vertex_count;
        obj->initialVertices = bullet_data;
        obj->vertices = (Vector2*)RL_CALLOC(obj->vertexCount, sizeof(Vector2));

        Bullet* bullet = &bullets[i];
        bullet->lifetime = 0;
        bullet->objectId = game_object_count;
        game_object_count++;
    }

    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        asteroids[i] = (Asteroid){ -1, -1 };
    }

    for (int i = 0; i < asteroidVertexCount; ++i) {
        asteroidDataMedium[i] = Vector2Scale(asteroidDataLarge[i], .5);
        asteroidDataSmall[i] = Vector2Scale(asteroidDataLarge[i], .25);
    }

    AddAsteroid();
    AddAsteroid();

    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;
}

// React to user input, calculate new orientation, spawn bullets
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

    // Spawn Bullets and shoot
    bool doShoot = IsKeyPressed(KEY_SPACE);

    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i].lifetime = Clamp(bullets[i].lifetime - dt, -1.0f, 999.0f);

        if (bullets[i].lifetime <= 0.0) {
            Object* obj = &gameobjects[bullets[i].objectId];
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

void CheckCollisions() {
    // Check Ship againts Asteroids
    // Check Bullets against Asteroids
    
    Object* ship = &gameobjects[0];

    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->objectId < 0) continue;
        Object* aObj = &gameobjects[asteroid->objectId];

        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, aObj->position, 0.3f * asteroid->size * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit by asteroid %d", i);
            //ResetShip();
            //ResetBullets();
            break;
        }

        for (int j = 0; j < MAX_BULLETS; ++j) {
            Object* bObj = &gameobjects[bullets[j].objectId];
            if (!bObj->active) continue;
            if (CheckCollisionPointCircle(bObj->position, aObj->position, 0.3f * asteroid->size * gameScale)) {
                TraceLog(LOG_INFO, "Asteroid hit by bullet");
                BreakAsteroid(asteroid, aObj);
                bullets[j].lifetime = -1;
                bObj->active = false;
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

    UpdateShip(&gameobjects[0]);

    // Update all game objects data

    for (int i = 0; i < MAX_GAME_OBJECTS; ++i) {
        Object* obj = &gameobjects[i];

        if (!obj->active) continue;

        obj->position = Vector2Add(obj->position, obj->velocity);
        obj->position.x = Wrap(obj->position.x, 0, (float)GetScreenWidth());
        obj->position.y = Wrap(obj->position.y, 0, (float)GetScreenHeight());

        obj->rot = Wrap(obj->rot + obj->rotVel, 0.0f, 360.0f);

        for (int v = 0; v < obj->vertexCount; ++v) {
            obj->vertices[v] = Vector2Add(Vector2Scale(Vector2Rotate(obj->initialVertices[v], obj->rot * PI
                / 180.0f), gameScale), obj->position);
        }
    }

    CheckCollisions();
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // TODO: Draw GAMEPLAY screen here!
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    //Vector2 pos = { 20, 10 };
    //DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize * 3.0f, 4, MAROON);
    //DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

    for (int i = 0; i < MAX_GAME_OBJECTS; ++i) {
        Object* obj = &gameobjects[i];
        if (!obj->active) continue;
        DrawLineStrip(obj->vertices, obj->vertexCount, RAYWHITE);
    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    for (int i = 0; i < MAX_GAME_OBJECTS; ++i) {
        Object* obj = &gameobjects[i];
        free(obj->vertices);
    }

    stack.current = 0;
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}