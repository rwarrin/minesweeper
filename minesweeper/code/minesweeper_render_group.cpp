static render_group *
AllocateRenderGroup(memory_arena *Arena, u32 MaxPushBufferSize)
{
    render_group *Result = PushStruct(Arena, render_group);
    Result->PushBufferBase = (u8 *)PushSize(Arena, MaxPushBufferSize);

    Result->PushBufferSize = 0;
    Result->MaxPushBufferSize = MaxPushBufferSize;

    return(Result);
}

#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderEntryType_##type)
static void *
PushRenderElement_(render_group *RenderGroup, u32 Size, render_entry_type Type)
{
    void *Result = 0;

    Size += sizeof(render_entry_header);

    if((RenderGroup->PushBufferSize + Size) < RenderGroup->MaxPushBufferSize)
    {
        render_entry_header *Header = (render_entry_header *)(RenderGroup->PushBufferBase + RenderGroup->PushBufferSize);
        Header->Type = Type;
        Result = (u8 *)Header + sizeof(*Header);
        RenderGroup->PushBufferSize += Size;
    }
    else
    {
        InvalidCodePath;
    }

    return(Result);
}

inline void
PushClear(render_group *RenderGroup, v4 Color)
{
    render_entry_clear *Entry = PushRenderElement(RenderGroup, render_entry_clear);
    if(Entry)
    {
        Entry->Color = Color;
    }
}

inline void
PushRectangle(render_group *RenderGroup, v2 Min, v2 Max, v4 Color)
{
    render_entry_rectangle *Entry = PushRenderElement(RenderGroup, render_entry_rectangle);
    if(Entry)
    {
        Entry->Min = Min;
        Entry->Max = Max;
        Entry->Color = Color;
    }
}

inline void
PushRectangle(render_group *RenderGroup, rect2 Rectangle, v4 Color)
{
    PushRectangle(RenderGroup, Rectangle.Min, Rectangle.Max, Color);
}

inline void
PushRectangleOutline(render_group *RenderGroup, rect2 Rectangle, v4 Color, f32 Thickness = 1.0f)
{
    // TODO(rick): Remove the DrawRectangleOutline function and turn this into
    // four PushRectangle calls that do the same math as the function.
    render_entry_rectangle_outline *Entry = PushRenderElement(RenderGroup, render_entry_rectangle_outline);
    if(Entry)
    {
        Entry->Rect = Rectangle;
        Entry->Color = Color;
        Entry->Thickness = Thickness;
    }
}

inline void
PushBitmap(render_group *RenderGroup, bitmap *Bitmap, v2 P, v4 Color = V4(1, 1, 1, 1))
{
    render_entry_bitmap *Entry = PushRenderElement(RenderGroup, render_entry_bitmap);
    if(Entry)
    {
        Entry->Bitmap = *Bitmap;
        Entry->P = P;
        Entry->Color = Color;
    }
}

static void
RenderGroupToOutput(render_group *RenderGroup, bitmap *DrawBuffer, rect2 ClipRect)
{
    // NOTE(rick): Render memory must be aligned on a 16-byte boundary
    Assert(((umm)DrawBuffer->Data & 15) == 0);

    for(u32 BaseAddress = 0;
        BaseAddress < RenderGroup->PushBufferSize;
       )
    {
        render_entry_header *Header = (render_entry_header *)(RenderGroup->PushBufferBase + BaseAddress);
        BaseAddress += sizeof(*Header);

        void *Data = (u8 *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderEntryType_render_entry_clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)Data;

                ClearSIMD(DrawBuffer, Entry->Color, ClipRect);

                BaseAddress += sizeof(*Entry);
            } break;
            case RenderEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;

                DrawRectangleSIMD4W(DrawBuffer, Entry->Min, Entry->Max, Entry->Color, ClipRect);

                BaseAddress += sizeof(*Entry);
            } break;
            case RenderEntryType_render_entry_rectangle_outline:
            {
                render_entry_rectangle_outline *Entry = (render_entry_rectangle_outline *)Data;

                DrawRectangleOutline(DrawBuffer, Entry->Rect, Entry->Color, ClipRect, Entry->Thickness);

                BaseAddress += sizeof(*Entry);
            } break;
            case RenderEntryType_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;

                DrawBitmapSIMD4W(DrawBuffer, &Entry->Bitmap, Entry->P.x, Entry->P.y, Entry->Color, ClipRect);

                BaseAddress += sizeof(*Entry);
            } break;

            InvalidDefaultCase;
        }
    }
}

static void
DoTiledRenderWork(void *Data)
{
    tiled_render_work *Work = (tiled_render_work *)Data;
    RenderGroupToOutput(Work->RenderGroup, Work->DrawBuffer, Work->ClipRect);
}

static void
TiledRenderGroupToOutput(render_group *RenderGroup, bitmap *DrawBuffer)
{
    u32 TileCountX = 2;
    u32 TileCountY = 3;

    u32 TileWidth = DrawBuffer->Width / TileCountX;
    u32 TileHeight = DrawBuffer->Height / TileCountY;

    tiled_render_work WorkArray[64] = {};
    Assert((TileCountX * TileCountY) < ArrayCount(WorkArray));

    u32 WorkIndex = 0;
    for(u32 TileY = 0; TileY < TileCountY; ++TileY)
    {
        for(u32 TileX = 0; TileX < TileCountX; ++TileX)
        {
            tiled_render_work *Work = WorkArray + WorkIndex++;

            rect2 ClipRect;
            ClipRect.Min.x = TileX*TileWidth;
            ClipRect.Max.x = ClipRect.Min.x + TileWidth;
            ClipRect.Min.y = TileY*TileHeight;
            ClipRect.Max.y = ClipRect.Min.y + TileHeight;

            Work->RenderGroup = RenderGroup;
            Work->DrawBuffer = DrawBuffer;
            Work->ClipRect = ClipRect;

#if 1
            PushRectangleOutline(RenderGroup, ClipRect, V4(1, 0, 1, 1), 1);
            Platform.AddWorkToQueue(Platform.WorkQueue, DoTiledRenderWork, Work);
#else
            DoTiledRenderWork(Work);
#endif
        }
    }

#if 1
    Platform.CompleteAllWork(Platform.WorkQueue);
#endif
}

