/**
 * TODO(rick):
 *
 * GAMEPLAY
 * - Push strings onto RenderGroup/TransArena
 *
 * RENDERING
 * - Weird alpha blending problem in draw rectangle
 * - Support for top-down and bottom-up bitmaps
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
 *
 * --- CODE CLEAN UP ---
 **/

#include <stdio.h>

#include "font_lib.h"
#include "minesweeper_platform.h"
#include "minesweeper_memory.h"
#include "minesweeper_intrinsics.h"
#include "minesweeper_math.h"
#include "minesweeper_render_group.h"
#include "minesweeper.h"

static platform_api Platform;

#include "minesweeper_renderer.cpp"
#include "minesweeper_render_group.cpp"


static color_settings
GetDefaultColorSettings()
{
    color_settings Result = {};
    Result.None = {0, 0, 0, 1};
    Result.Background = {0, 0, 0, 1};
    Result.TextDefault = {1, 1, 1, 1};
    Result.TextShadow = {0, 0, 0, 1};
    Result.TextMenu = {1, 1, 1, 1};
    Result.TextMenuCurrent = {0.9f, 0.3f, 0.2f, 1.0f};
    Result.TextMenuSetting = {1.0f, 0.8f, 0.4f, 1.0f};
    Result.TextHUDTime = {1, 1, 1, 1};
    Result.TextHUDFlags = {1, 1, 1, 1};
    Result.TextCell = {1, 1, 1, 1};
    Result.TextWin = {0.4f, 0.8f, 0.4f, 1.0f};
    Result.TextLose = {0.9f, 0.2f, 0.4f, 1.0f};
    Result.CellHidden = {0, 0, 0, 1};
    Result.CellRevealed = {0.15f, 0.15f, 0.15f, 0.45f};
    Result.CellFlagged = {1.0f, 0.9f, 0.4f, 1.0f};
    Result.CellBomb = {0.9f, 0.3f, 0.2f, 1.0f};
    Result.CellBorder = {0.1f, 0.1f, 0.1f, 0.7f};
    Result.CellFocused = {0, 1, 1, 1};
    Result.OverlayBackground = {0, 0, 0, 0.7f};
    Result.OverlayBorder = {0, 0, 0, 1.0f};
    return(Result);
}

static b32
LoadColorSettingsFromFile(char *FileName, color_settings *ColorOut)
{
    b32 Result = false;

    file_result ColorFile = Platform.ReadFile(FileName);
    if(ColorFile.Contents)
    {
        color_settings Settings = {};

        char *At = (char *)ColorFile.Contents;
        char NameBuffer[64] = {};
        while((*At != 0) && ((At - ColorFile.Contents) < ColorFile.FileSize))
        {
            v4 Color = {};
            sscanf(At, "%s = {%f, %f, %f, %f}", NameBuffer,
                   &Color.r, &Color.g, &Color.b, &Color.a);
            if(ScanTo(&At, '\n'))
            {
                ++At;
            }

            v4 *Dest = 0;
            if(StringCompare("None", NameBuffer) == 0) { Dest = &Settings.None; }
            else if(StringCompare("Background", NameBuffer) == 0) { Dest = &Settings.Background; }
            else if(StringCompare("TextDefault", NameBuffer) == 0) { Dest = &Settings.TextDefault; }
            else if(StringCompare("TextShadow", NameBuffer) == 0) { Dest = &Settings.TextShadow; }
            else if(StringCompare("TextMenu", NameBuffer) == 0) { Dest = &Settings.TextMenu; }
            else if(StringCompare("TextMenuCurrent", NameBuffer) == 0) { Dest = &Settings.TextMenuCurrent; }
            else if(StringCompare("TextMenuSetting", NameBuffer) == 0) { Dest = &Settings.TextMenuSetting; }
            else if(StringCompare("TextHUDTime", NameBuffer) == 0) { Dest = &Settings.TextHUDTime; }
            else if(StringCompare("TextHUDFlags", NameBuffer) == 0) { Dest = &Settings.TextHUDFlags; }
            else if(StringCompare("TextCell", NameBuffer) == 0) { Dest = &Settings.TextCell; }
            else if(StringCompare("TextWin", NameBuffer) == 0) { Dest = &Settings.TextWin; }
            else if(StringCompare("TextLose", NameBuffer) == 0) { Dest = &Settings.TextLose; }
            else if(StringCompare("CellHidden", NameBuffer) == 0) { Dest = &Settings.CellHidden; }
            else if(StringCompare("CellRevealed", NameBuffer) == 0) { Dest = &Settings.CellRevealed; }
            else if(StringCompare("CellFlagged", NameBuffer) == 0) { Dest = &Settings.CellFlagged; }
            else if(StringCompare("CellBomb", NameBuffer) == 0) { Dest = &Settings.CellBomb; }
            else if(StringCompare("CellBorder", NameBuffer) == 0) { Dest = &Settings.CellBorder; }
            else if(StringCompare("CellFocused", NameBuffer) == 0) { Dest = &Settings.CellFocused; }
            else if(StringCompare("OverlayBackground", NameBuffer) == 0) { Dest = &Settings.OverlayBackground; }
            else if(StringCompare("OverlayBorder", NameBuffer) == 0) { Dest = &Settings.OverlayBorder; }
            if(Dest)
            {
                *Dest = Color;
            }
        }

        Result = true;
        *ColorOut = Settings;
    }

    return(Result);
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

static void
BeginNewGame(game_state *GameState)
{
    // NOTE(rick): Game setup
    GameState->CellSize = 32.0f;
    GameState->CellSpacing = 0.0f;

    u32 CellCount = GameState->GridWidth*GameState->GridHeight;
    GameState->BombCount = (s32)((f32)CellCount * GameState->BombFrequency);
    GameState->FlagCount = GameState->BombCount;

    ZeroMemory(GameState->GameGrid, CellCount * sizeof(cell));
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

    GameState->EndState = EndState_None;
    GameState->GameMode = GameMode_Playing;
    GameState->TimerActive = false;
    GameState->GameBeginMSCount = GameState->GameCurrentMSCount = Platform.GetMSElapsed64();
}

static void
ChangeGameSettings(game_state *GameState, difficulty_type Difficulty, grid_size_type GridSize)
{
    GameState->BombFrequency = 0;
    GameState->GridWidth = 0;
    GameState->GridHeight = 0;

    switch(Difficulty)
    {
        case DifficultyType_Easy:
        {
            GameState->BombFrequency = 0.10f;
        } break;
        case DifficultyType_Normal:
        {
            GameState->BombFrequency = 0.14f;
        } break;
        case DifficultyType_Hard:
        {
            GameState->BombFrequency = 0.17f;
        } break;

        InvalidDefaultCase;
    }

    switch(GridSize)
    {
        case GridSizeType_Small:
        {
            GameState->GridWidth = 7;
            GameState->GridHeight = 10;
        } break;
        case GridSizeType_Medium:
        {
            GameState->GridWidth = 15;
            GameState->GridHeight = 19;
        } break;
        case GridSizeType_Large:
        {
            GameState->GridWidth = 26;
            GameState->GridHeight = 30;
        } break;

        InvalidDefaultCase;
    }
}

static v2
CenterTextAtPoint(game_state *GameState, v2 At, char *String)
{
    v2 Result = At;
    rect2 TextBounds = GetTextBounds(GameState->FontMap, String);
    Result.x -= 0.5f*(TextBounds.Max.x - TextBounds.Min.x);
    return(Result);
}

static b32
LoadBitmapAsset(game_state *GameState, memory_arena *Arena, bitmap_asset_id BitmapID, char *FileName)
{
    b32 Result = false;
    Assert(BitmapID < BitmapID_Count);

    struct bitmap *Bitmap = GameState->Bitmaps + BitmapID;
    file_result BitmapFileResult = Platform.ReadFile(FileName);
    if(BitmapFileResult.Contents)
    {
        bitmap_file *BitmapFile = (bitmap_file *)BitmapFileResult.Contents;

        Bitmap->Width = BitmapFile->InfoHeader.Width;
        Bitmap->Height = Absolute(BitmapFile->InfoHeader.Height);
        Bitmap->Pitch = Bitmap->Width*4;
        Bitmap->RedMask = GetColorMask(BitmapFile->InfoHeader.RedMask);
        Bitmap->GreenMask = GetColorMask(BitmapFile->InfoHeader.GreenMask);
        Bitmap->BlueMask = GetColorMask(BitmapFile->InfoHeader.BlueMask);
        Bitmap->AlphaMask = GetColorMask(~(BitmapFile->InfoHeader.RedMask | BitmapFile->InfoHeader.GreenMask | BitmapFile->InfoHeader.BlueMask));

        u32 DataSize = Bitmap->Height*Bitmap->Pitch;
        Bitmap->Data = (u8 *)PushSize(Arena, DataSize);
        Copy(Bitmap->Data, (u8 *)BitmapFile + BitmapFile->FileHeader.DataOffset, DataSize);

        Result = true;
    }

    return(Result);
}

static b32
BitmapButton(game_state *GameState, render_group *RenderGroup, v2 At, v2 MouseAt,
             bitmap_asset_id ID, v4 Color, v4 HotColor)
{
    b32 IsHot = false;
    bitmap *Bitmap = &GameState->Bitmaps[ID];

    v4 RenderColor = Color;
    if(IsInRectangle(Rect2(At, At + V2(Bitmap->Width, Bitmap->Height)), MouseAt))
    {
        RenderColor = HotColor;
        IsHot = true;
    }
    PushBitmap(RenderGroup, Bitmap, At, RenderColor);

    return(IsHot);
}

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

        // TODO(rick): Move this into the input struct and seed it from a time
        // based rand() call in the platform?
        // TODO(rick): Seed this from Platform.GetMSElapsed64() plus some
        // randomization?
        GameState->PRNGState = 101377;

        GameState->MaxGridWidth = 256;
        GameState->MaxGridHeight = 256;
        u32 MaxCellCount = GameState->MaxGridHeight * GameState->MaxGridWidth;
        GameState->GameGrid = PushArray(&GameState->GameArena, MaxCellCount, cell);
        Assert(GameState->GameGrid);

        GameState->GameSettings.Colors = GetDefaultColorSettings();
#if 0
        // TODO(rick): Load saved configuration?
        color_settings ColorSettingsFromFile = {};
        if(LoadColorSettingsFromFile(&ColorSettingsFromFile))
        {
            GameState->GameSettings.Colors = ColorSettingsFromFile;
        }
#endif

        GameState->GameMode = GameMode_Menu;
        //ChangeGameSettings(GameState, DifficultyType_Easy, GridSizeType_Small);
        //BeginNewGame(GameState);

        GameState->IsInitialized = true;
    }

    transient_state *TransState = (transient_state *)AppMemory->TransientStorage;
    if(!TransState->IsInitialized)
    {
        InitializeArena(&TransState->TransArena,
                        (u8 *)AppMemory->TransientStorage + sizeof(transient_state),
                        AppMemory->TransientStorageSize - sizeof(transient_state));

        // TODO(rick): Use the TransState instead of GameState
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Back, "back.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Check, "check.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Clock, "clock.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Exclamation, "exclamation.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Flag, "flag.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Gear, "gear.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Home, "home.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Leaderboard, "leaderboard.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Left, "left.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Light, "light.bmp"));
        Assert(LoadBitmapAsset(GameState, &TransState->TransArena, BitmapID_Right, "right.bmp"));

        TransState->IsInitialized = true;
    }

    if(0)
    {
        temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransState->TransArena, Kilobytes(128));

        PushClear(RenderGroup, V4(0, 0, 0, 1));
        PushBitmap(RenderGroup, &GameState->Bitmaps[BitmapID_Clock], V2(10, 10), V4(1, 1, 1, 1));

        TiledRenderGroupToOutput(RenderGroup, DrawBuffer);
        EndTemporaryMemory(RenderMemory);
        CheckArena(&TransState->TransArena);
    }
    else if(GameState->GameMode == GameMode_Menu)
    {
        menu_state *Menu = &GameState->MainMenuState;
        if(!GameState->MainMenuState.Initialized)
        {
            Menu->Items[Menu->ItemCount++] = {MenuItem_Play};
            Menu->Items[Menu->ItemCount++] = {MenuItem_Scores};
            Menu->Items[Menu->ItemCount++] = {MenuItem_Options};
            Menu->Items[Menu->ItemCount++] = {MenuItem_Quit};
            Assert(Menu->ItemCount < ArrayCount(Menu->Items));

            Menu->DifficultyIndex = 0;
            Menu->DifficultyCount = ArrayCount(MenuDifficultyStringTable);

            Menu->GridSizeIndex = 0;
            Menu->GridSizeCount = ArrayCount(MenuSizeStringTable);

            Menu->Initialized = true;
        }

        if(WasDown(Input->Keyboard_Escape))
        {
            Platform.RequestExit();
        }

        if(WasDown(Input->Keyboard_W))
        {
            --Menu->HotItem;
            if(Menu->HotItem < 0)
            {
                Menu->HotItem = 0;
            }
        }
        else if(WasDown(Input->Keyboard_S))
        {
            ++Menu->HotItem;
            if(Menu->HotItem >= Menu->ItemCount)
            {
                Menu->HotItem = Menu->ItemCount - 1;
            }
        }
        if(WasDown(Input->Keyboard_X))
        {
            Menu->GridSizeIndex = (++Menu->GridSizeIndex % Menu->GridSizeCount);
        }
        if(WasDown(Input->Keyboard_Z))
        {
            Menu->DifficultyIndex = (++Menu->DifficultyIndex % Menu->DifficultyCount);
        }

        temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransState->TransArena, Kilobytes(128));


        PushClear(RenderGroup, GameState->GameSettings.Colors.None);
        //PushText(RenderGroup, GameState->FontMap, 0.5f*V2(DrawBuffer->Width, DrawBuffer->Height), V4(1, 0, 1, 1), "qypj,._");

        v2 MouseAt = {(f32)Input->MouseX, (f32)Input->MouseY};
        v2 MenuTextAt = V2(DrawBuffer->Width, DrawBuffer->Height)*0.5f;
        for(u32 MenuIndex = 0; MenuIndex < Menu->ItemCount; ++MenuIndex)
        {
            menu_item *MenuItem = Menu->Items + MenuIndex;
            char *Label = MenuItemStringTable[(s32)MenuItem->ID];

            rect2 TextBounds = GetTextBounds(GameState->FontMap, Label);
            v2 TextAt = MenuTextAt;
            TextAt.x -= 0.5f*(TextBounds.Max.x - TextBounds.Min.x);
            TextBounds.Min += TextAt - V2(4, GetLineAdvance(GameState->FontMap) - 3);
            TextBounds.Max += TextAt - V2(-4, GetLineAdvance(GameState->FontMap) - 3);

            b32 ItemSelected = false;
            if(IsInRectangle(TextBounds, MouseAt))
            {
                Menu->HotItem = MenuIndex;
                if(WasDown(Input->MouseButton_Left))
                {
                    ItemSelected = true;
                }
            }

            v4 Color = GameState->GameSettings.Colors.TextMenu;
            if(MenuIndex == Menu->HotItem)
            {
                if(WasDown(Input->Keyboard_Space))
                {
                    ItemSelected = true;
                }

                Color = GameState->GameSettings.Colors.TextMenuCurrent;
                PushRectangleOutline(RenderGroup, TextBounds, Color, 1.0f);
            }

            if(ItemSelected)
            {
                menu_item_id ItemID = (menu_item_id)MenuItem->ID;
                if(ItemID == MenuItem_Play)
                {
                    GameState->GameMode = GameMode_Playing;
                    ChangeGameSettings(GameState, (difficulty_type)Menu->DifficultyIndex, (grid_size_type)Menu->GridSizeIndex);
                    BeginNewGame(GameState);
                }
                else if(ItemID == MenuItem_Quit)
                {
                    Platform.RequestExit();
                }
                else if(ItemID == MenuItem_Options)
                {
                    GameState->GameMode = GameMode_Options;
                }
                else if(ItemID == MenuItem_Scores)
                {
                    GameState->GameMode = GameMode_Scores;
                }
            }
            
            PushText(RenderGroup, GameState->FontMap, TextAt, Color, Label);
            MenuTextAt.y += GetLineAdvance(GameState->FontMap);
        }

        MenuTextAt.y += 2.0f*GetLineAdvance(GameState->FontMap);
        v2 FormatTextShift = CenterTextAtPoint(GameState, MenuTextAt, "##CX.X,X.X,X.X,X.X##");
        v2 Offset = MenuTextAt - FormatTextShift;

        char SettingColorString[16] = {};
        snprintf(SettingColorString, ArrayCount(SettingColorString), "%01.1f,%01.1f,%01.1f,%01.1f",
                 GameState->GameSettings.Colors.TextMenuCurrent.r,
                 GameState->GameSettings.Colors.TextMenuCurrent.g,
                 GameState->GameSettings.Colors.TextMenuCurrent.b,
                 GameState->GameSettings.Colors.TextMenuCurrent.a);

        // TODO(rick): This is using the macro version of PushText rather than
        // the function. Fix this to avoid the double snprintf..
        char DifficultyHelpText[256] = {};
        snprintf(DifficultyHelpText, ArrayCount(DifficultyHelpText), "Difficulty: ##C%s##%s", SettingColorString, MenuDifficultyStringTable[Menu->DifficultyIndex]);
        PushText(RenderGroup, GameState->FontMap, CenterTextAtPoint(GameState, MenuTextAt, DifficultyHelpText) + Offset, GameState->GameSettings.Colors.TextMenu, DifficultyHelpText);
        {
            rect2 TextRect = GetTextBounds(GameState->FontMap, DifficultyHelpText);
            TextRect.Max.x *= 0.49f;
            TextRect.Min = CenterTextAtPoint(GameState, MenuTextAt, DifficultyHelpText) + Offset - V2(3, GetLineAdvance(GameState->FontMap) - 3);
            TextRect.Max += TextRect.Min;
            // PushRectangleOutline(RenderGroup, TextRect, GameState->GameSettings.Colors.TextDefault, 1.0f);
            if(BitmapButton(GameState, RenderGroup, TextRect.Min - V2(GameState->Bitmaps[BitmapID_Left].Width, -1), MouseAt, BitmapID_Left, V4(1, 1, 1, 1), V4(1, 0, 0, 1)) &&
               WasDown(Input->MouseButton_Left))
            {
                --Menu->DifficultyIndex;
                if(Menu->DifficultyIndex < 0)
                {
                    Menu->DifficultyIndex = DifficultyType_Count - 1;
                }
            }

            if(BitmapButton(GameState, RenderGroup, V2(TextRect.Max.x, TextRect.Min.y) , MouseAt, BitmapID_Right, V4(1, 1, 1, 1), V4(1, 0, 0, 1)) &&
               WasDown(Input->MouseButton_Left))
            {
                ++Menu->DifficultyIndex;
                if(Menu->DifficultyIndex >= DifficultyType_Count)
                {
                    Menu->DifficultyIndex = 0;
                }
            }
        }

        MenuTextAt.y += GetLineAdvance(GameState->FontMap);

        char GridSizeHelpText[256] = {};
        snprintf(GridSizeHelpText, ArrayCount(GridSizeHelpText), "Grid size: ##C%s##%s", SettingColorString, MenuSizeStringTable[Menu->GridSizeIndex]);
        PushText(RenderGroup, GameState->FontMap, CenterTextAtPoint(GameState, MenuTextAt, GridSizeHelpText) + Offset, GameState->GameSettings.Colors.TextMenu, GridSizeHelpText, "small");
        {
            rect2 TextRect = GetTextBounds(GameState->FontMap, GridSizeHelpText);
            TextRect.Max.x *= 0.49f;
            TextRect.Min = CenterTextAtPoint(GameState, MenuTextAt, GridSizeHelpText) + Offset - V2(3, GetLineAdvance(GameState->FontMap) - 3);
            TextRect.Max += TextRect.Min;
            // PushRectangleOutline(RenderGroup, TextRect, GameState->GameSettings.Colors.TextDefault, 1.0f);
            if(BitmapButton(GameState, RenderGroup, TextRect.Min - V2(GameState->Bitmaps[BitmapID_Left].Width + 5, -1), MouseAt, BitmapID_Left, V4(1, 1, 1, 1), V4(1, 0, 0, 1)) &&
               WasDown(Input->MouseButton_Left))
            {
                --Menu->GridSizeIndex;
                if(Menu->GridSizeIndex < 0)
                {
                    Menu->GridSizeIndex = GridSizeType_Count - 1;
                }
            }

            if(BitmapButton(GameState, RenderGroup, V2(TextRect.Max.x, TextRect.Min.y) , MouseAt, BitmapID_Right, V4(1, 1, 1, 1), V4(1, 0, 0, 1)) &&
               WasDown(Input->MouseButton_Left))
            {
                ++Menu->GridSizeIndex;
                if(Menu->GridSizeIndex >= GridSizeType_Count)
                {
                    Menu->GridSizeIndex = 0;
                }
            }
        }

        //rect2 ClipRect = {0.5f*V2(0, 0), 0.5f*V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height)};
        //RenderGroupToOutput(RenderGroup, DrawBuffer, ClipRect);
        TiledRenderGroupToOutput(RenderGroup, DrawBuffer);

        EndTemporaryMemory(RenderMemory);
        CheckArena(&TransState->TransArena);
    }
    else if(GameState->GameMode == GameMode_Options)
    {
        options_state *OptionsState = &GameState->OptionsState;
        if(!OptionsState->IsInitialized)
        {
            OptionsState->Themes = Platform.EnumerateFiles("*.thm");
            OptionsState->HotItemIndex = 0;
            OptionsState->IsInitialized = true;
        }

        temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransState->TransArena, Kilobytes(64));
        PushClear(RenderGroup, GameState->GameSettings.Colors.None);

        v2 MouseAt = {(f32)Input->MouseX, (f32)Input->MouseY};

        s32 TotalItemIndex = 0;
        v2 LineAdvance = {0.0f, (f32)GetLineAdvance(GameState->FontMap)};
        v2 TextAt = {0.5f*DrawBuffer->Width, 64.0f};
        for(platform_file_enumeration_result *ThemeGroup = OptionsState->Themes;
            ThemeGroup;
            ThemeGroup = ThemeGroup->Next)
        {
            for(u32 ItemIndex = 0; ItemIndex < ThemeGroup->Used; ++ItemIndex)
            {
                v2 TextOutAt = CenterTextAtPoint(GameState, TextAt, ThemeGroup->Items[ItemIndex]);
                rect2 TextRect = GetTextBounds(GameState->FontMap, ThemeGroup->Items[ItemIndex]);
                TextRect.Min += TextOutAt - LineAdvance + V2(0, 2);
                TextRect.Max += TextOutAt - LineAdvance + V2(0, 2);

                v4 Color = GameState->GameSettings.Colors.TextMenu;
                b32 ShouldLoadTheme = false;
                if(IsInRectangle(TextRect, MouseAt))
                {
                    OptionsState->HotItemIndex = TotalItemIndex;

                    Color = GameState->GameSettings.Colors.TextMenuCurrent;
                    ShouldLoadTheme = WasDown(Input->MouseButton_Left);
                }
                else if(TotalItemIndex == OptionsState->HotItemIndex)
                {
                    Color = GameState->GameSettings.Colors.TextMenuCurrent;
                    ShouldLoadTheme = WasDown(Input->Keyboard_Space);
                }

                if(ShouldLoadTheme)
                {
                    color_settings NewColorTheme = {};
                    if(LoadColorSettingsFromFile(ThemeGroup->Items[ItemIndex], &NewColorTheme))
                    {
                        GameState->GameSettings.Colors = NewColorTheme;
                    }
                }
                //PushRectangleOutline(RenderGroup, TextRect, V4(1, 1, 1, 1));
                PushText(RenderGroup, GameState->FontMap, TextOutAt, Color, ThemeGroup->Items[ItemIndex]);

                TextAt += LineAdvance;
                ++TotalItemIndex;
            }
        }

        if(WasDown(Input->Keyboard_W))
        {
            --OptionsState->HotItemIndex;
            if(OptionsState->HotItemIndex < 0)
            {
                OptionsState->HotItemIndex = TotalItemIndex - 1;
            }
        }
        else if(WasDown(Input->Keyboard_S))
        {
            ++OptionsState->HotItemIndex;
            if(OptionsState->HotItemIndex > (TotalItemIndex - 1))
            {
                OptionsState->HotItemIndex = 0;
            }
        }

        if((BitmapButton(GameState, RenderGroup, V2(4, 4), MouseAt, BitmapID_Back, V4(1, 1, 1, 1), V4(1, 0, 0, 1)) &&
           WasDown(Input->MouseButton_Left)) ||
           WasDown(Input->Keyboard_Escape))
        {
            GameState->GameMode = GameMode_Menu;
            Platform.FreeEnumeratedFileResult(OptionsState->Themes);
            OptionsState->IsInitialized = false;
        }

        //rect2 ClipRect = {V2(0, 0), V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height)};
        //RenderGroupToOutput(RenderGroup, DrawBuffer, ClipRect);
        TiledRenderGroupToOutput(RenderGroup, DrawBuffer);
        EndTemporaryMemory(RenderMemory);
        CheckArena(&TransState->TransArena);
    }
    else if(GameState->GameMode == GameMode_Scores)
    {
        scores_state *ScoresState = &GameState->ScoresState;
        if(!ScoresState->IsInitialized)
        {
            ScoresState->ScoresFile = Platform.ReadFile("Scores.dat");
            if(ScoresState->ScoresFile.Contents)
            {
                ScoresState->ScoreData = (saved_score_data *)ScoresState->ScoresFile.Contents;
                // TODO(rick): Better sort?
                saved_score_data *ScoresBase = ScoresState->ScoreData;
                saved_score_data *OnePastLast = (saved_score_data *)((u8 *)ScoresBase + ScoresState->ScoresFile.FileSize);
                s32 ScoresCount = (OnePastLast - ScoresBase);
                for(s32 i = 0; i < (ScoresCount - 1); ++i)
                {
                    saved_score_data *ScoreA = ScoresBase + i;
                    for(s32 j = i + 1; j < ScoresCount; ++j)
                    {
                        saved_score_data *ScoreB = ScoresBase + j;
                        if(ScoreB->Time < ScoreA->Time)
                        {
                            saved_score_data Temp = *ScoreA;
                            *ScoreA = *ScoreB;
                            *ScoreB = Temp;
                        }
                    }
                }

                ScoresState->ItemsPerPage = 10;
                ScoresState->CurrentPage = 0;
                ScoresState->TotalPages = Ceil((f32)ScoresCount / (f32)ScoresState->ItemsPerPage);
                ScoresState->TotalItems = ScoresCount;
            }

            ScoresState->IsInitialized = true;
        }

        v2 MouseAt = {(f32)Input->MouseX, (f32)Input->MouseY};

        temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransState->TransArena, Kilobytes(64));
        PushClear(RenderGroup, GameState->GameSettings.Colors.None);

        v2 PageTextAt = {0.5f*DrawBuffer->Width, 70.0f};
        char PaginationBuffer[64] = {};
        snprintf(PaginationBuffer, ArrayCount(PaginationBuffer), "Page %d of %d",
                 ScoresState->CurrentPage + 1, ScoresState->TotalPages);
        PushText(RenderGroup, GameState->FontMap, CenterTextAtPoint(GameState, PageTextAt, PaginationBuffer),
                 GameState->GameSettings.Colors.TextDefault, PaginationBuffer);
        if(BitmapButton(GameState, RenderGroup, PageTextAt - V2(130.0f, 16.0f), MouseAt, BitmapID_Left,
                        GameState->GameSettings.Colors.TextDefault, GameState->GameSettings.Colors.TextMenuCurrent) &&
           WasDown(Input->MouseButton_Left))
        {
            ScoresState->CurrentPage = MAX(ScoresState->CurrentPage - 1, 0);
        }
        if(BitmapButton(GameState, RenderGroup, PageTextAt + V2(110.0f, -16.0f), MouseAt, BitmapID_Right,
                        GameState->GameSettings.Colors.TextDefault, GameState->GameSettings.Colors.TextMenuCurrent) &&
                WasDown(Input->MouseButton_Left))
        {
            ScoresState->CurrentPage = MIN(ScoresState->CurrentPage + 1, ScoresState->TotalPages - 1);
        }

        v2 TextAt = {5.0f, 100.0f};
        PushText(RenderGroup, GameState->FontMap, TextAt, GameState->GameSettings.Colors.TextDefault, 
                 "Difficulty Size    Date           Time");
        TextAt.y += GetLineAdvance(GameState->FontMap) + 5;
        saved_score_data *Scores = (saved_score_data *)ScoresState->ScoreData;

        for(s32 ScoreIndex = 0;
            ((ScoreIndex < ScoresState->ItemsPerPage) &&
             ((ScoresState->CurrentPage * ScoresState->ItemsPerPage) + ScoreIndex) < ScoresState->TotalItems);
            ++ScoreIndex)
        {
            saved_score_data *Score = Scores + (ScoresState->CurrentPage * ScoresState->ItemsPerPage) + ScoreIndex;
            
            PushText(RenderGroup, GameState->FontMap, TextAt, GameState->GameSettings.Colors.TextDefault, 
                     "%-11s%-8s%04d-%02d-%02d %10.3fs",
                     MenuDifficultyStringTable[Score->Difficulty],
                     MenuSizeStringTable[Score->GridSize],
                     Score->Year, Score->Month, Score->Day, Score->Time);

            TextAt.y += GetLineAdvance(GameState->FontMap);
        }

        if((BitmapButton(GameState, RenderGroup, V2(4, 4), MouseAt, BitmapID_Back, 
                         GameState->GameSettings.Colors.TextDefault, GameState->GameSettings.Colors.TextMenuCurrent) &&
           WasDown(Input->MouseButton_Left)) ||
           WasDown(Input->Keyboard_Escape))
        {
            GameState->GameMode = GameMode_Menu;
            ScoresState->IsInitialized = false;
            Platform.FreeMemory(ScoresState->ScoresFile.Contents);
        }

        //rect2 ClipRect = {V2(0, 0), V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height)};
        //RenderGroupToOutput(RenderGroup, DrawBuffer, ClipRect);
        TiledRenderGroupToOutput(RenderGroup, DrawBuffer);
        EndTemporaryMemory(RenderMemory);
        CheckArena(&TransState->TransArena);
    }
    else if(GameState->GameMode == GameMode_Playing || GameState->GameMode == GameMode_End)
    {
        if(WasDown(Input->Keyboard_Escape))
        {
            GameState->GameMode = GameMode_Menu;
        }

        if(WasDown(Input->Keyboard_N))
        {
            BeginNewGame(GameState);
        }

        if(GameState->GameMode == GameMode_End)
        {
            if(WasDown(Input->Keyboard_Space) ||
               WasDown(Input->Keyboard_Return))
            {
                BeginNewGame(GameState);
            }
        }

        v2 WindowCenterPoint = 0.5f*V2(DrawBuffer->Width, DrawBuffer->Height);
        rect2 GameGridRect = Rect2(V2(0, 0), GameState->CellSize*V2(GameState->GridWidth, GameState->GridHeight) + GameState->CellSpacing*V2(GameState->GridWidth, GameState->GridHeight));
        v2 GameGridCenterPoint = 0.5f*V2(GameGridRect.Max.x - GameGridRect.Min.x, GameGridRect.Max.y - GameGridRect.Min.y);
        v2 GameGridOffset = WindowCenterPoint - GameGridCenterPoint;
        GameGridOffset.x = Clamp(0.0f, GameGridOffset.x, (f32)DrawBuffer->Width);
        GameGridOffset.y = Clamp(0.0f, GameGridOffset.y, (f32)DrawBuffer->Height);

        v2 MouseAt = {(f32)Input->MouseX, (f32)Input->MouseY};
        // NOTE(rick): Game logic
        {
            u32 CorrectlyPlacedFlags = 0;
            cell *GridCell = GameState->GameGrid;
            for(u32 Y = 0; Y < GameState->GridHeight; ++Y)
            {
                for(u32 X = 0; X < GameState->GridWidth; ++X)
                {
                    if(GameState->GameMode == GameMode_Playing)
                    {
                        v2 MinPoint = GameGridOffset + GameState->CellSize*V2(X, Y) + GameState->CellSpacing*V2(X, Y);
                        v2 MaxPoint = MinPoint + V2(GameState->CellSize, GameState->CellSize);
                        rect2 CellRectangle = Rect2(MinPoint, MaxPoint);

                        if(IsInRectangle(CellRectangle, MouseAt))
                        {
                            if(!GameState->TimerActive &&
                               (WasDown(Input->MouseButton_Left) || 
                                WasDown(Input->MouseButton_Right)))
                            {
                                GameState->TimerActive = true;
                                GameState->GameBeginMSCount = GameState->GameCurrentMSCount = Platform.GetMSElapsed64();
                            }

                            if(WasDown(Input->MouseButton_Left))
                            {
                                if(!GridCell->IsFlagged)
                                {
                                    if(GridCell->IsBomb)
                                    {
                                        // TODO(rick): Game over
                                        GameState->GameMode = GameMode_End;
                                        GameState->TimerActive = false;
                                        GameState->EndState = EndState_Lose;
                                    }
                                    else if(!GridCell->IsRevealed)
                                    {
                                        // GridCell->IsRevealed = true;
                                        // ++GameState->RevealedCount;
                                        RecursiveFloodFillReveal(GameState, X, Y);
                                    }
                                }
                            }
                            else if(WasDown(Input->MouseButton_Right) && !GridCell->IsRevealed)
                            {
                                b32 NewState = !GridCell->IsFlagged;
                                GridCell->IsFlagged = NewState;
                                GameState->FlagCount += (NewState ? -1 : 1);
                            }
                        }
                    }

                    if(GridCell->IsBomb && GridCell->IsFlagged)
                    {
                        ++CorrectlyPlacedFlags;
                    }

                    if(GameState->GameMode == GameMode_End)
                    {
                        if(!GridCell->IsBomb)
                        {
                            GridCell->IsRevealed = true;
                        }
                    }

                    ++GridCell;
                }
            }

            if((GameState->FlagCount == 0) &&
               (GameState->BombCount == CorrectlyPlacedFlags) &&
               (GameState->GameMode == GameMode_Playing))
            {
                // TODO(rick): Replace snprintf
                f32 SecondsTaken = (f64)(GameState->GameCurrentMSCount - GameState->GameBeginMSCount) / 1000.0f;
                platform_datetime_result DateTime = Platform.GetDateTime();
#if 0
                char TempBuffer[256] = {};
                s32 Size = snprintf(TempBuffer, ArrayCount(TempBuffer),
                                    "%4d-%02d-%02d %02d:%02d:%02d - %010.03f\n",
                                    DateTime.Year, DateTime.Month, DateTime.Day,
                                    DateTime.Hour, DateTime.Minute, DateTime.Second,
                                    SecondsTaken);
#endif
                saved_score_data Data = {};
                Data.Difficulty = GameState->MainMenuState.DifficultyIndex;
                Data.GridSize = GameState->MainMenuState.GridSizeIndex;
                Data.Time = SecondsTaken;
                Data.Year = DateTime.Year;
                Data.Month = DateTime.Month;
                Data.Day = DateTime.Day;
                u32 BytesWritten = 0;
                Platform.WriteFile("Scores.dat", &Data, sizeof(Data), &BytesWritten);
                Assert(BytesWritten == sizeof(Data));

                GameState->GameMode = GameMode_End;
                GameState->TimerActive = false;
                GameState->EndState = EndState_Win;
            }
        }

        temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransArena);
        render_group *RenderGroup = AllocateRenderGroup(&TransState->TransArena, Kilobytes(512));

        PushClear(RenderGroup, GameState->GameSettings.Colors.None);
        PushText(RenderGroup, GameState->FontMap, V2(20, 40), V4(1, 1, 1, 1), "%3.0f fps", Platform.FPS);
#if 0
        for(u32 BitmapID = 0; BitmapID < BitmapID_Count; ++BitmapID)
        {
            PushBitmap(RenderGroup, &GameState->Bitmaps[BitmapID], V2(BitmapID * 20, 10), V4(1, 0, 1, 1));
        }
#endif

        f32 GameGridOutlineThickness = 1.0f;
        v2 GameGridOutlineSize = {GameGridOutlineThickness, GameGridOutlineThickness};
        rect2 GameGridDim = Rect2(GameGridOffset + V2(0, 0) - GameGridOutlineSize,
                                  GameGridOffset + GameState->CellSize*V2(GameState->GridWidth, GameState->GridHeight) +
                                  GameGridOutlineSize + GameState->CellSpacing*V2(GameState->GridWidth-1, GameState->GridHeight-1));
        PushRectangleOutline(RenderGroup, GameGridDim, GameState->GameSettings.Colors.CellBorder, GameGridOutlineThickness);

        f32 BombColorBucket = (255.0f / 9.0f);
        cell *CellToDraw = GameState->GameGrid;
        for(u32 Y = 0; Y < GameState->GridHeight; ++Y)
        {
            for(u32 X = 0; X < GameState->GridWidth; ++X)
            {
                v2 MinPoint = GameGridOffset + GameState->CellSize*V2(X, Y) + GameState->CellSpacing*V2(X, Y);
                v2 MaxPoint = MinPoint + V2(GameState->CellSize, GameState->CellSize);
                rect2 CellRectangle = Rect2(MinPoint, MaxPoint);

                bitmap_asset_id BitmapID = BitmapID_None;
                v4 Color = GameState->GameSettings.Colors.CellHidden;
                if(CellToDraw->IsRevealed)
                {
                    Color = GameState->GameSettings.Colors.CellRevealed;
                }
                else if(CellToDraw->IsBomb && (GameState->GameMode == GameMode_End))
                {
                    Color = GameState->GameSettings.Colors.CellBomb;
                    BitmapID = BitmapID_Light;
                }
                else if(CellToDraw->IsFlagged)
                {
                    BitmapID = BitmapID_Flag;
                    Color = GameState->GameSettings.Colors.CellFlagged;
                }

                PushRectangle(RenderGroup, CellRectangle, Color);
                PushRectangleOutline(RenderGroup, CellRectangle, GameState->GameSettings.Colors.CellBorder);

                if(BitmapID != BitmapID_None)
                {
                    v2 BitmapOffset = 0.4f*V2(GameState->Bitmaps[BitmapID].Width, GameState->Bitmaps[BitmapID].Height);
                    v2 BitmapP = (MinPoint) + BitmapOffset;
                    PushBitmap(RenderGroup, &GameState->Bitmaps[BitmapID], BitmapP, V4(0, 0, 0, 1.0f));
                }

                if(!CellToDraw->IsBomb && CellToDraw->NeighboringBombCount && CellToDraw->IsRevealed)
                {
                    rect2 CellTextBounds = GetTextBounds(GameState->FontMap, NumberStringTable[CellToDraw->NeighboringBombCount]);
                    s32 AlignX = 0;
                    s32 AlignY = 0;
                    GetAlignmentForForCodePoint(GameState->FontMap, NumberStringTable[CellToDraw->NeighboringBombCount][0], &AlignX, &AlignY);
                    v2 TextP = V2(CellRectangle.Min.x, CellRectangle.Max.y) + 0.5f*V2(GameState->CellSize, -GameState->CellSize) +
                        0.5f*V2(-(CellTextBounds.Max.x - CellTextBounds.Min.x), (CellTextBounds.Max.y - CellTextBounds.Min.y) - 6);
                    PushText(RenderGroup, GameState->FontMap, TextP+V2(1, 1), GameState->GameSettings.Colors.TextShadow, NumberStringTable[CellToDraw->NeighboringBombCount]);
                    PushText(RenderGroup, GameState->FontMap, TextP, GameState->GameSettings.Colors.TextCell, NumberStringTable[CellToDraw->NeighboringBombCount]);
                }

#if 1
                if(CellToDraw->IsFlagged)
                {
                    PushRectangleOutline(RenderGroup, CellRectangle, GameState->GameSettings.Colors.CellFlagged, 0.15f*GameState->CellSize);
                }
#endif

                if(IsInRectangle(CellRectangle, V2(Input->MouseX, Input->MouseY)))
                {
                    PushRectangleOutline(RenderGroup, CellRectangle, GameState->GameSettings.Colors.CellFocused);
                }

                ++CellToDraw;
            }
        }

        {
            if((GameState->GameMode == GameMode_Playing) && GameState->TimerActive)
            {
                GameState->GameCurrentMSCount = Platform.GetMSElapsed64();
            }
            f32 SecondsTaken = (f64)(GameState->GameCurrentMSCount - GameState->GameBeginMSCount) / 1000.0f;
            v2 TimerP = GameGridOffset + V2(0, -2);
            bitmap *ClockBitmap = &GameState->Bitmaps[BitmapID_Clock];
            PushBitmap(RenderGroup, ClockBitmap, TimerP - V2(0, ClockBitmap->Height), GameState->GameSettings.Colors.TextHUDTime);
            TimerP += V2(ClockBitmap->Width + 4, 0);
            PushText(RenderGroup, GameState->FontMap, TimerP, GameState->GameSettings.Colors.TextShadow, "%.02fs", SecondsTaken);
            PushText(RenderGroup, GameState->FontMap, TimerP-V2(1, 1), GameState->GameSettings.Colors.TextHUDTime, "%.02fs", SecondsTaken);

            char FlagsCountTextBuffer[16] = {};
            snprintf(FlagsCountTextBuffer, ArrayCount(FlagsCountTextBuffer), "%d", GameState->FlagCount);
            rect2 FlagTextDims = GetTextBounds(GameState->FontMap, FlagsCountTextBuffer);
            bitmap *FlagBitmap = &GameState->Bitmaps[BitmapID_Flag];
            v2 FlagTextP = GameGridOffset + V2(0, -2) + V2(GameGridRect.Max.x, 0) - V2(FlagBitmap->Width, 0)/*- V2(FlagTextDims.Max.x - FlagTextDims.Min.x, 0)*/;
            PushBitmap(RenderGroup, FlagBitmap, FlagTextP - V2(0, FlagBitmap->Height), GameState->GameSettings.Colors.TextHUDFlags);
            FlagTextP -= V2(FlagTextDims.Max.x - FlagTextDims.Min.x, 0);
            PushText(RenderGroup, GameState->FontMap, FlagTextP, GameState->GameSettings.Colors.TextShadow, FlagsCountTextBuffer);
            PushText(RenderGroup, GameState->FontMap, FlagTextP-V2(1, 1), GameState->GameSettings.Colors.TextHUDFlags, FlagsCountTextBuffer);
        }

        if((GameState->GameMode == GameMode_End) &&
           (GameState->EndState != EndState_None))
        {
            v2 EndStateP = 0.5f*V2(DrawBuffer->Width, DrawBuffer->Height);
            char *EndStateText = 0;
            v4 Color = GameState->GameSettings.Colors.None;
            if(GameState->EndState == EndState_Win)
            {
                EndStateText = "WIN";
                Color = GameState->GameSettings.Colors.TextWin;
            }
            else if(GameState->EndState == EndState_Lose)
            {
                EndStateText = "GAME OVER";
                Color = GameState->GameSettings.Colors.TextLose;
            }

            rect2 EndStateTextRect = GetTextBounds(GameState->FontMap, EndStateText);
            EndStateP -= V2(0.5f*(EndStateTextRect.Max.x - EndStateTextRect.Min.x),
                            -0.5f*(EndStateTextRect.Max.y - EndStateTextRect.Min.y));

            rect2 BannerRect = {};
            BannerRect.Min = V2(0, DrawBuffer->Height*0.5f - 14);
            BannerRect.Max = V2(DrawBuffer->Width, DrawBuffer->Height*0.5f + 14);
            PushRectangle(RenderGroup, BannerRect, GameState->GameSettings.Colors.OverlayBackground); 
            PushRectangleOutline(RenderGroup, BannerRect, GameState->GameSettings.Colors.OverlayBorder); 

            PushText(RenderGroup, GameState->FontMap, EndStateP, GameState->GameSettings.Colors.TextShadow, "%s", EndStateText);
            PushText(RenderGroup, GameState->FontMap, EndStateP-V2(1, 1), Color, "%s", EndStateText);
        }

        v2 HomeP = GameGridOffset + GameGridRect.Max - 0.5f*V2(GameGridRect.Max.x - GameGridRect.Min.x, 0) + V2(0, 6);
        if(BitmapButton(GameState, RenderGroup, HomeP, MouseAt, BitmapID_Home, V4(1, 1, 1, 1), V4(1, 0, 0, 1)) &&
           WasDown(Input->MouseButton_Left))
        {
            GameState->GameMode = GameMode_Menu;
        }

#if 0
        f32 VertMinX = DrawBuffer->Width*0.5f - 1;
        f32 VertMaxX = VertMinX + 2;
        f32 HorzMinY = DrawBuffer->Height*0.5f - 1;
        f32 HorzMaxY = HorzMinY + 2;
        PushRectangle(RenderGroup, V2(VertMinX, 0), V2(VertMaxX, DrawBuffer->Height), V4(1, 1, 1, 1));
        PushRectangle(RenderGroup, V2(0, HorzMinY), V2(DrawBuffer->Width, HorzMaxY), V4(1, 1, 1, 1));
#endif

        //rect2 ClipRect = {V2(0, 0), V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height)};
        //RenderGroupToOutput(RenderGroup, DrawBuffer, ClipRect);
        TiledRenderGroupToOutput(RenderGroup, DrawBuffer);

        EndTemporaryMemory(RenderMemory);
        CheckArena(&TransState->TransArena);
    }
}

#define FONT_LIB_IMPLEMENTATION
#include "font_lib.h"
