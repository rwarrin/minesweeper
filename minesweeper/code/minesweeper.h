#ifndef MINESWEEPER_H

#include "minesweeper_platform.h"
#include "minesweeper_memory.h"
#include "minesweeper_math.h"

struct cell
{
    // TODO(rick) Compact this stuff down to a single byte?
    u8 NeighboringBombCount;
    b8 IsRevealed;
    b8 IsFlagged;
    b8 IsBomb;
};

struct game_state
{
    memory_arena GameArena;

    u32 GridWidth;
    u32 GridHeight;
    cell *GameGrid;

    f32 CellSize;
    f32 BombFrequency;

    s32 BombCount;
    s32 FlagCount;

    u32 PRNGState;

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

#define MINESWEEPER_H
#endif
