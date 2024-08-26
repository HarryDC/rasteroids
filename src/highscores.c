#include <string.h>
#include <stdio.h>

#include "raylib.h"
#include "screens.h"


bool LoadHigscores(const char* fileName, Highscore scores[], int maxScores) {
    char* highscoreText = LoadFileText(fileName);
    if (highscoreText == 0) {
        TraceLog(LOG_WARNING, "Could not load highscores file %s/%s", GetWorkingDirectory(), "high.txt");
        return false;
    }

    int count = 0;
    char** splits = TextSplit(highscoreText, ',', &count);

    if (count < 2 || count % 2 != 0) {
        TraceLog(LOG_WARNING, "Invalid forma for highscore file");
        count = 0;
    }

    int i = 0;
    while (i < count && i / 2 < maxScores) {
        strcpy_s(scores[i / 2].name, 4, splits[i]);
        strcpy_s(scores[i / 2].score, 32, splits[i + 1]);
        i += 2;
    }
    
    UnloadFileText(highscoreText);

    return count != 0;
}

int IsHighscore(Highscore scores[], int maxScores, int score) {
    int found = -1;
    for (int i = maxScores - 1; i >= 0; --i) {
        if (score < TextToInteger(scores[i].score)) {
            break;
        }
        found = i;
    }
    return found;
}

void AddHighscore(Highscore scores[], int maxScores, int at, char* name, int score)
{
    if (at < 0 || at >= maxScores) {
        TraceLog(LOG_WARNING, "Invalid High Score Positoin");
    }

    if (at >= 0) {
        for (int i = maxScores - 1; i >= at + 1; --i) {
            strcpy_s(scores[i].name, 4, scores[i - 1].name);
            strcpy_s(scores[i].score, 32, scores[i - 1].score);
        }
    }

    strcpy_s(scores[at].name, 4, name);
    sprintf_s(scores[at].score, 32, "%i", score);
}

void WriteHigscores(const char* fileName, Highscore scores[], int maxScores)
{
    const char* comma = ",";

    char buffer[1024] = { 0 };
    for (int i = 0; i < maxScores; ++i) {
        strcat_s(buffer, 1024, scores[i].name);
        strcat_s(buffer, 1024, comma);
        strcat_s(buffer, 1024, scores[i].score);
        if (i < maxScores - 1) {
            strcat_s(buffer, 1024, comma);
        }
    }
    SaveFileText(fileName, buffer);
}

void DrawCenteredLine(Font font, const char* text, float y, float spacing)
{
    Vector2 pos = MeasureTextEx(font, text, (float)font.baseSize * 1.0f, 1.0f);
    pos.y = y;
    pos.x = (GetScreenWidth() - pos.x) / 2.0f;
    DrawTextEx(font,text, pos, (float)font.baseSize, 1.0f, WHITE);

}

void DrawHighscores(Font font, float top, float lineSpace, float gap, Highscore* scores, int maxScores)
{
    Vector2 sizeName = MeasureTextEx(font, "AAA", (float)font.baseSize, 1.0);
    
    DrawCenteredLine(font, "HIGHSCORES", top, 1.0f);
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