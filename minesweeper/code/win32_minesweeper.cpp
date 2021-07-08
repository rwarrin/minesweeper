#include <windows.h>
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

    u32 MemorySize = Width*Height*BYTES_PER_PIXEL;
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
                        GlobalRunning = false;
                    }
                    else if(KeyCode == 'A')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_A, IsDown);
                    }
                    else if(KeyCode == 'D')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_D, IsDown);
                    }
                    else if(KeyCode == 'S')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_S, IsDown);
                    }
                    else if(KeyCode == 'W')
                    {
                        Win32ProcessKeyboardInput(&Input->Keyboard_W, IsDown);
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

int
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
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

    AppMemory.PermanentStorageSize = Megabytes(10);
    AppMemory.PermanentStorage = VirtualAlloc((void *)Terabytes(2), AppMemory.PermanentStorageSize,
                                              MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    AppMemory.TransientStorageSize = Megabytes(1);
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
            DebugWriteLine("%02.2f FPS (%02.2f ms/f)\n", FPS, MSElapsed);

            QueryPerformanceCounter(&LastTime);
        }
#else
        LastTime = EndTime;
#endif
    }

    return(0);
}
