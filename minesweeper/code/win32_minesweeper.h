#ifndef WIN32_MINESWEEPER_H

#define DebugWriteLine(format, ...) { \
    char Buffer[256] = {}; \
    snprintf(Buffer, ArrayCount(Buffer), format, __VA_ARGS__); \
    OutputDebugStringA(Buffer); \
}

struct win32_window_dims
{
    u32 Width;
    u32 Height;
};

struct win32_back_buffer
{
    bitmap_file BitmapFile;
    bitmap Bitmap;
};

struct app_code
{
    HMODULE AppCodeDLL;
    FILETIME LastFileTime;

    update_and_render *UpdateAndRender;

    b32 IsValid;
};

#define WIN32_MINESWEEPER_H
#endif
