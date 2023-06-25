/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

#include <windows.h>
#include <stdio.h>


#include "create_tilemap.h"
#include "create_tilemap.cpp"

#include "win32_create_tilemap.h"

// NOTE(casey): This is a global now
global_variable b32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable i64 GlobalPerfCountFrequency;

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Height = ClientRect.bottom - ClientRect.top;
    Result.Width = ClientRect.right - ClientRect.left;

    return(Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO(casey): Bulletproof this
    // Maybe don't free first, free after, then free first if that fails

    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO(casey): Probably clear this to black

    Buffer->Pitch = Width * BytesPerPixel;

}    

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // TODO(casey): Centering / black bars??
    if((WindowWidth >= Buffer->Width * 2) &&
       (WindowHeight >= Buffer->Height * 2))
    {
        StretchDIBits(DeviceContext,
                      0, 0, Buffer->Width * 2, Buffer->Height * 2,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        int OffsetX = 0;
        int OffsetY = 0;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);
    
        // NOTE(casey): For prototyping purposes, we're going to always blit
        // 1-to-1 pixels to make sure we don't introduce artifacts with
        // stretching while we are learning to code the renderer!
        StretchDIBits(DeviceContext,
                      OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
}


internal LRESULT CALLBACK
Win32MainWindowCallBack(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam
                        )
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_DESTROY:
        {
            // TODO(casey): Handle this with a message to the user?
            GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
            // TODO(casey): Handle this as an error - recreate window?
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {

        } break;
        
        case WM_PAINT:
        {   
            
            OutputDebugStringA("WM_PAINT\n");

            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);

        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;


    }

    return(Result);
}


internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, b32 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

internal void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
    MSG Message;
                        
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;                
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {                        
                u32 VKCode = (u32)Message.wParam;
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);

                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);

                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);

                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);

                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                }

                b32  AltKeyWasDown = (Message.lParam & (1 << 29));
                if((VKCode == VK_F4) && AltKeyWasDown)
                {
                    GlobalRunning = false;
                }
                    
            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }

}
                    
inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((r32)(End.QuadPart - Start.QuadPart) /
                     (r32)GlobalPerfCountFrequency);
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // NOTE(casey): Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular.
    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
    
    WNDCLASS WindowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallBack;
    WindowClass.hInstance = Instance;
//  WindowClass.hIcon;
    WindowClass.lpszClassName = "TileMap_Editor";

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
    r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;

    if(RegisterClass(&WindowClass))
    {
        HWND Window = 
            CreateWindowEx(
                0, 
                WindowClass.lpszClassName, 
                "TileMap Editor", 
                WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
                CW_USEDEFAULT, 
                CW_USEDEFAULT, 
                CW_USEDEFAULT, 
                CW_USEDEFAULT, 
                0, 
                0, 
                Instance, 
                0);
        
        GlobalRunning = true;
        
        if(Window)
        {
            HDC DeviceContext = GetDC(Window);

            LPVOID BaseAddress = 0;
            
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);

            u64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage +
                                           GameMemory.PermanentStorageSize);

            if(GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();

                u64 LastCycleCount = __rdtsc();

                while(GlobalRunning)
                {
                    // TODO(casey): Zeroing macro
                    // TODO(casey): We can't zero everything because the up/down state will be wrong!
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);  
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);        
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }
                    
                    Win32ProcessPendingMessages(NewKeyboardController);

                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackBuffer.Memory;
                    Buffer.Width = GlobalBackBuffer.Width; 
                    Buffer.Height = GlobalBackBuffer.Height;
                    Buffer.Pitch = GlobalBackBuffer.Pitch;
                    Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

                    GameUpdateAndRender(&GameMemory, Input, &Buffer);

                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    r32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                    r32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f * (DWORD)(TargetSecondsPerFrame -
                                                                      SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                            
                        }

                        // TODO(handy): TargetSecondsPerFrame
                        //sometimes bigger than  TestSecondsElapsedForFrame??

                            
                        r32 TestSecondsElapsedForFrame =
                            (Win32GetSecondsElapsed(LastCounter, Win32GetWallClock()));
                            
                        Assert(TestSecondsElapsedForFrame < TargetSecondsPerFrame);
                        
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {

                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                            Win32GetWallClock());                        
                        }
                    }
                    else
                    {
                        // TODO(casey): MISSED FRAMERATE!
                        // TODO(casey): Logging
                    }

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    r32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                    LastCounter = EndCounter;
                
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                               Dimension.Width, Dimension.Height);
                    ReleaseDC(Window, DeviceContext);

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                                        
                    u64 EndCycleCount = __rdtsc();
                    u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;
                       
                    r64 FPS = 0.0f;
                    r64 MCPF = ((r64)CyclesElapsed / (1000.0f * 1000.0f));

                    char B[256];
                    _snprintf_s(B, sizeof(B),
                                "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringA(B);                

                }
            }
        }
        else
        {
            // TODO(casey): Logging
        }
        
    }
    else
    {
        // TODO(casey): Logging
    }
    
    return(0);
}
