#ifndef MINESWEEPER_H

#include <intrin.h>

#include "minesweeper_platform.h"
#include "minesweeper_memory.h"
#include "minesweeper_intrinsics.h"
#include "minesweeper_math.h"
#include "minesweeper_render_group.h"
#include "font_lib.h"

#include <stdio.h>

struct cell
{
    // TODO(rick) Compact this stuff down to a single byte?
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

struct game_state
{
    memory_arena GameArena;

    u32 GridWidth;
    u32 GridHeight;
    cell *GameGrid;

    f32 CellSize;
    f32 CellSpacing;
    f32 BombFrequency;

    s32 BombCount;
    s32 FlagCount;
    s32 RevealedCount;

    u32 PRNGState;
    u64 GameBeginMSCount;

    font_map *FontMap;

    bitmap TestBitmap;

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

#define MINESWEEPER_H
#endif
