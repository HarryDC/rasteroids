/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Ending Screen Functions Definitions (Init, Update, Draw, Unload)
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

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

#define CHAR_COUNT 27
static char availableCharacters[CHAR_COUNT] = "   ";
static int cursorPos = 0;
static char *inputChars = "   ";
static int currentChar = 0;
static char* text[2] = { "You qualified for a high score, enter it",
"using the left and right keys and shot to confirm." };

static float editBlinkInterval = 0.5f;
static float editBlinkCurrent = 0.0f;
static bool currentBlinkState = true;

//----------------------------------------------------------------------------------
// Ending Screen Functions Definition
//----------------------------------------------------------------------------------

// Ending Screen Initialization logic
void InitEndingScreen(void)
{
    // TODO: Initialize ENDING screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    editBlinkCurrent = editBlinkInterval;

    availableCharacters[0] = ' ';
    char currentChar = 'A';
    for (int i = 1; i < CHAR_COUNT; ++i, ++currentChar) {
        availableCharacters[i] = currentChar;
    }
}

// Ending Screen Update logic
void UpdateEndingScreen(void)
{
    if (IsKeyPressed(KEY_SPACE)) {
        ++cursorPos;
    }
    if (cursorPos >= 3) {
        finishScreen = 1;
        int pos = GetHighscorePosition(scores, MAX_HIGHSCORES, lastGameScore);
        InsertHighscore(scores, MAX_HIGHSCORES, pos, inputChars, lastGameScore);
        WriteHigscores("hight.txt", scores, MAX_HIGHSCORES);
        return;
    }
    if (IsKeyPressed(KEY_D)) {
        currentChar++;
    }
    if (IsKeyPressed(KEY_A)) {
        currentChar--;
    }

    currentChar = (currentChar + CHAR_COUNT) % CHAR_COUNT;
    inputChars[cursorPos] = availableCharacters[currentChar];
}

// Ending Screen Draw logic
void DrawEndingScreen(void)
{
    char* charText = " ";
    editBlinkCurrent -= GetFrameTime();
    if (editBlinkCurrent < 0) {
        editBlinkCurrent = editBlinkInterval;
        currentBlinkState = !currentBlinkState;
    }

    // TODO: Draw ENDING screen here!
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        
    float y = (float)GetScreenHeight() / 3.0f;
    for (int i = 0; i < 2; ++i) {
        DrawTextLineCentered(smallFont, text[i], y, 1.0);
        y += (float)largeFont.baseSize * 1.1f;
    }

    float charWidth = 40;
    float x = (float)GetScreenWidth() / 2.0f - charWidth - charWidth / 2.0f;

    for (int i = 0; i < 3; ++i) {
        if (i == cursorPos && currentBlinkState) {
            DrawTextEx(largeFont, "_", (Vector2) { x, y }, (float)largeFont.baseSize, 1.0, RAYWHITE);
        }
        charText[0] = inputChars[i];
        DrawTextEx(largeFont, charText, (Vector2) { x, y }, (float)largeFont.baseSize, 1.0, RAYWHITE);
        x = x + charWidth;
    }
}

// Ending Screen Unload logic
void UnloadEndingScreen(void)
{
}

// Ending Screen should finish?
int FinishEndingScreen(void)
{
    return finishScreen;
}