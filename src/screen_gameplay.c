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
// Resourses: see https://www.classicgaming.cc/classics/asteroids/
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
// TODO Hyperspace
// TODO UFO


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

static int shipVertexCount = 4;

static float shipRotationFactor = 2.0f;
static float shipAccelartionFactor = .2f;
static float shipDecelrationFactor = .995f;
static float shipMaxSpeed = 14.0f;
static float shipSpeedCutoff = 0.05f;
static float gameScale = 28;

static Vector2 shipDeadData[2] = {
    {0, 0.25f},
    {0, -0.25f}
};
static int shipDeadObjects[4] = { 0 };

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

enum GameState {
    LEVEL_START,
    RUNNING,
    DYING,
    HYPERSPACE,
};

enum Enemies {
    NONE = 0,
    ASTEROID_SMALL = 1, 
    ASTEROID_MEDIUM = 2,
    UNUSED = 3,
    ASTEROID_LARGE = 4, 
    SAUCER_LARGE = 5, 
    SAUCER_SMALL = 6,
    MAX_TYPES = 7
};

typedef struct Game {
    int score;
    int lives;
    int hyperspace;
    int state;
    float stateTime;
} Game;

Game game;

typedef struct BackgroundSound {
    float elapsed;
    float interval;
    int beat;
} BackgroundSound;

BackgroundSound sound;

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
    for (int i = stack.size-1; i > 1; --i) {
        StackPush(&stack, i);
    }
}

void SetState(int state) {
    game.state = state;
    game.stateTime = 0;
}

void AddScore(int type) {
    static int scores[MAX_TYPES] = { -1, 100, 50, -1, 20, 200, 1000 };
    if (type > 0 && type < MAX_TYPES) {
        game.score += scores[type];
    }
    else {
        TraceLog(LOG_WARNING, "AddScore: called with invalid type");
    }
}

//----------------------------------------------------------------------------------
// Ship Functions
//----------------------------------------------------------------------------------

// Resets Ship to original position and orientation
void ResetShip() {
    Object* ship = &gameobjects[0];
    ship->active = true;
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

void ResetFragments() {
    for (int i = 0; i < 4; ++i) {
        gameobjects[shipDeadObjects[i]].active = false;
    }
}

bool CheckCollisionAsteroids(Vector2 pos, float radius) {
    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->objectId < 0) continue;
        Object* aObj = &gameobjects[asteroid->objectId];

        if (CheckCollisionCircles(pos, radius * gameScale, aObj->position, 0.3f * asteroid->size * gameScale)) {
            return true;
        }
    }
    return false; 
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

    if (IsKeyDown(KEY_S) && game.hyperspace > 0) {
        SetState(HYPERSPACE);
        // Calculate new position for ship, trying to get a bit of 
        // distance from any asteroids
        do {
            ship->position.x = (float)GetRandomValue(0, GetScreenWidth());
            ship->position.y = (float)GetRandomValue(0, GetScreenHeight());
        } while (CheckCollisionAsteroids(ship->position, 1.5));
        --game.hyperspace;
        ship->active = false;
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

    // TODO Draw Engine graphics when firing
}

void BreakShip(Object* ship) {
    ship->active = false;
    for (int i = 0; i < 3; ++i) {
        Object* obj = &gameobjects[shipDeadObjects[i]];
        obj->active = true;
        obj->position = ship->vertices[i];
        obj->velocity = Vector2Scale(Vector2Subtract(obj->position, ship->position), 0.01f);
        obj->rot = (float)GetRandomValue(0, 360);
        obj->rotVel = (float)GetRandomValue(0, 200) / 100.0;
    }
}


//----------------------------------------------------------------------------------
// Asteroid Functions
//----------------------------------------------------------------------------------

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
    asteroids[asteroidId].size = ASTEROID_LARGE; // Sizes 1,2,4

    obj->active = true;
    if (obj->vertexCount < asteroidVertexCount || obj->vertices == 0) {
        free(obj->vertices);
        obj->vertices = (Vector2*)RL_CALLOC(asteroidVertexCount, sizeof(Vector2));
    }
    obj->vertexCount = asteroidVertexCount;
    obj->initialVertices = asteroidDataLarge;

    // Start from a border
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

    AddScore(asteroid->size);

    if (asteroid->size == ASTEROID_SMALL) {
        StackPush(&stack, asteroid->objectId);
        *asteroid = (Asteroid){ -1, -1 };
        obj->active = false;
        AddScore(ASTEROID_SMALL);
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

    // Spawn a new asteroid
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

// Returns true if ship was destroyed
bool CheckCollisions() {

    Object* ship = &gameobjects[0];

    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->objectId < 0) continue;
        Object* aObj = &gameobjects[asteroid->objectId];

        // Check Collision of Asteroid w/ ship
        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, aObj->position, 0.3f * asteroid->size * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit by asteroid %d", i);
            BreakAsteroid(asteroid, aObj);
            BreakShip(ship);
            game.lives -= 1;
            return true;
        }

        // Check Collision of Asteroid w/ ship bullets
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
    return false;
}

void ResetLevel() {
    ResetShip();
    gameobjects[0].active = true;
    ResetBullets();
    ResetFragments();

    AddAsteroid();
    AddAsteroid();
    SetState(LEVEL_START);
}

void UpdateGameObjects() {
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
}

void UpdateBackgroundSound() {
    sound.elapsed += dt;
    if (sound.elapsed > sound.interval) {
        sound.elapsed = 0;
        PlaySound(sounds[sound.beat]);
        sound.beat = (sound.beat == SOUND_BEAT_1) ? SOUND_BEAT_2 : SOUND_BEAT_1;
    }
}

//----------------------------------------------------------------------------------
// Main Gameplay Functions (called by rayling_game.c
//----------------------------------------------------------------------------------

void InitGameplayScreen(void)
{
    InitGameObjectStack();

    game = (Game){ .score = 0, .lives = 1, .hyperspace = 2, .state = LEVEL_START, .stateTime = 0 };
    sound = (BackgroundSound){ .interval = 1, .elapsed = 0, .beat = SOUND_BEAT_1 };

    Object* ship = &gameobjects[0];
    ship->vertexCount = 4;
    ship->active = true;
    ship->initialVertices = shipData;
    ship->vertices = (Vector2*)RL_CALLOC(ship->vertexCount, sizeof(Vector2));
    ResetShip();

    for (int i = 0; i < MAX_BULLETS; ++i) {
        int objId = StackPop(&stack);
        Object* obj = &gameobjects[objId];
        obj->active = false;
        obj->vertexCount = bullet_vertex_count;
        obj->initialVertices = bullet_data;
        obj->vertices = (Vector2*)RL_CALLOC(obj->vertexCount, sizeof(Vector2));

        Bullet* bullet = &bullets[i];
        bullet->lifetime = 0;
        bullet->objectId = objId;
    }

    // Ship fragments
    for (int i = 0; i < 4; ++i) {
        int objId = StackPop(&stack);
        Object* obj = &gameobjects[objId];
        shipDeadObjects[i] = objId;
        obj->active = false;
        obj->vertexCount = 2;
        obj->initialVertices = shipDeadData;
        obj->vertices = (Vector2*)RL_CALLOC(ship->vertexCount, sizeof(Vector2));
    }

    // Asteroids are fully dynamic
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        asteroids[i] = (Asteroid){ -1, -1 };
    }


    for (int i = 0; i < asteroidVertexCount; ++i) {
        asteroidDataMedium[i] = Vector2Scale(asteroidDataLarge[i], .5);
        asteroidDataSmall[i] = Vector2Scale(asteroidDataLarge[i], .25);
    }

    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    ResetLevel();
}

void UpdateGameplayScreen(void)
{
    dt = GetFrameTime();
    game.stateTime += dt;
    // TODO: Update GAMEPLAY screen variables here!


    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
    }

    switch (game.state) {
    case LEVEL_START:
        if (game.stateTime > 3.0) {
            gameobjects[0].active = true;
            SetState(RUNNING);
        } 
        break;
    case HYPERSPACE:
    {
        UpdateBackgroundSound();
        if (game.stateTime > .75) {
            SetState(RUNNING);
            gameobjects[0].active = true;
        }
        UpdateGameObjects();
        break;
    }
    case RUNNING:
    {
        UpdateBackgroundSound();
        UpdateShip(&gameobjects[0]);
        UpdateGameObjects();
        if (CheckCollisions()) {
            SetState(DYING);
        }
        break;
    }
    case DYING:
    {
        UpdateGameObjects();
        if (game.stateTime > 3.0) {
            if (game.lives > 0) {
                ResetLevel();
            }
            else {
                finishScreen = 1;
            }
        }
        break;
    }
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

    // HUD
    DrawTextEx(font, TextFormat("%i", game.score), (Vector2) { 20, 20 }, font.baseSize, 1.0f,RAYWHITE);

    Vector2 pos = { 20, font.baseSize + 1.2f * gameScale };

    for (int i = 0; i < game.lives; ++i) {
        Vector2 start = Vector2Add(Vector2Scale(shipData[0], gameScale), pos);
        for (int v = 1; v < shipVertexCount; ++v)
        {
            Vector2 end = Vector2Add(Vector2Scale(shipData[v], gameScale), pos);
            DrawLineV(start, end, RAYWHITE);
            start = end;
        }
        pos.x += .8f * gameScale;
    }

    for (int i = 0; i < MAX_GAME_OBJECTS; ++i) {
        Object* obj = &gameobjects[i];
        if (!obj->active) continue;
        DrawLineStrip(obj->vertices, obj->vertexCount, RAYWHITE);
    }

    switch (game.state) {
    case LEVEL_START:
    {
        Vector2 center = (Vector2){ GetScreenWidth() / 2.0f - 17 , GetScreenHeight() / 2.0f - 2 };
        DrawTextEx(font, "PLAYER 1", center, font.baseSize, 1.0, RAYWHITE);
    }
    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    for (int i = 0; i < MAX_GAME_OBJECTS; ++i) {
        Object* obj = &gameobjects[i];
        free(obj->vertices);
        obj->vertices = 0;
        obj->active = false;
        *obj = (Object){ 0 };
    }

    stack.current = 0;
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}