static render_group *
AllocateRenderGroup(memory_arena *Arena, u32 MaxPushBufferSize)
{
    render_group *Result = PushStruct(Arena, render_group);
    Result->PushBufferBase = (u8 *)PushSize(Arena, MaxPushBufferSize);

    Result->PushBufferSize = 0;
    Result->MaxPushBufferSize = MaxPushBufferSize;

    return(Result);
}

static void
RenderGroupToOutput(render_group *RenderGroup, bitmap *DrawBuffer)
{
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

                Clear(DrawBuffer, Entry->Color);

                BaseAddress += sizeof(*Entry);
            } break;
            case RenderEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;

                DrawRectangle(DrawBuffer, Entry->Min, Entry->Max, Entry->Color);

                BaseAddress += sizeof(*Entry);
            } break;
            case RenderEntryType_render_entry_rectangle_outline:
            {
                render_entry_rectangle_outline *Entry = (render_entry_rectangle_outline *)Data;

                DrawRectangleOutline(DrawBuffer, Entry->Rect, Entry->Color, Entry->Thickness);

                BaseAddress += sizeof(*Entry);
            } break;
            case RenderEntryType_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;

                DrawBitmap(DrawBuffer, &Entry->Bitmap, Entry->P.x, Entry->P.y, Entry->Color);

                BaseAddress += sizeof(*Entry);
            } break;

            InvalidDefaultCase;
        }
    }
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
