
inline v4
RGB255ToLinear1(v4 C)
{
    f32 Inv255 = 1.0f / 255.0f;

    v4 Result = {};
    Result.r = C.r*Inv255;
    Result.g = C.g*Inv255;
    Result.b = C.b*Inv255;
    Result.a = C.a*Inv255;
    return(Result);
}

inline v4
Linear1ToRGB255(v4 C)
{
    v4 Result = {};
    Result.r = 255.0f*C.r;
    Result.g = 255.0f*C.g;
    Result.b = 255.0f*C.b;
    Result.a = 255.0f*C.a;
    return(Result);
}

inline color_mask
GetColorMask(u32 Mask)
{
    color_mask Result = {};
    Result.Mask = Mask;
    Result.ShiftAmount = BitScanForward(Mask);
    return(Result);
}

inline void
Clear(bitmap *DrawBuffer, v4 ColorIn, rect2 ClipRect)
{
    u32 Color = ( ((u8)(ColorIn.r*255.0f) << 16) |
                  ((u8)(ColorIn.g*255.0f) << 8) |
                  ((u8)(ColorIn.b*255.0f) << 0) |
                  ((u8)(ColorIn.a*255.0f) << 24) );

    u32 MinX = ClipRect.Min.x;
    u32 MinY = ClipRect.Min.y;
    u32 MaxX = ClipRect.Max.x;
    u32 MaxY = ClipRect.Max.y;

    u32 *Row = ((u32 *)DrawBuffer->Data + MinY*DrawBuffer->Width);
    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = Row + MinX;
        for(u32 X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }

        Row += DrawBuffer->Width;
    }
}

inline void
ClearSIMD(bitmap *DrawBuffer, v4 ColorIn, rect2 ClipRect)
{
    u32 Color = ( ((u8)(ColorIn.r*255.0f) << 16) |
                  ((u8)(ColorIn.g*255.0f) << 8) |
                  ((u8)(ColorIn.b*255.0f) << 0) |
                  ((u8)(ColorIn.a*255.0f) << 24) );
    __m128i _Color = _mm_set1_epi32(Color);

    u32 MinX = ClipRect.Min.x;
    u32 MinY = ClipRect.Min.y;
    u32 MaxX = ClipRect.Max.x;
    u32 MaxY = ClipRect.Max.y;

    u32 *Row = ((u32 *)DrawBuffer->Data + MinY*DrawBuffer->Width);
    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = Row + MinX;
        for(u32 X = MinX; X < MaxX; X += 4)
        {
            // TODO(rick): We're overwriting here
            *(__m128i *)Pixel = _Color;
            Pixel += 4;
        }

        Row += DrawBuffer->Width;
    }
}

static void
DrawRectangle(bitmap *DrawBuffer, v2 Min, v2 Max, v4 ColorIn, rect2 ClipRect)
{
    ColorIn.r = ColorIn.r*ColorIn.a;
    ColorIn.g = ColorIn.g*ColorIn.a;
    ColorIn.b = ColorIn.b*ColorIn.a;
    u32 Color = ( ((u8)(ColorIn.r*255.0f) << 16) |
                  ((u8)(ColorIn.g*255.0f) << 8) |
                  ((u8)(ColorIn.b*255.0f) << 0) |
                  ((u8)(ColorIn.a*255.0f) << 24) );

    s32 MinX = Min.x;
    s32 MinY = Min.y;
    s32 MaxX = Max.x;
    s32 MaxY = Max.y;

    if(MinX < ClipRect.Min.x) { MinX = ClipRect.Min.x; }
    if(MinY < ClipRect.Min.y) { MinY = ClipRect.Min.y; }
    if(MaxX > ClipRect.Max.x) { MaxX = ClipRect.Max.x; }
    if(MaxY > ClipRect.Max.y) { MaxY = ClipRect.Max.y; }

    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = ((u32 *)DrawBuffer->Data + (Y * DrawBuffer->Width) + MinX);
        for(u32 X = MinX; X < MaxX; ++X)
        {
            v4 Dest = { (f32)((*Pixel >> 16) & 0xFF),
                        (f32)((*Pixel >> 8) & 0xFF),
                        (f32)((*Pixel >> 0) & 0xFF),
                        (f32)((*Pixel >> 24) & 0xFF) };
            Dest = RGB255ToLinear1(Dest);
            v4 Result = (1.0f - ColorIn.a)*Dest + ColorIn;
            Result = Linear1ToRGB255(Result);
            *Pixel++ = ( ((u32)(Result.r) << 16) | 
                         ((u32)(Result.g) << 8) |
                         ((u32)(Result.b) << 0) |
                         ((u32)(Result.a) << 24) );
        }
    }
}

static void
DrawRectangleSIMD(bitmap *DrawBuffer, v2 Min, v2 Max, v4 ColorIn, rect2 ClipRect)
{
    f32 ColorInr = ColorIn.r*ColorIn.a;
    f32 ColorIng = ColorIn.g*ColorIn.a;
    f32 ColorInb = ColorIn.b*ColorIn.a;
    f32 ColorIna = ColorIn.a*255.0f;

    s32 MinX = Min.x;
    s32 MinY = Min.y;
    s32 MaxX = Max.x;
    s32 MaxY = Max.y;

    if(MinX < ClipRect.Min.x) { MinX = ClipRect.Min.x; }
    if(MinY < ClipRect.Min.y) { MinY = ClipRect.Min.y; }
    if(MaxX > ClipRect.Max.x) { MaxX = ClipRect.Max.x; }
    if(MaxY > ClipRect.Max.y) { MaxY = ClipRect.Max.y; }

    f32 Inv255 = 1.0f / 255.0f;
    f32 SourceA = (1.0f - ColorIn.a);

    for(u32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = ((u32 *)DrawBuffer->Data + (Y * DrawBuffer->Width) + MinX);
        for(u32 X = MinX; X < MaxX; ++X)
        {
            // NOTE(rick): Load
            f32 Destr = (f32)((*Pixel >> 16) & 0xFF);
            f32 Destg = (f32)((*Pixel >> 8) & 0xFF);
            f32 Destb = (f32)((*Pixel >> 0) & 0xFF);
            f32 Desta = (f32)((*Pixel >> 24) & 0xFF);

            // NOTE(rick): RGB255 to Linear1
            Destr = Destr*Inv255;
            Destg = Destg*Inv255;
            Destb = Destb*Inv255;
            Desta = Desta*Inv255;

            // NOTE(rick): Alpha Blend
            // (1.0f - ColorIn.a)*Dest.X + ColorIn.X
            f32 Resultr = SourceA*Destr + ColorInr;
            f32 Resultg = SourceA*Destg + ColorIng;
            f32 Resultb = SourceA*Destb + ColorInb;
            f32 Resulta = SourceA*Desta + ColorIna;

            // NOTE(rick): Linear1 To RGB255
            Resultr = 255.0f*Resultr;
            Resultg = 255.0f*Resultg;
            Resultb = 255.0f*Resultb;
            Resulta = 255.0f*Resulta;

            // NOTE(rick): Store
            *Pixel++ = ( ((u32)(Resultr) << 16) | 
                         ((u32)(Resultg) << 8) |
                         ((u32)(Resultb) << 0) |
                         ((u32)(Resulta) << 24) );
        }
    }
}

static void
DrawRectangleSIMDRGBA(bitmap *DrawBuffer, v2 Min, v2 Max, v4 ColorIn, rect2 ClipRect)
{

    f32 ColorInr = ColorIn.r*ColorIn.a;
    f32 ColorIng = ColorIn.g*ColorIn.a;
    f32 ColorInb = ColorIn.b*ColorIn.a;
    f32 ColorIna = ColorIn.a*255.0f;

    s32 MinX = Min.x;
    s32 MinY = Min.y;
    s32 MaxX = Max.x;
    s32 MaxY = Max.y;

    if(MinX < ClipRect.Min.x) { MinX = ClipRect.Min.x; }
    if(MinY < ClipRect.Min.y) { MinY = ClipRect.Min.y; }
    if(MaxX > ClipRect.Max.x) { MaxX = ClipRect.Max.x; }
    if(MaxY > ClipRect.Max.y) { MaxY = ClipRect.Max.y; }

    __m128 Inv255 = _mm_set1_ps(1.0f / 255.0f);
    __m128 _255 = _mm_set1_ps(255.0f);
    __m128 DestAContrib = _mm_set1_ps(1.0f - ColorIn.a);
    __m128 Color = _mm_setr_ps(ColorIna, ColorInr, ColorIng, ColorInb);

    for(s32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = ((u32 *)DrawBuffer->Data + (Y * DrawBuffer->Width) + MinX);
        for(s32 X = MinX; X < MaxX; ++X)
        {
            // NOTE(rick): Load
            __m128 Dest = _mm_setr_ps((*Pixel >> 24) & 0xFF,
                                      (*Pixel >> 16) & 0xFF,
                                      (*Pixel >> 8) & 0xFF,
                                      (*Pixel >> 0) & 0xFF);

            // NOTE(rick): RGB255 to Linear1
            Dest = _mm_mul_ps(Dest, Inv255);

            // NOTE(rick): Alpha Blend
            // (1.0f - ColorIn.a)*Dest.X + ColorIn.X
            __m128 Result = _mm_add_ps(_mm_mul_ps(DestAContrib, Dest), Color);

            // NOTE(rick): Linear1 To RGB255
            Result = _mm_mul_ps(Result, _255);

            // NOTE(rick): Store
            *Pixel++ = ( ((u32)Result.m128_f32[0] << 24) |
                         ((u32)Result.m128_f32[1] << 16) |
                         ((u32)Result.m128_f32[2] <<  8) |
                         ((u32)Result.m128_f32[3] <<  0) );
        }
    }
}

static void
DrawRectangleSIMD4W(bitmap *DrawBuffer, v2 Min, v2 Max, v4 ColorIn, rect2 ClipRect)
{

    f32 ColorInr = ColorIn.r*ColorIn.a;
    f32 ColorIng = ColorIn.g*ColorIn.a;
    f32 ColorInb = ColorIn.b*ColorIn.a;
    f32 ColorIna = ColorIn.a*255.0f;

    s32 MinX = Min.x;
    s32 MinY = Min.y;
    s32 MaxX = Max.x;
    s32 MaxY = Max.y;

    if(MinX < ClipRect.Min.x) { MinX = ClipRect.Min.x; }
    if(MinY < ClipRect.Min.y) { MinY = ClipRect.Min.y; }
    if(MaxX > ClipRect.Max.x) { MaxX = ClipRect.Max.x; }
    if(MaxY > ClipRect.Max.y) { MaxY = ClipRect.Max.y; }

    __m128 Inv255 = _mm_set1_ps(1.0f / 255.0f);
    __m128 _255 = _mm_set1_ps(255.0f);
    __m128 DestAContrib = _mm_set1_ps(1.0f - ColorIn.a);
    __m128i MaxPixelX = _mm_set1_epi32(MaxX+1);

    __m128 Colora = _mm_setr_ps(ColorIna, ColorIna, ColorIna, ColorIna);
    __m128 Colorr = _mm_setr_ps(ColorInr, ColorInr, ColorInr, ColorInr);
    __m128 Colorg = _mm_setr_ps(ColorIng, ColorIng, ColorIng, ColorIng);
    __m128 Colorb = _mm_setr_ps(ColorInb, ColorInb, ColorInb, ColorInb);
    __m128i MaskFF = _mm_set1_epi32(0xFF);

    for(s32 Y = MinY; Y < MaxY; ++Y)
    {
        u32 *Pixel = ((u32 *)DrawBuffer->Data + (Y * DrawBuffer->Width) + MinX);
        for(s32 X = MinX; X < MaxX; X += 4)
        {
            // NOTE(rick): Create mask
            __m128i PixelX = _mm_setr_epi32(X, X+1, X+2, X+3);
            __m128i WriteMask = _mm_cmplt_epi32(PixelX, MaxPixelX);

            // NOTE(rick): Load
            __m128i OriginalDest = _mm_load_si128((__m128i *)Pixel);
            __m128i OriginalDesta = _mm_and_si128(_mm_srli_si128(OriginalDest, 24), MaskFF);
            __m128i OriginalDestr = _mm_and_si128(_mm_srli_si128(OriginalDest, 16), MaskFF);
            __m128i OriginalDestg = _mm_and_si128(_mm_srli_si128(OriginalDest,  8), MaskFF);
            __m128i OriginalDestb = _mm_and_si128(_mm_srli_si128(OriginalDest,  0), MaskFF);

            // NOTE(rick): 
            __m128 Desta = _mm_cvtepi32_ps(OriginalDesta);
            __m128 Destr = _mm_cvtepi32_ps(OriginalDestr);
            __m128 Destg = _mm_cvtepi32_ps(OriginalDestg);
            __m128 Destb = _mm_cvtepi32_ps(OriginalDestb);

                // NOTE(rick): RGB255 to Linear0
            __m128 LinearDesta = _mm_mul_ps(Desta, Inv255);
            __m128 LinearDestr = _mm_mul_ps(Destr, Inv255);
            __m128 LinearDestg = _mm_mul_ps(Destg, Inv255);
            __m128 LinearDestb = _mm_mul_ps(Destb, Inv255);

            // NOTE(rick): Alpha Blend
            // (1.0f - ColorIn.a)*Dest.X + ColorIn.X
            __m128 Resulta = _mm_add_ps(_mm_mul_ps(DestAContrib, LinearDesta), Colora);
            __m128 Resultr = _mm_add_ps(_mm_mul_ps(DestAContrib, LinearDestr), Colorr);
            __m128 Resultg = _mm_add_ps(_mm_mul_ps(DestAContrib, LinearDestg), Colorg);
            __m128 Resultb = _mm_add_ps(_mm_mul_ps(DestAContrib, LinearDestb), Colorb);

            // NOTE(rick): Linear1 To RGB255
            Resulta = _mm_mul_ps(Resulta, _255);
            Resultr = _mm_mul_ps(Resultr, _255);
            Resultg = _mm_mul_ps(Resultg, _255);
            Resultb = _mm_mul_ps(Resultb, _255);

            // NOTE(rick): Convert to integer
            __m128i ResultInta = _mm_cvtps_epi32(Resulta);
            __m128i ResultIntr = _mm_cvtps_epi32(Resultr);
            __m128i ResultIntg = _mm_cvtps_epi32(Resultg);
            __m128i ResultIntb = _mm_cvtps_epi32(Resultb);

            // NOTE(rick): Reconstruct 4byte pixels
            __m128i Pixa = _mm_slli_epi32(ResultInta, 24);
            __m128i Pixr = _mm_slli_epi32(ResultIntr, 16);
            __m128i Pixg = _mm_slli_epi32(ResultIntg,  8);
            __m128i Pixb = _mm_slli_epi32(ResultIntb,  0);
            __m128i Out = _mm_or_si128(_mm_or_si128(Pixa, Pixr), _mm_or_si128(Pixg, Pixb));

            // NOTE(rick): Apply write mask
            Out = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                               _mm_andnot_si128(WriteMask, OriginalDest));

            // NOTE(rick): Store
            *(__m128i *)Pixel = Out;
            Pixel += 4;
        }
    }
}

static void
DrawRectangle(bitmap *DrawBuffer, rect2 Rectangle, v4 Color, rect2 ClipRect)
{
    DrawRectangle(DrawBuffer, Rectangle.Min, Rectangle.Max, Color, ClipRect);
}

static void
DrawRectangleOutline(bitmap *DrawBuffer, rect2 Rect, v4 Color, rect2 ClipRect, f32 Thickness = 1.0f)
{
#if 0
    // NOTE(rick): RGBA Method is faster for drawing outlines
    DrawRectangleSIMD4W(DrawBuffer, Rect.Min, V2(Rect.Max.x, Rect.Min.y + Thickness), Color, ClipRect);
    DrawRectangleSIMD4W(DrawBuffer, V2(Rect.Min.x, Rect.Max.y - Thickness), Rect.Max, Color, ClipRect);
    DrawRectangleSIMD4W(DrawBuffer, Rect.Min + V2(0, 1), V2(Rect.Min.x + Thickness, Rect.Max.y) - V2(0, 1), Color, ClipRect);
    DrawRectangleSIMD4W(DrawBuffer, V2(Rect.Max.x - Thickness, Rect.Min.y) + V2(0, 1), Rect.Max - V2(0, 1), Color, ClipRect);
#else
    // NOTE(rick): Top, Bottom, Left, Right
    DrawRectangleSIMDRGBA(DrawBuffer, Rect.Min, V2(Rect.Max.x, Rect.Min.y + Thickness), Color, ClipRect);
    DrawRectangleSIMDRGBA(DrawBuffer, V2(Rect.Min.x, Rect.Max.y - Thickness), Rect.Max, Color, ClipRect);
    DrawRectangleSIMDRGBA(DrawBuffer, Rect.Min + V2(0, 1), V2(Rect.Min.x + Thickness, Rect.Max.y) - V2(0, 1), Color, ClipRect);
    DrawRectangleSIMDRGBA(DrawBuffer, V2(Rect.Max.x - Thickness, Rect.Min.y) + V2(0, 1), Rect.Max - V2(0, 1), Color, ClipRect);
#endif
}

inline void
DrawBitmap(bitmap *DrawBuffer, bitmap *Bitmap, f32 XIn, f32 YIn, v4 Color, rect2 ClipRect)
{
    s32 XMinOut = XIn;
    if(XMinOut < ClipRect.Min.x)
    {
        XMinOut = ClipRect.Min.x;
    }

    s32 XMaxOut = XIn + Bitmap->Width;
    if(XMaxOut > (s32)ClipRect.Max.x)
    {
        XMaxOut = ClipRect.Max.x;
    }

    s32 YMinOut = YIn;
    if(YMinOut < ClipRect.Min.y)
    {
        YMinOut = ClipRect.Min.y;
    }

    s32 YMaxOut = YIn + Bitmap->Height;
    if(YMaxOut > (s32)ClipRect.Max.y)
    {
        YMaxOut = ClipRect.Max.y;
    }

    s32 XMinSource = 0;
    if(XIn < XMinOut)
    {
        XMinSource += Absolute(XMinOut - XIn);
    }

    s32 YMinSource = 0;
    if(YIn < YMinOut)
    {
        YMinSource += Absolute(YMinOut - YIn);
    }

    //u32 *SourceRow = (u32 *)Bitmap->Data + (YMinSource * Bitmap->Width) + XMinSource;
    u32 *SourceRow = (u32 *)Bitmap->Data + (((Bitmap->Height-1)-YMinSource) * Bitmap->Width) + XMinSource;
    u32 *DestRow = (u32 *)DrawBuffer->Data + (YMinOut * DrawBuffer->Width) + XMinOut;
    for(s32 Y = YMinOut; Y < YMaxOut; ++Y)
    {
        u32 *SourcePixel = SourceRow;
        u32 *DestPixel = DestRow;
        for(s32 X = XMinOut; X < XMaxOut; ++X)
        {
            u32 ColorPixel = *SourcePixel++;
            v4 SourcePixelV4 = { (f32)((ColorPixel & Bitmap->RedMask.Mask) >> Bitmap->RedMask.ShiftAmount),
                                 (f32)((ColorPixel & Bitmap->GreenMask.Mask) >> Bitmap->GreenMask.ShiftAmount),
                                 (f32)((ColorPixel & Bitmap->BlueMask.Mask) >> Bitmap->BlueMask.ShiftAmount),
                                 (f32)((ColorPixel & Bitmap->AlphaMask.Mask) >> Bitmap->AlphaMask.ShiftAmount) };
            f32 SourceAlpha = ((f32)SourcePixelV4.a / 255.0f) * Color.a;
            SourcePixelV4.rgb = SourcePixelV4.rgb * SourceAlpha;
            SourcePixelV4.r *= Color.a*Color.r;
            SourcePixelV4.g *= Color.a*Color.g;
            SourcePixelV4.b *= Color.a*Color.b;

            v4 DestV4 = U32ToV4(*DestPixel);
            DestV4.rgb = SourcePixelV4.rgb + ((1.0f - SourceAlpha)*DestV4.rgb);
            u32 ColorOut = V4ToU32(DestV4);

            *DestPixel++ = ColorOut;
        }
        
        SourceRow -= Bitmap->Width;
        //SourceRow += Bitmap->Width;
        DestRow += DrawBuffer->Width;
    }
}

inline void
DrawBitmapSIMD(bitmap *DrawBuffer, bitmap *Bitmap, f32 XIn, f32 YIn, v4 Color, rect2 ClipRect)
{
    s32 XMinOut = XIn;
    if(XMinOut < ClipRect.Min.x)
    {
        XMinOut = ClipRect.Min.x;
    }

    s32 XMaxOut = XIn + Bitmap->Width;
    if(XMaxOut > (s32)ClipRect.Max.x)
    {
        XMaxOut = ClipRect.Max.x;
    }

    s32 YMinOut = YIn;
    if(YMinOut < ClipRect.Min.y)
    {
        YMinOut = ClipRect.Min.y;
    }

    s32 YMaxOut = YIn + Bitmap->Height;
    if(YMaxOut > (s32)ClipRect.Max.y)
    {
        YMaxOut = ClipRect.Max.y;
    }

    s32 XMinSource = 0;
    if(XIn < XMinOut)
    {
        XMinSource += Absolute(XMinOut - XIn);
    }

    s32 YMinSource = 0;
    if(YIn < YMinOut)
    {
        YMinSource += Absolute(YMinOut - YIn);
    }

    f32 Inv255 = 1.0f / 255.0f;

    u32 *SourceRow = (u32 *)Bitmap->Data + (((Bitmap->Height-1)-YMinSource) * Bitmap->Width) + XMinSource;
    u32 *DestRow = (u32 *)DrawBuffer->Data + (YMinOut * DrawBuffer->Width) + XMinOut;
    for(s32 Y = YMinOut; Y < YMaxOut; ++Y)
    {
        u32 *SourcePixelPtr = SourceRow;
        u32 *DestPixel = DestRow;
        for(s32 X = XMinOut; X < XMaxOut; ++X)
        {
            // NOTE(rick): Load
            u32 SourcePixel = *SourcePixelPtr++;
            f32 Sourcer = (f32)((SourcePixel & Bitmap->RedMask.Mask) >> Bitmap->RedMask.ShiftAmount);
            f32 Sourceg = (f32)((SourcePixel & Bitmap->GreenMask.Mask) >> Bitmap->GreenMask.ShiftAmount);
            f32 Sourceb = (f32)((SourcePixel & Bitmap->BlueMask.Mask) >> Bitmap->BlueMask.ShiftAmount);
            f32 Sourcea = (f32)((SourcePixel & Bitmap->AlphaMask.Mask) >> Bitmap->AlphaMask.ShiftAmount);

            // NOTE(rick): Pre-multiply source alpha and modulate by color alpha
            f32 SourceAlpha = ((f32)Sourcea * Inv255) * Color.a;
            Sourcer = Sourcer * SourceAlpha;
            Sourceg = Sourceg * SourceAlpha;
            Sourceb = Sourceb * SourceAlpha;
            Sourcea = Sourcea * SourceAlpha;

            // NOTE(rick): Modulate source by color;
            Sourcer *= Color.a*Color.r;
            Sourceg *= Color.a*Color.g;
            Sourceb *= Color.a*Color.b;

            // NOTE(rick): Load destination
            f32 Desta = ((*DestPixel >> 24) & 0xFF);
            f32 Destr = ((*DestPixel >> 16) & 0xFF);
            f32 Destg = ((*DestPixel >>  8) & 0xFF);
            f32 Destb = ((*DestPixel >>  0) & 0xFF);

            // NOTE(rick): Alpha blend source and dest
            f32 Resulta = Sourcea + ((1.0f - SourceAlpha) * Desta);
            f32 Resultr = Sourcer + ((1.0f - SourceAlpha) * Destr);
            f32 Resultg = Sourceg + ((1.0f - SourceAlpha) * Destg);
            f32 Resultb = Sourceb + ((1.0f - SourceAlpha) * Destb);

            // NOTE(rick): Store
            *DestPixel++ = ( ((u32)Resulta << 24) |
                             ((u32)Resultr << 16) |
                             ((u32)Resultg <<  8) |
                             ((u32)Resultb <<  0) );
        }
        
        SourceRow -= Bitmap->Width;
        DestRow += DrawBuffer->Width;
    }
}

inline void
DrawBitmapSIMDRGBA(bitmap *DrawBuffer, bitmap *Bitmap, f32 XIn, f32 YIn, v4 Color, rect2 ClipRect)
{
    s32 XMinOut = XIn;
    if(XMinOut < ClipRect.Min.x)
    {
        XMinOut = ClipRect.Min.x;
    }

    s32 XMaxOut = XIn + Bitmap->Width;
    if(XMaxOut > (s32)ClipRect.Max.x)
    {
        XMaxOut = ClipRect.Max.x;
    }

    s32 YMinOut = YIn;
    if(YMinOut < ClipRect.Min.y)
    {
        YMinOut = ClipRect.Min.y;
    }

    s32 YMaxOut = YIn + Bitmap->Height;
    if(YMaxOut > (s32)ClipRect.Max.y)
    {
        YMaxOut = ClipRect.Max.y;
    }

    s32 XMinSource = 0;
    if(XIn < XMinOut)
    {
        XMinSource += Absolute(XMinOut - XIn);
    }

    s32 YMinSource = 0;
    if(YIn < YMinOut)
    {
        YMinSource += Absolute(YMinOut - YIn);
    }

    __m128 Inv255 = _mm_set1_ps(1.0f / 255.0f);
    __m128 Colora = _mm_set1_ps(Color.a);
    __m128 _Color = _mm_setr_ps(Color.a, Color.r, Color.g, Color.b);
    __m128 AlphaColor = _mm_mul_ps(Colora, _Color);

    u32 *SourceRow = (u32 *)Bitmap->Data + (((Bitmap->Height-1)-YMinSource) * Bitmap->Width) + XMinSource;
    u32 *DestRow = (u32 *)DrawBuffer->Data + (YMinOut * DrawBuffer->Width) + XMinOut;
    for(s32 Y = YMinOut; Y < YMaxOut; ++Y)
    {
        u32 *SourcePixelPtr = SourceRow;
        u32 *DestPixel = DestRow;
        for(s32 X = XMinOut; X < XMaxOut; ++X)
        {
            // NOTE(rick): Load
            u32 SourcePixel = *SourcePixelPtr++;
            __m128 Source = _mm_setr_ps((f32)((SourcePixel & Bitmap->AlphaMask.Mask) >> Bitmap->AlphaMask.ShiftAmount),
                                        (f32)((SourcePixel & Bitmap->RedMask.Mask) >> Bitmap->RedMask.ShiftAmount),
                                        (f32)((SourcePixel & Bitmap->GreenMask.Mask) >> Bitmap->GreenMask.ShiftAmount),
                                        (f32)((SourcePixel & Bitmap->BlueMask.Mask) >> Bitmap->BlueMask.ShiftAmount));

            // NOTE(rick): Pre-multiply source alpha and modulate by color alpha
            __m128 _SourceAlpha = _mm_set1_ps(((f32)Source.m128_f32[0] * Inv255.m128_f32[0]) * Color.a);
            Source = _mm_mul_ps(Source, _SourceAlpha);

            // NOTE(rick): Modulate source by color;
            Source = _mm_mul_ps(Source, AlphaColor);

            // NOTE(rick): Load destination
            __m128 Dest = _mm_setr_ps((*DestPixel >> 24) & 0xFF,
                                      (*DestPixel >> 16) & 0xFF,
                                      (*DestPixel >>  8) & 0xFF,
                                      (*DestPixel >>  0) & 0xFF);

            // NOTE(rick): Alpha blend source and dest
            __m128 DestAlphaContrib = _mm_set1_ps(1.0f - _SourceAlpha.m128_f32[0]);
            __m128 Result = _mm_add_ps(Source, _mm_mul_ps(Dest, DestAlphaContrib));

            // NOTE(rick): Store
            *DestPixel++ = ( ((u32)Result.m128_f32[0] << 24) |
                             ((u32)Result.m128_f32[1] << 16) |
                             ((u32)Result.m128_f32[2] <<  8) |
                             ((u32)Result.m128_f32[3] <<  0) );
        }
        
        SourceRow -= Bitmap->Width;
        DestRow += DrawBuffer->Width;
    }
}

inline void
DrawBitmapSIMD4W(bitmap *DrawBuffer, bitmap *Bitmap, f32 XIn, f32 YIn, v4 Color, rect2 ClipRect)
{
    s32 XMinOut = XIn;
    if(XMinOut < ClipRect.Min.x)
    {
        XMinOut = ClipRect.Min.x;
    }

    s32 XMaxOut = XIn + Bitmap->Width;
    if(XMaxOut > (s32)ClipRect.Max.x)
    {
        XMaxOut = ClipRect.Max.x;
    }

    s32 YMinOut = YIn;
    if(YMinOut < ClipRect.Min.y)
    {
        YMinOut = ClipRect.Min.y;
    }

    s32 YMaxOut = YIn + Bitmap->Height;
    if(YMaxOut > (s32)ClipRect.Max.y)
    {
        YMaxOut = ClipRect.Max.y;
    }

    s32 XMinSource = 0;
    if(XIn < XMinOut)
    {
        XMinSource += Absolute(XMinOut - XIn);
    }

    s32 YMinSource = 0;
    if(YIn < YMinOut)
    {
        YMinSource += Absolute(YMinOut - YIn);
    }


    __m128 Inv255 = _mm_set1_ps(1.0f / 255.0f);
    __m128 Colora = _mm_set1_ps(Color.a);
    __m128 Colorr = _mm_set1_ps(Color.r);
    __m128 Colorg = _mm_set1_ps(Color.g);
    __m128 Colorb = _mm_set1_ps(Color.b);

    __m128i AlphaMask = _mm_set1_epi32(Bitmap->AlphaMask.Mask);
    __m128i RedMask = _mm_set1_epi32(Bitmap->RedMask.Mask);
    __m128i GreenMask = _mm_set1_epi32(Bitmap->GreenMask.Mask);
    __m128i BlueMask = _mm_set1_epi32(Bitmap->BlueMask.Mask);
    s32 AlphaShift = Bitmap->AlphaMask.ShiftAmount;
    s32 RedShift = Bitmap->RedMask.ShiftAmount;
    s32 GreenShift = Bitmap->GreenMask.ShiftAmount;
    s32 BlueShift = Bitmap->BlueMask.ShiftAmount;

    __m128 One = _mm_set1_ps(1.0f);
    __m128 DestMaska = _mm_cvtepi32_ps(_mm_set1_epi32(0xFF000000));
    __m128 DestMaskr = _mm_cvtepi32_ps(_mm_set1_epi32(0x00FF0000));
    __m128 DestMaskg = _mm_cvtepi32_ps(_mm_set1_epi32(0x0000FF00));
    __m128 DestMaskb = _mm_cvtepi32_ps(_mm_set1_epi32(0x000000FF));

    __m128i MaxPixelX = _mm_set1_epi32(XMaxOut);

    __m128i MaskFF = _mm_set1_epi32(0xFF);

    u32 *SourceRow = (u32 *)Bitmap->Data + (((Bitmap->Height-1)-YMinSource) * Bitmap->Width) + XMinSource;
    u32 *DestRow = (u32 *)DrawBuffer->Data + (YMinOut * DrawBuffer->Width) + XMinOut;
    for(s32 Y = YMinOut; Y < YMaxOut; ++Y)
    {
        u32 *SourcePixelPtr = SourceRow;
        u32 *DestPixel = DestRow;
        for(s32 X = XMinOut; X < XMaxOut; X += 4)
        {
            // NOTE(rick): Create mask
            __m128i PixelX = _mm_setr_epi32(X, X+1, X+2, X+3);
            __m128i WriteMask = _mm_cmplt_epi32(PixelX, MaxPixelX);

            // NOTE(rick): Load
            __m128i Source = _mm_load_si128((__m128i *)SourcePixelPtr);
            __m128i OriginalSourcea = _mm_and_si128(_mm_srli_epi32(Source, AlphaShift), MaskFF);
            __m128i OriginalSourcer = _mm_and_si128(_mm_srli_epi32(Source, RedShift), MaskFF);
            __m128i OriginalSourceg = _mm_and_si128(_mm_srli_epi32(Source, GreenShift), MaskFF);
            __m128i OriginalSourceb = _mm_and_si128(_mm_srli_epi32(Source, BlueShift), MaskFF);

            __m128 Sourcea = _mm_cvtepi32_ps(OriginalSourcea);
            __m128 Sourcer = _mm_cvtepi32_ps(OriginalSourcer);
            __m128 Sourceg = _mm_cvtepi32_ps(OriginalSourceg);
            __m128 Sourceb = _mm_cvtepi32_ps(OriginalSourceb);

            // NOTE(rick): Pre-multiply source alpha and modulate by color alpha
            __m128 SourceAlpha = _mm_mul_ps(_mm_mul_ps(Sourcea, Inv255), Colora);
            //Sourcea = _mm_mul_ps(Sourcea, SourceAlpha);
            Sourcer = _mm_mul_ps(Sourcer, SourceAlpha);
            Sourceg = _mm_mul_ps(Sourceg, SourceAlpha);
            Sourceb = _mm_mul_ps(Sourceb, SourceAlpha);

            // NOTE(rick): Modulate source by color;
            Sourcea = _mm_mul_ps(Sourcea, _mm_mul_ps(Colora, Colora));
            Sourcer = _mm_mul_ps(Sourcer, _mm_mul_ps(Colora, Colorr));
            Sourceg = _mm_mul_ps(Sourceg, _mm_mul_ps(Colora, Colorg));
            Sourceb = _mm_mul_ps(Sourceb, _mm_mul_ps(Colora, Colorb));

            // NOTE(rick): Load destination
            __m128i Dest = _mm_load_si128((__m128i *)DestPixel);
            __m128i OriginalDesta = _mm_and_si128(_mm_srli_epi32(Dest, AlphaShift), MaskFF);
            __m128i OriginalDestr = _mm_and_si128(_mm_srli_epi32(Dest, RedShift), MaskFF);
            __m128i OriginalDestg = _mm_and_si128(_mm_srli_epi32(Dest, GreenShift), MaskFF);
            __m128i OriginalDestb = _mm_and_si128(_mm_srli_epi32(Dest, BlueShift), MaskFF);
            __m128 Desta = _mm_cvtepi32_ps(OriginalDesta);
            __m128 Destr = _mm_cvtepi32_ps(OriginalDestr);
            __m128 Destg = _mm_cvtepi32_ps(OriginalDestg);
            __m128 Destb = _mm_cvtepi32_ps(OriginalDestb);

            // NOTE(rick): Alpha blend source and dest
            __m128 DestAlphaContrib = _mm_sub_ps(One, SourceAlpha);
            __m128i Resulta = _mm_cvtps_epi32(_mm_add_ps(Sourcea, _mm_mul_ps(DestAlphaContrib, Desta)));
            __m128i Resultr = _mm_cvtps_epi32(_mm_add_ps(Sourcer, _mm_mul_ps(DestAlphaContrib, Destr)));
            __m128i Resultg = _mm_cvtps_epi32(_mm_add_ps(Sourceg, _mm_mul_ps(DestAlphaContrib, Destg)));
            __m128i Resultb = _mm_cvtps_epi32(_mm_add_ps(Sourceb, _mm_mul_ps(DestAlphaContrib, Destb)));


            // NOTE(rick): Reconstruct 4byte pixels
            __m128i Pixa = _mm_slli_epi32(Resulta, 24);
            __m128i Pixr = _mm_slli_epi32(Resultr, 16);
            __m128i Pixg = _mm_slli_epi32(Resultg,  8);
            __m128i Pixb = _mm_slli_epi32(Resultb,  0);
            __m128i Out = _mm_or_si128(_mm_or_si128(Pixa, Pixr), _mm_or_si128(Pixg, Pixb));

            // NOTE(rick): Apply mask
            Out = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                               _mm_andnot_si128(WriteMask, Dest));

            // NOTE(rick): Store
            *(__m128i *)DestPixel = Out;

            DestPixel += 4;
            SourcePixelPtr += 4;
        }
        
        SourceRow -= Bitmap->Width;
        DestRow += DrawBuffer->Width;
    }
}

inline void
DrawText(render_group *RenderGroup, font_map *FontMap, f32 XIn, f32 YIn, char *String, v4 Color)
{
    s32 AtX = XIn;
    s32 AtY = YIn;

    for(char *Character = String; *Character; ++Character)
    {
        if((Character[0] == '#') && (Character[1] == '#') && (Character[2] == 'C'))
        {
            Character += 3;

            f32 Red = 0.0f;
            f32 Green = 0.0f;
            f32 Blue = 0.0f;
            f32 Alpha = 0.0f;

            // TODO(rick): Parse this myself
            sscanf(Character, "%f,%f,%f,%f", &Red, &Green, &Blue, &Alpha);

            Color = {Red, Green, Blue, Alpha};

            for(;*Character; ++Character)
            {
                if((Character[0] == '#') && (Character[1] == '#'))
                {
                    ++Character;
                    break;
                }
            }
        }
        else
        {
            bitmap GlyphBitmap = {};
            GetGlyphBitmapForCodePoint(FontMap, *Character,
                                       (s32 *)&GlyphBitmap.Width, (s32 *)&GlyphBitmap.Height,
                                       (s32 *)&GlyphBitmap.Pitch, &GlyphBitmap.Data);

            // NOTE(rick): Using the RED channel mask as the ALPHA mask. I should
            // upgrade the font tool so that it creates correctly alpha'd bitmaps.
            // The header should also upgrade so that it writes out the mask
            // information to the font_map.
            GlyphBitmap.RedMask = GetColorMask(0x00FF0000);
            GlyphBitmap.GreenMask = GetColorMask(0x0000FF00);
            GlyphBitmap.BlueMask = GetColorMask(0x000000FF);
            GlyphBitmap.AlphaMask = GetColorMask(0x00FF0000);

            s32 AlignX = 0;
            s32 AlignY = 0;
            GetAlignmentForForCodePoint(FontMap, *Character, &AlignX, &AlignY);

            s32 OutX = AtX + GetHorizontalAdjustmentForCodePoint(FontMap, *Character);
            s32 OutY = AtY - GlyphBitmap.Height + AlignY;

            PushBitmap(RenderGroup, &GlyphBitmap, V2(OutX, OutY), Color);
            AtX += GetHorizontalAdvanceForCodepoint(FontMap, *Character);
        }
    }
}

static void
PushText(render_group *RenderGroup, font_map *FontMap, v2 P, char *String, v4 Color)
{
    DrawText(RenderGroup, FontMap, P.x, P.y, String, Color);
}

// TODO(rick): Push strings onto the render group?!?!
#define PushText(RenderGroup, FontMap, P, Color, Format, ...) \
{ \
    char Buffer[256] = {}; \
    snprintf(Buffer, ArrayCount(Buffer), Format, __VA_ARGS__); \
    PushText(RenderGroup, FontMap, P, Buffer, Color); \
}

static rect2
GetTextBounds(font_map *FontMap, char *String)
{
    rect2 Result = {};
    Result.Min = V2(0, 0);
    Result.Max = V2(0, GetLineAdvance(FontMap));

    for(char *Character = String; *Character; ++Character)
    {
        if(*Character == '\n')
        {
            Result.Max.y += GetLineAdvance(FontMap);
        }
        else
        {
            Result.Max.x += GetHorizontalAdvanceForCodepoint(FontMap, *Character);
        }
    }

    return(Result);
}
