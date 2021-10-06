#include <stdio.h>
#include <stdlib.h>

#include "minesweeper_platform.h"

#include <windows.h>

struct bitmap_file_list
{
    bitmap_file *Bitmap;
    bitmap_file_list *Next;
};

struct font_file_list
{
    void *FontFile;
    u32 FileSize;
    font_file_list *Next;
};

static bitmap_file_list *
LoadBitmapFromFile(char *FileName)
{
    bitmap_file_list *Result = (bitmap_file_list *)malloc(sizeof(*Result));
    Assert(Result);
    Result->Next = 0;

    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        u32 FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        bitmap_file *FileData = (bitmap_file *)malloc(FileSize);
        Assert(FileData);

        fread(FileData, FileSize, 1, File);
        Result->Bitmap = FileData;

        fclose(File);
    }

    return(Result);
}

static font_file_list *
LoadFontFromFile(char *FileName)
{
    font_file_list *Result = (font_file_list *)malloc(sizeof(*Result));
    Assert(Result);
    Result->Next = 0;

    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        Result->FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        void *FileData = (void *)malloc(Result->FileSize);
        Assert(FileData);

        fread(FileData, Result->FileSize, 1, File);
        Result->FontFile = FileData;

        fclose(File);
    }

    return(Result);
}

int main(void)
{
    FILE *AssetHeaderFile = fopen("/minesweeper/code/minesweeper_asset_ids.h", "wb");
    Assert(AssetHeaderFile);

    // NOTE(rick): Load bitmaps from disk
    u32 BitmapFileCount = 0;
    bitmap_file_list BitmapFileListSentinel = {};

    WIN32_FIND_DATAA FindData = {};
    HANDLE FindHandle = FindFirstFileA("*.bmp", &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE)
    {
        fprintf(AssetHeaderFile, "enum bitmap_asset_id\n{\n");
        fprintf(AssetHeaderFile, "    BitmapID_None,\n\n");

        bitmap_file_list *BitmapFileListEntry = &BitmapFileListSentinel;
        BOOL FindNextIsValid = false;
        do
        {
            printf("%s\n", FindData.cFileName);

            bitmap_file_list *NewBitmapEntry = LoadBitmapFromFile(FindData.cFileName);
            if(NewBitmapEntry->Bitmap->InfoHeader.Size == 124)
            {
                BitmapFileListEntry->Next = NewBitmapEntry;
                BitmapFileListEntry = NewBitmapEntry;

                ++BitmapFileCount;

                char BitmapName[256] = {0};
                char *NameAt = BitmapName;
                for(char *At = FindData.cFileName;
                    *At;
                    ++At)
                {
                    if(((*At >= 'a') && (*At <= 'z')) ||
                       ((*At >= 'A') && (*At <= 'Z')) ||
                       ((*At >= '0') && (*At <= '9')) ||
                       (*At == '_'))
                    {
                        *NameAt++ = *At;
                    }
                    else if(*At == '.')
                    {
                        break;
                    }
                }
                BitmapName[0] &= ~0x20;

                fprintf(AssetHeaderFile, "    BitmapID_%s,\n", BitmapName);
            }
            FindNextIsValid = FindNextFile(FindHandle, &FindData);
        } while(FindNextIsValid);

        fprintf(AssetHeaderFile, "\n    BitmapID_Count,\n");
        fprintf(AssetHeaderFile, "};");
    }

    // NOTE(rick): Load fonts from disk
    u32 FontFileCount = 0;
    font_file_list FontFileListSentinel = {};

    WIN32_FIND_DATAA FindFontData = {};
    HANDLE FindFontHandle = FindFirstFileA("*.fmp", &FindFontData);
    if(FindFontHandle != INVALID_HANDLE_VALUE)
    {
        fprintf(AssetHeaderFile, "\n\nenum font_id\n{\n");
        fprintf(AssetHeaderFile, "    FontID_None,\n\n");

        font_file_list *FontFileListEntry = &FontFileListSentinel;
        BOOL FindNextIsValid = false;
        do
        {
            printf("%s\n", FindFontData.cFileName);

            font_file_list *NewFontEntry = LoadFontFromFile(FindFontData.cFileName);
            FontFileListEntry->Next = NewFontEntry;
            FontFileListEntry = NewFontEntry;

            ++FontFileCount;

            char FontName[256] = {0};
            char *NameAt = FontName;
            for(char *At = FindFontData.cFileName;
                *At;
                ++At)
            {
                if(((*At >= 'a') && (*At <= 'z')) ||
                   ((*At >= 'A') && (*At <= 'Z')) ||
                   ((*At >= '0') && (*At <= '9')) ||
                   (*At == '_'))
                {
                    *NameAt++ = *At;
                }
                else if(*At == '.')
                {
                    break;
                }
            }

            FontName[0] &= ~0x20;
            fprintf(AssetHeaderFile, "    FontID_%s,\n", FontName);

            FindNextIsValid = FindNextFile(FindFontHandle, &FindFontData);
        } while(FindNextIsValid);

        fprintf(AssetHeaderFile, "\n    FontID_Count,\n");
        fprintf(AssetHeaderFile, "};");
    }

    fclose(AssetHeaderFile);

    asset_file AssetFileHeader = {};
    AssetFileHeader.MagicNumber = FILE_MAGIC_NUMBER;
    AssetFileHeader.Version[0] = '1';  // NOTE: Major
    AssetFileHeader.Version[1] = '0';  // NOTE: Minor
    AssetFileHeader.Version[2] = '0';  // NOTE: Patch
    AssetFileHeader.BitmapCount = BitmapFileCount;
    AssetFileHeader.BitmapsOffset = sizeof(AssetFileHeader);
    AssetFileHeader.FontCount = FontFileCount;

    bitmap_file *BitmapArray = (bitmap_file *)malloc(sizeof(*BitmapArray) * BitmapFileCount);
    Assert(BitmapArray);

    FILE *OutputFile = fopen("data.mdp", "wb");
    Assert(OutputFile);

    fwrite(&AssetFileHeader, sizeof(AssetFileHeader), 1, OutputFile);
    fwrite(BitmapArray, sizeof(bitmap_file)*BitmapFileCount, 1, OutputFile);

    Assert(ftell(OutputFile) == sizeof(AssetFileHeader) + (sizeof(bitmap_file)*BitmapFileCount));

    u32 TotalImageDataBytesWritten = 0;
    u32 ExpectedImageDataBytesWritten = 0;

    bitmap_file *Bitmap = BitmapArray;
    for(bitmap_file_list *Entry = BitmapFileListSentinel.Next;
        Entry;
        Entry = Entry->Next)
    {
        *Bitmap = *(Entry->Bitmap);

        u32 Offset = ftell(OutputFile);
        Assert(Entry->Bitmap->InfoHeader.ImageSize == (Entry->Bitmap->InfoHeader.Width *
                                                       abs(Entry->Bitmap->InfoHeader.Height) * 4));
        u32 BytesWritten = fwrite((u8 *)Entry->Bitmap + Entry->Bitmap->FileHeader.DataOffset,
                                  1, Entry->Bitmap->InfoHeader.ImageSize, OutputFile);
        Bitmap->FileHeader.DataOffset = Offset;

        TotalImageDataBytesWritten += BytesWritten;
        ExpectedImageDataBytesWritten += Entry->Bitmap->InfoHeader.ImageSize;
        Assert(TotalImageDataBytesWritten == ExpectedImageDataBytesWritten);

        ++Bitmap;
    }
    Assert(TotalImageDataBytesWritten == ExpectedImageDataBytesWritten);
    
    AssetFileHeader.FontsOffset = ftell(OutputFile);

    u32 *FontOffsetsArray = (u32 *)malloc(sizeof(*FontOffsetsArray)*FontFileCount);
    u32 *FontOffsetEntry = FontOffsetsArray;
    fwrite(FontOffsetsArray, sizeof(*FontOffsetsArray)*FontFileCount, 1, OutputFile);
    for(font_file_list *Entry = FontFileListSentinel.Next;
        Entry;
        Entry = Entry->Next)
    {
        *FontOffsetEntry = ftell(OutputFile);

        fwrite(Entry->FontFile, Entry->FileSize, 1, OutputFile);

        ++FontOffsetEntry;
    }

    fseek(OutputFile, AssetFileHeader.FontsOffset, SEEK_SET);
    fwrite(FontOffsetsArray, sizeof(*FontOffsetsArray)*FontFileCount, 1, OutputFile);

    fseek(OutputFile, 0, SEEK_SET);
    fwrite(&AssetFileHeader, sizeof(AssetFileHeader), 1, OutputFile);
    fwrite(BitmapArray, sizeof(bitmap_file)*BitmapFileCount, 1, OutputFile);

    fclose(OutputFile);

    return(0);
}
