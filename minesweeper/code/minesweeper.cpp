/**
 * TODO(rick):
 *
 * GAMEPLAY
 * - Floodfill reveal logic
 * - Timer - Performance Counter/Frequency?
 * - Menu
 * - "Themes"
 * - Different block types?
 * - Levels/progression? Currated games?
 *
 * RENDERING
 * - Push Buffer Renderer
 * - Bitmaps
 *   - Loading from files
 *   - Blitting
 * - Alpha blending
 * - Text Rendering
 * - Blur effect for "glass" tiles?
 * - "RTX ON"
 *
 * AUDIO
 * - Platform playback
 * - Sound effects
 * - Music
 *
 * ENGINE FEATURES
 * - Assets
 *   - Asset bundle file
 *   - Loading assets
 *   - Referencing assets
 **/

#include "minesweeper.h"

inline void
Clear(bitmap *DrawBuffer, v4 ColorIn)
{
    u32 Color = ( ((u8)(ColorIn.r*255.0f) << 16) |
                  ((u8)(ColorIn.g*255.0f) << 8) |
                  ((u8)(ColorIn.b*255.0f) << 0) |
                  ((u8)(ColorIn.a*255.0f) << 24) );

    u32 *Pixel = (u32 *)DrawBuffer->Data;
    for(u32 Y = 0; Y < DrawBuffer->Height; ++Y)
    {
        for(u32 X = 0; X < DrawBuffer->Width; ++X)
        {
            *Pixel++ = Color;
        }
    }
}

static void
DrawRectangle(bitmap *DrawBuffer, v2 Min, v2 Max, v4 ColorIn)
{
    u32 Color = ( ((u8)(ColorIn.r*255.0f) << 16) |
                  ((u8)(ColorIn.g*255.0f) << 8) |
                  ((u8)(ColorIn.b*255.0f) << 0) |
                  ((u8)(ColorIn.a*255.0f) << 24) );

    s32 MinX = Min.x;
    s32 MinY = Min.y;
    s32 MaxX = Max.x;
    s32 MaxY = Max.y;

    if(MinX < 0) { MinX = 0; }
    if(MinY < 0) { MinY = 0; }
    if(MaxX > DrawBuffer->Width) { MaxX = DrawBuffer->Width; }
    if(MaxY > DrawBuffer->Height) { MaxY = DrawBuffer->Height; }

    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = ((u32 *)DrawBuffer->Data + (Y * DrawBuffer->Width) + MinX);
        for(u32 X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }
    }
}

static void
DrawRectangle(bitmap *DrawBuffer, rect2 Rectangle, v4 Color)
{
    DrawRectangle(DrawBuffer, Rectangle.Min, Rectangle.Max, Color);
}

static void
DrawRectangleOutline(bitmap *DrawBuffer, rect2 Rect, v4 Color, f32 Thickness = 1.0f)
{
    DrawRectangle(DrawBuffer, Rect.Min, V2(Rect.Max.x, Rect.Min.y + Thickness), Color);
    DrawRectangle(DrawBuffer, V2(Rect.Min.x, Rect.Max.y - Thickness), Rect.Max, Color);
    DrawRectangle(DrawBuffer, Rect.Min, V2(Rect.Min.x + Thickness, Rect.Max.y), Color);
    DrawRectangle(DrawBuffer, V2(Rect.Max.x - Thickness, Rect.Min.y), Rect.Max, Color);
}

UPDATE_AND_RENDER(UpdateAndRender)
{
    game_state *GameState = (game_state *)AppMemory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        InitializeArena(&GameState->GameArena,
                        (u8 *)AppMemory->PermanentStorage + sizeof(game_state),
                        AppMemory->PermanentStorageSize - sizeof(game_state));

        // TODO(rick): Move this into the input struct and seed it from a time
        // based rand() call in the platform?
        GameState->PRNGState = 101377;

        GameState->CellSize = 30.0f;
        GameState->BombFrequency = 0.1f;
        GameState->GridWidth = 9;
        GameState->GridHeight = 12;

        u32 CellCount = GameState->GridWidth*GameState->GridHeight;
        GameState->GameGrid = PushArray(&GameState->GameArena, CellCount, cell);
        Assert(GameState->GameGrid);

        GameState->BombCount = (s32)((f32)CellCount * GameState->BombFrequency);
        for(u32 BombIndex = 0; BombIndex < GameState->BombCount; ++BombIndex)
        {
            b32 BombPlaced = false;
            while(!BombPlaced)
            {
                u32 CellIndex = GetNextRandomNumber(&GameState->PRNGState) % CellCount;
                s32 Row = CellIndex / GameState->GridWidth;
                s32 Column = CellIndex % GameState->GridWidth;
                Assert(Row < GameState->GridHeight);
                Assert(Column < GameState->GridWidth);

                cell *Target = (GameState->GameGrid + (Row * GameState->GridWidth) + Column);
                if(!Target->IsBomb)
                {
                    Target->IsBomb = true;
                    BombPlaced = true;

                    u32 MinY = MAX(Row - 1, 0);
                    u32 MaxY = MIN(Row + 1, GameState->GridHeight - 1);
                    u32 MinX = MAX(Column - 1, 0);
                    u32 MaxX = MIN(Column + 1, GameState->GridWidth - 1);

                    cell *CellRow = GameState->GameGrid + (MinY * GameState->GridWidth);
                    for(u32 Y = MinY; Y <= MaxY; ++Y)
                    {
                        cell *CellAt = CellRow + MinX;
                        for(u32 X = MinX; X <= MaxX; ++X)
                        {
                            ++CellAt->NeighboringBombCount;
                            ++CellAt;
                        }
                        CellRow += GameState->GridWidth;
                    }
                }
            }
        }
        GameState->GameGrid[0].IsRevealed = true;
        GameState->GameGrid[1].IsFlagged = true;

        GameState->IsInitialized = true;
    }

    v2 WindowCenterPoint = 0.5f*V2(DrawBuffer->Width, DrawBuffer->Height);
    rect2 GameGridRect = Rect2(V2(0, 0), GameState->CellSize*V2(GameState->GridWidth, GameState->GridHeight));
    v2 GameGridCenterPoint = 0.5f*V2(GameGridRect.Max.x - GameGridRect.Min.x, GameGridRect.Max.y - GameGridRect.Min.y);
    v2 GameGridOffset = WindowCenterPoint - GameGridCenterPoint;
    GameGridOffset.x = Clamp(0.0f, GameGridOffset.x, (f32)DrawBuffer->Width);
    GameGridOffset.y = Clamp(0.0f, GameGridOffset.y, (f32)DrawBuffer->Height);

    cell *GridCell = GameState->GameGrid;
    for(u32 Y = 0; Y < GameState->GridHeight; ++Y)
    {
        for(u32 X = 0; X < GameState->GridWidth; ++X)
        {
            v2 MinPoint = GameGridOffset + V2(X*GameState->CellSize, Y*GameState->CellSize);
            v2 MaxPoint = MinPoint + V2(GameState->CellSize, GameState->CellSize);
            rect2 CellRectangle = Rect2(MinPoint, MaxPoint);
            if(IsInRectangle(CellRectangle, V2(Input->MouseX, Input->MouseY)))
            {
                if(WasDown(Input->MouseButton_Left) && !GridCell->IsFlagged)
                {
                    GridCell->IsRevealed = true;
                }
                else if(WasDown(Input->MouseButton_Right) && !GridCell->IsRevealed)
                {
                    GridCell->IsFlagged = !GridCell->IsFlagged;
                }
            }

            ++GridCell;
        }
    }

    Clear(DrawBuffer, V4(1, 0, 1, 1));
    DrawRectangle(DrawBuffer, GameGridOffset + V2(0, 0),
                  GameGridOffset + V2(GameState->GridWidth*GameState->CellSize, GameState->GridHeight*GameState->CellSize),
                  V4(0, 0, 0, 1));

    f32 BombColorBucket = (255.0f / 9.0f);
    cell *CellToDraw = GameState->GameGrid;
    for(u32 Y = 0; Y < GameState->GridHeight; ++Y)
    {
        for(u32 X = 0; X < GameState->GridWidth; ++X)
        {
            v2 MinPoint = GameGridOffset + V2(X*GameState->CellSize, Y*GameState->CellSize);
            v2 MaxPoint = MinPoint + V2(GameState->CellSize, GameState->CellSize);
            rect2 CellRectangle = Rect2(MinPoint, MaxPoint);

            v4 Color = {1, 0, 0, 1};
            if(CellToDraw->IsBomb)
            {
                Color = V4(1, 1, 0, 1);
            }
            else if(CellToDraw->IsFlagged)
            {
                Color = V4(0, 0, 1, 1);
            }
            else if(CellToDraw->IsRevealed)
            {
                Color = V4(0, 1, 0, 1);
            }
            else
            {
                f32 ColorValue = (CellToDraw->NeighboringBombCount * BombColorBucket) / 255.0f;
                Color = V4(ColorValue, ColorValue, ColorValue, 1);
            }

            DrawRectangleOutline(DrawBuffer, CellRectangle, Color);

            if(IsInRectangle(CellRectangle, V2(Input->MouseX, Input->MouseY)))
            {
                DrawRectangleOutline(DrawBuffer, CellRectangle, V4(0, 1, 1, 1), 1.0f);
            }

            ++CellToDraw;
        }
    }

    // DrawRectangle(DrawBuffer, V2(0, 0), V2(30, 30), V4(1, 0, 0, 1));
    // DrawRectangle(DrawBuffer, V2(30, 30), V2(60, 60), V4(0, 0, 1, 1));
}
