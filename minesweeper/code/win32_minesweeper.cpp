#include <windows.h>
#include <wincrypt.h>
#include <intrin.h>
#include <stdio.h>

#include "minesweeper_platform.h"
#include "win32_minesweeper.h"

static b32 GlobalRunning;
static win32_back_buffer GlobalBackBuffer;

inline void
Win32ProcessKeyboardInput(button_state *ButtonState, b32 IsDown)
{
    if(ButtonState->IsDown != IsDown)
    {
        ButtonState->IsDown = IsDown;
        ++ButtonState->TransitionCount;
    }
}

inline app_code
Win32LoadAppCode(char *SourceName, char *TempName, char *LockName)
{
    app_code Result = {};
    WIN32_FILE_ATTRIBUTE_DATA NotUsed;
    if(!GetFileAttributesEx(LockName, GetFileExInfoStandard, &NotUsed))
    {
        WIN32_FILE_ATTRIBUTE_DATA SourceDLLData;
        if(GetFileAttributesEx(SourceName, GetFileExInfoStandard, &SourceDLLData))
        {
            Result.LastFileTime = SourceDLLData.ftLastWriteTime;

            CopyFile(SourceName, TempName, FALSE);

            Result.AppCodeDLL = LoadLibraryA(TempName);
            if(Result.AppCodeDLL)
            {
                Result.UpdateAndRender = (update_and_render *)GetProcAddress(Result.AppCodeDLL, "UpdateAndRender");

                Result.IsValid = (Result.UpdateAndRender != 0);
            }
        }
    }

    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
    }

    return(Result);
}

inline void
Win32UnloadAppCode(app_code *AppCode)
{
    if(AppCode->AppCodeDLL)
    {
        FreeLibrary(AppCode->AppCodeDLL);
        AppCode->AppCodeDLL = 0;
    }

    AppCode->IsValid = 0;
    AppCode->UpdateAndRender = 0;
}

inline win32_window_dims
Win32GetWindowDims(HWND Window)
{
    win32_window_dims Result = {};
    RECT ClientRect = {};
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return(Result);
}

inline void
Win32ResizeDIBSection(win32_back_buffer *BackBuffer, u32 Width, u32 Height)
{
    if(BackBuffer->Bitmap.Data)
    {
        VirtualFree(BackBuffer->Bitmap.Data, 0, MEM_RELEASE);
    }

    BackBuffer->BitmapFile.InfoHeader.Size = sizeof(BackBuffer->BitmapFile.InfoHeader);
    BackBuffer->BitmapFile.InfoHeader.Width = Width;
    BackBuffer->BitmapFile.InfoHeader.Height = -Height;
    BackBuffer->BitmapFile.InfoHeader.Planes = 1;
    BackBuffer->BitmapFile.InfoHeader.BitsPerPixel = 32;
    BackBuffer->BitmapFile.InfoHeader.Compression = BI_RGB;

    u32 BYTES_PER_PIXEL = 4;
    BackBuffer->Bitmap.Width = Width;
    BackBuffer->Bitmap.Height = Height;
    BackBuffer->Bitmap.Pitch = Width*BYTES_PER_PIXEL;

    u32 MemorySize = (Width*Height*BYTES_PER_PIXEL) + 1;
    BackBuffer->Bitmap.Data = (u8 *)VirtualAlloc(0, MemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    Assert(BackBuffer->Bitmap.Data);
}

inline void
Win32DisplayBufferInWindow(win32_back_buffer *BackBuffer, HDC DeviceContext)
{
    StretchDIBits(DeviceContext,
                  0, 0, BackBuffer->Bitmap.Width, BackBuffer->Bitmap.Height,
                  0, 0, BackBuffer->Bitmap.Width, BackBuffer->Bitmap.Height,
                  (void *)BackBuffer->Bitmap.Data, (BITMAPINFO *)&BackBuffer->BitmapFile.InfoHeader,
                  DIB_RGB_COLORS, SRCCOPY);
}

static
PLATFORM_FREE_MEMORY(Win32FreeMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

static
PLATFORM_READ_FILE(Win32ReadFile)
{
    file_result Result = {};

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize = {};
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 Size = (u32)FileSize.LowPart;
            Result.Contents = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead = 0;
                if(ReadFile(FileHandle, Result.Contents, Size, &BytesRead, 0) && (Size == BytesRead))
                {
                    Result.FileSize = Size;
                }
                else
                {
                    Win32FreeMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
        }

        CloseHandle(FileHandle);
    }

    return(Result);
}

static
PLATFORM_GET_MS_ELAPSED64(Win32GetMSElapsed64)
{
    u64 Result = GetTickCount64();
    return(Result);
}

static
PLATFORM_WRITE_FILE(Win32WriteFile)
{
    b32 Result = false;

    HANDLE FileHandle = CreateFileA(FileName, FILE_APPEND_DATA, 0, 0, OPEN_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD NumBytesWritten = 0;
        if(WriteFile(FileHandle, Data, DataLength, &NumBytesWritten, 0))
        {
            Result = true;
            if(BytesWritten)
            {
                *BytesWritten = NumBytesWritten;
            }
        }

        CloseHandle(FileHandle);
    }

    if(!Result && BytesWritten)
    {
        *BytesWritten = 0;
    }

    return(Result);
}

static
PLATFORM_GET_DATETIME(Win32GetDateTime)
{
    platform_datetime_result Result = {};

    SYSTEMTIME LocalTime;
    GetLocalTime(&LocalTime);

    Result.Year = LocalTime.wYear;
    Result.Month = LocalTime.wMonth;
    Result.Day = LocalTime.wDay;
    Result.Hour = LocalTime.wHour;
    Result.Minute = LocalTime.wMinute;
    Result.Second = LocalTime.wSecond;

    return(Result);
}

static
PLATFORM_REQUEST_EXIT(RequestExit)
{
    GlobalRunning = false;
    PostQuitMessage(0);
}

static
PLATFORM_ENUMERATE_FILES(Win32EnumerateFiles)
{
    platform_file_enumeration_result *Result =
        (platform_file_enumeration_result *)VirtualAlloc(0, sizeof(*Result), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    platform_file_enumeration_result *CurrentBucket = Result;
    WIN32_FIND_DATA FindData = {};
    HANDLE FindFileHandle = FindFirstFileA(Pattern, &FindData);
    if(FindFileHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if((CurrentBucket->Used + 1) > ArrayCount(CurrentBucket->Items))
            {
                platform_file_enumeration_result *NewBucket =
                    (platform_file_enumeration_result *)VirtualAlloc(0, sizeof(*Result), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                NewBucket->Prev = CurrentBucket;
                CurrentBucket->Next = NewBucket;
                CurrentBucket = NewBucket;
            }

            CopyMemory(CurrentBucket->Items[CurrentBucket->Used++],
                       FindData.cFileName, strlen(FindData.cFileName));
        } while(FindNextFileA(FindFileHandle, &FindData));
    }

    return(Result);
}

static
PLATFORM_FREE_ENUMERATED_FILE_RESULT(Win32FreeEnumeratedFileResult)
{
    while(Data)
    {
        Data = Data->Next;
        VirtualFree(Data, 0, MEM_RELEASE);
    }
}

static
PLATFORM_RANDOM_NUMBER(Win32GenerateRandomNumber)
{
    u32 Result = 0;

    HCRYPTPROV CryptoServiceProvider = {};
    BOOL GotCryptoProvider = CryptAcquireContextA(&CryptoServiceProvider, 0, 0, PROV_RSA_FULL, 0);
    if(GotCryptoProvider)
    {
        u8 Buffer[4] = {0};
        BOOL GotRandom = CryptGenRandom(CryptoServiceProvider, ArrayCount(Buffer), Buffer);
        if(GotRandom)
        {
            Result = *(u32 *)Buffer;
        }

        CryptReleaseContext(CryptoServiceProvider, 0);
    }

    return(Result);
}

static void
ProcessPendingMessages(input *Input)
{
    MSG Message;
    for(;;)
    {
        BOOL GotMessage = PeekMessage(&Message, 0, 0, 0, PM_REMOVE);
        if(!GotMessage)
        {
            break;
        }

        switch(Message.message)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            {
                u32 KeyCode = (u32)Message.wParam;
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);

                if(IsDown != WasDown)
                {
                    if(KeyCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_Escape, IsDown);
                    }
                    else if(KeyCode == 'A')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_A, IsDown);
                    }
                    else if(KeyCode == 'D')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_D, IsDown);
                    }
                    else if(KeyCode == 'N')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_N, IsDown);
                    }
                    else if(KeyCode == 'S')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_S, IsDown);
                    }
                    else if(KeyCode == 'W')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_W, IsDown);
                    }
                    else if(KeyCode == 'X')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_X, IsDown);
                    }
                    else if(KeyCode == 'Z')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_Z, IsDown);
                    }
                    else if(KeyCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_Space, IsDown);
                    }
                    else if(KeyCode == VK_RETURN)
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_Return, IsDown);
                    }
                    else if(KeyCode === VK_LEFT)
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_ArrowLeft, IsDown);
                    }
                }
            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

LRESULT CALLBACK
Win32WindowsCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        case WM_CLOSE:
        {
            GlobalRunning = 0;
        } break;
        case WM_SIZE:
        {
            win32_window_dims Dims = Win32GetWindowDims(Window);
            Win32ResizeDIBSection(&GlobalBackBuffer, Dims.Width, Dims.Height);
        } break;
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

struct win32_thread_info
{
    u32 ID;
};

struct work_queue_entry
{
    platform_work_callback *Proc;
    void *Data;
};

struct platform_work_queue
{
    u32 volatile EntryCount;
    u32 volatile NextEntryToDo;
    u32 volatile EntryCompletionCount;
    HANDLE SemaphoreHandle;
    work_queue_entry Entries[256];
};

static PLATFORM_ADD_WORK_TO_QUEUE(Win32AddWorkToQueue)
{
    Assert(WorkQueue->EntryCount < ArrayCount(WorkQueue->Entries));

    work_queue_entry *Entry = WorkQueue->Entries + WorkQueue->EntryCount;
    Entry->Proc = Proc;
    Entry->Data = Data;

    WriteBarrier;
    ++WorkQueue->EntryCount;
    ReleaseSemaphore(WorkQueue->SemaphoreHandle, 1, 0);
}

inline b32
DoWorkerWork(platform_work_queue *WorkQueue)
{
    b32 DidWork = false;

    u32 OriginalNextEntryToDo = WorkQueue->NextEntryToDo;
    if(WorkQueue->NextEntryToDo < WorkQueue->EntryCount)
    {
        u32 EntryIndex = InterlockedCompareExchange((LONG volatile *)&WorkQueue->NextEntryToDo,
                                                    OriginalNextEntryToDo + 1,
                                                    OriginalNextEntryToDo);
        if(EntryIndex == OriginalNextEntryToDo)
        {
            ReadBarrier;
            work_queue_entry *Entry = WorkQueue->Entries + EntryIndex;
            Entry->Proc(Entry->Data);

            InterlockedIncrement((LONG volatile *)&WorkQueue->EntryCompletionCount);
            DidWork = true;
        }
    }

    return(DidWork);
}

static void
ClearWorkQueue(platform_work_queue *WorkQueue)
{
    WorkQueue->EntryCount = 0;
    WorkQueue->EntryCompletionCount = 0;
    WorkQueue->NextEntryToDo = 0;
}

static PLATFORM_COMPLETE_ALL_WORK(Win32CompleteAllWorkQueueWork)
{
    while(WorkQueue->EntryCompletionCount < WorkQueue->EntryCount)
    {
        DoWorkerWork(WorkQueue);
    }

    ClearWorkQueue(WorkQueue);
}

DWORD
ThreadProc(void *Data)
{
    platform_work_queue *WorkQueue = (platform_work_queue *)Data;

    for(;;)
    {
        if(!DoWorkerWork(WorkQueue))
        {
            WaitForSingleObjectEx(WorkQueue->SemaphoreHandle, INFINITE, FALSE);
        }
    }

    return(0);
}

static void
StringWorkProc(void *Data)
{
    char *String = (char *)Data;
    OutputDebugStringA(String);
}

int
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    u32 InitialCount = 0;
    u32 ThreadCount = 11;
    HANDLE SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    platform_work_queue WorkQueue = {};
    WorkQueue.SemaphoreHandle = SemaphoreHandle;

    for(u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
    {
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, &WorkQueue, 0, 0);
        CloseHandle(ThreadHandle);
    }

    WNDCLASSEXA WindowClass = {};
    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WindowsCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "minesweeper";

    if(!RegisterClassExA(&WindowClass))
    {
        MessageBox(0, "Failed to register window class.", "Error", MB_OK);
        return(1);
    }

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Minesweeper",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  480, 640,
                                  0, 0, Instance, 0);
    if(!Window)
    {
        MessageBox(0, "Failed to create window.", "Error", MB_OK);
        return(2);
    }

    win32_window_dims WindowDims = Win32GetWindowDims(Window);
    Win32ResizeDIBSection(&GlobalBackBuffer, WindowDims.Width, WindowDims.Height);

    if(timeBeginPeriod(1) == TIMERR_NOCANDO)
    {
        DebugWriteLine("Failed to get high resolution timer.\n");
    }

    f32 TargetFPS = 60.0f;
    f32 dt = 1.0 / TargetFPS;
    f32 TargetSecondsPerFrame = dt;
    f32 TargetMSPerFrame = TargetSecondsPerFrame*1000.0f;

    app_code AppCode = {};
    input Input = {};
    Input.dt = dt;

    app_memory AppMemory = {};
    AppMemory.PlatformAPI.ReadFile = Win32ReadFile;
    AppMemory.PlatformAPI.FreeMemory = Win32FreeMemory;
    AppMemory.PlatformAPI.GetMSElapsed64 = Win32GetMSElapsed64;
    AppMemory.PlatformAPI.WriteFile = Win32WriteFile;
    AppMemory.PlatformAPI.GetDateTime = Win32GetDateTime;
    AppMemory.PlatformAPI.RequestExit = RequestExit;
    AppMemory.PlatformAPI.EnumerateFiles = Win32EnumerateFiles;
    AppMemory.PlatformAPI.FreeEnumeratedFileResult = Win32FreeEnumeratedFileResult;
    AppMemory.PlatformAPI.GenerateRandomNumber = Win32GenerateRandomNumber;
    AppMemory.PlatformAPI.AddWorkToQueue = Win32AddWorkToQueue;
    AppMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWorkQueueWork;
    AppMemory.PlatformAPI.WorkQueue = &WorkQueue;

    AppMemory.PermanentStorageSize = Megabytes(16);
    AppMemory.PermanentStorage = VirtualAlloc((void *)Terabytes(2), AppMemory.PermanentStorageSize,
                                              MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    AppMemory.TransientStorageSize = Megabytes(8);
    AppMemory.TransientStorage = VirtualAlloc((void *)Terabytes(3), AppMemory.TransientStorageSize,
                                              MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    AppMemory.FrameStorageSize = Kilobytes(64);
    AppMemory.FrameStorage = VirtualAlloc((void *)Terabytes(4), AppMemory.FrameStorageSize,
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    LARGE_INTEGER CPUFrequency; QueryPerformanceFrequency(&CPUFrequency);
    LARGE_INTEGER LastTime; QueryPerformanceCounter(&LastTime);
    GlobalRunning = true;
    while(GlobalRunning)
    {
        WIN32_FILE_ATTRIBUTE_DATA DLLFileAttributes;
        if(GetFileAttributesEx("../../build/minesweeper.dll", GetFileExInfoStandard, &DLLFileAttributes))
        {
            FILETIME LastFileTime = DLLFileAttributes.ftLastWriteTime;
            if(CompareFileTime(&LastFileTime, &AppCode.LastFileTime))
            {
                Win32UnloadAppCode(&AppCode);
                for(u32 RetryCount = 0; !AppCode.IsValid && (RetryCount < 100); ++RetryCount)
                {
                    AppCode = Win32LoadAppCode("../../build/minesweeper.dll",
                                               "../../build/minesweeper_temp.dll",
                                               "../../build/lock.tmp");
                    Sleep(10);
                }
            }
        }

        {
            POINT MousePoint = {};
            GetCursorPos(&MousePoint);
            ScreenToClient(Window, &MousePoint);
            Input.MouseX = MousePoint.x;
            Input.MouseY = MousePoint.y;
            Input.MouseZ = 0;
            for(u32 ButtonIndex = 0; ButtonIndex < ArrayCount(Input.Mouse); ++ButtonIndex)
            {
                button_state *Button = Input.Mouse + ButtonIndex;
                Button->TransitionCount = 0;
                Win32ProcessKeyboardInput(Button, (GetKeyState(ButtonIndex + 1) & (1 << 15)));
            }
        }

        {
            for(button_state *Key = &Input.Keyboard[0];
                Key != &Input.Keyboard_DoNotUse;
                ++Key)
            {
                Key->TransitionCount = 0;
            }
        }

        ProcessPendingMessages(&Input);

        if(AppCode.UpdateAndRender)
        {
            AppCode.UpdateAndRender(&AppMemory, &Input, &GlobalBackBuffer.Bitmap);
        }

        HDC DeviceContext = GetDC(Window);
        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext);
        ReleaseDC(Window, DeviceContext);

        LARGE_INTEGER EndTime; QueryPerformanceCounter(&EndTime);
        f32 MSElapsed = ((f32)(EndTime.QuadPart - LastTime.QuadPart) / (f32)CPUFrequency.QuadPart) * 1000.0f;
        s32 MSToSleep = TargetMSPerFrame - MSElapsed;
        if(MSToSleep > 0)
        {
            Sleep(MSToSleep);
        }

#if 1
        {
            LARGE_INTEGER FinalTime; QueryPerformanceCounter(&FinalTime);
            f32 TotalMSElapsed = ((f32)(FinalTime.QuadPart - LastTime.QuadPart) / (f32)(CPUFrequency.QuadPart)) * 1000.0f;
            f32 FPS = 1000.0f / TotalMSElapsed;
            AppMemory.PlatformAPI.FPS = FPS;
            AppMemory.PlatformAPI.FrameTimeActual = MSElapsed;
            AppMemory.PlatformAPI.FrameTimeTotal = TotalMSElapsed;
            AppMemory.PlatformAPI.FrameTimeSlept = MSToSleep > 0 ? MSToSleep : 0;
            //DebugWriteLine("%02.2f FPS (%02.2f ms/f %0dms slept)\n", FPS, MSElapsed, MSToSleep > 0 ? MSToSleep : 0);

            QueryPerformanceCounter(&LastTime);
        }
#else
        LastTime = EndTime;
#endif
    }

    return(0);
}
