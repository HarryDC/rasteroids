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

// TODO Use non bold font for large text
// TODO Fix Asteroid speeds (smaller faster)
// TODO Thrust Graphics
// TODO Level Progression
// TODO Small Saucer brain, have it avoid asteroids
// TODO Speed up background pulse with level progression
// TODO scale velocity over dt
// TODO Add pause
// TODO Make Layout relative to screen size ? 

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Objects Definition
//----------------------------------------------------------------------------------

static const Vector2 yUp = { 0, -1 };

typedef struct Object {
    bool active;
    Vector2 position;
    Vector2 velocity;
    float rot;
    float rotVel;
    int vertexCount;
    Vector2* initialVertices;   // Not Owned here
    Vector2* vertices;          // Owned by the object
    int type;                   // type of data, allows cast
    void* data;                 // Not Owned here
} Object;

#define MAX_GAME_OBJECTS 100
static Object gameobjects[MAX_GAME_OBJECTS] = { 0 };

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
static float shipAccelrationFactor = .2f;
static float shipDecelrationFactor = .995f;
static float shipMaxSpeed = 14.0f;
static float shipSpeedCutoff = 0.05f;
static float gameScale = 28;

static Vector2 shipDeadData[2] = {
    {0, 0.25f},
    {0, -0.25f}
};
static Object* shipDeadObjects[4] = { NULL };

static int nextShipInterval = 5000;
static int nextShip = 5000;
static int nextHyperSpaceInverval = 2500;
static int nextHyperspace = 2500;

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
    Object* object;
    float lifetime;
} Bullet;

static Bullet bullets[MAX_BULLETS] = { 0 };

static float bulletInitialLifetime = 1.0f; // seconds
static float bulletInitialVelocity = 14.0f;

//----------------------------------------------------------------------------------
// Asteroid Definition
//---------------------------------------------------------------------------------- 

typedef struct Asteroid {
    Object *object;
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
// Saucer Definition
//---------------------------------------------------------------------------------- 
const int saucerVertexCount = 13;
static Vector2 saucerDataLarge[13] = {
    {-0.75f, 0.2f}, // Bottom CCW 
    {-0.4f, 0.5f},
    {0.4f, 0.5f},
    {0.75f, 0.2f},
    {-0.75f, 0.2f},
    {-0.4f, -0.1f}, // Middle CW
    {0.4f, -0.1f},
    {0.75f, 0.2f},
    {-0.75f, 0.2f},
    {-0.4f, -0.1f}, // Repeated (as we're drawing linestrip)
    {-0.3f, -0.5f}, // Top
    {0.3f, -0.5f},
    {0.4f, -0.1f}
};

static Vector2 saucerDataSmall[13] = { 0 };

typedef struct Saucer {
    Object* object;
    int type;
    int maxBullets;
    float shotFreq;
    float shotElapsed; // time since last shot
    float noSaucerElapsed;
} Saucer;

static float saucerSpawnFrequency = 100.0f;
static float saucerSpawnChance = 0.1f;

Saucer saucer;

//----------------------------------------------------------------------------------
// Game global Definition
//---------------------------------------------------------------------------------- 

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
    float dt;
    float stateTime;
} Game;

Game game;

typedef struct BackgroundSound {
    float elapsed;
    float interval;
    int beat;
} BackgroundSound;

BackgroundSound sound;

typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
} Particle;

#define MAX_PARTICLES 100 

typedef struct ParticleSystem {
    Particle particles[MAX_PARTICLES];
    int back;
} ParticleSystem;

ParticleSystem particleSystem;



//----------------------------------------------------------------------------------
// Gameobject Stack
//----------------------------------------------------------------------------------

typedef struct Stack {
    int current;
    int size;
    Object* content[MAX_GAME_OBJECTS];
} Stack;

Stack stack;

void StackPush(Stack *stack, Object* object) {
    if (stack->current < stack->size) {
        stack->content[stack->current++] = object;
    }
    else {
        TraceLog(LOG_FATAL, "Push onto full stack");
    }
}

Object* StackPop(Stack* stack) {
    if (stack->current > 0) {
        return stack->content[--stack->current];
    }
    else {
        TraceLog(LOG_FATAL, "Pop for empty stack");
        return NULL;
    }
}

// Assumes stack and objects have the same size 
void InitGameObjectStack(Stack *stack, Object objects[]) {
    stack->size = MAX_GAME_OBJECTS;
    for (int i = stack->size-1; i >= 0; --i) {
        StackPush(stack, &objects[i]);
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

Vector2 GetRandomEdgePosition() {
    // Start from a border
    int sector = GetRandomValue(0, 3);
    switch (sector) {
    case 0:
        return (Vector2){ 0,(float)GetRandomValue(0, GetScreenHeight()) };
    case 1:
        return (Vector2){ (float)GetScreenWidth() , (float)GetRandomValue(0, GetScreenHeight()) };
    case 2:
        return (Vector2) { 0, (float)GetRandomValue(0, GetScreenHeight()) };
    case 3:
        return (Vector2){ (float)GetScreenWidth() ,(float)GetRandomValue(0, GetScreenHeight()) };
    default:
        TraceLog(LOG_WARNING, "Sector switch received invalid sector");
        return Vector2Zero();
    }
}

//----------------------------------------------------------------------------------
// ParticleSystem Functions
//----------------------------------------------------------------------------------
void InitParticleSystem(ParticleSystem* system) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        system->particles[i].lifetime = -1;
    }
    system->back = -1;
}

bool AddParticle(ParticleSystem *system, Vector2 pos, Vector2 vel, float lifetime) {
    if (system->back < MAX_PARTICLES - 1) {
        ++system->back;
        system->particles[system->back] = (Particle){ .position = pos, .velocity = vel, .lifetime = lifetime };
        return true;
    }
    return false;
}

void UpdateParticles(ParticleSystem* system, float dt) {

    int i = 0;
    while (i <= system->back) {
        system->particles[i].lifetime -= dt;
        if (system->particles[i].lifetime < 0) {
            if (i < system->back) {
                system->particles[i] = system->particles[system->back];
            }
            --system->back;
            continue;
        }

        system->particles[i].position = Vector2Add(system->particles[i].position, system->particles[i].velocity);
        ++i;
    }
}

void DrawParticles(ParticleSystem* system) {
    for (int i = 0; i <= system->back; ++i) {
        DrawCircle((int)system->particles[i].position.x,
                   (int)system->particles[i].position.y, 2, WHITE);
    }
}

void SpawnExplosion(ParticleSystem* system, Vector2 pos, int count) {
    for (int i = 0; i < count; ++i) {
        int angle = GetRandomValue(0, 360);
        Vector2 vel = Vector2Scale(Vector2Rotate(yUp, angle * PI / 180.0f), 0.5f);
        if (!AddParticle(system, pos, vel, 2.5)) {
            TraceLog(LOG_WARNING,"Out of Particles");
            return;
        }
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
        if (bullet->object == NULL) continue;
        bullet->object->active = false;
        bullet->lifetime = -1;
    }
}

void ResetFragments() {
    for (int i = 0; i < 4; ++i) {
        shipDeadObjects[i]->active = false;
    }
}

bool CheckCollisionAsteroids(Vector2 pos, float radius) {
    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->object == NULL ) continue;
        if (CheckCollisionCircles(pos, radius * gameScale, asteroid->object->position, 0.3f * asteroid->size * gameScale)) {
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
        bullets[i].lifetime = Clamp(bullets[i].lifetime - game.dt, -1.0f, 999.0f);

        if (bullets[i].lifetime <= 0.0) {
            Object* obj = bullets[i].object;
            obj->active = false;
            if (doShoot)
            {
                PlaySound(sounds[SOUND_FIRE]);
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
        Object* obj = shipDeadObjects[i];
        obj->active = true;
        obj->position = ship->vertices[i];
        obj->velocity = Vector2Scale(Vector2Subtract(obj->position, ship->position), 0.01f);
        obj->rot = (float)GetRandomValue(0, 360);
        obj->rotVel = (float)GetRandomValue(0, 200) / 100.0f;
    }
}

//----------------------------------------------------------------------------------
// Bullet Functions
//----------------------------------------------------------------------------------

void SpawnBullet(Vector2 pos, Vector2 vel) {
    int id = -1;
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (bullets[i].lifetime < 0) {
            id = i;
            break;
        }
    }

    if (id == -1) {
        TraceLog(LOG_WARNING, "Out of bullets");
        return;
    }

    bullets[id].lifetime = bulletInitialLifetime;
    Object* obj = bullets[id].object;
    obj->active = true;
    obj->position = pos;
    obj->velocity = vel;
}


//----------------------------------------------------------------------------------
// Asteroid Functions
//----------------------------------------------------------------------------------

// Adds asteroid on the border, size is always large
void AddAsteroid() {
    int asteroidId = -1;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object == 0) {
            asteroidId = i;
            break;
        }
    }

    if (asteroidId < 0) {
        TraceLog(LOG_FATAL, "Out of Asteroids");
        return;
    }

    TraceLog(LOG_INFO, "Adding Asteroid");

    Object* obj = StackPop(&stack);

    asteroids[asteroidId].object = obj;
    asteroids[asteroidId].size = ASTEROID_LARGE; // Sizes 1,2,4

    obj->active = true;
    if (obj->vertexCount < asteroidVertexCount || obj->vertices == 0) {
        free(obj->vertices);
        obj->vertices = (Vector2*)RL_CALLOC(asteroidVertexCount, sizeof(Vector2));
    }
    obj->vertexCount = asteroidVertexCount;
    obj->initialVertices = asteroidDataLarge;
    obj->position = GetRandomEdgePosition();

    float rot = (float)GetRandomValue(0, 359) * PI / 180.0f;
    float vel = 4.0f;
    obj->velocity = Vector2Scale(Vector2Rotate(yUp, rot), vel);
    obj->rotVel = (float)GetRandomValue(-100, 100) / 1000.0f;
}

void BreakAsteroid(Asteroid* asteroid) {

    AddScore(asteroid->size);
    SpawnExplosion(&particleSystem, asteroid->object->position, 5);

    if (asteroid->size == ASTEROID_SMALL) {
        asteroid->object->active = false;
        StackPush(&stack, asteroid->object);
        *asteroid = (Asteroid){ .object = NULL, .size = -1 };
        AddScore(ASTEROID_SMALL);
        PlaySound(sounds[SOUND_BANG_SMALL]);
        return;
    }

    int newSize;
    Vector2* initialVertexData;
    if (asteroid->size == 4) {
        newSize = 2;
        initialVertexData = asteroidDataMedium;
        PlaySound(sounds[SOUND_BANG_LARGE]);
    }
    else {
        newSize = 1;
        initialVertexData = asteroidDataSmall;
        PlaySound(sounds[SOUND_BANG_MEDIUM]);

    }

    Object* obj = asteroid->object;
    // Reuse the Original GameObject and add a new one
    // Vertex Count stays the same
    obj->initialVertices = initialVertexData;
    obj->velocity = Vector2Rotate(obj->velocity, PI / 2.0f);
    obj->rotVel = (float)GetRandomValue(-100, 100) / 1000.0f;

    // Spawn a new asteroid
    int newAsteroid = -1;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object == NULL) {
            newAsteroid = i;
            break;
        }
    }

    if (newAsteroid < 0) {
        TraceLog(LOG_FATAL, "Out of asteroids");
        return;
    }


    Object* newObj = StackPop(&stack);
    asteroids[newAsteroid] = (Asteroid){ .object= newObj, .size = newSize };

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

void ResetAsteroids() {
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object != NULL) {
            asteroids[i].object->active = false;
            StackPush(&stack, asteroids[i].object);
            asteroids[i] = (Asteroid){ .object = NULL, .size = -1 };
        }
    }
}

//----------------------------------------------------------------------------------
// Saucer Functions
//----------------------------------------------------------------------------------

void SpawnSaucer(int type) {
    Object* obj = saucer.object;

    // Always Spawn on the RIM
    obj->active = true;
    obj->initialVertices = (type == SAUCER_LARGE) ? saucerDataLarge : saucerDataSmall;
    obj->velocity = (Vector2){ 1, 1 };
    obj->position = GetRandomEdgePosition();
    obj->rot = 0;
    obj->rotVel = 0;
}

// Source https://gamedev.net/forums/topic/401165-target-prediction-system--target-leading
float largest_root_of_quadratic_equation(float A, float B, float C) {
    return (B + sqrtf(B * B - 4 * A * C)) / (2 * A);
}

Vector2 intercept(Vector2 const shooter, float bullet_speed, Vector2 const target, Vector2 const target_velocity) {
    float a = bullet_speed * bullet_speed - Vector2DotProduct(target_velocity, target_velocity);
    Vector2 shooterToTarget = Vector2Subtract(target, shooter);
    float b = -2 * Vector2DotProduct(target_velocity, shooterToTarget);
    float c = -Vector2DotProduct(shooterToTarget, shooterToTarget);
    return Vector2Add(target, Vector2Scale(target_velocity, largest_root_of_quadratic_equation(a, b, c)));
}

Vector2 shoot_at(Vector2 const shooter, Vector2 const interception, float bullet_speed) {
    Vector2 v = Vector2Subtract(interception,shooter);
    float scale = bullet_speed / Vector2Length(v);
    return Vector2Scale(v, scale);
}

void LargeSaucerShoot() {
    int count = 0;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object != NULL) {
            ++count;
        }
    }

    count = GetRandomValue(0, count - 1);

    Object* target = NULL;
    int num = GetRandomValue(0, count - 1);
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object != NULL) {
            if (count == 0) {
                target = asteroids[i].object;
                break;
            }
            --count;
        }
    }
    if (target == NULL) {
        TraceLog(LOG_ERROR, "Could not find target for saucer");
        return;
    }

    Object* shooter = saucer.object;
    Vector2 p = intercept(shooter->position, bulletInitialVelocity, target->position, target->velocity);
    Vector2 bulletVel = shoot_at(shooter->position, p, bulletInitialVelocity);

    SpawnBullet(shooter->position, bulletVel);
    PlaySound(sounds[SOUND_FIRE]);
}

void UpdateSaucer() {
    static int soundIds[2] = { SOUND_SAUCER_BIG, SOUND_SAUCER_SMALL };
    typedef void (*ShootFunc)();
    static ShootFunc shootFunc[2] = { LargeSaucerShoot, LargeSaucerShoot };

    Object* obj = saucer.object;
    if (obj->active) {
        if (!IsSoundPlaying(sounds[soundIds[saucer.type]])) {
            PlaySound(sounds[soundIds[saucer.type]]);
        }
        saucer.shotElapsed += game.dt;
        if (saucer.shotElapsed > saucer.shotFreq) {
            shootFunc[saucer.type]();
            saucer.shotElapsed = 0;
        }
    }
    else {
        saucer.noSaucerElapsed += game.dt;
        if (saucer.noSaucerElapsed < saucerSpawnFrequency) return;

        float ran = GetRandomValue(0, 100) / 100.0f;
        if (ran < saucerSpawnChance) {
            saucer.noSaucerElapsed = 0;
            if (game.score < 100000 || GetRandomValue(0,10) < 3) {
                SpawnSaucer(SAUCER_LARGE);
            }
            else {
                SpawnSaucer(SAUCER_SMALL);
            }
        }
    }
}


//----------------------------------------------------------------------------------
// General Functions
//----------------------------------------------------------------------------------

// Returns true if ship was destroyed
bool CheckCollisions() {

    Object* ship = &gameobjects[0];
    Object* saucerObj = saucer.object;

    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->object == NULL) continue;
        Object* aObj = asteroid->object;

        // Check Collision of Asteroid w/ ship
        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, aObj->position, 0.3f * asteroid->size * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit by asteroid %d", i);
            BreakAsteroid(asteroid);
            BreakShip(ship);
            game.lives -= 1;
            return true;
        }

        // Check Collision of Asteroid w/ ship bullets
        for (int j = 0; j < MAX_BULLETS; ++j) {
            Object* bObj = bullets[j].object;
            if (!bObj->active) continue;
            if (CheckCollisionPointCircle(bObj->position, aObj->position, 0.3f * asteroid->size * gameScale)) {
                TraceLog(LOG_INFO, "Asteroid hit by bullet");
                BreakAsteroid(asteroid);
                bullets[j].lifetime = -1;
                bObj->active = false;
            }
        }
    }

    if (saucerObj->active) {
        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, saucerObj->position, 0.7f * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit saucer");
            BreakShip(ship);
            game.lives -= 1;
            return true;
        }
    }

    return false;
}

void ResetLevel() {
    ResetShip();
    gameobjects[0].active = true;
    ResetBullets();
    ResetFragments();
    ResetAsteroids();
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
    sound.elapsed += game.dt;
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
    InitGameObjectStack(&stack, gameobjects);
    InitParticleSystem(&particleSystem);

    game = (Game){ .score = 0, .lives = 3, .hyperspace = 2, .state = LEVEL_START, .stateTime = 0 };
    sound = (BackgroundSound){ .interval = 1, .elapsed = 0, .beat = SOUND_BEAT_1 };

    // Ship
    Object* ship = StackPop(&stack);
    ship->vertexCount = 4;
    ship->active = true;
    ship->initialVertices = shipData;
    ship->vertices = (Vector2*)RL_CALLOC(ship->vertexCount, sizeof(Vector2));
    ResetShip();

    //Saucer
    saucer = (Saucer){ .object = 0, .maxBullets = 2, .shotElapsed = 0, .shotFreq = 1.5f, .type = 0, .noSaucerElapsed = 0 };
    saucer.object = StackPop(&stack);
    saucer.object->active = false;
    saucer.object->vertexCount = saucerVertexCount;
    saucer.object->initialVertices = saucerDataLarge;
    saucer.object->vertices = (Vector2*)RL_CALLOC(saucerVertexCount, sizeof(Vector2));


    for (int i = 0; i < MAX_BULLETS; ++i) {
        Object* obj = StackPop(&stack);
        obj->active = false;
        obj->vertexCount = bullet_vertex_count;
        obj->initialVertices = bullet_data;
        obj->vertices = (Vector2*)RL_CALLOC(obj->vertexCount, sizeof(Vector2));

        Bullet* bullet = &bullets[i];
        bullet->lifetime = 0;
        bullet->object = obj;
    }

    // Ship fragments
    for (int i = 0; i < 4; ++i) {
        Object* obj = StackPop(&stack);
        shipDeadObjects[i] = obj;
        obj->active = false;
        obj->vertexCount = 2;
        obj->initialVertices = shipDeadData;
        obj->vertices = (Vector2*)RL_CALLOC(ship->vertexCount, sizeof(Vector2));
    }

    // Asteroids are fully dynamic
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        asteroids[i] = (Asteroid){ .object = NULL, .size = -1 };
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
    game.dt = GetFrameTime();
    game.stateTime += game.dt;

    if (game.score > nextShip) {
        game.lives += 1;
        nextShip += nextShipInterval;
    }

    if (game.score > nextHyperspace) {
        game.hyperspace += 1;
        nextHyperspace += nextHyperSpaceInverval;
    }

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
        UpdateSaucer();
        UpdateParticles(&particleSystem, game.dt);
        UpdateGameObjects();
        break;
    }
    case RUNNING:
    {
        UpdateBackgroundSound();
        UpdateShip(&gameobjects[0]);
        UpdateParticles(&particleSystem, game.dt);
        UpdateSaucer();
        UpdateGameObjects();
        if (CheckCollisions()) {
            SetState(DYING);
        }
        break;
    }
    case DYING:
    {
        UpdateGameObjects();
        UpdateParticles(&particleSystem, game.dt);
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

    DrawTextEx(smallFont, TextFormat("%i", game.score), (Vector2) { 20, 20 }, (float)smallFont.baseSize, 1.0f,RAYWHITE);

    Vector2 pos = { 20, smallFont.baseSize + 1.2f * gameScale };

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

    pos = (Vector2){ 15, pos.y + 1.2f * gameScale };

    for (int i = 0; i < game.hyperspace; ++i) {
        DrawRectangleLines(pos.x, pos.y, 10, 20, RAYWHITE);
        pos.x += .8f * gameScale;
    }

    pos.x += 10;
    for (int i = 0; i < MAX_GAME_OBJECTS; ++i) {
        Object* obj = &gameobjects[i];
        if (!obj->active) continue;
        DrawLineStrip(obj->vertices, obj->vertexCount, RAYWHITE);
    }

    DrawParticles(&particleSystem);

    switch (game.state) {
    case LEVEL_START:
    {
        DrawTextLineCentered(largeFont, "PLAYER 1", GetScreenHeight() / 2.0f, 1.0);
        break;
    }
    case HYPERSPACE: 
    {
        DrawTextLineCentered(largeFont, "HYPERSPACE", GetScreenHeight() / 3.0f, 1.0);
        break;
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

    lastGameScore = game.score;
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}