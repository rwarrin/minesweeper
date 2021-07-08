/**
 * FONT LIB
 *
 * NOTES:
 *   - Users load the font map file .fmp into memory and pass a pointer to the
 *   data to the library. The Library will provide helper methods for outputting
 *   text.
 *   - IMPORTANT - ALL glyph bitmaps are BOTTOM-UP!
 *
 * USAGE:
 *   font_map *FontMap = (font_map *)FontFile.fmp_Contents_In_Memory;
 *   InitializeFontMap(FontMap);
 *   u32 AtX = 10;
 *   u32 AtY = 10;
 *
 *   int32_t *Width = 0;
 *   int32_t *Height = 0;
 *   int32_t *Pitch = 0;
 *   uint8_t *Memory = 0;
 *   GetGlyphBitmapForCodePoint(FontMap, Char, &Width, &Height, &Pitch, &Memory);
 *
 *   int32_t AlignX = 0;
 *   int32_t AlignY = 0;
 *   GetAlignmentForForCodePoint(FontMap, Char, &AlignX, &AlignY);
 *
 *   // Call user-defined render bitmap function
 *   RenderBitmap(&DestinationBitmap,
 *                AtX + GetHorizontalAdjustmentForCodePoint(FontMap, Char),
 *                AtY - AlignY,
 *                Memory, Width, Height, Pitch);
 *
 *   AtX += GetHorizontalAdvanceForCodepoint(FontMap, Char);
 **/

#ifndef FONT_LIB
#define FONT_LIB

#include <stdint.h>

#define FONTLIB_MAX_GLYPH_COUNT 256
#define FL_Statement(stmt) do { stmt } while(0);
#define FL_Assert(Condition) FL_Statement( if(!(Condition)) { *(int *)0 = 0; } )

#pragma pack(push, 1)
union fl_v2
{
    struct
    {
        float X, Y;
    };
};

struct fl_bitmap
{
    uint8_t *Memory;
    int32_t Width;
    int32_t Height;
    int32_t Pitch;
};

struct font_map
{
    uint32_t MaxGlyphCount;
    uint32_t FileSize;

    int32_t AscentHeight;
    int32_t DescentHeight;
    int32_t ExternalLeadingHeight;
    int32_t FontHeight;
    int32_t MaxCharacterWidth;
    int32_t AverageCharacterWidth;

    fl_bitmap BitmapData[FONTLIB_MAX_GLYPH_COUNT];
    uint32_t BitmapOffset[FONTLIB_MAX_GLYPH_COUNT];
    uint32_t BitmapEnd[FONTLIB_MAX_GLYPH_COUNT]; // NOTE(rick): Debugging only
    int32_t CharacterWidth[FONTLIB_MAX_GLYPH_COUNT];
    fl_v2 BitmapAlignmentData[FONTLIB_MAX_GLYPH_COUNT];

    uint32_t KerningDataOffset;
    uint32_t KerningDataSize;
    float *KerningData;
};
#pragma pack(pop)

int32_t InitializeFontMap(font_map *FontMap);
int32_t GetLineAdvance(font_map *FontMap);
void GetAlignmentForForCodePoint(font_map *FontMap, uint32_t CodePoint, int32_t *XOut, int32_t *YOut);
fl_bitmap *GetGlyphBitmapForCodePoint(font_map *FontMap, uint32_t CodePoint);
void GetGlyphBitmapForCodePoint(font_map *FontMap, uint32_t CodePoint, int32_t *Width, int32_t *Height, int32_t *Pitch, uint8_t **Memory);
int32_t GetHorizontalAdjustmentForCodePoint(font_map *FontMap, uint32_t CodePoint);
int32_t GetHorizontalAdvanceForCodepoint(font_map *FontMap, uint32_t CodePoint);

#endif

#ifdef FONT_LIB_IMPLEMENTATION

int32_t
InitializeFontMap(font_map *FontMap)
{
    int32_t Result = 0;

    if(FontMap)
    {
        FontMap->KerningData = (float *)(FontMap + FontMap->KerningDataOffset);

        for(uint32_t CodePointIndex = ' ';
            CodePointIndex <= '~';
            ++CodePointIndex)
        {
            fl_bitmap *Bitmap = FontMap->BitmapData + CodePointIndex;

            size_t BitmapOffset = *(size_t *)&Bitmap->Memory;
            FL_Assert(BitmapOffset == FontMap->BitmapOffset[CodePointIndex]);

            Bitmap->Memory = (uint8_t *)FontMap + BitmapOffset;

            size_t BitmapDataSize = Bitmap->Height*Bitmap->Pitch;
            FL_Assert((BitmapOffset + BitmapDataSize) == FontMap->BitmapEnd[CodePointIndex]);
            FL_Assert((Bitmap->Memory + BitmapDataSize) <= ((uint8_t *)FontMap + FontMap->FileSize));

            Result = 1;
        }
    }

    return(Result);
}

inline int32_t
GetLineAdvance(font_map *FontMap)
{
    int32_t Result = FontMap->AscentHeight + FontMap->DescentHeight + FontMap->ExternalLeadingHeight;
    return(Result);
}

inline void
GetAlignmentForForCodePoint(font_map *FontMap, uint32_t CodePoint,
                            int32_t *XOut, int32_t *YOut)
{
    fl_v2 *Alignment = FontMap->BitmapAlignmentData + CodePoint;
    *XOut = Alignment->X;
    *YOut = Alignment->Y;
}

inline fl_bitmap *
GetGlyphBitmapForCodePoint(font_map *FontMap, uint32_t CodePoint)
{
    fl_bitmap *GlyphBitmap = FontMap->BitmapData + CodePoint;
    return(GlyphBitmap);
}

inline void
GetGlyphBitmapForCodePoint(font_map *FontMap, uint32_t CodePoint,
                           int32_t *Width, int32_t *Height, int32_t *Pitch,
                           uint8_t **Memory)
{
    fl_bitmap *GlyphBitmap = FontMap->BitmapData + CodePoint;
    *Width = GlyphBitmap->Width;
    *Height = GlyphBitmap->Height;
    *Pitch = GlyphBitmap->Pitch;
    *Memory = GlyphBitmap->Memory;
}

inline int32_t
GetHorizontalAdjustmentForCodePoint(font_map *FontMap, uint32_t CodePoint)
{
    int32_t Result = 0;

    fl_bitmap *GlyphBitmap = GetGlyphBitmapForCodePoint(FontMap, CodePoint);
    int32_t Width = *(FontMap->CharacterWidth + CodePoint);

    if(GlyphBitmap->Width != Width)
    {
        float HalfFullWidth = 0.5f*(float)Width;
        float HalfGlyphWidth = 0.5f*(float)GlyphBitmap->Width;
        Result = (HalfFullWidth - HalfGlyphWidth);
    }

    return(Result);
}

inline int32_t
GetHorizontalAdvanceForCodepoint(font_map *FontMap, uint32_t CodePoint)
{
    int32_t Result = 0;

    int32_t Width = *(FontMap->CharacterWidth + CodePoint);
    if(Width >= 0)
    {
        Result = Width;
    }
    else
    {
        int32_t X = 0;
        int32_t Y = 0;
        GetAlignmentForForCodePoint(FontMap, CodePoint, &X, &Y);
        Result = X*(GetGlyphBitmapForCodePoint(FontMap, CodePoint)->Width);
    }

    return(Result);
}

#endif
