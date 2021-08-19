#ifndef MINESWEEPER_RENDER_GROUP

enum render_entry_type
{
    RenderEntryType_render_entry_clear,
    RenderEntryType_render_entry_rectangle,
    RenderEntryType_render_entry_rectangle_outline,
    RenderEntryType_render_entry_bitmap,
};

struct render_entry_header
{
    render_entry_type Type;
};

struct render_entry_clear
{
    v4 Color;
};

struct render_entry_rectangle
{
    v2 Min;
    v2 Max;
    v4 Color;
};

struct render_entry_rectangle_outline
{
    rect2 Rect;
    v4 Color;
    f32 Thickness;
};

struct render_entry_bitmap
{
    v2 P;
    bitmap Bitmap;
    v4 Color;
};

struct render_group
{
    u8 *PushBufferBase;
    u32 MaxPushBufferSize;
    u32 PushBufferSize;
};

struct tiled_render_work
{
    render_group *RenderGroup;
    bitmap *DrawBuffer;
    rect2 ClipRect;
};

void PushBitmap(render_group *RenderGroup, bitmap *Bitmap, v2 P, v4 Color);

#define MINESWEEPER_RENDER_GROUP
#endif
