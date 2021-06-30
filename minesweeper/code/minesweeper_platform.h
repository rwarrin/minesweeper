#ifndef MINESWEEPER_PLATFORM

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;
typedef int32_t b32;
typedef int16_t b16;
typedef int8_t b8;
typedef size_t mem_size;
typedef intptr_t smm;
typedef uintptr_t umm;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Assert(Condition) do { if(!(Condition)) { *(int *)0 = 0; } }while(0);

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define MAX(A, B) ((A) > (B) ? A : B)
#define MIN(A, B) ((A) < (B) ? A : B)

struct bitmap_file
{
    struct
    {
        u16 Signature;
        u32 FileSize;
        u16 Reserved0;
        u16 Reserved1;
        u32 DataOffset;
    } FileHeader;
    struct
    {
        u32 Size;
        s32 Width;
        s32 Height;
        u16 Planes;
        u16 BitsPerPixel;
        u32 Compression;
        u32 ImageSize;
        s32 XPelsPerMeter;
        s32 YPelsPerMeter;
        u32 ColorsUsed;
        u32 ColorsImportant;
    } InfoHeader;
    u8 *Data;
};

struct bitmap
{
    u32 Width;
    u32 Height;
    u32 Pitch;
    u8 *Data;
};

struct button_state
{
    s32 TransitionCount;
    b32 IsDown;
};

struct input
{
    f32 dt;

    s32 MouseX;
    s32 MouseY;
    s32 MouseZ;

    union
    {
        button_state Mouse[6];
        struct
        {
            button_state MouseButton_Left;
            button_state MouseButton_Right;
            button_state MouseButton__Unused;
            button_state MouseButton_Middle;
            button_state MouseButton_XButton1;
            button_state MouseButton_XButton2;
        };
    };

    union
    {
        button_state Keyboard[5];
        struct
        {
            button_state Keyboard_A;
            button_state Keyboard_D;
            button_state Keyboard_S;
            button_state Keyboard_W;

            button_state Keyboard_DoNotUse;
        };
    };
};

struct app_memory
{
    u32 PermanentStorageSize;
    void *PermanentStorage;

    u32 TransientStorageSize;
    void *TransientStorage;

    u32 FrameStorageSize;
    void *FrameStorage;
};

#define WasDown(Key) ((Key.IsDown == 0) && (Key.TransitionCount != 0))
#define IsDown(Key) (Key.IsDown)

#define UPDATE_AND_RENDER(name) void name(app_memory *AppMemory, input *Input, bitmap *DrawBuffer)
typedef UPDATE_AND_RENDER(update_and_render);

#define MINESWEEPER_PLATFORM
#endif
