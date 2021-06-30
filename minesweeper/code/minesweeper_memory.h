#ifndef MINESWEERPER_MEMORY_H

struct memory_arena
{
    u8 *Base;
    mem_size Used;
    mem_size Size;

    s32 TempCount;
};

struct temp_arena
{
    memory_arena *Arena;
    mem_size Used;
};

static void
InitializeArena(memory_arena *Arena, void *Base, u32 Size)
{
    Assert(Arena);
    Arena->Size = Size;
    Arena->Base = (u8 *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, count, type) (type *)PushSize_(Arena, sizeof(type)*(count))
#define PushSize(Arena, size) PushSize_(Arena, size)

inline void *
PushSize_(memory_arena *Arena, u32 Size)
{
    void *Result = 0;

    Assert(Arena->Used + Size <= Arena->Size);
    if(Arena->Used + Size < Arena->Size)
    {
        Result = Arena->Base + Arena->Used;
        Arena->Used += Size;
    }

    return(Result);
}

inline temp_arena
BeginTemporaryArena(memory_arena *Arena)
{
    temp_arena Result = {};

    ++Arena->TempCount;
    Result.Arena = Arena;
    Result.Used = Arena->Used;

    return(Result);
}

inline void
EndTemporaryMemory(temp_arena TempArena)
{
    memory_arena *Arena = TempArena.Arena;
    Assert(Arena->Used >= TempArena.Used);
    Arena->Used = TempArena.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

#define MINESWEEPER_MEMORY_H
#endif
