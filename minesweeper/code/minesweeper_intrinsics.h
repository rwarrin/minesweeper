#ifndef MINESWEEPER_INTRINSICS_H

inline u32
BitScanForward(u32 Value)
{
    u32 Result = 0;

    for(u32 Index = 0; Index < 32; ++Index)
    {
        if((Value >> Index) & 0x1)
        {
            Result = Index;
            break;
        }
    }

    return(Result);
}

inline u64
RDTSC()
{
    u64 Result = __rdtsc();
    return(Result);
}

inline u64
ReadCPUFrequency()
{
    u64 Result = *(u64 *)0x7ffe0300;
    return(Result);
}

#define MINESWEEPER_INTRINSICS_H
#endif
