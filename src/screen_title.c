/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Title Screen Functions Definitions (Init, Update, Draw, Unload)
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
#include "screens.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

static char* anyKey = "press <return> to start\npress <o> for options";
static Vector2 anyKeyPos = { 0 };

//----------------------------------------------------------------------------------
// Title Screen Functions Definition
//----------------------------------------------------------------------------------

// Title Screen Initialization logic
void InitTitleScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;

    Vector2 anyKeySize = MeasureTextEx(smallFont, anyKey, (float)smallFont.baseSize, 1.0);
    anyKeyPos.x = (GetScreenWidth() - anyKeySize.x) / 2.0f;
    anyKeyPos.y = 600;
}

// Title Screen Update logic
void UpdateTitleScreen(void)
{
    int key = GetKeyPressed();
    if (key == KEY_ENTER) {
        finishScreen = 2;
    }
    else if (key == KEY_O) {
        finishScreen = 1;
    }
}

// Title Screen Draw logic
void DrawTitleScreen(void)
{
    DrawTextEx(smallFont, TextFormat("%i", lastGameScore), (Vector2) { 20, 20 }, (float)smallFont.baseSize, 1.0f, RAYWHITE);
    DrawHighscores(smallFont, GetScreenHeight() / 3.0f,(float)smallFont.baseSize * 1.05f , 200.0f, scores, MAX_HIGHSCORES);
    DrawTextEx(smallFont, anyKey, anyKeyPos, (float)smallFont.baseSize,1.0, RAYWHITE);
}

// Title Screen Unload logic
void UnloadTitleScreen(void) {}

// Title Screen should finish?
int FinishTitleScreen(void)
{
    return finishScreen;
}