/*******************************************************************************************
*
*   raylib game template
*
*   <Game title>
*   <Game description>
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2021 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "screens.h"    // NOTE: Declares global (extern) variables and screens functions

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.h
//----------------------------------------------------------------------------------
GameScreen currentScreen = SCREEN_LOGO;
Font smallFont = { 0 };
Font largeFont = { 0 };

int lastGameScore = 0;

static char* soundFiles[SOUND_MAX] = {
    "resources/bangLarge.wav",
    "resources/bangMedium.wav",
    "resources/bangSmall.wav",
    "resources/beat1.wav",
    "resources/beat2.wav",
    "resources/extraShip.wav",
    "resources/fire.wav",
    "resources/saucerBig.wav",
    "resources/saucerSmall.wav",
    "resources/thrust.wav"
};

Sound sounds[SOUND_MAX] = {0};
Highscore scores[MAX_HIGHSCORES];
int controlKeys[CONTROL_MAX] = { 'A', 'D', 'W', ' ', 'S' };

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 1024;
static const int screenHeight = 768;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha = 0.0f;
static bool onTransition = false;
static bool transFadeOut = false;
static int transFromScreen = -1;
static GameScreen transToScreen = SCREEN_UNKOWN;

/*
typedef void (*ScreenCall)();
static ScreenCall initCalls[SCREEN_MAX] =
{
    InitLogoScreen, InitTitleScreen, InitOptionsScreen, InitGameplayScreen, InitEndingScreen
};

static ScreenCall updateCalls[SCREEN_MAX] =
{
    UpdateLogoScreen, UpdateTitleScreen, UpdateOptionsScreen, UpdateGameplayScreen, UpdateEndingScreen
};

static ScreenCall drawCalls[SCREEN_MAX] =
{
    DrawLogoScreen, DrawTitleScreen, DrawOptionsScreen, DrawGameplayScreen, DrawEndingScreen
};

static ScreenCall finishCalls[SCREEN_MAX] =
{
    FinishLogoScreen, FinishTitleScreen, FinishOptionsScreen, FinishGameplayScreen, FinishEndingScreen
};

static ScreenCall unloadCalls[SCREEN_MAX] =
{
    UnloadLogoScreen, UnloadTitleScreen, UnloadOptionsScreen, UnloadGameplayScreen, UnloadEndingScreen
};
*/

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void ChangeToScreen(int screen);     // Change to screen, no transition effect

static void TransitionToScreen(int screen); // Request transition to next screen
static void UpdateTransition(void);         // Update transition effect
static void DrawTransition(void);           // Draw transition effect (full-screen rectangle)

static void UpdateDrawFrame(void);          // Update and draw one frame


typedef void (*ScreenCallback)();

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "rAsteroids");

    InitAudioDevice();      // Initialize audio device

    // Load global data (assets that must be available in all screens, i.e. font)
    // font = LoadFont("resources/mecha.png");
    TraceLog(LOG_INFO, "Loading Assets from : %s", GetWorkingDirectory());
    smallFont = LoadFont("resources/Hyperspace.ttf");
    SetTextLineSpacing(30);
    largeFont = LoadFontEx("resources/Hyperspace.ttf", 72, NULL, 0);

    for (int i = 0; i < SOUND_MAX; ++i) {
        sounds[i] = LoadSound(soundFiles[i]);
    }

    lastGameScore = 0;

    // Setup and init first screen
    currentScreen = SCREEN_TITLE;
    InitTitleScreen();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    switch (currentScreen)
    {
        case SCREEN_LOGO: UnloadLogoScreen(); break;
        case SCREEN_TITLE: UnloadTitleScreen(); break;
        case SCREEN_GAMEPLAY: UnloadGameplayScreen(); break;
        case SCREEN_ENDING: UnloadEndingScreen(); break;
        default: break;
    }

    // Unload global data loaded
    UnloadFont(smallFont);
    
    for (int i = 0; i < SOUND_MAX; ++i) {
        UnloadSound(sounds[i]);
    }

    CloseAudioDevice();     // Close audio context

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------
// Change to next screen, no transition
static void ChangeToScreen(GameScreen screen)
{
    // Unload current screen
    switch (currentScreen)
    {
        case SCREEN_LOGO: UnloadLogoScreen(); break;
        case SCREEN_TITLE: UnloadTitleScreen(); break;
        case SCREEN_GAMEPLAY: UnloadGameplayScreen(); break;
        case SCREEN_ENDING: UnloadEndingScreen(); break;
        default: break;
    }

    // Init next screen
    switch (screen)
    {
        case SCREEN_LOGO: InitLogoScreen(); break;
        case SCREEN_TITLE: InitTitleScreen(); break;
        case SCREEN_GAMEPLAY: InitGameplayScreen(); break;
        case SCREEN_ENDING: InitEndingScreen(); break;
        default: break;
    }

    currentScreen = screen;
}

// Request transition to next screen
static void TransitionToScreen(GameScreen screen)
{
    onTransition = true;
    transFadeOut = false;
    transFromScreen = currentScreen;
    transToScreen = screen;
    transAlpha = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void)
{
    if (!transFadeOut)
    {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f instead of 1.05f
        // For that reason we compare against 1.01f, to avoid last frame loading stop
        if (transAlpha > 1.01f)
        {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen)
            {
                case SCREEN_LOGO: UnloadLogoScreen(); break;
                case SCREEN_TITLE: UnloadTitleScreen(); break;
                case SCREEN_OPTIONS: UnloadOptionsScreen(); break;
                case SCREEN_GAMEPLAY: UnloadGameplayScreen(); break;
                case SCREEN_ENDING: UnloadEndingScreen(); break;
                default: break;
            }

            // Load next screen
            switch (transToScreen)
            {
                case SCREEN_LOGO: InitLogoScreen(); break;
                case SCREEN_TITLE: InitTitleScreen(); break;
                case SCREEN_GAMEPLAY: InitGameplayScreen(); break;
                case SCREEN_ENDING: InitEndingScreen(); break;
                default: break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    }
    else  // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f)
        {
            transAlpha = 0.0f;
            transFadeOut = false;
            onTransition = false;
            transFromScreen = -1;
            transToScreen = SCREEN_UNKOWN;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha));
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    if (!onTransition)
    {
        switch(currentScreen)
        {
            case SCREEN_LOGO:
            {
                UpdateLogoScreen();

                if (FinishLogoScreen()) TransitionToScreen(SCREEN_TITLE);

            } break;
            case SCREEN_TITLE:
            {
                UpdateTitleScreen();

                if (FinishTitleScreen() == 1) TransitionToScreen(SCREEN_OPTIONS);
                else if (FinishTitleScreen() == 2) TransitionToScreen(SCREEN_GAMEPLAY);

            } break;
            case SCREEN_OPTIONS:
            {
                UpdateOptionsScreen();

                if (FinishOptionsScreen()) TransitionToScreen(SCREEN_TITLE);

            } break;
            case SCREEN_GAMEPLAY:
            {
                UpdateGameplayScreen();

                if (FinishGameplayScreen() == 1) TransitionToScreen(SCREEN_ENDING);
                else if (FinishGameplayScreen() == 2) TransitionToScreen(SCREEN_TITLE);

            } break;
            case SCREEN_ENDING:
            {
                UpdateEndingScreen();

                if (FinishEndingScreen() == 1) TransitionToScreen(SCREEN_TITLE);

            } break;
            default: break;
        }
    }
    else UpdateTransition();    // Update transition (fade-in, fade-out)
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(BLACK);

        switch(currentScreen)
        {
            case SCREEN_LOGO: DrawLogoScreen(); break;
            case SCREEN_TITLE: DrawTitleScreen(); break;
            case SCREEN_OPTIONS: DrawOptionsScreen(); break;
            case SCREEN_GAMEPLAY: DrawGameplayScreen(); break;
            case SCREEN_ENDING: DrawEndingScreen(); break;
            default: break;
        }

        // Draw full screen rectangle in front of everything
        if (onTransition) DrawTransition();

        //DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
