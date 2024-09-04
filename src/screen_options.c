/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Options Screen Functions Definitions (Init, Update, Draw, Unload)
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

static int cursor = 0;

static float blinkTime = 0.5f;
static float currentBlinkTime = 0.5f;
static bool showCursor = true;

static int localKeys[CONTROL_MAX] = { 'a', 'd', 'w', 's', ' ' };

static char* errorMessage = NULL;

static char* invalidKeyError = "Invalid key, please use another one";
static char* doubleKeysError = "There are duplicate keys, please resolve";

void DrawTextLine(const char* text, Vector2* pos, float linespacing)
{
    DrawTextEx(smallFont, text, *pos, (float)smallFont.baseSize, 1.0, WHITE);
    pos->y += (float)smallFont.baseSize * linespacing;
}

//----------------------------------------------------------------------------------
// Options Screen Functions Definition
//----------------------------------------------------------------------------------

// Options Screen Initialization logic
void InitOptionsScreen(void)
{
    // TODO: Initialize OPTIONS screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    currentBlinkTime = blinkTime;
    for (int i = 0; i < CONTROL_MAX; ++i) {
        localKeys[i] = controlKeys[i];
    }
}

static char* GetKeyName(int key) {
    static char* charString = " ";

    if (key > 32 && key <= 126) {
        charString[0] = key;
        return charString;
    }
    switch (key) {
    case KEY_SPACE:
        return "space";
    case KEY_LEFT:
        return "left";
    case KEY_RIGHT:
        return "right";
    case KEY_UP:
        return "up";
    case KEY_DOWN:
        return "down";
    default:
        return NULL;
    }
}

static char* HasDuplicates(int* controlList) {
    for (int i = 0; i < CONTROL_MAX; ++i) {
        for (int j = 0; j < CONTROL_MAX; ++j) {
            if (i != j && controlList[i] == controlList[j]) {
                return doubleKeysError;
            }
        }
    }
    return NULL;
}

// Options Screen Update logic
void UpdateOptionsScreen(void)
{
    currentBlinkTime -= GetFrameTime();
    if (currentBlinkTime < 0) {
        currentBlinkTime = blinkTime;
        showCursor = !showCursor;
    }

    int key = GetKeyPressed();
    if (key == 0) return;

    if (key == KEY_ENTER && errorMessage == NULL) {
        for (int i = 0; i < CONTROL_MAX; ++i) {
            controlKeys[i] = localKeys[i];
        }
        finishScreen = 1;
        return;
    }

    if (key == KEY_BACKSPACE) {
        finishScreen = 1;
        return;
    }
    
    if (GetKeyName(key) == NULL) {
        errorMessage = invalidKeyError;
        return;
    };
    

    if (key != ' ' && key >= 'a' && key <= 'z') {
        key = key - ('a' - 'A');
    }

    localKeys[cursor] = key;
    errorMessage = HasDuplicates(localKeys);
    cursor = (cursor + 1) % CONTROL_MAX;
}

// Options Screen Draw logic
void DrawOptionsScreen(void)
{
    Vector2 pos = { 20, 20 };
    float linespacing = 1.1f;

    DrawTextLine("Change Key Assignments:", &pos, linespacing);
    DrawTextLine("Left", &pos, linespacing);
    DrawTextLine("Right", &pos, linespacing);
    DrawTextLine("Thrust", &pos, linespacing);
    DrawTextLine("Fire", &pos, linespacing);
    DrawTextLine("Hyperspace", &pos, linespacing);
    if (errorMessage != NULL) {
        DrawTextLine(errorMessage, &pos, linespacing);
    }
    else {
        pos.y += smallFont.baseSize * linespacing;
    }
    DrawTextLine("<Enter> to confirm <Backspace> to cancel", &pos, linespacing);

    pos = (Vector2){ 200, 20 + smallFont.baseSize * linespacing };
   
    for (int i = 0; i < CONTROL_MAX; ++i) {
        if (i == cursor && showCursor) {
            DrawTextEx(smallFont, "_", pos, (float)smallFont.baseSize, 1.0, WHITE);
        }
        DrawTextLine(GetKeyName(localKeys[i]), &pos, linespacing);
    }
}

// Options Screen Unload logic
void UnloadOptionsScreen(void)
{
    // TODO: Unload OPTIONS screen variables here!
}

// Options Screen should finish?
int FinishOptionsScreen(void)
{
    return finishScreen;
}