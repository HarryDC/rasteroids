#include <string.h>
#include <stdio.h>

#include "raylib.h"
#include "screens.h"

// Local Function to fill scores with default values
static void SetDefaultScores(Highscore scores[], int maxScores)
{
    int high = 15000;
    for (int i = 0; i < maxScores; ++i) {
        
        TextCopy(scores[i].name, "HAS");
        TextCopy(scores[i].score, TextFormat("%i", high));
        high -= 2000;
    }
}

// Loads values for key board input from file into given map array
void LoadControlMap(const char* fileName, int map[], int maxEntries) {
    int size = 0;
    unsigned char *mapData = LoadFileData(fileName, &size);
    if (mapData == 0) {
        TraceLog(LOG_WARNING, "Could not load control map file %s/%s", GetWorkingDirectory(), fileName);
        return;
    }
    // Copy into local memory
    if (size == sizeof(int) * maxEntries) {
        int *dataCast = (int*)mapData;
        for (int i = 0; i < maxEntries; ++i) {
            map[i] = dataCast[i];
        }
    }
    else {
        TraceLog(LOG_WARNING, "Mapfile has different size %i, expected was %i", size, sizeof(int) * maxEntries);
    }

    UnloadFileData(mapData);
}

// Writes values for keyboard input into file
// SaveFileData already reports errors if this fails
void WriteControlMap(const char* fileName, int map[], int maxEntries) {
    bool success = SaveFileData(fileName, map, sizeof(int) * maxEntries);
}

// Load highscores from file, expects comma separated list of name,score pairs 
void LoadHigscores(const char* fileName, Highscore scores[], int maxScores) {
    char* highscoreText = LoadFileText(fileName);
    if (highscoreText == 0) {
        TraceLog(LOG_WARNING, "Could not load highscores file %s/%s", GetWorkingDirectory(), fileName);
        SetDefaultScores(scores, maxScores);
        return;
    }

    int count = 0;
    char** splits = TextSplit(highscoreText, ',', &count);

    if (count < 2 || count % 2 != 0) {
        TraceLog(LOG_WARNING, "Invalid forma for highscore file");
        SetDefaultScores(scores, maxScores);
        count = 0;
    }

    int i = 0;
    while (i < count && i / 2 < maxScores) {
        TextCopy(scores[i / 2].name, splits[i]);
        TextCopy(scores[i / 2].score, splits[i + 1]);
        i += 2;
    }
    
    UnloadFileText(highscoreText);
}

// Finds a new scores position in the highscore table, returns -1 if its
// not a new highscore in the give table
int GetHighscorePosition(Highscore scores[], int maxScores, int score) {
    int found = -1;
    for (int i = maxScores - 1; i >= 0; --i) {
        if (score < TextToInteger(scores[i].score)) {
            break;
        }
        found = i;
    }
    return found;
}

// Inserts a new higscore into the highscore table 
void InsertHighscore(Highscore scores[], int maxScores, int at, char* name, int score)
{
    if (at < 0 || at >= maxScores) {
        TraceLog(LOG_WARNING, "Invalid High Score Position");
        return;
    }
    if (TextLength(name) > 3) {
        TraceLog(LOG_WARNING, "Name is too long for highscore table '%s'", name);
        return;
    }

    // Move all entries below 'at' down by one spot, overwriting the
    // last entry in the table, by going in reverse no temp variables
    // are needed
    for (int i = maxScores - 1; i >= at + 1; --i) {
        TextCopy(scores[i].name, scores[i - 1].name);
        TextCopy(scores[i].score, scores[i - 1].score);
    }

    TextCopy(scores[at].name, name);
    TextCopy(scores[at].score, TextFormat("%i", score));
}

// Writes Highscores as simple comma separated list
void WriteHigscores(const char* fileName, Highscore scores[], int maxScores)
{
    char buffer[1024] = { 0 };
    int position = 0;
    for (int i = 0; i < maxScores; ++i) {
        TextAppend(buffer, scores[i].name, &position);
        TextAppend(buffer, ",", &position);
        TextAppend(buffer, scores[i].score, &position);
        if (i < maxScores - 1) {
            TextAppend(buffer, ",", &position);
        }
    }
    SaveFileText(fileName, buffer);
}

// Utility function to draw a text line that's centered horizontally on the screen
void DrawTextLineCentered(Font font, const char* text, float y, float spacing)
{
    Vector2 pos = MeasureTextEx(font, text, (float)font.baseSize * 1.0f, 1.0f);
    pos.y = y;
    pos.x = (GetScreenWidth() - pos.x) / 2.0f;
    DrawTextEx(font,text, pos, (float)font.baseSize, 1.0f, WHITE);
}

// Draw the Highscore table
void DrawHighscores(Font font, float top, float lineSpace, float gap, Highscore* scores, int maxScores)
{
    Vector2 sizeName = MeasureTextEx(font, "AAA", (float)font.baseSize, 1.0);
    
    DrawTextLineCentered(font, "HIGHSCORES", top, 1.0f);
    top = top + font.baseSize * 1.1f;

    float textXpos = ((float)GetScreenWidth() - gap) / 2.0f - sizeName.x;
    float numberXpos = ((float)GetScreenWidth() + gap) / 2.0f;
    float y = top;
    for (int i = 0; i < maxScores; ++i) {
        DrawTextEx(font, scores[i].name, (Vector2) { textXpos, y }, (float)font.baseSize, 1.0, WHITE);
        DrawTextEx(font, scores[i].score, (Vector2) { numberXpos, y }, (float)font.baseSize, 1.0, WHITE);
        y += lineSpace;
    }
}