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

// Small Saucer brain, have it avoid asteroids
// Add pause
// Fullscreen mode (16:10 Aspect Ratio check ...) 
// Add Instructions

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;


static const Vector2 yUp = { 0, -1 };

//----------------------------------------------------------------------------------
// Objects Definition
//----------------------------------------------------------------------------------

// GameObject, every animated thing on the screen is one of these, every frame
// each active object gets its position and rot updated, and then those are 
// applied to all the vertices. Which in turn can be drawn, objects that are
// not active are neither updated nor drawn and do not participate in collision
typedef struct Object {
    bool active;
    Vector2 position;
    Vector2 velocity;
    float rot;                  // In Degrees
    float rotVel;               // Degrees per sec
    int vertexCount;            // Amount of vertices in initialVertices and vertices
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

// Ship vertex data (a simple triangle) for size and scaling the ship is 1 unit long
static Vector2 shipVertices[4] = {
    {-0.25f, 0.5f},
    {0.0f, -0.5f},
    {0.25f, 0.5f},
    {-0.25f, 0.5f},
};
static int shipVertexCount = 4;

// Graphics for powered engines
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

// Static values that drive the ship behavior
static float shipAccelerationFactor = .2f;      // Used under thrust
static float shipRotationFactor = 2.0f;         // Used when the player turns 
static float shipDecelerationFactor = .995f;    // Slows down the ship
static float shipMaxSpeed = 14.0f;
static float shipSpeedCutoff = 0.05f;           // When ship is slower than this, stop it

// Ship is supposed to be 40 px on a fixed 1024 screen
// Model is 1 unit long => GameScale = 40
// This affects ALL values and can be used to move the game to a different sized screen 
// while maintaining the same relative sizes
static float gameScale = 40;

static Vector2 shipDebrisVertices[2] = {
    {0, 0.25f},
    {0, -0.25f}
};
static int shipDebrisVertexCount = 2;

// If the score is larger than threshold, gain one, then increase threshold
// by distance
static int nextShipInterval = 5000;         // Distance to next free ship
static int nextShip = 5000;                 // Threshold for next free ship
static int nextHyperSpaceInverval = 2500;   // Distance to next hyperspace
static int nextHyperspace = 2500;           // Threshold for hyperspace

// Contains all the objects of a ship
typedef struct Ship {
    Object* ship;
    Object* thrust[2];
    int thrustCount;
    Object* debris[4];
    int debrisCount;
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

static float bulletInitialLifetime = 3.0f;  // Lifetime in seconds
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
static int asteroidVertexCount = 11;

// Static data for different sized vertices, will be initialized at start 
static Vector2 asteroidVerticesMedium[11] = { 0 };
static Vector2 asteroidVerticesSmall[11] = { 0 };
 

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
static float asteroidRadius[ASTEROID_SIZE_NUM] = { 1.2f, 0.6f, 0.3f};   // Used in collision

static int maxAsteroids = 8;                // Each level adds asteroids, this is the max
static int startingAsteroids = 2;           // Starting number of asteroids
static float levelSpeedIncrease = 0.1f;     // Gets multiplied with level and added to speed for each new asteroid

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

enum {
    SAUCER_SIZE_LARGE,
    SAUCER_SIZE_SMALL
};

typedef struct Saucer {
    Object* object;
    int type;
    int maxBullets;
    float shotFreq;
    float toShootTime;      // Counts down for next shot
    float toNextActionTime; // Counts down for next course change
} Saucer;

static float saucerSpawnFrequency = 5.0f;
static float saucerSpawnChance = 0.1f;
static float saucerActionTime = 3.0f;

Saucer saucer;

//----------------------------------------------------------------------------------
// Game Definition
//---------------------------------------------------------------------------------- 

// Used to map any inputs into actions 
enum Action {
    ACTION_NONE,
    ACTION_LEFT = 0x1,
    ACTION_RIGHT = 0x2,
    ACTION_THRUST = 0x4,
    ACTION_HYPER = 0x8,
    ACTION_FIRE = 0x10,
};

static int currentInput = 0;

// Drives the display and game flow
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

// Translates "local" enums to the main types
static int asteroidSizeToObject[3] = { ASTEROID_LARGE, ASTEROID_MEDIUM, ASTEROID_SMALL };
static int saucerSizeToObject[2] = { SAUCER_LARGE, SAUCER_SMALL };

typedef struct Game {
    int score;
    int lives;          // current number of lives
    int hyperspace;     // current number of jumps
    int level;          // current level
    int state;          // current GameState
    float dt;           // dt 
    float stateTime;    // time spent in current state
} Game;

Game game;

//----------------------------------------------------------------------------------
// Sound Definition
//---------------------------------------------------------------------------------- 

typedef struct BackgroundSound {
    float elapsed;
    float interval;
    int beat;
} BackgroundSound;

BackgroundSound sound;

//----------------------------------------------------------------------------------
// Particle system Definition
//---------------------------------------------------------------------------------- 

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

// Assumes stack and objects have the same size, initializes
// the stack with the objects from 
void StackInit(Stack *stack, Object objects[], int numObjects) {
    stack->size = MAX_GAME_OBJECTS;
    if (numObjects > MAX_GAME_OBJECTS) {
        TraceLog(LOG_FATAL, "Trying to initialize the stack with more objects than available.");
    }
    for (int i = stack->size-1; i >= 0; --i) {
        objects[i] = (Object){ 0 };
        StackPush(stack, &objects[i]);
    }
}

// Initializes an object, will allocate new space if there was none, 
// or if the new object needs more space for vertices
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

    *obj = (Object){ 0 };
    obj->initialVertices = initialVertices;
    if (obj->vertices == NULL || vertexCount > obj->vertexCount) {
        free(obj->vertices);
        obj->vertices = (Vector2*)RL_CALLOC(vertexCount, sizeof(Vector2));
    }
    obj->vertexCount = vertexCount;
}

// Used when changing game state
void SetState(int state) {
    game.state = state;
    game.stateTime = 0;
}

// Adds the score for one specific object that was destroyed
void AddScore(int type) {
    static int scores[MAX_TYPES] = { -1, 100, 50, -1, 20, 200, 1000 };
    if (type > 0 && type < MAX_TYPES) {
        game.score += scores[type];
    }
    else {
        TraceLog(LOG_WARNING, "AddScore: called with invalid type");
    }
}

// Generate a random position on the edge of the screen
Vector2 GetRandomEdgePosition() {
    // Start from a border
    int sector = GetRandomValue(0, 3);
    switch (sector) {
    case 0:
        return (Vector2){ 0,(float)GetRandomValue(0, GetScreenHeight()) };
    case 1:
        return (Vector2){ (float)GetScreenWidth() , (float)GetRandomValue(0, GetScreenHeight()) };
    case 2:
        return (Vector2) { (float)GetRandomValue(0, GetScreenWidth()), 0 };
    case 3:
        return (Vector2){ (float)GetRandomValue(0, GetScreenWidth()), (float)GetScreenHeight() };
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

// Adds a particle at a give position and velocity, particles fill up from the 
// back like a stack
bool AddParticle(ParticleSystem *system, Vector2 pos, Vector2 vel, float lifetime) {
    if (system->back < MAX_PARTICLES - 1) {
        ++system->back;
        system->particles[system->back] = (Particle){ .position = pos, .velocity = vel, .lifetime = lifetime };
        return true;
    }
    return false;
}

// Updates particle position and lifetime, when a particle dies
// move the last one in the stack 
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

// Creates a particle explosion at the given position
void SpawnExplosion(ParticleSystem* system, Vector2 pos, int count) {
    for (int i = 0; i < count; ++i) {
        int angle = GetRandomValue(0, 360);
        float speed = (float)GetRandomValue(25, 75) / 100.0f;
        Vector2 vel = Vector2Scale(Vector2Rotate(yUp, angle * PI / 180.0f), speed);
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

// Clear all bullets 
void ResetBullets() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        Bullet* bullet = &bullets[i];
        if (bullet->object == NULL) continue;
        bullet->object->active = false;
        bullet->lifetime = -1;
    }
}

// Clear the fragments from a ship explosion
void ResetFragments() {
    for (int i = 0; i < parts.debrisCount; ++i) {
        parts.debris[i]->active = false;
    }
}

// Used when jumping in hyperspace so you don't materialize 
// inside an asteroid, uses a bit bigger radius than the normal collision
bool CheckCollisionAsteroids(Vector2 pos, float radius) {
    for (int i = MAX_ASTEROIDS - 1; i >= 0; --i) {
        Asteroid* asteroid = &asteroids[i];
        if (asteroid->object == NULL ) continue;
        if (CheckCollisionCircles(pos, radius * gameScale, asteroid->object->position, asteroidRadius[asteroid->size] * gameScale * 1.2f)) {
            return true;
        }
    }
    return false; 
}

// Read the input in translate it to the appropriate action
// This allows to read input from multiple sources but process 
// through one loop
int UpdateInput()
{
    int input = 0;
    //TraceLog(LOG_INFO, "Button: %i", GetKeyPressed());
    if (IsKeyDown(controlKeys[CONTROL_LEFT]) || IsGamepadButtonDown(0, 9)) {
        input = input | ACTION_LEFT;
    }
    if (IsKeyDown(controlKeys[CONTROL_RIGHT]) || IsGamepadButtonDown(0, 11)) {
        input = input | ACTION_RIGHT;
    }
    if (IsKeyDown(controlKeys[CONTROL_HYPERSPACE]) || IsGamepadButtonDown(0, 3)) {
        input = input | ACTION_HYPER;
    }
    if (IsKeyDown(controlKeys[CONTROL_THRUST]) || IsGamepadButtonDown(0, 7)) {
        input = input | ACTION_THRUST;
    }
    if (IsKeyPressed(controlKeys[CONTROL_FIRE]) || IsGamepadButtonPressed(0, 8)) {
        input = input | ACTION_FIRE;
    }
    return input;
}

// React to user input, calculate new orientation, spawn bullets
void UpdateShip(Object* ship) {
    int input = UpdateInput();
    if ((input & ACTION_LEFT) != 0)
    {
        ship->rot -= shipRotationFactor;
    }
    if ((input & ACTION_RIGHT) != 0) {
        ship->rot += shipRotationFactor;
    }

    if ( ((input & ACTION_HYPER) != 0) && game.hyperspace > 0) {
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

    ship->velocity = Vector2Scale(ship->velocity, shipDecelerationFactor);

    Vector2 fwd = Vector2Rotate(yUp, ship->rot * PI / 180.0f);
    Vector2 accell = Vector2Scale(fwd, .1f);
    if ((input & ACTION_THRUST) != 0) {
        ship->velocity = Vector2Add(ship->velocity, accell);

        for (int i = 0; i < parts.thrustCount; ++i) {
            Object* obj = parts.thrust[i];
            obj->active = true;
            obj->position = ship->position;
            obj->velocity = ship->velocity;
            obj->rot = ship->rot;
            obj->rotVel = ship->rotVel;
        }
    }
    else {
        for (int i = 0; i < parts.thrustCount; ++i) {
            parts.thrust[i]->active = false;
        }
    }

    float mag = Vector2Length(ship->velocity);
    if (mag < shipSpeedCutoff) {
        ship->velocity = Vector2Zero();
    }
    else {
        ship->velocity = Vector2ClampValue(ship->velocity, 0, shipMaxSpeed);
    }

    if ((input & ACTION_FIRE) != 0) {
        SpawnBullet(0, SHIP_MAX_BULLETS, ship->position, Vector2Scale(fwd, bulletInitialVelocity));
    }
}

// Breaks the ship into debris parts floating around on the screen
void BreakShip(ShipParts* parts) {
    PlaySound(sounds[SOUND_BANG_MEDIUM]);
    parts->ship->active = false;
    for (int i = 0; i < parts->debrisCount; ++i) {
        Object* obj = parts->debris[i];
        obj->active = true;
        obj->position = parts->ship->vertices[i];
        obj->velocity = Vector2Scale(Vector2Subtract(obj->position, parts->ship->position), (float)GetRandomValue(10, 20) * 0.001f);
        obj->rot = (float)GetRandomValue(0, 360);
        obj->rotVel = (float)GetRandomValue(0, 200) / 100.0f;
    }
    for (int i = 0; i < parts->thrustCount; ++i) {
        parts->thrust[i]->active = false;
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

// Used when an asteroid is hit, breaks it into smaller pieces
// or removes it from the game, also plays sound and scores accordingly
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

// Use for debugging to show asteroid collision regions
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

void ResetSaucer() {
    saucer.object->active = false;
    saucer.toNextActionTime = saucerSpawnFrequency;
}

void SpawnSaucer(int type) {
    Object* obj = saucer.object;
    saucer.type = type;

    // Always Spawn on the RIM
    obj->active = true;
    obj->initialVertices = (type == SAUCER_SIZE_LARGE) ? saucerDataLarge : saucerDataSmall;
    float speed = 4 + (float)game.score / 10000.0f;
    speed = Clamp(speed, 0, 7);
    obj->velocity = (Vector2Rotate(yUp, GetRandomAngleRad(180)));
    obj->position = GetRandomEdgePosition();
    obj->rot = 0;
    obj->rotVel = 0;
}

// Source https://gamedev.net/forums/topic/401165-target-prediction-system--target-leading
// Code to calculate the correct vector to hit a moving target from a moving platform
// works ok-ish
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
    AddScore(saucerSizeToObject[saucer.type]);
    saucer.toNextActionTime = saucerSpawnFrequency;
    SpawnExplosion(&particleSystem, saucer.object->position, 8);
    PlaySound(sounds[SOUND_BANG_MEDIUM]);
}


// Picks a target when the saucer is the big version
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

// Picks a target when the saucer is the small version
Object* SmallSaucerSelectTarget() {
    if (GetRandomValue(0, 100) > 10) {
        return parts.ship;
    }
    else {
        return LargeSaucerSelectTarget();
    }
}

// Shoots at a selected target from the saucer
void ShootSaucer(Object* shooter, Object* target, float bulletVelocity) {
    Vector2 p = intercept(shooter->position, bulletVelocity, target->position, target->velocity);
    Vector2 bulletVel = shoot_at(shooter->position, p, bulletVelocity);

    SpawnBullet(SHIP_MAX_BULLETS, MAX_BULLETS, shooter->position, bulletVel);
    PlaySound(sounds[SOUND_FIRE]);
}

// Moves the large saucer
void LargeSaucerUpdate() {
    if (saucer.toNextActionTime < 0) {
        float angle = GetRandomAngleRad(90);
        saucer.object->velocity = Vector2Rotate(saucer.object->velocity, PI / 2.0f + GetRandomAngleRad(40));
        saucer.toNextActionTime = saucerActionTime + (float)GetRandomValue(0, (int)saucerActionTime * 10)/10.0f;
    }
}

// Moves the small saucer
void SmallSaucerUpdate() {
    if (saucer.toNextActionTime < 0) {
        float angle = GetRandomAngleRad(90);
        saucer.object->velocity = Vector2Rotate(saucer.object->velocity, PI / 2.0f + GetRandomAngleRad(40));
        saucer.toNextActionTime = saucerActionTime + (float)GetRandomValue(0, (int)saucerActionTime * 10)/10.0f;
    }
}

void UpdateSaucer() {
    static int soundIds[2] = { SOUND_SAUCER_LARGE, SOUND_SAUCER_SMALL };

    // Rather than using case statements to switch between the two types use function
    // tables to modify the behavior for each type of saucer
    typedef Object* (*ShootFunc)();
    static ShootFunc targetFunc[2] = { LargeSaucerSelectTarget, SmallSaucerSelectTarget};
    
    typedef void (*UpdateFunc)();
    static UpdateFunc updateFunc[2] = { LargeSaucerUpdate, SmallSaucerUpdate };

    // When this hits zero, a new saucer may spawn or an existing one may change course
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
            saucer.toNextActionTime = saucerActionTime;
            if (game.score < 100000 || GetRandomValue(0,10) < 3) {
                SpawnSaucer(SAUCER_SIZE_LARGE);
            }
            else {
                SpawnSaucer(SAUCER_SIZE_SMALL);
            }
            saucer.toShootTime = saucer.shotFreq;
        }
    }
}


//----------------------------------------------------------------------------------
// General Functions
//----------------------------------------------------------------------------------

// Checks for collisions between a variety of objects
// will return true if the ship was hit
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
            AddScore(asteroidSizeToObject[asteroid->size]);
            BreakAsteroid(asteroid);
            BreakShip(&parts);
            game.lives -= 1;
            return true;
        }

        // Check collision of Asteroid w/ saucer
        if (saucerObj->active && 
            CheckCollisionCircles(saucerObj->position, 0.7f * gameScale, aObj->position, asteroidRadius[asteroid->size] * gameScale)) {
                TraceLog(LOG_INFO, "Asteroid hit saucer");
                BreakSaucer(saucer.object);
                AddScore(asteroidSizeToObject[asteroid->size]);
                BreakAsteroid(asteroid);
                continue;
        }

        // Check Collision of Asteroid w/ bullets
        for (int j = 0; j < MAX_BULLETS; ++j) {
            Object* bObj = bullets[j].object;
            if (!bObj->active) continue;
            if (CheckCollisionPointCircle(bObj->position, aObj->position, asteroidRadius[asteroid->size] * gameScale)) {
                TraceLog(LOG_INFO, "Asteroid hit by bullet");
                AddScore(asteroidSizeToObject[asteroid->size]);
                BreakAsteroid(asteroid);
                bullets[j].lifetime = -1;
                bObj->active = false;
                break;
            }
        }
    }

    if (saucerObj->active) {
        // Check collision of ship w/ saucer
        if (CheckCollisionCircles(ship->position, 0.5f * gameScale, saucerObj->position, 0.7f * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit saucer");
            BreakShip(&parts);
            game.lives -= 1;
            return true;
        }
        // Check Collision of Saucer w/ ship bullets
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

    // Check Collision of Ship with Saucer bullets
    for (int j = SHIP_MAX_BULLETS; j < MAX_BULLETS; ++j) {
        Object* bObj = bullets[j].object;
        if (!bObj->active) continue;
        if (CheckCollisionPointCircle(bObj->position, ship->position, 0.5f * gameScale)) {
            TraceLog(LOG_INFO, "Ship hit by bullet");
            BreakShip(&parts);
            bObj->active = false;
            game.lives -= 1;
            return true;
        }
    }

    return false;
}

// Apply velocity and rotation velocity, and transform the vertices into the correct
// position for drawing
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

// When all asteroids are gone and the saucer is destroyed the level is finished
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

// Reseed a level with asteroids
void CreateLevel() {
    int count = (int)Clamp((float)(startingAsteroids + game.level), (float)startingAsteroids, (float)maxAsteroids);
    for (int i = 0; i < count; ++i) {
        AddAsteroid();
    }
}

void NextLevel() {
    game.level += 1;
}

void ResetLevel() {
    ResetShip();
    ResetSaucer();
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
    StackInit(&stack, gameobjects, MAX_GAME_OBJECTS);
    InitParticleSystem(&particleSystem);

    game = (Game){ .score = 0, .lives = 3, .hyperspace = 2, .level = -1, .state = LEVEL_START, .stateTime = 0 };
    sound = (BackgroundSound){ .interval = 2, .elapsed = 0, .beat = SOUND_BEAT_1 };

    // Ship
    parts.ship = StackPop(&stack);
    ObjectInit(parts.ship, shipVertices, shipVertexCount);
    ResetShip();

    // Ship Debris
    parts.debrisCount = 3;
    for (int i = 0; i < parts.debrisCount; ++i) {
        parts.debris[i] = StackPop(&stack);
        ObjectInit(parts.debris[i], shipDebrisVertices, shipDebrisVertexCount);;
    }

    // Thust Graphics
    parts.thrustCount = 2;
    for (int i = 0; i < parts.thrustCount; ++i) {
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

    for (int i = 0; i < saucerVertexCount; ++i) {
        saucerDataSmall[i] = Vector2Scale(saucerDataLarge[i], 0.6f);
    }

    // Bullets
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
        parts.ship->active = false;
        if (game.stateTime > 3.0) {
            parts.ship->active = true;
            CreateLevel();
            SetState(RUNNING);
        } 
        break;
    case HYPERSPACE:
    {
        UpdateBackgroundSound();
        if (game.stateTime > .75f) {
            SetState(RUNNING);
            parts.ship->active = true;
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

                if (GetHighscorePosition(scores, MAX_HIGHSCORES, lastGameScore) < 0)
                {
                    finishScreen = 2; // Return to title screen
                }
                else {
                    finishScreen = 1; // Go to ending scree
                }
            }
        }
        break;
    }
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
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
    lastGameScore = game.score;
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}