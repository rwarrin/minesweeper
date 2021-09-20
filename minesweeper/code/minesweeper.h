#ifndef MINESWEEPER_H

struct cell
{
    u8 NeighboringBombCount;
    b8 IsRevealed;
    b8 IsFlagged;
    b8 IsBomb;
};

struct transient_state
{
    memory_arena TransArena;

    b32 IsInitialized;
};

enum game_mode
{
    GameMode_Menu,
    GameMode_Options,
    GameMode_Scores,
    GameMode_Playing,
    GameMode_End,
};

enum end_state
{
    EndState_None,
    EndState_Win,
    EndState_Lose,
};

enum menu_item_id
{
    MenuItem_None,

    MenuItem_Play,
    MenuItem_Scores,
    MenuItem_Options,
    MenuItem_Quit,

    MenuItem_Count,
};
static char *MenuItemStringTable[] =
{
    "",

    "Play",
    "Scores",
    "Options",
    "Quit",

    "",
};
enum difficulty_type
{
    DifficultyType_Easy,
    DifficultyType_Normal,
    DifficultyType_Hard,

    DifficultyType_Count,
};
static char *MenuDifficultyStringTable[] =
{
    "Easy  ",
    "Normal",
    "Hard  ",
};
enum grid_size_type
{
    GridSizeType_Small,
    GridSizeType_Medium,
    GridSizeType_Large,

    GridSizeType_Count,
};
static char *MenuSizeStringTable[] =
{
    "Small ",
    "Medium",
    "Large ",
};
struct menu_item
{
    menu_item_id ID;
};
struct menu_state
{
    b32 Initialized;
    f32 dt;
    s32 HotItem;

    s32 DifficultyIndex;
    s32 DifficultyCount;

    s32 GridSizeIndex;
    s32 GridSizeCount;

    u32 ItemCount;
    menu_item Items[8];
};

enum color_id
{
    ColorID_None,

    ColorID_Background,

    ColorID_TextDefault,
    ColorID_TextShadow,
    ColorID_TextMenu,
    ColorID_TextMenuCurrent,
    ColorID_TextMenuSetting,
    ColorID_TextHUDTime,
    ColorID_TextHUDFlags,
    ColorID_TextCell,
    ColorID_TextWin,
    ColorID_TextLose,

    ColorID_CellHidden,
    ColorID_CellRevealed,
    ColorID_CellFlagged,
    ColorID_CellBomb,
    ColorID_CellBorder,
    ColorID_CellFocused,

    ColorID_OverlayBackground,
    ColorID_OverlayBorder,

    ColorID_Count,
};

union color_settings
{
    struct {
        v4 None;
        v4 Background;
        v4 TextDefault;
        v4 TextShadow;
        v4 TextMenu;
        v4 TextMenuCurrent;
        v4 TextMenuSetting;
        v4 TextHUDTime;
        v4 TextHUDFlags;
        v4 TextCell;
        v4 TextWin;
        v4 TextLose;
        v4 CellHidden;
        v4 CellRevealed;
        v4 CellFlagged;
        v4 CellBomb;
        v4 CellBorder;
        v4 CellFocused;
        v4 OverlayBackground;
        v4 OverlayBorder;
    };
    v4 Colors[ColorID_Count];
};

struct options_state
{
    platform_file_enumeration_result *Themes;
    s32 HotItemIndex;
    b32 IsInitialized;
};

#pragma pack(push, 1)
struct saved_score_data
{
    s32 Difficulty;
    s32 GridSize;
    f32 Time;
    s32 Year;
    s32 Month;
    s32 Day;
};
#pragma pack(pop)

struct scores_list
{
    u32 Used;
    saved_score_data Scores[16];
    scores_list *Next;
    scores_list *Prev;
};
struct scores_state
{
    file_result ScoresFile;
    saved_score_data *ScoreData;
    s32 ItemsPerPage;
    s32 CurrentPage;
    s32 TotalPages;
    s32 TotalItems;
    b32 IsInitialized;
};

struct game_state
{
    memory_arena GameArena;

    u32 PRNGState;

    game_mode GameMode;
    end_state EndState;
    b32 TimerActive;
    u64 GameBeginMSCount;
    u64 GameCurrentMSCount;

    u32 MaxGridWidth;
    u32 MaxGridHeight;
    u32 GridWidth;
    u32 GridHeight;
    cell *GameGrid;

    f32 CellSize;
    f32 CellSpacing;
    f32 BombFrequency;

    s32 BombCount;
    s32 FlagCount;
    s32 RevealedCount;

    font_map *Fonts[FontID_Count];

    bitmap Bitmaps[BitmapID_Count];

    color_settings Colors;
    menu_state MainMenuState;
    options_state OptionsState;
    scores_state ScoresState;

    b32 IsInitialized;
};

inline u32
GetNextRandomNumber(u32 *State)
{
    u32 NewState = *State;
    NewState ^= NewState << 13;
    NewState ^= NewState >> 17;
    NewState ^= NewState << 5;
    *State = NewState;
    return(NewState);
}

static char *NumberStringTable[] =
{
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
};

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

#define MINESWEEPER_H
#endif
