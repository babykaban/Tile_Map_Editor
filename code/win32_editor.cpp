/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   ======================================================================== */

/*
  TODO(casey):  THIS IS NOT A FINAL PLATFORM LAYER!!!

  - Make the right calls so Windows doesn't think we are "still loading" for a bit
    after we actually start
  - Saved game locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Threading (launch a thread)
  - Raw Input (support for multiple keyboards)
  - ClipCursor() (for multimonitor support)
  - QueryCancelAutoplay
  - WM_ACTIVATEAPP (for when we are not the active application)
  - Blit speed improvements (BitBlt)
  - Hardware acceleration (OpenGL or Direct3D or BOTH??)
  - GetKeyboardLayout (for French keyboards, international WASD support)
  - ChangeDisplaySettings option if we detect slow fullscreen blit??
  
   Just a partial list of stuff!!
*/

#include "editor_platform.h"
#include "editor_shared.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <gl/gl.h>

#include "win32_editor.h"

global_variable platform_api Platform;

enum win32_rendering_type
{
    Win32RenderType_RenderOpenGL_DisplayOpenGL,
    Win32RenderType_RenderSoftware_DisplayOpenGL,
    Win32RenderType_RenderSoftware_DisplayGDI,
};

global_variable win32_rendering_type GlobalRenderingType;
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable bool32 GlobalAppIsActive;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable bool32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};
global_variable GLuint GlobalBlitTextureHandle;

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB        0x20A9

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext,
    const int *attribList);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_iv_arb(HDC hdc,
    int iPixelFormat,
    int iLayerPlane,
    UINT nAttributes,
    const int *piAttributes,
    int *piValues);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_fv_arb(HDC hdc,
    int iPixelFormat,
    int iLayerPlane,
    UINT nAttributes,
    const int *piAttributes,
    FLOAT *pfValues);

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc,
    const int *piAttribIList,
    const FLOAT *pfAttribFList,
    UINT nMaxFormats,
    int *piFormats,
    UINT *nNumFormats);

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
typedef const char * WINAPI wgl_get_extensions_string_ext(void);

global_variable wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
global_variable wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
global_variable wgl_swap_interval_ext *wglSwapIntervalEXT;
global_variable wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;
global_variable b32 OpenGLSupportsSRGBFramebuffer;
global_variable GLuint OpenGLDefaultInternalTextureFormat;
global_variable GLuint OpenGLReservedBlitTexture;

#include "editor_sort.cpp"
#include "editor_opengl.cpp"
#include "editor_render.cpp"

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    // TODO(casey): Dest bounds checking!
    
    for(int Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(int Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

internal void
Win32GetEXEFileName(win32_state *State)
{
    // NOTE(casey): Never use MAX_PATH in code that is user-facing, because it
    // can be dangerous and lead to bad results.
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for(char *Scan = State->EXEFileName;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

internal int
StringLength(char *String)
{
    int Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return(Count);
}

internal void
Win32BuildEXEPathFileName(win32_state *State, char *FileName,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
               StringLength(FileName), FileName,
               DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // NOTE(casey): File read successfully
                    Result.ContentsSize = FileSize32;
                }
                else
                {                    
                    // TODO(casey): Logging
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
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

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = false;
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // NOTE(casey): File read successfully
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(Result);
}

inline FILETIME
Win32GetLastWriteTime(char *FileName)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(FileName, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return(LastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFileName)
{
    win32_game_code Result = {};


    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, FALSE);
    
        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if(Result.GameCodeDLL)
        {
            Result.UpdateAndRender = (game_update_and_render *)
                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.IsValid = true;
        }
    }
    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
    }

    return(Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
}

int Win32OpenGLAttribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(casey): Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if EDITOR_INTERNAL
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};

internal win32_thread_startup
Win32GetThreadStartupForGL(HDC OpenGLDC, HGLRC ShareContext)
{
    win32_thread_startup Result = {};

    Result.OpenGLDC = OpenGLDC;
    if(wglCreateContextAttribsARB)
    {
        Result.OpenGLRC = wglCreateContextAttribsARB(OpenGLDC, ShareContext, Win32OpenGLAttribs);
    }

    return(Result);
}

internal void
Win32SetPixelFormat(HDC WindowDC)
{
    int SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;
    if(wglChoosePixelFormatARB)
    {
        int IntAttribList[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0,
        };

        if(!OpenGLSupportsSRGBFramebuffer)
        {
            IntAttribList[10] = 0;
        }

        wglChoosePixelFormatARB(WindowDC, IntAttribList, 0, 1, 
            &SuggestedPixelFormatIndex, &ExtendedPick);
    }

    if(!ExtendedPick)
    {
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }

    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

internal void
Win32LoadWGLExtensions(void)
{
    WNDCLASSA WindowClass = {};

    WindowClass.lpfnWndProc = DefWindowProcA;
    WindowClass.hInstance = GetModuleHandle(0);
    WindowClass.lpszClassName = "EditorWGLLoader";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Editor Saga",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            WindowClass.hInstance,
            0);

        HDC WindowDC = GetDC(Window);
        Win32SetPixelFormat(WindowDC);
        HGLRC OpenGLRC = wglCreateContext(WindowDC);
        if(wglMakeCurrent(WindowDC, OpenGLRC))        
        {
            wglChoosePixelFormatARB = 
                (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB =
               (wgl_create_context_attribs_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
            wglSwapIntervalEXT = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)wglGetProcAddress("wglGetExtensionsStringEXT");

            if(wglGetExtensionsStringEXT)
            {
                char *Extensions = (char *)wglGetExtensionsStringEXT();
                char *At = Extensions;
                while(*At)
                {
                    while(IsWhitespace(*At)) {++At;}
                    char *End = At;
                    while(*End && !IsWhitespace(*End)) {++End;}

                    umm Count = End - At;        

                    if(0) {}
                    else if(StringsAreEqual(Count, At, "WGL_EXT_framebuffer_sRGB")) {OpenGLSupportsSRGBFramebuffer = true;}

                    At = End;
                }
            }

            wglMakeCurrent(0, 0);
        }

        wglDeleteContext(OpenGLRC);
        ReleaseDC(Window, WindowDC);
        DestroyWindow(Window);
    }
}

internal HGLRC
Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();

    b32 ModernContext = true;
    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB)
    {
        Win32SetPixelFormat(WindowDC);
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }

    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC = wglCreateContext(WindowDC);
    }

    if(wglMakeCurrent(WindowDC, OpenGLRC))
    {
        OpenGLInit(ModernContext, OpenGLSupportsSRGBFramebuffer);
        if(wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(1);
        }

        glGenTextures(1, &OpenGLReservedBlitTexture);
    }

    return(OpenGLRC);
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    // NOTE(casey): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE(casey): Thank you to Chris Hecker of Spy Party fame
    // for clarifying the deal with StretchDIBits and BitBlt!
    // No more DC for us.
    Buffer->Pitch = Align16(Width*BytesPerPixel);
    int BitmapMemorySize = (Buffer->Pitch*Buffer->Height);
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    // NOTE(casey): VirtualAlloc should _only_ have given us back
    // zero'd memory which is all black, so we don't need to clear it.
}

internal void
Win32DisplayBufferInWindow(platform_work_queue *RenderQueue, game_render_commands *Commands,
                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight, 
                           void *SortMemory, void *ClipRectMemory)
{
    SortEntries(Commands, SortMemory);
    LinearizeClipRects(Commands, ClipRectMemory);

    if(GlobalRenderingType == Win32RenderType_RenderOpenGL_DisplayOpenGL)
    {
        OpenGLRenderCommands(Commands, WindowWidth, WindowHeight);        
        SwapBuffers(DeviceContext);
    }
    else
    {
        loaded_bitmap OutputTarget;
        OutputTarget.Memory = GlobalBackbuffer.Memory;
        OutputTarget.Width = GlobalBackbuffer.Width;
        OutputTarget.Height = GlobalBackbuffer.Height;
        OutputTarget.Pitch = GlobalBackbuffer.Pitch;

        SoftwareRenderCommands(RenderQueue, Commands, &OutputTarget);        

        if(GlobalRenderingType == Win32RenderType_RenderSoftware_DisplayOpenGL)
        {
            OpenGLDisplayBitmap(GlobalBackbuffer.Width, GlobalBackbuffer.Height, GlobalBackbuffer.Memory,
                                GlobalBackbuffer.Pitch, WindowWidth, WindowHeight,
                                OpenGLReservedBlitTexture);
            SwapBuffers(DeviceContext);
        }
        else
        {
            Assert(GlobalRenderingType == Win32RenderType_RenderSoftware_DisplayGDI);

            if((WindowWidth >= GlobalBackbuffer.Width*2) &&
               (WindowHeight >= GlobalBackbuffer.Height*2))
            {
                StretchDIBits(DeviceContext,
                              0, 0, 2*GlobalBackbuffer.Width, 2*GlobalBackbuffer.Height,
                              0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                              GlobalBackbuffer.Memory,
                              &GlobalBackbuffer.Info,
                              DIB_RGB_COLORS, SRCCOPY);
            }
            else
            {
                int OffsetX = 0;
                int OffsetY = 0;

                // NOTE(casey): For prototyping purposes, we're going to always blit
                // 1-to-1 pixels to make sure we don't introduce artifacts with
                // stretching while we are learning to code the renderer!
                StretchDIBits(DeviceContext,
                              OffsetX, OffsetY, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                              0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                              GlobalBackbuffer.Memory,
                              &GlobalBackbuffer.Info,
                              DIB_RGB_COLORS, SRCCOPY);
            }
        }
    }
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{       
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        {
            // TODO(casey): Handle this with a message to the user?
            GlobalRunning = false;
        } break;

        case WM_SETCURSOR:
        {
            if(DEBUGGlobalShowCursor)
            {
                Result = DefWindowProcA(Window, Message, WParam, LParam);
            }
            else
            {
                SetCursor(0);
            }

        } break;
        
        case WM_ACTIVATEAPP:
        {
            GlobalAppIsActive = (b32)WParam;
#if 0
            if(WParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
#endif
        } break;

        case WM_DESTROY:
        {
            // TODO(casey): Handle this as an error - recreate window?
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
//            OutputDebugStringA("default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

internal void
Win32GetInputFileLocation(win32_state *State, bool32 InputStream,
                          int SlotIndex, int DestCount, char *Dest)
{
    char Temp[64];
    wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
    Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal void
ToggleFullscreen(HWND Window)
{
    // NOTE(casey): This follows Raymond Chen's prescription
    // for fullscreen toggling, see:
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY),
                           &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown, bool32 Reset = false)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->Reset = Reset;
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController, s16 *MouseRotated)
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

            case WM_MOUSEWHEEL:
            {
                *MouseRotated = (s16)(Message.wParam >> 16);
            } break;
            
            case WM_SYSKEYUP:
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)Message.wParam;

                // NOTE(casey): Since we are comparing WasDown to IsDown,
                // we MUST use == and != to convert these bit tests to actual
                // 0 or 1 values.
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown, true);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown, true);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown, true);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown, true);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if(VKCode == '1')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Biome, IsDown, true);
                    }
                    else if(VKCode == '2')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Type, IsDown, true);
                    }
                    else if(VKCode == '3')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Height, IsDown, true);
                    }
                    else if(VKCode == '4')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->CliffHillType, IsDown, true);
                    }
                    else if(VKCode == '5')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MainSurface, IsDown, true);
                    }
                    else if(VKCode == '6')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MergeSurface, IsDown, true);
                    }
                    else if(VKCode == 'M')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ChangeEditMode, IsDown, true);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown, true);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown, true);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown, true);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown, true);
                        OutputDebugStringA("Pressed\n");
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
                    else if(VKCode == VK_RETURN)
                    {
//                        Win32ProcessKeyboardMessage(&KeyboardController->ChangeTile, IsDown, true);
                    }
#if EDITOR_INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if(VKCode == 'L')
                    {
                    }
#endif
                    if(IsDown)
                    {
                        bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
                        if((VKCode == VK_F4) && AltKeyWasDown)
                        {
                            GlobalRunning = false;
                        }
                    
                        if((VKCode == VK_RETURN) && AltKeyWasDown)
                        {
                            if(Message.hwnd)
                            {
                                ToggleFullscreen(Message.hwnd);
                            }
                        }
                    }
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

internal void
Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
{
    // TODO(casey): Switch to InterlockedCompareExchange eventually
    // so that any thread can add?
    uint32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal bool32
Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    bool32 WeShouldSleep = false;

    uint32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    uint32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                                  NewNextEntryToRead,
                                                  OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }

    return(WeShouldSleep);
}

internal void
Win32CompleteAllWork(platform_work_queue *Queue)
{
    while(Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    win32_thread_startup *Thread = (win32_thread_startup *)lpParameter;
    platform_work_queue *Queue = Thread->Queue;

    u32 TestThreadID = GetThreadID();
    Assert(TestThreadID == GetCurrentThreadId());

    if(Thread->OpenGLRC)
    {
        wglMakeCurrent(Thread->OpenGLDC, Thread->OpenGLRC);
    }

    for(;;)
    {
        if(Win32DoNextWorkQueueEntry(Queue))
        {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }

//    return(0);
}

internal PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork)
{
    char Buffer[256];
    wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char *)Data);
    OutputDebugStringA(Buffer);
}

internal void
Win32MakeQueue(platform_work_queue *Queue, uint32 ThreadCount, win32_thread_startup *Startups)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;

    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;

    uint32 InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreEx(0,
                                               InitialCount,
                                               ThreadCount,
                                               0, 0, SEMAPHORE_ALL_ACCESS);
    for(uint32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        win32_thread_startup *Startup = Startups + ThreadIndex;
        Startup->Queue = Queue;

        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Startup, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}

struct win32_platform_file_handle
{
    HANDLE Win32Handle;
};

struct win32_platform_file_group
{
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;    
};

internal PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
    platform_file_group Result = {};
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(
        0, sizeof(win32_platform_file_group),
        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Result.Platform = Win32FileGroup;

    wchar_t *WildCard = L"*.*";
    switch(Type)
    {
        case PlatformFileType_AssetFile:
        {
            WildCard = L"*.ssa";
        } break;

        case PlatformFileType_SavedGameFile:
        {
            WildCard = L"*.hhs";
        } break;

        InvalidDefaultCase;
    }
    
    Result.FileCount = 0;

    WIN32_FIND_DATAW FindData;
    HANDLE FindHandle = FindFirstFileW(WildCard, &FindData);
    while(FindHandle != INVALID_HANDLE_VALUE)
    {
        ++Result.FileCount;

        if(!FindNextFileW(FindHandle, &FindData))
        {
            break;
        }
    }

    FindClose(FindHandle);

    Win32FileGroup->FindHandle = FindFirstFileW(WildCard, &Win32FileGroup->FindData);
    
    return(Result);
}

internal PLATFORM_GET_ALL_FILES_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup->Platform;
    if(Win32FileGroup)
    {
        FindClose(Win32FileGroup->FindHandle);

        VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
    }
}

internal PLATFORM_OPEN_NEXT_FILE(Win32OpenNextFile)
{
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup->Platform;
    platform_file_handle Result = {};

    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
    {
        // TODO(casey): If we want, someday, make an actual arena used by Win32
        win32_platform_file_handle *Win32Handle = (win32_platform_file_handle *)VirtualAlloc(
            0, sizeof(win32_platform_file_handle),
            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        Result.Platform = Win32Handle;
        
        if(Win32Handle)
        {
            wchar_t *FileName = Win32FileGroup->FindData.cFileName;
            Win32Handle->Win32Handle = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            Result.NoErrors = (Win32Handle->Win32Handle != INVALID_HANDLE_VALUE);
        }

        if(!FindNextFileW(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
        {
            FindClose(Win32FileGroup->FindHandle);
            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
        }
    }

    return(Result);
}

internal PLATFORM_FILE_ERROR(Win32FileError)
{
#if HANDMADE_INTERNAL
    OutputDebugStringA("WIN32 FILE ERROR: ");
    OutputDebugStringA(Message);
    OutputDebugStringA("\n");
#endif

    Handle->NoErrors = false;
}

internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
    if(PlatformNoFileErrors(Source))
    {
        win32_platform_file_handle *Handle = (win32_platform_file_handle *)Source->Platform;

        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);

        uint32 FileSize32 = SafeTruncateUInt64(Size);

        DWORD BytesRead;
        if(ReadFile(Handle->Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped) &&
           (FileSize32 == BytesRead))
        {
            // NOTE(casey): File read successfully
        }
        else
        {                    
            Win32FileError(Source, "Read file failed.");
        }
    }
}

internal PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    return(Result);
}

internal PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    VirtualFree(Memory, 0, MEM_RELEASE);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    DEBUGGlobalShowCursor = true;

    win32_state Win32State = {};
    
    Win32GetEXEFileName(&Win32State);

    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "editor.dll",
                              sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "editor_temp.dll",
                              sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "lock.tmp",
                              sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

    WNDCLASSA WindowClass = {};

    /* NOTE(casey): 1080p display mode is 1920x1080 -> Half of that is 960x540
       1920 -> 2048 = 2048 - 1920 -> 128 pixels
       1080 -> 2048 = 2048 - 1080 -> 968 pixels
       1024 + 128 = 1152
    */
    int ScreenDimX = GetSystemMetrics(SM_CXSCREEN);
    int ScreenDimY = GetSystemMetrics(SM_CYSCREEN);
    
    Win32ResizeDIBSection(&GlobalBackbuffer, ScreenDimX, ScreenDimY);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "EditorWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0, // WS_EX_TOPMOST|WS_EX_LAYERED,
                WindowClass.lpszClassName,
                "Editor",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(Window)
        {
            ToggleFullscreen(Window);
            HDC OpenGLDC = GetDC(Window);
            HGLRC OpenGLRC = 0;

            OpenGLRC = Win32InitOpenGL(OpenGLDC);

            win32_thread_startup HighPriStartups[3] = {};
            platform_work_queue HighPriorityQueue = {};
            Win32MakeQueue(&HighPriorityQueue, ArrayCount(HighPriStartups), HighPriStartups);

            win32_thread_startup LowPriStartups[2] = {};
            LowPriStartups[0] = Win32GetThreadStartupForGL(OpenGLDC, OpenGLRC);
            LowPriStartups[1] = Win32GetThreadStartupForGL(OpenGLDC, OpenGLRC);
            platform_work_queue LowPriorityQueue = {};
            Win32MakeQueue(&LowPriorityQueue, ArrayCount(LowPriStartups), LowPriStartups);
            
            // TODO(casey): How do we reliably query on this on Windows?
            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            real32 GameUpdateHz = (real32)MonitorRefreshHz;// / 2.0f);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

            GlobalRunning = true;

            umm CurrentSortMemorySize = Megabytes(1);
            void *SortMemory = Win32AllocateMemory(CurrentSortMemorySize);
            umm CurrentClipMemorySize = Megabytes(1);
            void *ClipMemory = Win32AllocateMemory(CurrentClipMemorySize);

            // TODO(casey): Decide what our pushbuffer size is!
            u32 PushBufferSize = Megabytes(64);
            void *PushBuffer = Win32AllocateMemory(PushBufferSize);
            
#if EDITOR_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(256);
            GameMemory.TransientStorageSize = Gigabytes(1);

            GameMemory.HighPriorityQueue = &HighPriorityQueue;
            GameMemory.LowPriorityQueue = &LowPriorityQueue;
            GameMemory.PlatformAPI.AddEntry = Win32AddEntry;
            GameMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;

            GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
            GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
            GameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
            GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
            GameMemory.PlatformAPI.FileError = Win32FileError;

            GameMemory.PlatformAPI.AllocateTexture = AllocateTexture;
            GameMemory.PlatformAPI.DeallocateTexture = DeallocateTexture;
            GameMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
            GameMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;

            GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;

            Platform = GameMemory.PlatformAPI;

            Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                                                      MEM_RESERVE|MEM_COMMIT,
                                                      PAGE_READWRITE);

            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage +
                                           GameMemory.PermanentStorageSize);

            if(GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                         TempGameCodeDLLFullPath,
                                                         GameCodeLockFullPath);

                while(GlobalRunning)
                {

                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    
                    NewInput->ExecutableReloaded = false;
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath,
                                                 GameCodeLockFullPath);
                        NewInput->ExecutableReloaded = true;
                    }

                    // TODO(casey): Zeroing macro
                    // TODO(casey): We can't zero everything because the up/down state will
                    // be wrong!!!
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        if(!(OldKeyboardController->Buttons[ButtonIndex].Reset))
                        {
                            NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                                OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                        }
                    }

                    s16 MouseZ = 0;
                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController, &MouseZ);

                    if(!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);

                        if(GlobalAppIsActive)
                        {
                            NewInput->MouseX = MouseP.x;
                            NewInput->MouseY = (GlobalBackbuffer.Height - 1) - MouseP.y;
                            // NOTE(paul): 120 is the mouse wheel delta, thus
                            // this division will convert the number to the
                            // number of rotations
                            NewInput->MouseZ = MouseZ / 120;
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
                                                        GetKeyState(VK_LBUTTON) & (1 << 15));
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
                                                        GetKeyState(VK_MBUTTON) & (1 << 15));
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
                                                        GetKeyState(VK_RBUTTON) & (1 << 15));
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
                                                        GetKeyState(VK_XBUTTON1) & (1 << 15));
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
                                                        GetKeyState(VK_XBUTTON2) & (1 << 15));
                        }
                        
                        game_render_commands RenderCommands = RenderCommandStruct(
                            PushBufferSize, PushBuffer,
                            (u32)GlobalBackbuffer.Width,
                            (u32)GlobalBackbuffer.Height);
                        
                        game_offscreen_buffer Buffer = {};
                        Buffer.Memory = GlobalBackbuffer.Memory;
                        Buffer.Width = GlobalBackbuffer.Width; 
                        Buffer.Height = GlobalBackbuffer.Height;
                        Buffer.Pitch = GlobalBackbuffer.Pitch;

                        if(Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&GameMemory, NewInput, &RenderCommands);
//                            HandleDebugCycleCounters(&GameMemory);
                        }

                    umm NeededSortMemorySize = RenderCommands.PushBufferElementCount * sizeof(sort_entry);
                    if(CurrentSortMemorySize < NeededSortMemorySize)
                    {
                        Win32DeallocateMemory(SortMemory);
                        CurrentSortMemorySize = NeededSortMemorySize;
                        SortMemory = Win32AllocateMemory(CurrentSortMemorySize);
                    }

                    // TODO(casey): Collapse this with above!
                    umm NeededClipMemorySize = RenderCommands.PushBufferElementCount * sizeof(render_entry_cliprect);
                    if(CurrentClipMemorySize < NeededClipMemorySize)
                    {
                        Win32DeallocateMemory(ClipMemory);
                        CurrentClipMemorySize = NeededClipMemorySize;
                        ClipMemory = Win32AllocateMemory(CurrentClipMemorySize);
                    }

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(&HighPriorityQueue, &RenderCommands, DeviceContext,
                                               Dimension.Width, Dimension.Height,
                                               SortMemory, ClipMemory);
                    ReleaseDC(Window, DeviceContext);

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

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
    }
    else
    {
        // TODO(casey): Logging
    }
    
    return(0);
}
