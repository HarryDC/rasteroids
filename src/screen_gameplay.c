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
// Data: see https://www.retrogamedeconstructionzone.com/2019/10/asteroids-by-numbers.html
// Resourses: see https://www.classicgaming.cc/classics/asteroids/
//----------------------------------------------------------------------------------
/*
Screen Res: 1024x768

Object	Length (in player ship lengths)
---------------------------------------
Screen              25 x 36 (40x20 px on a 1024/768 Screen)
Large Asteroids	    2.4
Medium Asteroid	    1.2
Small Asteroid	    0.6
Alien Ship (large)  1.5
Alien Ship (small)  0.75

The approximate speeds of objects in Asteroids
Object	                    Speed (in ship lengths per second)
Your ship	                0 - 17
Asteroids	                4 - 6.5
Alien ships (both sizes)	4 - 6.5 (depending on your score)
Bullets	                    17 (ship at rest)
*/

// TODO Small Saucer brain, have it avoid asteroids
// TODO Add pause
// TODO Fullscreen mode (16:10 Aspect Ratio check ...) 
// TODO Add Instructions
// TODO Key and Button Map

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
static Vector2 shipVertices[4] = {
    {-0.25f, 0.5f},
    {0.0f, -0.5f},
    {0.25f, 0.5f},
    {-0.25f, 0.5f},
};
static int shipVertexCount = 4;

static Vector2 shipThrustVertices[2][3] = {
{
    {-0.20f, 0.6f + 0.25f},
    {0.0, 0.6f},
    {0.20f, 0.6f + 0.25f}
},
{
    {-0.20f, 0.9f + 0.25f},
    {0.0, 0.9f},
    {0.20f, 0.9f + 0.25f}
},

};
static int shipThrustVertexCount = 3;

static float shipRotationFactor = 2.0f;
static float shipAccelrationFactor = .2f;
static float shipDecelrationFactor = .995f;
static float shipMaxSpeed = 14.0f;
static float shipSpeedCutoff = 0.05f;
// Ship is supposed to be 40 px on a fixed 1024 screen
// Model is 1 unit long => GameScale = 40
static float gameScale = 40;

static Vector2 shipDebrisVertices[2] = {
    {0, 0.25f},
    {0, -0.25f}
};
static int shipDebrisVertexCount = 2;

static int nextShipInterval = 5000;
static int nextShip = 5000;
static int nextHyperSpaceInverval = 2500;
static int nextHyperspace = 2500;

typedef struct Ship {
    Object* ship;
    Object* thrust[2];
    Object* debris[4];
} ShipParts;

static ShipParts parts;

//----------------------------------------------------------------------------------
// Bullet Definition
//---------------------------------------------------------------------------------- 

static Vector2 bulletVertices[5] = {
    {-0.1f, -0.1f},
    {0.1f, -0.1f},
    {0.1f, 0.1f},
    {-0.1f, 0.1f},
    {-0.1f, -0.1f}
};
static const int bulletVertexCount = 5;


#define SAUCER_MAX_BULLETS 5
#define SHIP_MAX_BULLETS 5
#define MAX_BULLETS SAUCER_MAX_BULLETS + SHIP_MAX_BULLETS
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

// Large Asteroid 2.4 x 2.4
static Vector2 asteroidVerticesLarge[11] = {
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

static Vector2 asteroidVerticesMedium[11] = { 0 };
static Vector2 asteroidVerticesSmall[11] = { 0 };
 
static int asteroidVertexCount = 11;

enum {
    ASTEROID_SIZE_LARGE,
    ASTEROID_SIZE_MEDIUM,
    ASTEROID_SIZE_SMALL,
    ASTEROID_SIZE_NUM
};

static Vector2* asteroidData[ASTEROID_SIZE_NUM] = {
    asteroidVerticesLarge,
    asteroidVerticesMedium,
    asteroidVerticesSmall,
};

static float asteroidVelocity[ASTEROID_SIZE_NUM] = { 4, 5, 6 };
static float asteroidRadius[ASTEROID_SIZE_NUM] = { 1.2f, 0.6f, 0.3f};

static int maxLevelAsteroids = 8;
static int startingAsteroids = 2;
static float levelSpeedIncrease = 0.1;

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
    float toShootTime; // time since last shot
    float toNextActionTime;
} Saucer;

static float saucerSpawnFrequency = 5.0f;
static float saucerSpawnChance = 0.1f;

Saucer saucer;

//----------------------------------------------------------------------------------
// Game global Definition
//---------------------------------------------------------------------------------- 

enum Input {
    INPUT_NONE,
    INPUT_LEFT = 0x1,
    INPUT_RIGHT = 0x2,
    INPUT_THRUST = 0x4,
    INPUT_HYPER = 0x8,
    INPUT_SHOOT = 0x10,
};

static int currentInput = 0;

enum GameState {
    LEVEL_START,
    LEVEL_DONE,
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

static int asteroidSizeToType[3] = { ASTEROID_LARGE, ASTEROID_MEDIUM, ASTEROID_SMALL };


typedef struct Game {
    int score;
    int lives;
    int hyperspace;
    int level;
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
void StackInit(Stack *stack, Object objects[]) {
    stack->size = MAX_GAME_OBJECTS;
    for (int i = stack->size-1; i >= 0; --i) {
        objects[i] = (Object){ 0 };
        StackPush(stack, &objects[i]);
    }
}

// Conservative Reinitialization
void ObjectInit(Object* obj, Vector2* initialVertices, int vertexCount)
{
    if (obj == NULL) {
        TraceLog(LOG_FATAL, "ObjectInit called with obj null");
        return;
    }
    if (initialVertices == NULL) {
        TraceLog(LOG_FATAL, "ObjectInit called with initialVertices null");
        return;
    }

    free(obj->vertices);
    *obj = (Object){ 0 };
    obj->initialVertices = initialVertices;
    if (obj->vertices == NULL || vertexCount > obj->vertexCount) {
        free(obj->vertices);
        obj->vertices = (Vector2*)RL_CALLOC(vertexCount, sizeof(Vector2));
    }
    obj->vertexCount = vertexCount;
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

// Produces a random angle amount from -degrees/2 to degrees/2 returns radians
// Use to perturb an angle by this random amount
inline static float GetRandomAngleRad(int degrees)
{
    return (float)GetRandomValue(-degrees/2, degrees/2) * PI / 180.0f;
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
// Bullet Functions
//----------------------------------------------------------------------------------

// Create a bullet from the given range [low, high)
void SpawnBullet(int low, int high, Vector2 pos, Vector2 vel) {
    for (int i = low; i < high; ++i)
    {
        if (bullets[i].lifetime < 0) {
            bullets[i].lifetime = bulletInitialLifetime;
            Object* obj = bullets[i].object;
            obj->active = true;
            obj->position = pos;
            obj->velocity = vel;
            return;
        }
    }

    TraceLog(LOG_WARNING, "Out of bullets in [%i, %i)", low, high);
}

void UpdateBullets() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i].lifetime = Clamp(bullets[i].lifetime - game.dt, -1.0f, 999.0f);

        if (bullets[i].lifetime <= 0.0) {
            Object* obj = bullets[i].object;
            obj->active = false;
        }
    }
}


//----------------------------------------------------------------------------------
// Ship Functions
//----------------------------------------------------------------------------------

// Resets Ship to original position and orientation
void ResetShip() {
    Object* ship = parts.ship;
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
        parts.debris[i]->active = false;
    }
}

bool CheckCollisionAsteroids(Vector2 pos, float radius) {
    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->object == NULL ) continue;
        if (CheckCollisionCircles(pos, radius * gameScale, asteroid->object->position, asteroidRadius[asteroid->size] * gameScale)) {
            return true;
        }
    }
    return false; 
}

int UpdateInput()
{
    int input = 0;
    // TraceLog(LOG_INFO, "Button: %i", GetGamepadButtonPressed());
    if (IsKeyDown(KEY_A) || IsGamepadButtonDown(0, 9)) {
        input = input | INPUT_LEFT;
    }
    if (IsKeyDown(KEY_D) || IsGamepadButtonDown(0, 11)) {
        input = input | INPUT_RIGHT;
    }
    if (IsKeyDown(KEY_S) || IsGamepadButtonDown(0, 3)) {
        input = input | INPUT_HYPER;
    }
    if (IsKeyDown(KEY_W) || IsGamepadButtonDown(0, 7)) {
        input = input | INPUT_THRUST;
    }
    if (IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, 8)) {
        input = input | INPUT_SHOOT;
    }
    return input;
}


// React to user input, calculate new orientation, spawn bullets
void UpdateShip(Object* ship) {
    int input = UpdateInput();
    if ((input & INPUT_LEFT) != 0)
    {
        ship->rot -= shipRotationFactor;
    }
    if ((input & INPUT_RIGHT) != 0) {
        ship->rot += shipRotationFactor;
    }

    if ( ((input & INPUT_HYPER) != 0) && game.hyperspace > 0) {
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
    if ((input & INPUT_THRUST) != 0) {
        ship->velocity = Vector2Add(ship->velocity, accell);

        for (int i = 0; i < 2; ++i) {
            Object* obj = parts.thrust[i];
            obj->active = true;
            obj->position = ship->position;
            obj->velocity = ship->velocity;
            obj->rot = ship->rot;
            obj->rotVel = ship->rotVel;
        }
    }
    else {
        for (int i = 0; i < 2; ++i) {
            parts.thrust[i]->active = false;
        }
    }

    float mag = Vector2Length(ship->velocity);
    if (mag < shipSpeedCutoff) {
        ship->velocity = Vector2Zero();
    }
    else {
        ship->velocity = Vector2ClampValue(ship->velocity, -shipMaxSpeed, shipMaxSpeed);
    }

    if ((input & INPUT_SHOOT) != 0) {
        SpawnBullet(0, SHIP_MAX_BULLETS, ship->position, Vector2Scale(fwd, bulletInitialVelocity));
    }

}

void BreakShip(Object* ship, Object* debris[], int debrisCount) {
    ship->active = false;
    for (int i = 0; i < debrisCount; ++i) {
        Object* obj = debris[i];
        obj->active = true;
        obj->position = ship->vertices[i];
        obj->velocity = Vector2Scale(Vector2Subtract(obj->position, ship->position), 0.01f);
        obj->rot = (float)GetRandomValue(0, 360);
        obj->rotVel = (float)GetRandomValue(0, 200) / 100.0f;
    }
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
    asteroids[asteroidId].size = ASTEROID_SIZE_LARGE; // Sizes 1,2,4

    obj->active = true;
    if (obj->vertexCount < asteroidVertexCount || obj->vertices == 0) {
        free(obj->vertices);
        obj->vertices = (Vector2*)RL_CALLOC(asteroidVertexCount, sizeof(Vector2));
    }
    obj->vertexCount = asteroidVertexCount;
    obj->initialVertices = asteroidVerticesLarge;
    obj->position = GetRandomEdgePosition();

    float rot = (float)GetRandomValue(0, 359) * PI / 180.0f;
    float vel = asteroidVelocity[ASTEROID_SIZE_LARGE] + levelSpeedIncrease * game.level;
    obj->velocity = Vector2Scale(Vector2Rotate(yUp, rot), vel);
    obj->rotVel = (float)GetRandomValue(-100, 100) / 200.0f;
}

void BreakAsteroid(Asteroid* asteroid) {
    SpawnExplosion(&particleSystem, asteroid->object->position, 5);

    if (asteroid->size == ASTEROID_SIZE_SMALL) {
        asteroid->object->active = false;
        StackPush(&stack, asteroid->object);
        *asteroid = (Asteroid){ .object = NULL, .size = -1 };
        AddScore(ASTEROID_SMALL);
        PlaySound(sounds[SOUND_BANG_SMALL]);
        return;
    }

    int sound = (asteroid->size == ASTEROID_SIZE_LARGE) ? SOUND_BANG_LARGE : SOUND_BANG_MEDIUM;
    PlaySound(sounds[sound]);

    asteroid->size += 1;
    Object* obj = asteroid->object;
    
    // Reuse the Original GameObject and add a new one
    // Vertex Count stays the same
    obj->initialVertices = asteroidData[asteroid->size];
    Vector2 oldVelocity = obj->velocity;
    Vector2 newVelocity = Vector2Rotate(obj->velocity, PI / 2.0f + GetRandomAngleRad(40));
    float newSpeed = asteroidVelocity[asteroid->size] + levelSpeedIncrease * game.level;
    obj->velocity = Vector2Scale(Vector2Normalize(newVelocity), newSpeed);
    obj->rotVel = (float)GetRandomValue(-100, 100) / 200.0f;

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
    asteroids[newAsteroid] = (Asteroid){ .object= newObj, .size = asteroid->size };
    ObjectInit(newObj, asteroidData[asteroid->size], asteroidVertexCount);
    newObj->active = true;

    newObj->position = obj->position;
    newVelocity = Vector2Rotate(oldVelocity, -(PI / 2.0f) + GetRandomAngleRad(40));
    newSpeed = asteroidVelocity[asteroid->size] + levelSpeedIncrease * game.level;
    newObj->velocity = Vector2Scale(Vector2Normalize(newVelocity), newSpeed);
    newObj->rotVel = (float)GetRandomValue(-100, 100) / 200.0f;
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

void DrawAsteroidCollisions() {
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object != NULL) {
            Object* obj = asteroids[i].object;
            DrawCircleLines((int)obj->position.x, (int)obj->position.y, asteroidRadius[asteroids[i].size] * gameScale, DARKGREEN);
        }
    }
}

int CountAsteroids() {
    int count = 0;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object != NULL) {
            ++count;
        }
    }
    return count;
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

void BreakSaucer() {
    saucer.object->active = false;
    AddScore(saucer.type);
    saucer.toNextActionTime = saucerSpawnFrequency;
    SpawnExplosion(&particleSystem, saucer.object->position, 8);
    PlaySound(sounds[SOUND_BANG_MEDIUM]);
}

Object* LargeSaucerSelectTarget() {
    int count = CountAsteroids();

    count = GetRandomValue(0, count - 1);
    Object* target = NULL;
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
        return NULL;
    }
    return target;
}

Object* SmallSaucerSelectTarget() {
    if (GetRandomValue(0, 100) > 10) {
        return parts.ship;
    }
    else {
        return LargeSaucerSelectTarget();
    }
}

void ShootSaucer(Object* shooter, Object* target, float bulletVelocity) {
    Vector2 p = intercept(shooter->position, bulletVelocity, target->position, target->velocity);
    Vector2 bulletVel = shoot_at(shooter->position, p, bulletVelocity);

    SpawnBullet(SHIP_MAX_BULLETS, MAX_BULLETS, shooter->position, bulletVel);
    PlaySound(sounds[SOUND_FIRE]);
}

void LargeSaucerUpdate() {
    if (saucer.toNextActionTime > 3.0 && GetRandomValue(0, 10) > 2) {
        float angle = GetRandomAngleRad(90.0f);
        saucer.object->velocity = Vector2Rotate(saucer.object->velocity, PI / 2.0f + GetRandomAngleRad(40));
    }
}

void SmallSaucerUpdate() {
    if (saucer.toNextActionTime > 3.0 && GetRandomValue(0, 10) > 2) {
        float angle = GetRandomAngleRad(90.0f);
        saucer.object->velocity = Vector2Rotate(saucer.object->velocity, PI / 2.0f + GetRandomAngleRad(40));
    }
}

void UpdateSaucer() {
    static int soundIds[2] = { SOUND_SAUCER_BIG, SOUND_SAUCER_SMALL };
    typedef Object* (*ShootFunc)();
    static ShootFunc targetFunc[2] = { LargeSaucerSelectTarget, SmallSaucerSelectTarget};
    
    typedef void (*UpdateFunc)();
    static UpdateFunc updateFunc[2] = { LargeSaucerUpdate, SmallSaucerUpdate };

    saucer.toNextActionTime -= game.dt;

    Object* obj = saucer.object;
    if (obj->active) {
        if (!IsSoundPlaying(sounds[soundIds[saucer.type]])) {
            PlaySound(sounds[soundIds[saucer.type]]);
        }
        updateFunc[saucer.type]();

        saucer.toShootTime -= game.dt;
        if (saucer.toShootTime < 0) {
            Object* target = targetFunc[saucer.type]();
            ShootSaucer(saucer.object, target, bulletInitialVelocity);
            saucer.toShootTime = saucer.shotFreq;
        }

    }
    else {
        if (saucer.toNextActionTime > 0) return;

        float ran = GetRandomValue(0, 100) / 100.0f;
        if (ran < saucerSpawnChance) {
            saucer.toNextActionTime = 0;
            if (game.score < 100000 || GetRandomValue(0,10) < 3) {
                SpawnSaucer(SAUCER_LARGE);
            }
            else {
                SpawnSaucer(SAUCER_SMALL);
            }
            saucer.toShootTime = saucer.shotFreq;
        }
    }
}


//----------------------------------------------------------------------------------
// General Functions
//----------------------------------------------------------------------------------

// Returns true if ship was destroyed
bool CheckCollisions() {

    Object* ship = parts.ship;
    Object* saucerObj = saucer.object;

    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->object == NULL) continue;
        Object* aObj = asteroid->object;

        // Check Collision of Asteroid w/ ship
        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, aObj->position, asteroidRadius[asteroid->size] * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit by asteroid %d", i);
            AddScore(asteroidSizeToType[asteroid->size]);
            BreakAsteroid(asteroid);
            BreakShip(ship, parts.debris, 3);
            game.lives -= 1;
            return true;
        }

        // Check collision of Asteroid w/ saucer
        if (saucerObj->active && 
            CheckCollisionCircles(saucerObj->position, 0.7f * gameScale, aObj->position, asteroidRadius[asteroid->size] * gameScale)) {
                TraceLog(LOG_INFO, "Asteroid hit saucer");
                BreakSaucer(saucer.object);
                AddScore(asteroidSizeToType[asteroid->size]);
                BreakAsteroid(asteroid);
                continue;
        }

        // Check Collision of Asteroid w/ ship bullets
        for (int j = 0; j < MAX_BULLETS; ++j) {
            Object* bObj = bullets[j].object;
            if (!bObj->active) continue;
            if (CheckCollisionPointCircle(bObj->position, aObj->position, asteroidRadius[asteroid->size] * gameScale)) {
                TraceLog(LOG_INFO, "Asteroid hit by bullet");
                AddScore(asteroidSizeToType[asteroid->size]);
                BreakAsteroid(asteroid);
                bullets[j].lifetime = -1;
                bObj->active = false;
                break;
            }
        }
    }

    if (saucerObj->active) {
        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, saucerObj->position, 0.7f * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit saucer");
            BreakShip(ship, parts.debris, 3);
            game.lives -= 1;
            return true;
        }
        // Check Collision of Ship w/ ship bullets
        for (int j = 0; j < SHIP_MAX_BULLETS; ++j) {
            Object* bObj = bullets[j].object;
            if (!bObj->active) continue;
            if (CheckCollisionPointCircle(bObj->position, saucerObj->position, 0.7f * gameScale)) {
                TraceLog(LOG_INFO, "Saucer hit by bullet");
                BreakSaucer(saucer.object);
                bullets[j].lifetime = -1;
                bObj->active = false;
            }
        }
    }
    return false;
}

void UpdateGameObjects() {
    Object* obj = &gameobjects[0];
    for (int i = 0; i < MAX_GAME_OBJECTS; ++i, ++obj) {
        if (!obj->active) continue;

        obj->position = Vector2Add(obj->position, Vector2Scale(obj->velocity, gameScale * game.dt));
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
    float val = (11.0f - Clamp((float)CountAsteroids(), 1, 10)) * 0.1f;
    sound.interval = 0.25f + 1.25f * val;
    sound.elapsed += game.dt;
    if (sound.elapsed > sound.interval) {
        sound.elapsed = 0;
        PlaySound(sounds[sound.beat]);
        sound.beat = (sound.beat == SOUND_BEAT_1) ? SOUND_BEAT_2 : SOUND_BEAT_1;
    }
}
//----------------------------------------------------------------------------------
// Levels
//----------------------------------------------------------------------------------
bool IsLevelDone() {
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].object != NULL && asteroids[i].object->active == true) {
            return false;
        }
    }
    if (saucer.object->active) {
        return false;
    }
    return true;
}

void CreateLevel() {
    int count = (int)Clamp((float)(startingAsteroids + game.level), (float)startingAsteroids, (float)maxLevelAsteroids);
    for (int i = 0; i < count; ++i) {
        AddAsteroid();
    }
}


void NextLevel() {
    game.level += 1;
}

void ResetLevel() {
    ResetShip();
    gameobjects[0].active = true;
    ResetBullets();
    ResetFragments();
    ResetAsteroids();
    SetState(LEVEL_START);
}

//----------------------------------------------------------------------------------
// Main Gameplay Functions
//----------------------------------------------------------------------------------

void InitGameplayScreen(void)
{
    StackInit(&stack, gameobjects);
    InitParticleSystem(&particleSystem);

    game = (Game){ .score = 0, .lives = 3, .hyperspace = 2, .level = -1, .state = LEVEL_START, .stateTime = 0 };
    sound = (BackgroundSound){ .interval = 2, .elapsed = 0, .beat = SOUND_BEAT_1 };

    // Ship
    parts.ship = StackPop(&stack);
    ObjectInit(parts.ship, shipVertices, shipVertexCount);
    ResetShip();

    // Ship Debris
    for (int i = 0; i < 4; ++i) {
        parts.debris[i] = StackPop(&stack);
        ObjectInit(parts.debris[i], shipDebrisVertices, shipDebrisVertexCount);;
    }

    // Thust Graphics
    for (int i = 0; i < 2; ++i) {
        parts.thrust[i] = StackPop(&stack);
        ObjectInit(parts.thrust[i], shipThrustVertices[i], shipThrustVertexCount);
    }

    //Saucer
    saucer = (Saucer){ .object = 0, .maxBullets = 2, .toShootTime = 0, .shotFreq = 1.5f, .type = 0, .toNextActionTime = saucerSpawnFrequency };
    saucer.object = StackPop(&stack);
    saucer.object->active = false;
    saucer.object->vertexCount = saucerVertexCount;
    saucer.object->initialVertices = saucerDataLarge;
    saucer.object->vertices = (Vector2*)RL_CALLOC(saucerVertexCount, sizeof(Vector2));


    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i] = (Bullet){ 0 };
        bullets[i].object = StackPop(&stack);
        ObjectInit(bullets[i].object, bulletVertices, bulletVertexCount);
    }

    // Asteroids are fully dynamic
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        asteroids[i] = (Asteroid){ .object = NULL, .size = -1 };
    }

    for (int i = 0; i < asteroidVertexCount; ++i) {
        asteroidVerticesMedium[i] = Vector2Scale(asteroidVerticesLarge[i], .5);
        asteroidVerticesSmall[i] = Vector2Scale(asteroidVerticesLarge[i], .25);
    }

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

    switch (game.state) {
    case LEVEL_START:
        if (game.stateTime > 3.0) {
            gameobjects[0].active = true;
            CreateLevel();
            SetState(RUNNING);
        } 
        break;
    case HYPERSPACE:
    {
        UpdateBackgroundSound();
        if (game.stateTime > .75f) {
            SetState(RUNNING);
            gameobjects[0].active = true;
        }
        UpdateSaucer();
        UpdateBullets();
        UpdateParticles(&particleSystem, game.dt);
        UpdateGameObjects();
        break;
    }
    case RUNNING:
    {
        UpdateBackgroundSound();
        UpdateShip(parts.ship);
        UpdateParticles(&particleSystem, game.dt);
        UpdateSaucer();
        UpdateBullets();
        UpdateGameObjects();
        if (CheckCollisions()) {
            SetState(DYING);
        }
        else if (IsLevelDone()) {
            SetState(LEVEL_DONE);
        }
        break;
    }
    case LEVEL_DONE:
        UpdateShip(parts.ship);
        UpdateParticles(&particleSystem, game.dt);
        UpdateBullets();
        if (game.stateTime > 2.0f) {
            NextLevel();
            SetState(RUNNING);
        }
        break;
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
        Vector2 start = Vector2Add(Vector2Scale(shipVertices[0], gameScale), pos);
        for (int v = 1; v < shipVertexCount; ++v)
        {
            Vector2 end = Vector2Add(Vector2Scale(shipVertices[v], gameScale), pos);
            DrawLineV(start, end, RAYWHITE);
            start = end;
        }
        pos.x += .8f * gameScale;
    }

    pos = (Vector2){ 15, pos.y + 1.2f * gameScale };

    for (int i = 0; i < game.hyperspace; ++i) {
        DrawRectangleLines((int)pos.x, (int)pos.y, 10, 20, RAYWHITE);
        pos.x += .8f * gameScale;
    }

    pos.x += 10;
    Object* obj = &gameobjects[0];
    for (int i = 0; i < MAX_GAME_OBJECTS; ++i, ++obj) {
        if (!obj->active) continue;
        DrawLineStrip(obj->vertices, obj->vertexCount, RAYWHITE);
    }

    DrawParticles(&particleSystem);

    switch (game.state) {
    case LEVEL_START:
    {
        DrawTextLineCentered(largeFont, "START", GetScreenHeight() / 3.0f, 1.0);
        break;
    }
    case HYPERSPACE: 
    {
        DrawTextLineCentered(largeFont, "HYPERSPACE", GetScreenHeight() / 3.0f, 1.0);
        break;
    }
    }

#ifdef DEBUGDRAW
    DrawAsteroidCollisions();
#endif
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