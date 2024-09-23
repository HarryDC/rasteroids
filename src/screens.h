/*******************************************************************************************
*
*   rAsteroids
*   Asteroids like game using raylib and raylib game template
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Raylib and raylib game template Copyright (c) 2021 Ramon Santamaria (@raysan5)
*
********************************************************************************************/


#ifndef SCREENS_H
#define SCREENS_H

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum GameScreen { 
    SCREEN_UNKOWN = -1, 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_OPTIONS, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING, 
    SCREEN_MAX } GameScreen;

typedef enum Sounds {
    SOUND_BANG_LARGE, SOUND_BANG_MEDIUM, SOUND_BANG_SMALL, 
    SOUND_BEAT_1, SOUND_BEAT_2,
    SOUND_EXTRA_SHIP, SOUND_FIRE, 
    SOUND_SAUCER_LARGE, SOUND_SAUCER_SMALL,
    SOUND_THRUST, SOUND_MAX
} Sounds;

typedef enum Controls{
    CONTROL_LEFT,
    CONTROL_RIGHT,
    CONTROL_THRUST,
    CONTROL_FIRE,
    CONTROL_HYPERSPACE,
    CONTROL_MAX,
} Keys;



//----------------------------------------------------------------------------------
// Global Variables Declaration (shared by several modules)
//----------------------------------------------------------------------------------
extern GameScreen currentScreen;
extern Font smallFont;
extern Font largeFont;
extern Sound sounds[SOUND_MAX];
extern int controlKeys[CONTROL_MAX];
extern int lastGameScore;



#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Logo Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitLogoScreen(void);
void UpdateLogoScreen(void);
void DrawLogoScreen(void);
void UnloadLogoScreen(void);
int FinishLogoScreen(void);

//----------------------------------------------------------------------------------
// Title Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitTitleScreen(void);
void UpdateTitleScreen(void);
void DrawTitleScreen(void);
void UnloadTitleScreen(void);
int FinishTitleScreen(void);

//----------------------------------------------------------------------------------
// Options Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitOptionsScreen(void);
void UpdateOptionsScreen(void);
void DrawOptionsScreen(void);
void UnloadOptionsScreen(void);
int FinishOptionsScreen(void);

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Declaration
//----------------------------------------------------------------------------------
void InitGameplayScreen(void);
void UpdateGameplayScreen(void);
void DrawGameplayScreen(void);
void UnloadGameplayScreen(void);
int FinishGameplayScreen(void);

void InitEndingScreen(void);
void UpdateEndingScreen(void);
void DrawEndingScreen(void);
void UnloadEndingScreen(void);
int FinishEndingScreen(void);

//----------------------------------------------------------------------------------
// Highscore Handling
//----------------------------------------------------------------------------------
typedef struct Highscore
{
    char name[4];// 3 chars + 0
    char score[32];
} Highscore;

#define MAX_HIGHSCORES 5
extern Highscore scores[MAX_HIGHSCORES];

void LoadHigscores(const char* fileName, Highscore scores[], int maxScores);
void WriteHigscores(const char* fileName, Highscore scores[], int maxScores);

int GetHighscorePosition(Highscore scores[], int maxScores, int score);
void InsertHighscore(Highscore scores[], int maxScores, int at, char* name, int score);

void DrawTextLineCentered(Font font, const char* text, float y, float spacing);
void DrawHighscores(Font font, float top, float lineSpace, float gap, Highscore* scores, int maxScores);

void LoadControlMap(const char* fileName, int map[], int maxEntries);
void WriteControlMap(const char* fileName, int map[], int maxEntries);

#ifdef __cplusplus
}
#endif

#endif // SCREENS_H