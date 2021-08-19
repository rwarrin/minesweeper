#ifndef MINESWEERPER_MEMORY_H

struct memory_arena
{
    u8 *Base;
    mem_size Used;
    mem_size Size;

    s32 TempCount;
};

struct temporary_memory
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

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result = {};

    ++Arena->TempCount;
    Result.Arena = Arena;
    Result.Used = Arena->Used;

    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempArena)
{
    memory_arena *Arena = TempArena.Arena;
    Assert(Arena->Used >= TempArena.Used);
    Arena->Used = TempArena.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void
Copy(void *Dest, void *Src, u32 Size)
{
    u8 *DestPtr = (u8 *)Dest;
    u8 *SrcPtr = (u8 *)Src;
    while(Size--)
    {
        *DestPtr++ = *SrcPtr++;
    }
}

inline void
ZeroMemory(void *Mem, u32 Size)
{
    u8 *Ptr = (u8 *)Mem;
    while(Size--)
    {
        *Ptr++ = 0;
    }
}

inline char
ScanTo(char **Stream, char Target)
{
    char *At = *Stream;
    while((At[0] != 0) && At[0] != Target)
    {
        ++At;
    }

    *Stream = At;
    return(*At);
}

inline s32
StringCompare(char *This, char *To)
{
    while(*This == *To)
    {
        if(*This == 0)
        {
            return 0;
        }

        ++This, ++To;
    }

    return(*This - *To);
}

#define MINESWEEPER_MEMORY_H
#endif
