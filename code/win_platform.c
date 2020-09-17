//TODO
//NOTE 
//INCOMPLETE 
//IMPORTANT 
//QUESTION 
//CONSIDER 
//FUTURE 
//STUDY
/*
    CopyFile();
    GetFileAttributesExA();
    CompareFileTime();
    MAX_PATH;
    GetModuleFileNameA();

    Assert();
    ArrayCount();

    LoadLibraryA();
    GetProcAddress();
    FreeLibrary();


    explain the concept of game as a service to the platform layer
    explain events
    explain controllers
    explain game as dll
    explain loading gamecode
    explain hotreloading
    explain live loop editing
}

*/
#include "game.h"

#include <windows.h>
#include <xinput.h>
#include <stdio.h>

#include "win_platform.h"


// TODO:declspec this bitch outa here, not sure how figure it out
static void
print(char *format, ...) {
    char buffer[4096] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s", buffer);
    OutputDebugStringA(buffer);
}

// TODO: re-organize this
global int global_running;
global WIN_Clock clock;
global WIN_RenderBuffer offscreen_render_buffer;
global Event events[256];
global uint32 event_i = 0;
global uint32 eventkey_mapping[0xFE] = {
    [VK_ESCAPE]=KEY_ESCAPE,
    ['W']=KEY_W,
    ['A']=KEY_A,
    ['S']=KEY_S,
    ['D']=KEY_D,
};
global uint32 eventpad_mapping[0x5837] = {
    [VK_PAD_DPAD_UP]=PAD_UP,
    [VK_PAD_DPAD_DOWN]=PAD_DOWN,
    [VK_PAD_DPAD_LEFT]=PAD_LEFT,
    [VK_PAD_DPAD_RIGHT]=PAD_RIGHT,
    [VK_PAD_BACK]=PAD_BACK,
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
#define X_INPUT_GET_KEYSTROKE(name) DWORD WINAPI name(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke)

typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);
typedef X_INPUT_GET_KEYSTROKE(x_input_get_keystroke);

X_INPUT_GET_STATE(XInputGetStateStub) { return(ERROR_DEVICE_NOT_CONNECTED); }
X_INPUT_SET_STATE(XInputSetStateStub) { return(ERROR_DEVICE_NOT_CONNECTED); }
X_INPUT_GET_KEYSTROKE(XInputGetKeystrokeStub) { return(ERROR_DEVICE_NOT_CONNECTED); }

global x_input_get_state *XInputGetState_ = XInputGetStateStub;
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
global x_input_get_keystroke *XInputGetKeystroke_ = XInputGetKeystrokeStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
#define XInputGetKeystroke XInputGetKeystroke_

inline FILETIME
WIN_get_file_write_time(char *filename){
    FILETIME write_time = {0};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if(GetFileAttributesExA(filename, GetFileExInfoStandard, &data)){
        write_time = data.ftLastWriteTime;
    }

    return(write_time);
}

static WIN_GameCode
WIN_load_gamecode(char *source_dll){
    WIN_GameCode result = {0};

    char *copy_dll = "copy_game.dll";

    result.write_time = WIN_get_file_write_time(source_dll);
    CopyFile(source_dll, copy_dll, FALSE);
    result.gamecode_dll = LoadLibraryA(copy_dll);
    if(result.gamecode_dll){
        result.main_game_loop = (MainGameLoop *)GetProcAddress(result.gamecode_dll, "main_game_loop");
        result.is_valid = result.main_game_loop && 1;
    }

    if(!result.is_valid){
        result.main_game_loop = 0;
    }

    return(result);
}

static void
WIN_unload_gamecode(WIN_GameCode *gamecode){
    if(gamecode->gamecode_dll){
        FreeLibrary(gamecode->gamecode_dll);
        gamecode->gamecode_dll = 0;
    }

    gamecode->is_valid = false;
    gamecode->main_game_loop = 0;
}

static void
WIN_load_xinput(void){
    // TODO: Test this on Windows 8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary){
        // TODO: Diagnostic
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }

    if(!XInputLibrary){
        // TODO: Diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary){
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}

        XInputGetKeystroke = (x_input_get_keystroke *)GetProcAddress(XInputLibrary, "XInputGetKeystroke");
        if(!XInputGetKeystroke) {XInputGetKeystroke = XInputGetKeystrokeStub;}
        // TODO: Diagnostic

    }
    else{
        // TODO: Diagnostic
    }
}

FREE_FILE_MEMORY(free_file_memory){
    if(memory){
        VirtualFree(memory, 0, MEM_RELEASE);
    }
    else{
        // TODO: Logging
    }
}

READ_ENTIRE_FILE(read_entire_file){
    FileData result = {0};

    HANDLE filehandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(filehandle != INVALID_HANDLE_VALUE){
        LARGE_INTEGER filesize;
        if(GetFileSizeEx(filehandle, &filesize)){
            //Assert(filesize.QuadPart <= 0xFFFFFFFF); // NOTE: temporary for now, this isnt the final file I/O
            result.content = VirtualAlloc(0, (uint32)filesize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE); // FUTURE: Dont use VirtualAlloc when this is more robust, use something like HeapAlloc
            if(result.content){
                DWORD bytes_read;
                if(ReadFile(filehandle, result.content, (uint32)filesize.QuadPart, &bytes_read, 0)){
                    result.size = (uint32)filesize.QuadPart;
                }
                else{
                    free_file_memory(result.content);
                    result.content = 0;
                    // TODO: Logging
                }
            }
            else{
                // TODO: Logging
            }
        }
        else{
            // TODO: Logging
        }
        CloseHandle(filehandle);
    }
    else{
        // TODO: Logging
    }

    return(result);
}

WRITE_ENTIRE_FILE(write_entire_file){
    bool result = false;

    HANDLE filehandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(filehandle != INVALID_HANDLE_VALUE){
        DWORD bytes_written;
        if(WriteFile(filehandle, memory, memory_size, &bytes_written, 0)){
            result = (bytes_written == memory_size);
        }
        else{
            // TODO: Logging
        }
        CloseHandle(filehandle);
    }
    else{
        // TODO: Logging
    }

    return(result);
}

static WIN_WindowDimensions
WIN_get_window_dimensions(HWND window){
    WIN_WindowDimensions result = {0};

    RECT rect;
    if(GetClientRect(window, &rect)){
        result.x = rect.left;
        result.y = rect.top;
        result.width = rect.right - rect.left;
        result.height = rect.bottom - rect.top;
    }
    else{
        // TODO: Logging
    }

    return(result);
}

static void
WIN_init_render_buffer(WIN_RenderBuffer *buffer, int width, int height){
    // NOTE: This is just a nice safegaurd incase its called again
    if(buffer->memory){
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = 4;
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    buffer->memory_size = buffer->width * buffer->height * buffer->bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, buffer->memory_size, MEM_COMMIT, PAGE_READWRITE);
}

static void
WIN_update_window(WIN_RenderBuffer buffer, HDC DC, int width, int height){
    StretchDIBits(DC,
                  0, 0, width, height,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory, &buffer.info, DIB_RGB_COLORS, SRCCOPY);
}

static void
WIN_process_controller_input(void){
    for(uint32 i=0; i < XUSER_MAX_COUNT; ++i){
        XINPUT_KEYSTROKE controller_state;
        if((XInputGetKeystroke(i, 0, &controller_state)) == ERROR_SUCCESS){
            Event e = {0};
            if(controller_state.Flags == XINPUT_KEYSTROKE_KEYDOWN){
                e.type = EVENT_PADDOWN;
            }
            if(controller_state.Flags == XINPUT_KEYSTROKE_KEYUP){
                e.type = EVENT_PADUP;
            }
            e.pad = eventpad_mapping[controller_state.VirtualKey];
            events[event_i++] = e;
        }

        // INCOMPLETE: I kind of like using GetState more than GetKeyStroke
        // because i feel like it gives me more controller and i dont think i want
        // repeat messages like i get from GetKeyStroke. will look into this more later
        //XINPUT_STATE controller_state;
        //if((XInputGetState(i, &controller_state)) == ERROR_SUCCESS){
        //    XINPUT_GAMEPAD *gamepad = &controller_state.Gamepad;
        //    if(gamepad->wButtons & XINPUT_GAMEPAD_A){
        //        //++xoffset;
        //    }
        //}
        //else{
        //    // NOTE: Controlloer not available
        //}
    }
}

LRESULT CALLBACK
Win32WindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam){
    LRESULT result = 0;

    switch(message){
        case WM_CHAR:{
            Event e = {0};
            e.type = EVENT_TEXT;
            e.text = (uint16)wParam;
            events[event_i++] = e;
        }break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            //WPARAM keycode = wParam;
            bool was_down = ((lParam & (1 << 30)) != 0);
            bool is_down = ((lParam & (1 << 31)) == 0);


            if(was_down != is_down){
                Event e = {0};
                e.type = is_down ? EVENT_KEYDOWN : EVENT_KEYUP;
                e.key = eventkey_mapping[wParam];
                events[event_i++] = e;
            }
        } break;
        case WM_CLOSE:
        {
            // TODO: Maybe use a message box here
            global_running = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC DC = BeginPaint(window, &paint);
            WIN_WindowDimensions wd = WIN_get_window_dimensions(window);
            WIN_update_window(offscreen_render_buffer, DC, wd.width, wd.height);
            EndPaint(window, &paint);
        } break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return(result);
}

static LARGE_INTEGER
WIN_get_clock(void){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);

    return(result);
}

static float
WIN_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end){
    float result;
    result = ((float)(end.QuadPart - start.QuadPart) / ((float)clock.frequency.QuadPart));

    return(result);
}

static void
WIN_sync_framerate(void){
    LARGE_INTEGER time_stamp = WIN_get_clock();
    float seconds_elapsed = WIN_get_seconds_elapsed(clock.start, time_stamp);
    if(seconds_elapsed < clock.target_seconds_per_frame){
        if(clock.sleep_granularity_set){
            DWORD sleep_ms = (DWORD)(1000.0f * (clock.target_seconds_per_frame - seconds_elapsed));
            if(sleep_ms > 0){
                Sleep(sleep_ms);
            }
        }

        float seconds_elapsed_remaining = WIN_get_seconds_elapsed(clock.start, WIN_get_clock());
        if(seconds_elapsed_remaining < clock.target_seconds_per_frame){
            // TODO: LOG MISSED SLEEP HERE
        }

        seconds_elapsed = WIN_get_seconds_elapsed(clock.start, time_stamp);
        while(seconds_elapsed < clock.target_seconds_per_frame){
            seconds_elapsed = WIN_get_seconds_elapsed(clock.start, WIN_get_clock());
        }
    }
    else{
        // TODO: MISSED FRAME RATE!
        // TODO: Logging
    }
}

// FUTURE: think about using int main (int argc, char *argv[]){} 
// being consistant with all your C programs could be helpfull later
// this will give you access to a console more easily if you wanted it 
// but if not maybe stick to using WinMain
// GetModuleHandle(0) -> gets hinstance
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd){
    WIN_load_xinput();
    WIN_init_render_buffer(&offscreen_render_buffer, 1280, 720);

    WNDCLASSA window_class = {0};
    window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = Win32WindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "window class";
    //window_class.hIcon; // TODO: place an icon here

    UINT sleep_granularity = 1;
    clock.sleep_granularity_set = (timeBeginPeriod(sleep_granularity) == TIMERR_NOERROR);

    if(RegisterClassA(&window_class)){
        HWND window = CreateWindowExA(0, window_class.lpszClassName, "game", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
        if(window){

            HDC DC = GetDC(window);
            // QUESTION: ask about this, and how does it always get 144, and how can i get the other monitors refresh rate
            //int refresh_rate = GetDeviceCaps(DC, VREFRESH);
            int monitor_refresh_hz = 60;
            float game_update_hz = (monitor_refresh_hz / 2.0f);
            clock.target_seconds_per_frame = 1.0f / (float)game_update_hz;

            // NOTE: maybe move this outside RegisterClassA
            clock.start = WIN_get_clock();
            QueryPerformanceFrequency(&clock.frequency);
            clock.cpu_start = __rdtsc();

            global_running = true;

#if DEBUG
            LPVOID base_address = (LPVOID)Terabytes(2);
#else
            LPVOID base_address = 0;
#endif
            // FUTURE: have different allocation sizes based on what the machine has available
            // this allows you to use higher/lower resolution sprites and so on to support
            // what may or may not be available
            GameMemory game_memory = {0};
            game_memory.permanent_storage_size = Megabytes(64);
            game_memory.temporary_storage_size = Gigabytes(4);
            game_memory.total_size = game_memory.permanent_storage_size + game_memory.temporary_storage_size;
            game_memory.total_storage = VirtualAlloc(base_address, game_memory.total_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            game_memory.permanent_storage = game_memory.total_storage;
            game_memory.temporary_storage = (uint8 *)game_memory.total_storage + game_memory.permanent_storage_size;
            game_memory.read_entire_file = read_entire_file;
            game_memory.write_entire_file = write_entire_file;
            game_memory.free_file_memory = free_file_memory;

            // INCOMPLETE: this might need to be moved inside the loop because some things might change such as width/height but im not sure yet
            RenderBuffer render_buffer = {0};
            render_buffer.memory = offscreen_render_buffer.memory;
            render_buffer.memory_size = offscreen_render_buffer.memory_size;
            render_buffer.bytes_per_pixel = offscreen_render_buffer.bytes_per_pixel;
            render_buffer.width = offscreen_render_buffer.width;
            render_buffer.height = offscreen_render_buffer.height;
            render_buffer.pitch = offscreen_render_buffer.pitch;

            char *gamecode_dll = "game.dll";
            WIN_GameCode gamecode = WIN_load_gamecode(gamecode_dll);

            // INCOMPLETE: might not want to check for render buffer if its going to be in the loop
            // and set every time. think about this.
            // QUESTION: ask Sir Martins
            if(game_memory.permanent_storage && game_memory.temporary_storage && render_buffer.memory){
                while(global_running){
                    FILETIME current_write_time = WIN_get_file_write_time(gamecode_dll);
                    if((CompareFileTime(&current_write_time, &gamecode.write_time)) != 0){
                        WIN_unload_gamecode(&gamecode);
                        gamecode = WIN_load_gamecode(gamecode_dll);
                    }

                    MSG message;
                    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)){
                        TranslateMessage(&message);
                        DispatchMessageA(&message);
                    }
                    WIN_process_controller_input();

                    gamecode.main_game_loop(&global_running, &game_memory, &render_buffer, events, event_i);


                    WIN_sync_framerate();
                    event_i = 0;

                    //float MSPF = 1000 * WIN_get_seconds_elapsed(clock.start, WIN_get_clock());
                    //float FPS = ((float)clock.frequency.QuadPart / (float)(WIN_get_clock().QuadPart - clock.start.QuadPart));
                    //float CPUCYCLES = (float)(__rdtsc() - clock.cpu_start) / (1000 * 1000);
                    //print("MSPF: %.02fms - FPS: %.02f - CPU: %.02f\n", MSPF, FPS, CPUCYCLES);

                    WIN_WindowDimensions wd = WIN_get_window_dimensions(window);
                    WIN_update_window(offscreen_render_buffer, DC, wd.width, wd.height);

                    clock.cpu_end = __rdtsc();
                    clock.end = WIN_get_clock();
                    clock.start = clock.end;
                    clock.cpu_start = clock.cpu_end;
                }
            }
            else{
                // TODO: Logging
            }
        }
        else{
            // TODO: Logging
        }
    }
    else{
        // TODO: Logging
    }

    return(0);
}
