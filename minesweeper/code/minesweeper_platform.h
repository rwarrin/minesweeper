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
#define InvalidCodePath Assert(!"Invalid Code Path!");
#define InvalidDefaultCase default: { Assert(!"Invalid Default Path"); } break

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define MAX(A, B) ((A) > (B) ? A : B)
#define MIN(A, B) ((A) < (B) ? A : B)

#define WriteBarrier _WriteBarrier(); _mm_sfence()
#define ReadBarrier _ReadBarrier(); _mm_lfence()
#define ReadWriteBarrier _ReadWriteBarrier(); _mm_mfence()

#pragma pack(push, 1)
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

        u32 RedMask;
        u32 GreenMask;
        u32 BlueMask;
        u32 AlphaMask;
    } InfoHeader;
};
#pragma pack(pop)

struct color_mask
{
    u32 Mask;
    u32 ShiftAmount;
};
struct bitmap
{
    u32 Width;
    u32 Height;
    u32 Pitch;

    color_mask RedMask;
    color_mask GreenMask;
    color_mask BlueMask;
    color_mask AlphaMask;

    u8 *Data;
};

#define MAGIC_NUMBER(a, b, c, d) ( ((u8)(a) << 0) | ((u8)(b) << 8) | ((u8)(c) << 16) | ((u8)(d) << 24) )

struct asset_file
{
#define FILE_MAGIC_NUMBER MAGIC_NUMBER('m', 's', 'w', 'p')
    u32 MagicNumber;

    u32 BitmapCount;
    u32 BitmapsOffset; // NOTE(rick): Offset to an array of bitmap structs

    u32 FontCount;
    u32 FontsOffset; // NOTE(rick): Offset to an array of u32's
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
            union
            {
                button_state Keyboard_A;
                button_state Keyboard_ArrowLeft;
            };

            button_state Keyboard_D;
            button_state Keyboard_N;
            button_state Keyboard_S;
            button_state Keyboard_W;
            button_state Keyboard_X;
            button_state Keyboard_Z;

            button_state Keyboard_Escape;
            button_state Keyboard_Return;
            button_state Keyboard_Space;

            button_state Keyboard_DoNotUse;
        };
    };
};

struct file_result
{
    void *Contents;
    u32 FileSize;
};
#define PLATFORM_READ_FILE(name) file_result name(char* FileName)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_FREE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_FREE_MEMORY(platform_free_memory);

#define PLATFORM_GET_MS_ELAPSED64(name) u64 name(void)
typedef PLATFORM_GET_MS_ELAPSED64(platform_get_ms_elapsed64);

#define PLATFORM_WRITE_FILE(name) b32 name(char *FileName, void *Data, u32 DataLength, u32 *BytesWritten)
typedef PLATFORM_WRITE_FILE(platform_write_file);

struct platform_datetime_result
{
    s16 Year;
    s16 Month;
    s16 Day;
    s16 Hour;
    s16 Minute;
    s16 Second;
};
#define PLATFORM_GET_DATETIME(name) platform_datetime_result name(void)
typedef PLATFORM_GET_DATETIME(platform_get_datetime);

#define PLATFORM_REQUEST_EXIT(name) void name(void)
typedef PLATFORM_REQUEST_EXIT(platform_request_exit);

struct platform_file_enumeration_result
{
    u32 Used;
    char Items[16][32];
    platform_file_enumeration_result *Prev;
    platform_file_enumeration_result *Next;
};
#define PLATFORM_ENUMERATE_FILES(name) platform_file_enumeration_result * name(char *Pattern)
typedef PLATFORM_ENUMERATE_FILES(platform_enumerate_files);
#define PLATFORM_FREE_ENUMERATED_FILE_RESULT(name) void name(platform_file_enumeration_result *Data)
typedef PLATFORM_FREE_ENUMERATED_FILE_RESULT(platform_free_enumerated_file_result);

#define PLATFORM_RANDOM_NUMBER(name) u32 name(void)
typedef PLATFORM_RANDOM_NUMBER(platform_random_number);

struct platform_work_queue;
#define PLATFORM_WORK_CALLBACK(name) void name(void *Data)
typedef PLATFORM_WORK_CALLBACK(platform_work_callback);

#define PLATFORM_ADD_WORK_TO_QUEUE(name) void name(platform_work_queue *WorkQueue, platform_work_callback *Proc, void *Data)
typedef PLATFORM_ADD_WORK_TO_QUEUE(platform_add_work_to_queue);
#define PLATFORM_COMPLETE_ALL_WORK(name) void name(platform_work_queue *WorkQueue)
typedef PLATFORM_COMPLETE_ALL_WORK(platform_complete_all_work);

struct platform_api
{
    platform_read_file *ReadFile;
    platform_free_memory *FreeMemory;
    platform_get_ms_elapsed64 *GetMSElapsed64;
    platform_write_file *WriteFile;
    platform_get_datetime *GetDateTime;
    platform_request_exit *RequestExit;
    platform_enumerate_files *EnumerateFiles;
    platform_free_enumerated_file_result *FreeEnumeratedFileResult;
    platform_random_number *GenerateRandomNumber;
    platform_add_work_to_queue *AddWorkToQueue;
    platform_complete_all_work *CompleteAllWork;

    platform_work_queue *WorkQueue;

    f32 FPS;
    f32 FrameTimeActual;
    f32 FrameTimeTotal;
    f32 FrameTimeSlept;
};

#pragma pack(push, 1)
struct app_memory
{
    platform_api PlatformAPI;

    u32 PermanentStorageSize;
    void *PermanentStorage;

    u32 TransientStorageSize;
    void *TransientStorage;

    u32 FrameStorageSize;
    void *FrameStorage;
};
#pragma pack(pop)

#define WasDown(Key) ((Key.IsDown == 0) && (Key.TransitionCount != 0))
#define IsDown(Key) (Key.IsDown)

#define UPDATE_AND_RENDER(name) void name(app_memory *AppMemory, input *Input, bitmap *DrawBuffer)
typedef UPDATE_AND_RENDER(update_and_render);

#define MINESWEEPER_PLATFORM
#endif
