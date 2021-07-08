/**
 * TODO(rick):
 *
 * GAMEPLAY
 * - Floodfill reveal logic
 *   - This should be double checked but is complete until then.
 * - Menu
 * - "Themes"
 * - Different block types?
 * - Levels/progression? Currated games?
 *
 * RENDERING
 * - Multiple Bitmaps
 * - Text Rendering
 * - Blur effect for "glass" tiles?
 * - "RTX ON"
 * - SIMD rectangle and bitmap drawing
 * - Support for top-down and bottom-up bitmaps
 * - Render Rectangle/Clip
 * - Multi-threaded rendering
 *
 * ENGINE FEATURES
 * - Memory Arenas
 *   - Alignment
 * - Assets
 *   - Asset bundle file
 *   - Loading assets
 *   - Referencing assets
 *   - Asset management
 *
 * AUDIO
 * - Platform playback
 * - Sound effects
 * - Music
 *
 * PLATFORM
 * - Worker threads
 *   - Complete all work API
 *   - Add work API
 *
 **/

#include "minesweeper.h"

#define USE_ALPHA_BLENDING 1

inline v4
SRGB255ToLinear1(v4 C)
{
    f32 Inv255 = 1.0f / 255.0f;

    v4 Result = {};
    Result.r = C.r*Inv255;
    Result.g = C.g*Inv255;
    Result.b = C.b*Inv255;
    Result.a = C.a*Inv255;
    return(Result);
}

inline v4
Linear1ToSRGB255(v4 C)
{
    v4 Result = {};
    Result.r = 255.0f*C.r;
    Result.g = 255.0f*C.g;
    Result.b = 255.0f*C.b;
    Result.a = 255.0f*C.a;
    return(Result);
}

inline v4
U32ToV4(u32 A)
{
    v4 Result = { (f32)((A >> 16) & 0xFF),
                  (f32)((A >> 8) & 0xFF),
                  (f32)((A >> 0) & 0xFF),
                  (f32)((A >> 24) & 0xFF) };
    return(Result);
}

inline u32
V4ToU32(v4 A)
{
    u32 Result = ( ((u8)(A.r) << 16) |
                   ((u8)(A.g) << 8) |
                   ((u8)(A.b) << 0) |
                   ((u8)(A.a) << 24) );
    return(Result);
}

inline color_mask
GetColorMask(u32 Mask)
{
    color_mask Result = {};
    Result.Mask = Mask;
    Result.ShiftAmount = BitScanForward(Mask);
    return(Result);
}

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
#if USE_ALPHA_BLENDING 
    ColorIn.r = ColorIn.r*ColorIn.a;
    ColorIn.g = ColorIn.g*ColorIn.a;
    ColorIn.b = ColorIn.b*ColorIn.a;
#endif
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
#if USE_ALPHA_BLENDING
            v4 Dest = { (f32)((*Pixel >> 16) & 0xFF),
                        (f32)((*Pixel >> 8) & 0xFF),
                        (f32)((*Pixel >> 0) & 0xFF),
                        (f32)((*Pixel >> 24) & 0xFF) };
            Dest = SRGB255ToLinear1(Dest);
            v4 Result = (1.0f - ColorIn.a)*Dest + ColorIn;
            Result = Linear1ToSRGB255(Result);
            *Pixel++ = ( ((u32)(Result.r) << 16) | 
                         ((u32)(Result.g) << 8) |
                         ((u32)(Result.b) << 0) |
                         ((u32)(Result.a) << 24) );
#else
            *Pixel++ = Color;
#endif
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

static void
DrawBitmap(bitmap *DrawBuffer, bitmap *Bitmap, f32 XIn, f32 YIn, v4 Color)
{
    s32 XMinOut = XIn;
    if(XMinOut < 0)
    {
        XMinOut = 0;
    }

    s32 XMaxOut = XIn + Bitmap->Width;
    if(XMaxOut > DrawBuffer->Width)
    {
        XMaxOut = DrawBuffer->Width;
    }

    s32 YMinOut = YIn;
    if(YMinOut < 0)
    {
        YMinOut = 0;
    }

    s32 YMaxOut = YIn + Bitmap->Height;
    if(YMaxOut > DrawBuffer->Height)
    {
        YMaxOut = DrawBuffer->Height;
    }

    s32 XMinSource = 0;
    if(XIn < XMinOut)
    {
        XMinSource += Absolute(XMinOut - XIn);
    }

    s32 YMinSource = 0;
    if(YIn < YMinOut)
    {
        YMinSource += Absolute(YMinOut - YIn);
    }

    //u32 *SourceRow = (u32 *)Bitmap->Data + (YMinSource * Bitmap->Width) + XMinSource;
    u32 *SourceRow = (u32 *)Bitmap->Data + ((Bitmap->Height-1) * Bitmap->Width) + XMinSource;
    u32 *DestRow = (u32 *)DrawBuffer->Data + (YMinOut * DrawBuffer->Width) + XMinOut;
    for(u32 Y = YMinOut; Y < YMaxOut; ++Y)
    {
        u32 *SourcePixel = SourceRow;
        u32 *DestPixel = DestRow;
        for(u32 X = XMinOut; X < XMaxOut; ++X)
        {
            u32 ColorPixel = *SourcePixel++;
            v4 SourcePixelV4 = { (f32)((ColorPixel & Bitmap->RedMask.Mask) >> Bitmap->RedMask.ShiftAmount),
                                 (f32)((ColorPixel & Bitmap->GreenMask.Mask) >> Bitmap->GreenMask.ShiftAmount),
                                 (f32)((ColorPixel & Bitmap->BlueMask.Mask) >> Bitmap->BlueMask.ShiftAmount),
                                 (f32)((ColorPixel & Bitmap->AlphaMask.Mask) >> Bitmap->AlphaMask.ShiftAmount) };
            f32 SourceAlpha = ((f32)SourcePixelV4.a / 255.0f) * Color.a;
            SourcePixelV4.rgb = SourcePixelV4.rgb * SourceAlpha;
            SourcePixelV4.r *= Color.a*Color.r;
            SourcePixelV4.g *= Color.a*Color.g;
            SourcePixelV4.b *= Color.a*Color.b;

            v4 DestV4 = U32ToV4(*DestPixel);
            DestV4.rgb = SourcePixelV4.rgb + ((1.0f - SourceAlpha)*DestV4.rgb);
            u32 ColorOut = V4ToU32(DestV4);

            *DestPixel++ = ColorOut;
        }
        
        SourceRow -= Bitmap->Width;
        //SourceRow += Bitmap->Width;
        DestRow += DrawBuffer->Width;
    }
}

#include "minesweeper_render_group.cpp"

static void
DrawText(render_group *RenderGroup, font_map *FontMap, f32 XIn, f32 YIn, char *String, v4 Color)
{
    s32 AtX = XIn;
    s32 AtY = YIn;

    for(char *Character = String; *Character; ++Character)
    {
        bitmap GlyphBitmap = {};
        GetGlyphBitmapForCodePoint(FontMap, *Character,
                                   (s32 *)&GlyphBitmap.Width, (s32 *)&GlyphBitmap.Height,
                                   (s32 *)&GlyphBitmap.Pitch, &GlyphBitmap.Data);

        // TODO(rick): Using the RED channel mask as the ALPHA mask. I should
        // upgrade the font tool so that it creates correctly alpha'd bitmaps.
        // The header should also upgrade so that it writes out the mask
        // information to the font_map.
        GlyphBitmap.RedMask = GetColorMask(0x00FF0000);
        GlyphBitmap.GreenMask = GetColorMask(0x0000FF00);
        GlyphBitmap.BlueMask = GetColorMask(0x000000FF);
        GlyphBitmap.AlphaMask = GetColorMask(0x00FF0000);

        s32 AlignX = 0;
        s32 AlignY = 0;
        GetAlignmentForForCodePoint(FontMap, *Character, &AlignX, &AlignY);

        s32 OutX = AtX + GetHorizontalAdjustmentForCodePoint(FontMap, *Character);
        s32 OutY = AtY - GlyphBitmap.Height + AlignY;

        PushBitmap(RenderGroup, &GlyphBitmap, V2(OutX, OutY), Color);
        AtX += GetHorizontalAdvanceForCodepoint(FontMap, *Character);
    }
}

static void
PushText(render_group *RenderGroup, font_map *FontMap, v2 P, char *String, v4 Color)
{
    DrawText(RenderGroup, FontMap, P.x, P.y, String, Color);
}

#define PushText(RenderGroup, FontMap, P, Color, Format, ...) \
{ \
    char Buffer[256] = {}; \
    snprintf(Buffer, ArrayCount(Buffer), Format, __VA_ARGS__); \
    PushText(RenderGroup, FontMap, P, Buffer, Color); \
}

static void
RecursiveFloodFillReveal(game_state *GameState, s32 XAt, s32 YAt)
{
    if((XAt < 0) || (XAt >= GameState->GridWidth) ||
       (YAt < 0) || (YAt >= GameState->GridHeight))
    {
        return;
    }

    cell *Cell = (GameState->GameGrid + (YAt * GameState->GridWidth) + XAt);
    if(Cell->IsRevealed || Cell->IsBomb || Cell->IsFlagged)
    {
        return;
    }

    Cell->IsRevealed = true;

    if(Cell->NeighboringBombCount)
    {
        return;
    }

    s32 YMin = YAt - 1;
    s32 XMin = XAt - 1;
    s32 YMax = YAt + 1;
    s32 XMax = XAt + 1;

    for(s32 Y = YMin; Y <= YMax; ++Y)
    {
        for(s32 X = XMin; X <= XMax; ++X)
        {
            if((X == XAt) && (Y == YAt))
            {
                continue;
            }

            RecursiveFloodFillReveal(GameState, X, Y);
        }
    }
}

static platform_api Platform;
UPDATE_AND_RENDER(UpdateAndRender)
{
    game_state *GameState = (game_state *)AppMemory->PermanentStorage;
    Platform = AppMemory->PlatformAPI;
    if(!GameState->IsInitialized)
    {
        InitializeArena(&GameState->GameArena,
                        (u8 *)AppMemory->PermanentStorage + sizeof(game_state),
                        AppMemory->PermanentStorageSize - sizeof(game_state));

        file_result FontFile = Platform.ReadFile("font.fmp");
        if(FontFile.Contents)
        {
            GameState->FontMap = (font_map *)PushSize(&GameState->GameArena, FontFile.FileSize);
            Copy(GameState->FontMap, FontFile.Contents, FontFile.FileSize);
            if(!InitializeFontMap(GameState->FontMap))
            {
                Assert(!"Failed to init font!");
            }

            Platform.FreeMemory(FontFile.Contents);
        }
        else
        {
            Assert(!"Failed to load font!");
        }

        //file_result TestBitmapFile = Platform.ReadFile("StructuredArt.bmp");
        //file_result TestBitmapFile = Platform.ReadFile("BackgroundLarge.bmp");
        //file_result TestBitmapFile = Platform.ReadFile("background.bmp");
        file_result TestBitmapFile = Platform.ReadFile("background2.bmp");
        //file_result TestBitmapFile = Platform.ReadFile("alpha.bmp");
        if(TestBitmapFile.Contents)
        {
            bitmap_file *BitmapFile = (bitmap_file *)TestBitmapFile.Contents;
            GameState->TestBitmap.Width = BitmapFile->InfoHeader.Width;
            GameState->TestBitmap.Height = Absolute(BitmapFile->InfoHeader.Height);
            GameState->TestBitmap.Data = (u8 *)BitmapFile + BitmapFile->FileHeader.DataOffset;
            GameState->TestBitmap.Pitch = GameState->TestBitmap.Width*4;
            GameState->TestBitmap.RedMask = GetColorMask(BitmapFile->InfoHeader.RedMask);
            GameState->TestBitmap.GreenMask = GetColorMask(BitmapFile->InfoHeader.GreenMask);
            GameState->TestBitmap.BlueMask = GetColorMask(BitmapFile->InfoHeader.BlueMask);
            GameState->TestBitmap.AlphaMask = GetColorMask(~(BitmapFile->InfoHeader.RedMask | BitmapFile->InfoHeader.GreenMask | BitmapFile->InfoHeader.BlueMask));
            // TODO(rick): Copy memory into arena and free platform memory!!!
        }

        // TODO(rick): Move this into the input struct and seed it from a time
        // based rand() call in the platform?
        GameState->PRNGState = 101377;
        {
            // NOTE(rick): Game setup
            GameState->CellSize = 30.0f;
            GameState->CellSpacing = 0.0f;
            GameState->BombFrequency = 0.2f;
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
        }

        GameState->GameBeginMSCount = Platform.GetMSElapsed64();
        GameState->IsInitialized = true;
    }

    transient_state *TransState = (transient_state *)AppMemory->TransientStorage;
    if(!TransState->IsInitialized)
    {
        InitializeArena(&TransState->TransArena,
                        (u8 *)AppMemory->TransientStorage + sizeof(transient_state),
                        AppMemory->TransientStorageSize - sizeof(transient_state));

        TransState->IsInitialized = true;
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
                if(WasDown(Input->MouseButton_Left))
                {
                    if(GridCell->IsBomb)
                    {
                        // TODO(rick): Game over
                    }
                    else if(!GridCell->IsFlagged && !GridCell->IsRevealed)
                    {
                        // GridCell->IsRevealed = true;
                        // ++GameState->RevealedCount;
                        RecursiveFloodFillReveal(GameState, X, Y);
                    }
                }
                else if(WasDown(Input->MouseButton_Right) && !GridCell->IsRevealed)
                {
                    GridCell->IsFlagged = !GridCell->IsFlagged;
                }
            }

            ++GridCell;
        }
    }

    temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransArena);
    render_group *RenderGroup = AllocateRenderGroup(&TransState->TransArena, Kilobytes(512));

    PushClear(RenderGroup, V4(1, 0, 1, 1));
    PushBitmap(RenderGroup, &GameState->TestBitmap, V2(-10, -10));

    f32 GameGridOutlineThickness = 1.0f;
    v2 GameGridOutlineSize = {GameGridOutlineThickness, GameGridOutlineThickness};
    rect2 OutlineRect = Rect2(GameGridOffset + V2(0, 0) - GameGridOutlineSize,
                              GameGridOffset + V2(GameState->GridWidth*GameState->CellSize, GameState->GridHeight*GameState->CellSize) + GameGridOutlineSize);
    PushRectangleOutline(RenderGroup, OutlineRect, V4(0, 0, 0, 0.6f), GameGridOutlineThickness);

    f32 BombColorBucket = (255.0f / 9.0f);
    cell *CellToDraw = GameState->GameGrid;
    for(u32 Y = 0; Y < GameState->GridHeight; ++Y)
    {
        for(u32 X = 0; X < GameState->GridWidth; ++X)
        {
            v2 MinPoint = GameGridOffset + GameState->CellSize*V2(X, Y) + GameState->CellSpacing*V2(X, Y);
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
#if 0
            else if(CellToDraw->IsRevealed)
            {
                Color = V4(0, 1, 0, 1);
            }
#endif
            else
            {
                f32 ColorValue = 0;//(CellToDraw->NeighboringBombCount * BombColorBucket) / 255.0f;
                Color = V4(ColorValue, ColorValue, ColorValue, 1);
            }

            PushRectangle(RenderGroup, CellRectangle, V4(Color.xyz, 0.5f));

            if(!CellToDraw->IsBomb && CellToDraw->NeighboringBombCount)
            {
                PushText(RenderGroup, GameState->FontMap,
                         V2(CellRectangle.Min.x, CellRectangle.Max.y) + V2(0.35f*GameState->CellSize, -0.27f*GameState->CellSize),
                         V4(0, 0, 0, 1), NumberStringTable[CellToDraw->NeighboringBombCount]);
                PushText(RenderGroup, GameState->FontMap,
                         V2(CellRectangle.Min.x, CellRectangle.Max.y)-V2(1, 1) + V2(0.35f*GameState->CellSize, -0.27f*GameState->CellSize),
                         V4(1, 1, 1, 1), NumberStringTable[CellToDraw->NeighboringBombCount]);
            }

            if(CellToDraw->IsRevealed)
            {
                PushRectangleOutline(RenderGroup, CellRectangle, V4(0, 1, 0, 1));
            }

            if(IsInRectangle(CellRectangle, V2(Input->MouseX, Input->MouseY)))
            {
                PushRectangleOutline(RenderGroup, CellRectangle, V4(0, 1, 1, 1), 1.0f);
            }

            ++CellToDraw;
        }
    }

    {
        u64 GameEndMSCount = Platform.GetMSElapsed64();
        f64 SecondsTaken = (f64)(GameEndMSCount - GameState->GameBeginMSCount) / 1000.0f;
        v2 TimerP = {6, (f32)GameState->FontMap->FontHeight};
        PushText(RenderGroup, GameState->FontMap, TimerP, V4(0, 0, 0, 1), "%.02fs %db %df", SecondsTaken, GameState->BombCount, GameState->FlagCount);
        PushText(RenderGroup, GameState->FontMap, TimerP-V2(1, 1), V4(1, 1, 1, 1), "%.02fs %db %df", SecondsTaken, GameState->BombCount, GameState->FlagCount);
    }

    RenderGroupToOutput(RenderGroup, DrawBuffer);

    EndTemporaryMemory(RenderMemory);
    CheckArena(&TransState->TransArena);
}

#define FONT_LIB_IMPLEMENTATION
#include "font_lib.h"
