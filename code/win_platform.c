//TODO

//STUDY
//INCOMPLETE
//QUESTION
//CONSIDER
//FUTURE

//IMPORTANT
//NOTE


/*
TODO: WIN PLATFORM CODE
    get rid of all int's replace with specific sizes

NOT DONE:
    entities - why? because it makes it easier to refer to things as a whole rather than little bitty points and bullshit
    layers
    add clipping
    simd
    add texturing

    transformation:
        shearing

    fill polygon
    anti aliasing


    overlap/intersection
        narrow

    make it fast (threads, simd)

DONE:
    transformation:
        translation
        rotation
        scaling

    color blending
    alpha blending

    draw circle
    draw wireframe circle

    overlap/intersection
        broad


*/

/*
TODO: RESUME
    NOT DONE:
        update linkden - actually completely finish it to the T
        create my own portfolio - my own website

    DONE:
        update resume - have at least 3 different versions for different fields
*/

/*
NOTE: Take notes of these in your dope ass notebook

    PatBlt();

    explain the concept of game as a service to the platform layer
    explain events
    explain controllers
    explain game as dll
    explain loading gamecode
    explain hotreloading
    explain live loop editing

	explain Nd array to be accessed in 1d array [col_count * row + col]

	c truncates towards 0
	triangle is simplist 2d shape
	designated initalizers

    QUESTION:
        - whats the point of inlining, when, why, where
        - how to know when to create i8, i16, i32, i64, int
        - how to declspec the print from win_platform to game

    FUTURE:
        MAX_PATH;
        GET_X_LPARAM();
        GET_y_LPARAM();
        MapViewOfFile();
        CreateFileMapping();
*/

/*
FUTURE STUDY: Things to study

    Naysayer
    - time complexity of algorithms
    - understanding the different paradigms of programming languages (functional, imperative, declarative, ...)
    - understand recursion
    - understand boolean logic VERY well
    - operating systems - what it really is, how it works, what services it provides, what a file system is and how it works
    - data structures - understand how they work and live in memory and be able to do things like "given a comparison function you should be able to build a binary tree and navigate the tree in order/ pre order. be able to insert something into the tree in order."
    - basics of sorting algorithms
    - how f32ing numbers are represented and how they work
    - concurrency - what data races are and how to avoid the problem
    - understand what databases are and how they work (try and made a database that has out of core storage)

    Me
    - sparse storage vs dense storage
    - Galois' Dream: Group Theory and Differential Equations - Michio Kuga
*/

/*
FUTURE TODO: Game dev stuff to accomplish
    write very simple rasterizer (drawing squares/triangles all over the screen)
    astroids
    space invaders
    tiny kaboom
    console
    ui
    scene manager
*/

#include "game.h"

#include <windows.h>
#include <windowsx.h>
#include <xinput.h>
#include <stdio.h>

#include "win_platform.h"


// TODO: declspec this bitch outa here, not sure how figure it out
//static void
//print(char *format, ...) {
//    char buffer[4096] = {0};
//    va_list args;
//    va_start(args, format);
//    vsnprintf(buffer, sizeof(buffer), format, args);
//    va_end(args);
//
//    printf("%s", buffer);
//    OutputDebugStringA(buffer);
//}


// TODO: re-organize this
global bool global_pause;
global int global_running;
global WIN_Clock win_clock;
global WIN_RenderBuffer offscreen_render_buffer;
global Events events;

global ui32 eventkey_mapping[0xFF] = {
    [VK_ESCAPE]=KEY_ESCAPE,
    ['W']=KEY_W,
    ['A']=KEY_A,
    ['S']=KEY_S,
    ['D']=KEY_D,
    ['L']=KEY_L,
    ['P']=KEY_P,
    ['1']=KEY_1,
    ['2']=KEY_2,
    ['3']=KEY_3,
};

global ui32 eventpad_mapping[0x5838] = {
    [VK_PAD_DPAD_UP]=PAD_UP,
    [VK_PAD_DPAD_DOWN]=PAD_DOWN,
    [VK_PAD_DPAD_LEFT]=PAD_LEFT,
    [VK_PAD_DPAD_RIGHT]=PAD_RIGHT,
    [VK_PAD_BACK]=PAD_BACK,
};

global ui32 eventmouse_mapping[0x0041] = {
    [MK_LBUTTON]=MOUSE_LBUTTON,
    [MK_MBUTTON]=MOUSE_MBUTTON,
    [MK_RBUTTON]=MOUSE_RBUTTON,
    [MK_XBUTTON1]=MOUSE_XBUTTON1,
    [MK_XBUTTON2]=MOUSE_XBUTTON2,
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
WIN_load_gamecode(char *source_dll, char *copy_dll){
    WIN_GameCode result = {0};

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
            //assert(filesize.QuadPart <= 0xFFFFFFFF); // NOTE: temporary for now, this isnt the final file I/O
            result.content = VirtualAlloc(0, (ui32)filesize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE); // FUTURE: Dont use VirtualAlloc when this is more robust, use something like HeapAlloc
            if(result.content){
                DWORD bytes_read;
                if(ReadFile(filehandle, result.content, (ui32)filesize.QuadPart, &bytes_read, 0)){
                    result.size = (ui32)filesize.QuadPart;
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
    int x_offset = 10;
    int y_offset = 10;

    PatBlt(DC, 0, 0, width, y_offset, BLACKNESS);
    PatBlt(DC, 0, 0, x_offset, height, BLACKNESS);
    PatBlt(DC, x_offset + buffer.width, 0, width, height, BLACKNESS);
    PatBlt(DC, 0, y_offset + buffer.height, width, height, BLACKNESS);

    StretchDIBits(DC,
                  //0, 0, width, height,
                  x_offset, y_offset, buffer.width, buffer.height,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory, &buffer.info, DIB_RGB_COLORS, SRCCOPY);
}

static void
WIN_process_controller_input(void){
    for(ui32 i=0; i < XUSER_MAX_COUNT; ++i){
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
            events.event[events.index++] = e;
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

static void
WIN_init_recording_handle(WIN_State *state, GameMemory *game_memory, int recording_index){
    //assert((ui64)recording_index < array_count(state->replay_buffers));
    // CONSIDER: maybe do this in the future of it ends up slowing down
    //WIN_ReplayBuffer *replay_buffer = &state->replay_buffers[recording_index];
    //if(replay_buffer->memory){
    if(state->replay_buffers[recording_index]){
        state->recording_index = recording_index;
        char recording_string[64];
        wsprintf(recording_string, "build\\recording_%d.out", recording_index);
        char full_path[256];
        cat_strings(state->root_dir, recording_string, full_path);
        state->recording_handle = CreateFileA(full_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        //state->recording_handle = replay_buffer->file_handle;

        CopyMemory(state->replay_buffers[recording_index], game_memory->permanent_storage, game_memory->total_size);
    }
}

static void
WIN_release_recording_handle(WIN_State *state){
    CloseHandle(state->recording_handle);
    state->recording_index = 0;
}


static void
WIN_record_input(WIN_State *state, Events *events){
    DWORD bytes_written;
    WriteFile(state->recording_handle, events, sizeof(*events), &bytes_written, 0);
}

static void
WIN_init_playback_handle(WIN_State *state, GameMemory *game_memory, int playback_index){
    //assert(playback_index < array_count(state->replay_buffers));
    // CONSIDER: maybe do this in the future of it ends up slowing down
    //WIN_ReplayBuffer *replay_buffer = &state->replay_buffers[playback_index];
    //if(replay_buffer->memory){
    if(state->replay_buffers[playback_index]){
        state->playback_index = playback_index;
        //state->playback_handle = replay_buffer->file_handle;

        char recording_string[64];
        wsprintf(recording_string, "build\\recording_%d.out", playback_index);
        char full_path[256];
        cat_strings(state->root_dir, recording_string, full_path);
        state->playback_handle = CreateFileA(full_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

        CopyMemory(game_memory->permanent_storage, state->replay_buffers[playback_index], game_memory->total_size);
    }
}

static void
WIN_release_playback_handle(WIN_State *state){
    CloseHandle(state->playback_handle);
    state->playback_index = 0;
}

static void
WIN_play_input(WIN_State *state, GameMemory *game_memory, Events *events){
    DWORD bytes_read;
    if(ReadFile(state->playback_handle, events, sizeof(*events), &bytes_read, 0)){
        if(bytes_read == 0){
            // NOTE: We've hit the end of the stream, go back to the beginning
            int playback_index = state->playback_index;
            WIN_release_playback_handle(state);
            WIN_init_playback_handle(state, game_memory, playback_index);
            ReadFile(state->playback_handle, events, sizeof(*events), &bytes_read, 0);
        }
    }
    else{
        // TODO: Loggin
    }
}

static LARGE_INTEGER
WIN_get_clock(void){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);

    return(result);
}

static f32
WIN_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end){
    f32 result;
    result = ((f32)(end.QuadPart - start.QuadPart) / ((f32)win_clock.frequency.QuadPart));

    return(result);
}

static void
WIN_sync_framerate(void){
    LARGE_INTEGER time_stamp = WIN_get_clock();
    f32 seconds_elapsed = WIN_get_seconds_elapsed(win_clock.start, time_stamp);
    if(seconds_elapsed < win_clock.target_seconds_per_frame){
        if(win_clock.sleep_granularity_set){
            DWORD sleep_ms = (DWORD)(1000.0f * (win_clock.target_seconds_per_frame - seconds_elapsed));
            if(sleep_ms > 0){
                Sleep(sleep_ms);
            }
        }

        f32 seconds_elapsed_remaining = WIN_get_seconds_elapsed(win_clock.start, WIN_get_clock());
        if(seconds_elapsed_remaining < win_clock.target_seconds_per_frame){
            // TODO: LOG MISSED SLEEP HERE
        }

        seconds_elapsed = WIN_get_seconds_elapsed(win_clock.start, time_stamp);
        while(seconds_elapsed < win_clock.target_seconds_per_frame){
            seconds_elapsed = WIN_get_seconds_elapsed(win_clock.start, WIN_get_clock());
        }
    }
    else{
        // TODO: MISSED FRAME RATE!
        // TODO: Logging
    }
}

LRESULT CALLBACK
Win32WindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam){
    LRESULT result = 0;

    switch(message){
        case WM_CHAR:{
            Event e = {0};
            e.type = EVENT_TEXT;
            e.text = (ui16)wParam;
            events.event[events.index++] = e;
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
                events.event[events.index++] = e;
            }
        } break;
        // TODO: maybe later case WM_MOUSEHWHEEL:
        case WM_MOUSEMOVE:
        {
            Event e = {0};
            e.type = EVENT_MOUSEMOTION;
            //QUESTION: ask about this being 8bytes but behaving like 4bytes
            e.mouse_x = (i32)(lParam & 0xFFFF);
            e.mouse_y = (i32)(lParam >> 16);
            events.event[events.index++] = e;
        } break;
        case WM_MOUSEWHEEL:
        {
            POINT mouse_pos;
            //QUESTION: ask about this being 8bytes but behaving like 4bytes
            //mouse_pos.x = (i32)(lParam & 0xFFFF);
            //mouse_pos.y = (i32)(lParam >> 16);
            mouse_pos.x = (i32)(lParam & 0xFFFF);
            mouse_pos.y = (i32)(lParam >> 16);
            HWND window_handle = GetActiveWindow();
            ScreenToClient(window_handle, &mouse_pos);

            i16 wheel_direction = GET_WHEEL_DELTA_WPARAM(wParam);
            Event e = {0};
            e.type = EVENT_MOUSEWHEEL;
            e.mouse_x = (i32)mouse_pos.x;
            e.mouse_y = (i32)mouse_pos.y;
            if(wheel_direction > 0){
                e.wheel_y = 1;
            }
            else{
                e.wheel_y = -1;
            }
            events.event[events.index++] = e;
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
		{
            Event e = {0};
            e.type = EVENT_MOUSEDOWN;
            e.mouse = eventmouse_mapping[wParam];
            //QUESTION: ask about this being 8bytes but behaving like 4bytes
            e.mouse_x = (i32)(lParam & 0xFFFF);
            e.mouse_y = (i32)(lParam >> 16);
            events.event[events.index++] = e;
		} break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        {
            Event e = {0};
            e.type = EVENT_MOUSEUP;
            e.mouse = eventmouse_mapping[wParam];
            e.mouse_x = (i32)lParam & 0xFFFF;
            e.mouse_y = (i32)lParam >> 16;
            events.event[events.index++] = e;
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

// FUTURE: think about using int main (int argc, char *argv[]){}
// being consistant with all your C programs could be helpfull later
// this will give you access to a console more easily if you wanted it
// but if not maybe stick to using WinMain
// GetModuleHandle(0) -> gets hinstance
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd){
    WIN_load_xinput();
    WIN_init_render_buffer(&offscreen_render_buffer, 960, 540);

    WNDCLASSA window_class = {0};
    window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = Win32WindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "window class";
    //window_class.hIcon; // TODO: place an icon here

    UINT sleep_granularity = 1;
    win_clock.sleep_granularity_set = (timeBeginPeriod(sleep_granularity) == TIMERR_NOERROR);

    if(RegisterClassA(&window_class)){
        HWND window = CreateWindowExA(0, window_class.lpszClassName, "game", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

        if(window){
            HDC DC = GetDC(window);
            // QUESTION: ask about this, and how does it always get 144, and how can i get the other monitors refresh rate
            //int refresh_rate = GetDeviceCaps(DC, VREFRESH);
            int gpu_monitor_refresh_hz = 60;
            f32 soft_monitor_refresh_hz = (gpu_monitor_refresh_hz / 2.0f);
            win_clock.target_seconds_per_frame = 1.0f / (f32)soft_monitor_refresh_hz;

            // NOTE: maybe move this outside RegisterClassA
            win_clock.start = WIN_get_clock();
            QueryPerformanceFrequency(&win_clock.frequency);
            win_clock.cpu_start = __rdtsc();

            WIN_State state = {0};

            char exe_path[MAX_PATH];
            GetModuleFileNameA(0, exe_path, sizeof(exe_path));
            char *starting_point = string_point_at_last(exe_path, '\\', 2);
            // QUESTION: I dont understand this
            state.root_dir_length = starting_point - exe_path;
            get_root_dir(state.root_dir, state.root_dir_length, exe_path);

            char gamecode_dll[] = "build\\game.dll";
            char gamecode_dll_fullpath[256];
            cat_strings(state.root_dir, gamecode_dll, gamecode_dll_fullpath);

            char copy_gamecode_dll[] = "build\\copy_game.dll";
            char copy_gamecode_dll_fullpath[256];
            cat_strings(state.root_dir, copy_gamecode_dll, copy_gamecode_dll_fullpath);

            WIN_GameCode gamecode = WIN_load_gamecode(gamecode_dll_fullpath, copy_gamecode_dll_fullpath);

#if DEBUG
            LPVOID base_address = (LPVOID)Terabytes(2);
#else
            LPVOID base_address = 0;
#endif
            // FUTURE: have different allocation sizes based on what the machine has available
            // this allows you to use higher/lower resolution sprites and so on to support
            // what may or may not be available
            GameMemory game_memory = {0};
            game_memory.running = true;
            game_memory.permanent_storage_size = Megabytes(64);
            game_memory.transient_storage_size = Gigabytes(1);

            game_memory.read_entire_file = read_entire_file;
            game_memory.write_entire_file = write_entire_file;
            game_memory.free_file_memory = free_file_memory;

            game_memory.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            game_memory.total_storage = VirtualAlloc(base_address, (size_t)game_memory.total_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            game_memory.permanent_storage = game_memory.total_storage;
            game_memory.transient_storage = (ui8 *)game_memory.total_storage + game_memory.permanent_storage_size;

            copy_string(state.root_dir, game_memory.root_dir, state.root_dir_length);
            char root_dir[100];
            copy_string(state.root_dir, root_dir, state.root_dir_length);
            char data[] = "data\\";
            cat_strings(root_dir, data, game_memory.data_dir);

            for(ui64 i=0; i<array_count(state.replay_buffers); ++i){
                state.replay_buffers[i] = VirtualAlloc(0, (size_t)game_memory.total_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

                // CONSIDER: maybe do this in the future if it ends up slowing down
                //WIN_ReplayBuffer *replay_buffer = &state.replay_buffers[i];
                //char recording_string[64];
                //wsprintf(recording_string, "build\\recording_%d.hmi", i);
                //cat_strings(state.root_dir, recording_string, replay_buffer->file_name);
                //replay_buffer->file_handle = CreateFileA(replay_buffer->file_name, GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

                //DWORD highorder_size = game_memory.total_size >> 32;
                //DWORD loworder_size = game_memory.total_size & 0xFFFFFFFF;
                //replay_buffer->file_memory_mapping = CreateFileMapping(replay_buffer->file_handle, 0, PAGE_READWRITE, highorder_size, loworder_size, 0);
                //replay_buffer->memory = MapViewOfFile(replay_buffer->file_memory_mapping, FILE_MAP_ALL_ACCESS, 0, 0, game_memory.total_size);
                //if(replay_buffer->memory){
                if(state.replay_buffers[i]){
                }
                else{
                    //TODO: Logging
                }
            }

            // INCOMPLETE: this might need to be moved inside the loop because some things might change such as width/height but im not sure yet
            RenderBuffer render_buffer = {0};
            render_buffer.memory = offscreen_render_buffer.memory;
            render_buffer.memory_size = offscreen_render_buffer.memory_size;
            render_buffer.bytes_per_pixel = offscreen_render_buffer.bytes_per_pixel;
            render_buffer.width = offscreen_render_buffer.width;
            render_buffer.height = offscreen_render_buffer.height;
            render_buffer.pitch = offscreen_render_buffer.pitch;

            events.size = 256;
            events.index = 0;

            Clock clock = {0};
            global_running = true;
            global_pause = false;

            if(game_memory.permanent_storage && game_memory.transient_storage && render_buffer.memory){
                while(global_running){
                    clock.dt = win_clock.target_seconds_per_frame;
                    FILETIME current_write_time = WIN_get_file_write_time(gamecode_dll);
                    if((CompareFileTime(&current_write_time, &gamecode.write_time)) != 0){
                        WIN_unload_gamecode(&gamecode);
                        gamecode = WIN_load_gamecode(gamecode_dll_fullpath, copy_gamecode_dll_fullpath);
                    }

                    MSG message;
                    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)){
                        TranslateMessage(&message);
                        DispatchMessageA(&message);

                        for(ui32 i=0; i < events.index; ++i){
                            Event *event = &events.event[i];
                            if(event->type == EVENT_KEYDOWN){
                                if(event->key == KEY_ESCAPE){
                                    global_running = false;
                                }
                                if(event->key == KEY_L){
                                    if(state.playback_index == 0){
                                        if(state.recording_index == 0){
                                            WIN_init_recording_handle(&state, &game_memory, 1);
                                        }
                                        else{
                                            WIN_release_recording_handle(&state);
                                            WIN_init_playback_handle(&state, &game_memory, 1);
                                        }
                                    }
                                    else{
                                        WIN_release_playback_handle(&state);
                                    }
                                    event->key = KEY_NONE;
                                }
                                if(event->key == KEY_P){
                                    global_pause = !global_pause;
                                    event->key = KEY_NONE;
                                }
                            }
                        }
                    }
                    WIN_process_controller_input();

                    if(!global_pause){
                        if(state.recording_index){
                            WIN_record_input(&state, &events);
                        }
                        if(state.playback_index){
                            WIN_play_input(&state, &game_memory, &events);
                        }
                        if(gamecode.main_game_loop){
                            gamecode.main_game_loop(&game_memory, &render_buffer, &events, &clock);
                            if(!game_memory.running){
                                global_running = game_memory.running;
                            }
                        }
                        events.index = 0;

                        WIN_sync_framerate();

                        //f32 MSPF = 1000 * WIN_get_seconds_elapsed(win_clock.start, WIN_get_clock());
                        //f32 FPS = ((f32)win_clock.frequency.QuadPart / (f32)(WIN_get_clock().QuadPart - win_clock.start.QuadPart));
                        //f32 CPUCYCLES = (f32)(__rdtsc() - win_clock.cpu_start) / (1000 * 1000);
                        //print("MSPF: %.02fms - FPS: %.02f - CPU: %.02f\n", MSPF, FPS, CPUCYCLES);

                        WIN_WindowDimensions wd = WIN_get_window_dimensions(window);
                        WIN_update_window(offscreen_render_buffer, DC, wd.width, wd.height);

                        win_clock.cpu_end = __rdtsc();
                        win_clock.end = WIN_get_clock();
                        win_clock.start = win_clock.end;
                        win_clock.cpu_start = win_clock.cpu_end;
                    }
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
