/*******************************************************************************************
*
*   raylib - classic game: tetris
*
*   Sample game developed by Marc Palau and Ramon Santamaria
*
*   This game has been created using raylib v1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#ifndef MENU_H
#define MENU_H

void DrawMenu(void);
void UpdateMenu(void);

#endif

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define SQUARE_SIZE             40

#define GRID_HORIZONTAL_SIZE    12
#define GRID_VERTICAL_SIZE      20

#define LATERAL_SPEED           10
#define TURNING_SPEED           12
#define FAST_FALL_AWAIT_COUNTER 30

#define FADING_TIME             33

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

typedef enum GridSquare { EMPTY, MOVING, FULL, BLOCK, FADING } GridSquare;

typedef enum ModifierID {
    MOD_NONE = 0,
    MOD_LINE_BLASTER,
    MOD_FILLUPPER,
    MOD_SLOW_MO,
    MOD_SPEED_SPIKE,
    MOD_CONTROL_FLIP,
    MOD_FOG_OF_WAR
} ModifierID;

typedef struct MeterSystem {
    float buffPoints;
    float dreadPoints;
    ModifierID activeModifierID;
    float modifierTimer;
} MeterSystem;

typedef struct Point {
    int x;
    int y;
} Point;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = 1920;
static const int screenHeight = 1080;

static bool gameOver = false;
static bool pause = false;

// Matrices
static GridSquare grid [GRID_HORIZONTAL_SIZE][GRID_VERTICAL_SIZE];
static GridSquare piece [4][4];
static GridSquare incomingPiece [4][4];

// Theese variables keep track of the active piece position
static int piecePositionX = 0;
static int piecePositionY = 0;

// Game parameters
static Color fadingColor;
//static int fallingSpeed;           // In frames

static bool beginPlay = true;      // This var is only true at the begining of the game, used for the first matrix creations
static bool pieceActive = false;
static bool detection = false;
static bool lineToDelete = false;

// Statistics
static int level = 1;
static int lines = 0;

// Counters
static int gravityMovementCounter = 0;
static int lateralMovementCounter = 0;
static int turnMovementCounter = 0;
static int fastFallMovementCounter = 0;

static int fadeLineCounter = 0;

// Based on level
static int gravitySpeed = 30;
static int baseGravitySpeed = 30;

static MeterSystem gameMeters = { 0.0f, 0.0f, MOD_NONE, 0.0f };
static float dreadUpdateAccumulator = 0.0f;
// Stores the player's held Tetris piece.
static GridSquare heldPiece[4][4];

// Checks whether a piece has already been stored.
static bool hasHeldPiece = false;

// Prevents the player from holding multiple times during one turn.
static bool canHoldPiece = true;
//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

// Additional module functions
static bool Createpiece();
static void GetRandompiece();
static void ResolveFallingMovement(bool *detection, bool *pieceActive);
static bool ResolveLateralMovement();
static bool ResolveTurnMovement();
static void CheckDetection(bool *detection);
static void CheckCompletion(bool *lineToDelete);
static int DeleteCompleteLines(void);

// TETWIST modifier functions
static void UpdateMeters(MeterSystem *meters, int clearedLines);
static void ActivateRandomBuff(void);
static void ActivateRandomDebuff(void);
static void TriggerLineBlaster(void);
static void TriggerPiecePurifier(void);
static void ApplyTimedModifiers(void);
static bool IsStackAboveHalf(void);
static bool HasCoveredHole(void);
static void ApplyLandingModifier(int centerX, int centerY);
static const char *GetModifierName(ModifierID id);
// Allows the player to store or swap the currently falling piece.
static void HoldCurrentPiece(void);
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "classic game: tetris");

    InitGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
#endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame();         // Unload loaded data (textures, sounds, models...)

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------
// Game Module Functions Definition
//--------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Initialize game statistics
    level = 1;
    lines = 0;

    fadingColor = GRAY;

    piecePositionX = 0;
    piecePositionY = 0;

    pause = false;
    hasHeldPiece = false;
    canHoldPiece = true;

for (int x = 0; x < 4; x++)
{
    for (int y = 0; y < 4; y++)
    {
        heldPiece[x][y] = EMPTY;
    }
}
    beginPlay = true;
    pieceActive = false;
    detection = false;
    lineToDelete = false;

    // Counters
    gravityMovementCounter = 0;
    lateralMovementCounter = 0;
    turnMovementCounter = 0;
    fastFallMovementCounter = 0;

    fadeLineCounter = 0;
    gravitySpeed = 30;
    baseGravitySpeed = 30;

    gameMeters.buffPoints = 0.0f;
    gameMeters.dreadPoints = 0.0f;
    gameMeters.activeModifierID = MOD_NONE;
    gameMeters.modifierTimer = 0.0f;
    dreadUpdateAccumulator = 0.0f;

    // Initialize grid matrices
    for (int i = 0; i < GRID_HORIZONTAL_SIZE; i++)
    {
        for (int j = 0; j < GRID_VERTICAL_SIZE; j++)
        {
            if ((j == GRID_VERTICAL_SIZE - 1) || (i == 0) || (i == GRID_HORIZONTAL_SIZE - 1)) grid[i][j] = BLOCK;
            else grid[i][j] = EMPTY;
        }
    }

    // Initialize incoming piece matrices
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j< 4; j++)
        {
            incomingPiece[i][j] = EMPTY;
        }
    }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver)
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (!pause)
{
    if (IsKeyPressed(KEY_F1))
{
    TriggerLineBlaster();
}

    if (IsKeyPressed(KEY_F2))
    {
        TriggerPiecePurifier();
    }

    if (IsKeyPressed(KEY_C))
    {
        HoldCurrentPiece();
    }

            // Update modifier timers and passive Dread meter.
            ApplyTimedModifiers();
            dreadUpdateAccumulator += GetFrameTime();
            if (dreadUpdateAccumulator >= 1.0f)
            {
                UpdateMeters(&gameMeters, 0);
                dreadUpdateAccumulator = 0.0f;
            }

            if (!lineToDelete)
            {
                if (!pieceActive)
                {
                    // If piece settled,Check if we fullfilled a line and if so, erase the line and pull down the lines above
                    CheckCompletion(&lineToDelete);
                    // Get another piece
                    pieceActive = Createpiece();

                    // We leave a little time before starting the fast falling down
                    fastFallMovementCounter = 0;
                }
                else    // Piece falling
                {
                    // Counters update
                    fastFallMovementCounter++;
                    gravityMovementCounter++;
                    lateralMovementCounter++;
                    turnMovementCounter++;

                    // We make sure to move if we've pressed the key this frame
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) lateralMovementCounter = LATERAL_SPEED;
                    if (IsKeyPressed(KEY_UP)) turnMovementCounter = TURNING_SPEED;

                    // Fall down
                    if (IsKeyDown(KEY_DOWN) && (fastFallMovementCounter >= FAST_FALL_AWAIT_COUNTER))
                    {
                        // We make sure the piece is going to fall this frame
                        gravityMovementCounter += gravitySpeed;
                    }

                    if (gravityMovementCounter >= gravitySpeed)
                    {
                        // Basic falling movement
                        CheckDetection(&detection);

                        // Check if the piece has collided with another piece or with the boundings
                        ResolveFallingMovement(&detection, &pieceActive);

                        gravityMovementCounter = 0;
                    }

                    // Move laterally at player's will
                    if (lateralMovementCounter >= LATERAL_SPEED)
                    {
                        // Update the lateral movement and if success, reset the lateral counter
                        if (!ResolveLateralMovement()) lateralMovementCounter = 0;
                    }

                    // Turn the piece at player's will
                    if (turnMovementCounter >= TURNING_SPEED)
                    {
                        // Update the turning movement and reset the turning counter
                        if (ResolveTurnMovement()) turnMovementCounter = 0;
                    }
                }

                // Game over logic
                for (int j = 0; j < 2; j++)
                {
                    for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
                    {
                        if (grid[i][j] == FULL)
                        {
                            gameOver = true;
                        }
                    }
                }
            }
            else
            {
                // Animation when deleting lines
                fadeLineCounter++;

                if (fadeLineCounter%8 < 4) fadingColor = MAROON;
                else fadingColor = GRAY;

                if (fadeLineCounter >= FADING_TIME)
                {
                    int deletedLines = 0;
                    deletedLines = DeleteCompleteLines();
                    fadeLineCounter = 0;
                    lineToDelete = false;

                    lines += deletedLines;
                    if (deletedLines > 0)
                    {
                        UpdateMeters(&gameMeters, deletedLines);
                        level = 1 + (lines / 10);
                        baseGravitySpeed = 30 - ((level - 1) * 2);
                        if (baseGravitySpeed < 5) baseGravitySpeed = 5;
                    }
                }
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        if (!gameOver)
        {
            // Draw gameplay area
            Vector2 offset;

            offset.x = (screenWidth - GRID_HORIZONTAL_SIZE*SQUARE_SIZE)/2;
            offset.y = (screenHeight - GRID_VERTICAL_SIZE*SQUARE_SIZE)/2;     // NOTE: Harcoded position!

            int controller = offset.x;

            for (int j = 0; j < GRID_VERTICAL_SIZE; j++)
            {
                for (int i = 0; i < GRID_HORIZONTAL_SIZE; i++)
                {
                    // Draw each square of the grid
                    if (grid[i][j] == EMPTY)
                    {
                        DrawLine(offset.x, offset.y, offset.x + SQUARE_SIZE, offset.y, LIGHTGRAY );
                        DrawLine(offset.x, offset.y, offset.x, offset.y + SQUARE_SIZE, LIGHTGRAY );
                        DrawLine(offset.x + SQUARE_SIZE, offset.y, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, LIGHTGRAY );
                        DrawLine(offset.x, offset.y + SQUARE_SIZE, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, LIGHTGRAY );
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[i][j] == FULL)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, GRAY);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[i][j] == MOVING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, DARKGRAY);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[i][j] == BLOCK)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, LIGHTGRAY);
                        offset.x += SQUARE_SIZE;
                    }
                    else if (grid[i][j] == FADING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, fadingColor);
                        offset.x += SQUARE_SIZE;
                    }
                }

                offset.x = controller;
                offset.y += SQUARE_SIZE;
            }

            // Draw modifier meters and current modifier status.
            DrawText("BUFF", 1250, 620, 24, DARKGREEN);
            DrawRectangle(1250, 650, 240, 22, LIGHTGRAY);
            DrawRectangle(1250, 650, (int)(240.0f * gameMeters.buffPoints / 100.0f), 22, GREEN);
            DrawText(TextFormat("%d%%", (int)gameMeters.buffPoints), 1500, 648, 22, DARKGREEN);

            DrawText("DREAD", 1250, 690, 24, MAROON);
            DrawRectangle(1250, 720, 240, 22, LIGHTGRAY);
            DrawRectangle(1250, 720, (int)(240.0f * gameMeters.dreadPoints / 100.0f), 22, RED);
            DrawText(TextFormat("%d%%", (int)gameMeters.dreadPoints), 1500, 718, 22, MAROON);

            if (gameMeters.activeModifierID != MOD_NONE)
            {
                DrawText(TextFormat("ACTIVE: %s", GetModifierName(gameMeters.activeModifierID)),
                         1250, 770, 22, DARKBLUE);
                if (gameMeters.modifierTimer > 0.0f)
                    DrawText(TextFormat("TIME: %.1fs", gameMeters.modifierTimer),
                             1250, 800, 20, DARKBLUE);
                else
                    DrawText("ACTIVATES ON LANDING", 1250, 800, 18, DARKBLUE);
            }

            // Draw incoming piece (hardcoded)
            offset.x = 1250;
            offset.y = 200;

            int controler = offset.x;

            for (int j = 0; j < 4; j++)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (incomingPiece[i][j] == EMPTY)
                    {
                        DrawLine(offset.x, offset.y, offset.x + SQUARE_SIZE, offset.y, LIGHTGRAY );
                        DrawLine(offset.x, offset.y, offset.x, offset.y + SQUARE_SIZE, LIGHTGRAY );
                        DrawLine(offset.x + SQUARE_SIZE, offset.y, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, LIGHTGRAY );
                        DrawLine(offset.x, offset.y + SQUARE_SIZE, offset.x + SQUARE_SIZE, offset.y + SQUARE_SIZE, LIGHTGRAY );
                        offset.x += SQUARE_SIZE;
                    }
                    else if (incomingPiece[i][j] == MOVING)
                    {
                        DrawRectangle(offset.x, offset.y, SQUARE_SIZE, SQUARE_SIZE, GRAY);
                        offset.x += SQUARE_SIZE;
                    }
                }

                offset.x = controler;
                offset.y += SQUARE_SIZE;
            }

            // Fog of War hides the upper half of the board and next-piece preview.

// PIXELATED FOG OF WAR
if (gameMeters.activeModifierID == MOD_FOG_OF_WAR)
{
    int boardX = (screenWidth - GRID_HORIZONTAL_SIZE * SQUARE_SIZE) / 2;
    int boardY = (screenHeight - GRID_VERTICAL_SIZE * SQUARE_SIZE) / 2;

    // Fog covers the bottom half of the board.
    for (int y = boardY + 10 * SQUARE_SIZE;
         y < boardY + 20 * SQUARE_SIZE; y += 20)
    {
        for (int x = boardX + SQUARE_SIZE;
             x < boardX + 11 * SQUARE_SIZE; x += 20)
        {
            int noise = (x * 17 + y * 31 + (x * y) % 19) % 100;
            float opacity = 0.25f + (noise / 200.0f);

            DrawRectangle(x, y, 20, 20, Fade(BLACK, opacity));
        }
    }

    // Pixelated fog over the next-piece preview.
    for (int y = 200; y < 360; y += 10)
    {
        for (int x = 1250; x < 1410; x += 10)
        {
            int noise = (x * 13 + y * 29) % 100;
            float opacity = 0.30f + (noise / 150.0f);

            DrawRectangle(x, y, 10, 10, Fade(BLACK, opacity));
        }
    }
}

            DrawText("NEXT PIECE", offset.x, offset.y - 120, 30, DARKGRAY);

            DrawText(TextFormat("LINES: %04i", lines), offset.x, offset.y + 40, 30, DARKGRAY);

            DrawText(TextFormat("LEVEL: %02i", level), offset.x, offset.y + 90, 30, DARKGRAY);

           if (pause)
{
    DrawText("GAME PAUSED",
        screenWidth/2 - MeasureText("GAME PAUSED", 40)/2,
        screenHeight/2 - 40, 40, GRAY);
}

// HOLD PIECE UI - LEFT SIDE

// HOLD PIECE UI - LEFT SIDE
DrawText("HOLD PIECE", 300, 200, 30, DARKGRAY);
DrawRectangleLines(300, 250, 160, 160, LIGHTGRAY);

for (int y = 0; y < 4; y++)
{
    for (int x = 0; x < 4; x++)
    {
        if (heldPiece[x][y] == MOVING)
        {
            DrawRectangle(300 + x * 40, 250 + y * 40, 40, 40, GRAY);
        }
    }
}

if (!hasHeldPiece)
    DrawText("EMPTY", 345, 315, 20, GRAY);

DrawText("PRESS C TO HOLD", 300, 430, 20, DARKGRAY);
}
else
{
    DrawText("PRESS [ENTER] TO PLAY AGAIN",
        GetScreenWidth()/2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20)/2,
        GetScreenHeight()/2 - 50, 20, GRAY);
}

EndDrawing();
}

// Unload game variables
            void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
            void UpdateDrawFrame(void)
{
            UpdateGame();
            DrawGame();
}

//--------------------------------------------------------------------------------------
// Additional module functions
//--------------------------------------------------------------------------------------
static bool Createpiece()
{
    piecePositionX = (int)((GRID_HORIZONTAL_SIZE - 4)/2);
    piecePositionY = 0;

    // If the game is starting and you are going to create the first piece, we create an extra one
    if (beginPlay)
    {
        GetRandompiece();
        beginPlay = false;
    }

    // We assign the incoming piece to the actual piece
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j< 4; j++)
        {
            piece[i][j] = incomingPiece[i][j];
        }
    }

    // We assign a random piece to the incoming one
    GetRandompiece();

    // Assign the piece to the grid
    for (int i = piecePositionX; i < piecePositionX + 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (piece[i - (int)piecePositionX][j] == MOVING) grid[i][j] = MOVING;
        }
    }
canHoldPiece = true;
    return true;
}

static void GetRandompiece()
{
    int random = GetRandomValue(0, 6);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            incomingPiece[i][j] = EMPTY;
        }
    }

    switch (random)
    {
        case 0: { incomingPiece[1][1] = MOVING; incomingPiece[2][1] = MOVING; incomingPiece[1][2] = MOVING; incomingPiece[2][2] = MOVING; } break;    //Cube
        case 1: { incomingPiece[1][0] = MOVING; incomingPiece[1][1] = MOVING; incomingPiece[1][2] = MOVING; incomingPiece[2][2] = MOVING; } break;    //L
        case 2: { incomingPiece[1][2] = MOVING; incomingPiece[2][0] = MOVING; incomingPiece[2][1] = MOVING; incomingPiece[2][2] = MOVING; } break;    //L inversa
        case 3: { incomingPiece[0][1] = MOVING; incomingPiece[1][1] = MOVING; incomingPiece[2][1] = MOVING; incomingPiece[3][1] = MOVING; } break;    //Recta
        case 4: { incomingPiece[1][0] = MOVING; incomingPiece[1][1] = MOVING; incomingPiece[1][2] = MOVING; incomingPiece[2][1] = MOVING; } break;    //Creu tallada
        case 5: { incomingPiece[1][1] = MOVING; incomingPiece[2][1] = MOVING; incomingPiece[2][2] = MOVING; incomingPiece[3][2] = MOVING; } break;    //S
        case 6: { incomingPiece[1][2] = MOVING; incomingPiece[2][2] = MOVING; incomingPiece[2][1] = MOVING; incomingPiece[3][1] = MOVING; } break;    //S inversa
    }
}
// Allows the player to hold or swap the current piece.
// Allows the player to hold or swap the current piece.
static void HoldCurrentPiece(void)
{
    if (!pieceActive || !canHoldPiece) return;

    GridSquare currentPiece[4][4];
    GridSquare previousHeldPiece[4][4];

    // Save the current and previously held pieces.
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
        {
            currentPiece[x][y] = piece[x][y];
            previousHeldPiece[x][y] = heldPiece[x][y];
        }
    }

    // Remove the current moving piece from the board.
    for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
    {
        for (int y = 0; y < GRID_VERTICAL_SIZE - 1; y++)
        {
            if (grid[x][y] == MOVING)
                grid[x][y] = EMPTY;
        }
        // Count the cleared row and update the level.
lines++;

level = 1 + (lines / 10);

baseGravitySpeed = 30 - ((level - 1) * 2);

if (baseGravitySpeed < 5)
    baseGravitySpeed = 5;
    }

    if (!hasHeldPiece)
    {
        // Store the current piece.
        for (int x = 0; x < 4; x++)
        {
            for (int y = 0; y < 4; y++)
                heldPiece[x][y] = currentPiece[x][y];
        }

        hasHeldPiece = true;

        // Take the next piece.
        for (int x = 0; x < 4; x++)
        {
            for (int y = 0; y < 4; y++)
                piece[x][y] = incomingPiece[x][y];
        }

        GetRandompiece();
    }
    else
    {
        // Swap the current piece with the held piece.
        for (int x = 0; x < 4; x++)
        {
            for (int y = 0; y < 4; y++)
            {
                heldPiece[x][y] = currentPiece[x][y];
                piece[x][y] = previousHeldPiece[x][y];
            }
        }
    }

    // Reset the replacement piece position.
    piecePositionX = (GRID_HORIZONTAL_SIZE - 4) / 2;
    piecePositionY = 0;

    // Place the replacement piece on the board.
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
        {
            if (piece[x][y] == MOVING)
                grid[piecePositionX + x][piecePositionY + y] = MOVING;
        }
    }

    // Prevent another hold until the next piece spawns.
    canHoldPiece = false;
}
static void ResolveFallingMovement(bool *detection, bool *pieceActive)
{
    // If we finished moving this piece, we stop it
    if (*detection)
    {
        int totalX = 0;
        int totalY = 0;
        int blockCount = 0;

        // Record the exact cells belonging to the falling piece before locking it.
        for (int j = 0; j < GRID_VERTICAL_SIZE - 1; j++)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[i][j] == MOVING)
                {
                    totalX += i;
                    totalY += j;
                    blockCount++;
                    grid[i][j] = FULL;
                    *detection = false;
                    *pieceActive = false;
                }
            }
        }

        // The landing modifier is applied only after the active piece locks.
        if (blockCount > 0)
            ApplyLandingModifier(totalX / blockCount, totalY / blockCount);
    }
    else    // We move down the piece
    {
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[i][j] == MOVING)
                {
                    grid[i][j+1] = MOVING;
                    grid[i][j] = EMPTY;
                }
            }
        }

        piecePositionY++;
    }
}

static bool ResolveLateralMovement()
{
    bool collision = false;
    bool moveLeft = IsKeyDown(KEY_LEFT);
    bool moveRight = IsKeyDown(KEY_RIGHT);

    if (gameMeters.activeModifierID == MOD_CONTROL_FLIP)
    {
        bool temp = moveLeft;
        moveLeft = moveRight;
        moveRight = temp;
    }

    // Piece movement
    if (moveLeft)        // Move left
    {
        // Check if is possible to move to left
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[i][j] == MOVING)
                {
                    // Check if we are touching the left wall or we have a full square at the left
                    if ((i-1 == 0) || (grid[i-1][j] == FULL)) collision = true;
                }
            }
        }

        // If able, move left
        if (!collision)
        {
            for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
            {
                for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)             // We check the matrix from left to right
                {
                    // Move everything to the left
                    if (grid[i][j] == MOVING)
                    {
                        grid[i-1][j] = MOVING;
                        grid[i][j] = EMPTY;
                    }
                }
            }

            piecePositionX--;
        }
    }
    else if (moveRight)  // Move right
    {
        // Check if is possible to move to right
        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[i][j] == MOVING)
                {
                    // Check if we are touching the right wall or we have a full square at the right
                    if ((i+1 == GRID_HORIZONTAL_SIZE - 1) || (grid[i+1][j] == FULL))
                    {
                        collision = true;

                    }
                }
            }
        }

        // If able move right
        if (!collision)
        {
            for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
            {
                for (int i = GRID_HORIZONTAL_SIZE - 1; i >= 1; i--)             // We check the matrix from right to left
                {
                    // Move everything to the right
                    if (grid[i][j] == MOVING)
                    {
                        grid[i+1][j] = MOVING;
                        grid[i][j] = EMPTY;
                    }
                }
            }

            piecePositionX++;
        }
    }

    return collision;
}

static bool ResolveTurnMovement()
{
    // Input for turning the piece
    if (IsKeyDown(KEY_UP))
    {
        GridSquare aux;
        bool checker = false;

        // Check all turning possibilities
        if ((grid[piecePositionX + 3][piecePositionY] == MOVING) &&
            (grid[piecePositionX][piecePositionY] != EMPTY) &&
            (grid[piecePositionX][piecePositionY] != MOVING)) checker = true;

        if ((grid[piecePositionX + 3][piecePositionY + 3] == MOVING) &&
            (grid[piecePositionX + 3][piecePositionY] != EMPTY) &&
            (grid[piecePositionX + 3][piecePositionY] != MOVING)) checker = true;

        if ((grid[piecePositionX][piecePositionY + 3] == MOVING) &&
            (grid[piecePositionX + 3][piecePositionY + 3] != EMPTY) &&
            (grid[piecePositionX + 3][piecePositionY + 3] != MOVING)) checker = true;

        if ((grid[piecePositionX][piecePositionY] == MOVING) &&
            (grid[piecePositionX][piecePositionY + 3] != EMPTY) &&
            (grid[piecePositionX][piecePositionY + 3] != MOVING)) checker = true;

        if ((grid[piecePositionX + 1][piecePositionY] == MOVING) &&
            (grid[piecePositionX][piecePositionY + 2] != EMPTY) &&
            (grid[piecePositionX][piecePositionY + 2] != MOVING)) checker = true;

        if ((grid[piecePositionX + 3][piecePositionY + 1] == MOVING) &&
            (grid[piecePositionX + 1][piecePositionY] != EMPTY) &&
            (grid[piecePositionX + 1][piecePositionY] != MOVING)) checker = true;

        if ((grid[piecePositionX + 2][piecePositionY + 3] == MOVING) &&
            (grid[piecePositionX + 3][piecePositionY + 1] != EMPTY) &&
            (grid[piecePositionX + 3][piecePositionY + 1] != MOVING)) checker = true;

        if ((grid[piecePositionX][piecePositionY + 2] == MOVING) &&
            (grid[piecePositionX + 2][piecePositionY + 3] != EMPTY) &&
            (grid[piecePositionX + 2][piecePositionY + 3] != MOVING)) checker = true;

        if ((grid[piecePositionX + 2][piecePositionY] == MOVING) &&
            (grid[piecePositionX][piecePositionY + 1] != EMPTY) &&
            (grid[piecePositionX][piecePositionY + 1] != MOVING)) checker = true;

        if ((grid[piecePositionX + 3][piecePositionY + 2] == MOVING) &&
            (grid[piecePositionX + 2][piecePositionY] != EMPTY) &&
            (grid[piecePositionX + 2][piecePositionY] != MOVING)) checker = true;

        if ((grid[piecePositionX + 1][piecePositionY + 3] == MOVING) &&
            (grid[piecePositionX + 3][piecePositionY + 2] != EMPTY) &&
            (grid[piecePositionX + 3][piecePositionY + 2] != MOVING)) checker = true;

        if ((grid[piecePositionX][piecePositionY + 1] == MOVING) &&
            (grid[piecePositionX + 1][piecePositionY + 3] != EMPTY) &&
            (grid[piecePositionX + 1][piecePositionY + 3] != MOVING)) checker = true;

        if ((grid[piecePositionX + 1][piecePositionY + 1] == MOVING) &&
            (grid[piecePositionX + 1][piecePositionY + 2] != EMPTY) &&
            (grid[piecePositionX + 1][piecePositionY + 2] != MOVING)) checker = true;

        if ((grid[piecePositionX + 2][piecePositionY + 1] == MOVING) &&
            (grid[piecePositionX + 1][piecePositionY + 1] != EMPTY) &&
            (grid[piecePositionX + 1][piecePositionY + 1] != MOVING)) checker = true;

        if ((grid[piecePositionX + 2][piecePositionY + 2] == MOVING) &&
            (grid[piecePositionX + 2][piecePositionY + 1] != EMPTY) &&
            (grid[piecePositionX + 2][piecePositionY + 1] != MOVING)) checker = true;

        if ((grid[piecePositionX + 1][piecePositionY + 2] == MOVING) &&
            (grid[piecePositionX + 2][piecePositionY + 2] != EMPTY) &&
            (grid[piecePositionX + 2][piecePositionY + 2] != MOVING)) checker = true;

        if (!checker)
        {
            aux = piece[0][0];
            piece[0][0] = piece[3][0];
            piece[3][0] = piece[3][3];
            piece[3][3] = piece[0][3];
            piece[0][3] = aux;

            aux = piece[1][0];
            piece[1][0] = piece[3][1];
            piece[3][1] = piece[2][3];
            piece[2][3] = piece[0][2];
            piece[0][2] = aux;

            aux = piece[2][0];
            piece[2][0] = piece[3][2];
            piece[3][2] = piece[1][3];
            piece[1][3] = piece[0][1];
            piece[0][1] = aux;

            aux = piece[1][1];
            piece[1][1] = piece[2][1];
            piece[2][1] = piece[2][2];
            piece[2][2] = piece[1][2];
            piece[1][2] = aux;
        }

        for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
        {
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                if (grid[i][j] == MOVING)
                {
                    grid[i][j] = EMPTY;
                }
            }
        }

        for (int i = piecePositionX; i < piecePositionX + 4; i++)
        {
            for (int j = piecePositionY; j < piecePositionY + 4; j++)
            {
                if (piece[i - piecePositionX][j - piecePositionY] == MOVING)
                {
                    grid[i][j] = MOVING;
                }
            }
        }

        return true;
    }

    return false;
}

static void CheckDetection(bool *detection)
{
    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
        {
            if ((grid[i][j] == MOVING) && ((grid[i][j+1] == FULL) || (grid[i][j+1] == BLOCK))) *detection = true;
        }
    }
}

static void CheckCompletion(bool *lineToDelete)
{
    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        int calculator = 0;

        for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
        {
            if (grid[i][j] == FULL)
            {
                calculator++;
            }
        }

        // Check if the row is complete
        if (calculator == GRID_HORIZONTAL_SIZE - 2)
        {
            *lineToDelete = true;

            // Mark the completed row
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                grid[i][j] = FADING;
            }
        }
    }
}

static int DeleteCompleteLines()
{
    int deletedLines = 0;

    for (int j = GRID_VERTICAL_SIZE - 2; j >= 0; j--)
    {
        if (grid[1][j] == FADING)
        {
            // Move all rows above down by one
            for (int j2 = j; j2 > 0; j2--)
            {
                for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
                {
                    grid[i][j2] = grid[i][j2 - 1];
                }
            }

            // Clear the top row
            for (int i = 1; i < GRID_HORIZONTAL_SIZE - 1; i++)
            {
                grid[i][0] = EMPTY;
            }

            deletedLines++;
            j++; // Check the same row again after shifting
        }
    }

    return deletedLines;
}


//--------------------------------------------------------------------------------------
// TETWIST BUFF AND DEBUFF SYSTEM
//--------------------------------------------------------------------------------------

// Returns true when the stack reaches the upper half of the playable board.
static bool IsStackAboveHalf(void)
{
    for (int y = 0; y < GRID_VERTICAL_SIZE / 2; y++)
    {
        for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
        {
            if (grid[x][y] == FULL) return true;
        }
    }
    return false;
}

// A covered hole is an EMPTY cell with at least one FULL block above it
// in the same column.
static bool HasCoveredHole(void)
{
    for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
    {
        bool blockAbove = false;

        for (int y = 0; y < GRID_VERTICAL_SIZE - 1; y++)
        {
            if (grid[x][y] == FULL) blockAbove = true;
            else if ((grid[x][y] == EMPTY) && blockAbove) return true;
        }
    }
    return false;
}

static const char *GetModifierName(ModifierID id)
{
    switch (id)
    {
        case MOD_LINE_BLASTER: return "LINE BLASTER";
        case MOD_FILLUPPER: return "FILL UPPER";
        case MOD_SLOW_MO: return "SLOW-MO";
        case MOD_SPEED_SPIKE: return "SPEED SPIKE";
        case MOD_CONTROL_FLIP: return "CONTROL FLIP";
        case MOD_FOG_OF_WAR: return "FOG OF WAR";
        default: return "NONE";
    }
}

// Adds meter points and activates a random modifier when a meter reaches 100%.
static void UpdateMeters(MeterSystem *meters, int clearedLines)
{
    if (meters == NULL) return;

    if (clearedLines > 0)
    {
        meters->buffPoints += 25.0f * (float)clearedLines;
        if (meters->buffPoints > 100.0f) meters->buffPoints = 100.0f;
    }

    if (IsStackAboveHalf() || HasCoveredHole())
    {
        meters->dreadPoints += 2.0f; // Evaluated once per second.
        if (meters->dreadPoints > 100.0f) meters->dreadPoints = 100.0f;
    }

    if (meters->activeModifierID == MOD_NONE)
    {
        if (meters->buffPoints >= 100.0f)
        {
            meters->buffPoints = 0.0f;
            ActivateRandomBuff();
        }
        else if (meters->dreadPoints >= 100.0f)
        {
            meters->dreadPoints = 0.0f;
            ActivateRandomDebuff();
        }
    }
}

static void ActivateRandomBuff(void)
{
    int choice = GetRandomValue(0, 2);

    if (choice == 0)
    {
        gameMeters.activeModifierID = MOD_LINE_BLASTER;
        gameMeters.modifierTimer = 0.0f;
    }
    else if (choice == 1)
    {
        gameMeters.activeModifierID = MOD_FILLUPPER;
        gameMeters.modifierTimer = 0.0f;
    }
    else
    {
        gameMeters.activeModifierID = MOD_SLOW_MO;
        gameMeters.modifierTimer = 12.0f;
    }
}
static void ActivateRandomDebuff(void)
{
    int choice = GetRandomValue(0, 2);

    if (choice == 0)
    {
        gameMeters.activeModifierID = MOD_SPEED_SPIKE;
        gameMeters.modifierTimer = 10.0f;
    }
    else if (choice == 1)
    {
        gameMeters.activeModifierID = MOD_CONTROL_FLIP;
        gameMeters.modifierTimer = 10.0f;
    }
    else
    {
        gameMeters.activeModifierID = MOD_FOG_OF_WAR;
        gameMeters.modifierTimer = 12.0f;
    }
}

// Clears a 3x3 area around the center, without deleting the board walls.
// Randomly clears one row containing blocks and shifts the rows above it downward.
static void TriggerLineBlaster(void)
{
    int availableRows[GRID_VERTICAL_SIZE];
    int rowCount = 0;

    // Find all rows that contain at least one settled block.
    for (int y = 0; y < GRID_VERTICAL_SIZE - 1; y++)
    {
        bool hasBlock = false;

        for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
        {
            if (grid[x][y] == FULL)
            {
                hasBlock = true;
                break;
            }
        }

        if (hasBlock)
        {
            availableRows[rowCount] = y;
            rowCount++;
        }
    }

    // Do nothing if there are no rows containing blocks.
    if (rowCount == 0)
        return;

    // Randomly select one eligible row.
    int selectedRow = availableRows[GetRandomValue(0, rowCount - 1)];

    // Shift every row above the selected row downward.
    for (int y = selectedRow; y > 0; y--)
    {
        for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
        {
            grid[x][y] = grid[x][y - 1];
        }
    }

    // Clear the top row.
    for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
    {
        grid[x][0] = EMPTY;
    }
}

// BFS from the bottom row identifies FULL blocks connected to the floor.
// Any FULL block not reached by the search is considered floating and removed.
// FILL UPPER: Moves blocks from the upper half
// into inaccessible empty spaces in the bottom half.
// FILL UPPER: Fills inaccessible empty spaces across the entire board.
static void TriggerPiecePurifier(void)
{
    bool reachable[GRID_HORIZONTAL_SIZE][GRID_VERTICAL_SIZE] = { false };

    Point queue[GRID_HORIZONTAL_SIZE * GRID_VERTICAL_SIZE];
    int front = 0;
    int rear = 0;

    const int dx[4] = { -1, 1, 0, 0 };
    const int dy[4] = { 0, 0, -1, 1 };

    const int floorRow = GRID_VERTICAL_SIZE - 2;

    // Find all empty spaces accessible from the top.
    for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
    {
        if (grid[x][0] == EMPTY)
        {
            reachable[x][0] = true;
            queue[rear++] = (Point){ x, 0 };
        }
    }

    // Flood-fill all connected empty spaces.
    while (front < rear)
    {
        Point current = queue[front++];

        for (int d = 0; d < 4; d++)
        {
            int nx = current.x + dx[d];
            int ny = current.y + dy[d];

            if (nx >= 1 && nx < GRID_HORIZONTAL_SIZE - 1 &&
                ny >= 0 && ny <= floorRow &&
                !reachable[nx][ny] &&
                grid[nx][ny] == EMPTY)
            {
                reachable[nx][ny] = true;
                queue[rear++] = (Point){ nx, ny };
            }
        }
    }

    // Find inaccessible empty spaces across the entire board.
    for (int y = floorRow; y >= 0; y--)
    {
        for (int x = 1; x < GRID_HORIZONTAL_SIZE - 1; x++)
        {
            if (grid[x][y] == EMPTY && !reachable[x][y])
            {
                // Find an existing FULL block anywhere on the board.
                for (int sourceY = 0; sourceY <= floorRow; sourceY++)
                {
                    bool moved = false;

                    for (int sourceX = 1;
                         sourceX < GRID_HORIZONTAL_SIZE - 1;
                         sourceX++)
                    {
                        if (grid[sourceX][sourceY] == FULL)
                        {
                            grid[x][y] = FULL;
                            grid[sourceX][sourceY] = EMPTY;

                            moved = true;
                            break;
                        }
                    }

                    if (moved)
                        break;
                }
            }
        }
    }
}

// Applies Line Blaster or Fill Upper after the current piece lands.
static void ApplyLandingModifier(int centerX, int centerY)
{
    (void)centerX;
    (void)centerY;

    if (gameMeters.activeModifierID == MOD_LINE_BLASTER)
    {
        TriggerLineBlaster();

        gameMeters.activeModifierID = MOD_NONE;
        gameMeters.modifierTimer = 0.0f;
    }
    else if (gameMeters.activeModifierID == MOD_FILLUPPER)
    {
        TriggerPiecePurifier();

        gameMeters.activeModifierID = MOD_NONE;
        gameMeters.modifierTimer = 0.0f;
    }
}

// Updates temporary effects. gravitySpeed is restored when the effect expires.
static void ApplyTimedModifiers(void)
{
    if (gameMeters.activeModifierID == MOD_SLOW_MO ||
        gameMeters.activeModifierID == MOD_SPEED_SPIKE ||
        gameMeters.activeModifierID == MOD_CONTROL_FLIP ||
        gameMeters.activeModifierID == MOD_FOG_OF_WAR)
    {
        gameMeters.modifierTimer -= GetFrameTime();

        if (gameMeters.modifierTimer <= 0.0f)
        {
            gameMeters.modifierTimer = 0.0f;
            gameMeters.activeModifierID = MOD_NONE;
        }
    }

    gravitySpeed = baseGravitySpeed;

    if (gameMeters.activeModifierID == MOD_SLOW_MO)
    {
        gravitySpeed = baseGravitySpeed * 2;
    }
    else if (gameMeters.activeModifierID == MOD_SPEED_SPIKE)
    {
        gravitySpeed = baseGravitySpeed / 2;
        if (gravitySpeed < 5) gravitySpeed = 5;
    }
}

