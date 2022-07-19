#include "base_inc.h"
#include "win32_base_inc.h"

typedef s64 GetTicks(void);
typedef f64 GetSecondsElapsed(s64 start, s64 end);
typedef f64 GetMsElapsed(s64 start, s64 end);

typedef struct Clock{
    f64 dt;
    s64 frequency;
    GetTicks* get_ticks;
    GetSecondsElapsed* get_seconds_elapsed;
    GetMsElapsed* get_ms_elapsed;
} Clock;

typedef struct RenderBuffer{
    void* base;
    size_t size;

    s32 width;
    s32 height;
    s32 bytes_per_pixel;
    s32 stride;

    BITMAPINFO bitmap_info;
    HDC device_context;
} RenderBuffer;

typedef struct Memory{
    void* base;
    size_t size;

    void* permanent_base;
    size_t permanent_size;
    void* transient_base;
    size_t transient_size;

    bool initialized;
} Memory;


global bool global_running;
global RenderBuffer render_buffer;
global Memory memory;
global Clock clock;

static s64 get_ticks(){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result.QuadPart);
}

static f64 get_seconds_elapsed(s64 start, s64 end){
    f64 result;
    result = ((f64)(end - start) / ((f64)clock.frequency));
    return(result);
}

static f64 get_ms_elapsed(s64 start, s64 end){
    f64 result;
    result = (1000 * ((f64)(end - start) / ((f64)clock.frequency)));
    return(result);
}

static void
init_clock(Clock* clock){
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock->frequency = frequency.QuadPart;
    clock->get_ticks = get_ticks;
    clock->get_seconds_elapsed = get_seconds_elapsed;
    clock->get_ms_elapsed = get_ms_elapsed;
}

static void
init_memory(Memory* memory){
    memory->permanent_size = MB(500);
    memory->transient_size = GB(1);
    memory->size = memory->permanent_size + memory->transient_size;
    memory->base = os_virtual_alloc(memory->size);
    memory->permanent_base = memory->base;
    memory->transient_base = (u8*)memory->base + memory->permanent_size;
}

static void
init_render_buffer(RenderBuffer* render_buffer, s32 width, s32 height){
    render_buffer->width = width;
    render_buffer->height = height;

    render_buffer->bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    render_buffer->bitmap_info.bmiHeader.biWidth = width;
    render_buffer->bitmap_info.bmiHeader.biHeight = -height;
    render_buffer->bitmap_info.bmiHeader.biPlanes = 1;
    render_buffer->bitmap_info.bmiHeader.biBitCount = 32;
    render_buffer->bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 bytes_per_pixel = 4;
    render_buffer->bytes_per_pixel = bytes_per_pixel;
    render_buffer->stride = width * bytes_per_pixel;
    render_buffer->size = width * height * bytes_per_pixel;
    render_buffer->base = os_virtual_alloc(render_buffer->size);
}

static void
update_window(HWND window, RenderBuffer render_buffer){
    //HDC device_context = GetDC(window); // this should probably be called once
    if(StretchDIBits(render_buffer.device_context,
                     0, 0, render_buffer.width, render_buffer.height,
                     0, 0, render_buffer.width, render_buffer.height,
                     render_buffer.base, &render_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY))
    {
    }
    else{
        OutputDebugStringA("StretchDIBits failed\n");
    }
    //ReleaseDC(window, device_context); // this should probably be called once
}


LRESULT win_message_handler_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param){
    LRESULT result = 0;
    switch(message){
        case WM_PAINT:{
            PAINTSTRUCT paint;
            BeginPaint(window, &paint);
            update_window(window, render_buffer);
            EndPaint(window, &paint);
        } break;
        case WM_CLOSE:
        case WM_QUIT:
        case WM_DESTROY:{
            OutputDebugStringA("quiting\n");
            global_running = false;
        } break;
        default:{
            result = DefWindowProc(window, message, w_param, l_param);
        } break;
    }
    return(result);
}

s32 WinMain(HINSTANCE instance, HINSTANCE pinstance, LPSTR command_line, s32 window_type){

    WNDCLASSW window_class = {0};
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = win_message_handler_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"window class";

    if(RegisterClassW(&window_class)){
        HWND window = CreateWindowW(window_class.lpszClassName, L"Title", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

        if(window){
            init_memory(&memory);
            init_clock(&clock);
            init_render_buffer(&render_buffer, 1280, 720);

            if(SetWindowPos(window, HWND_TOP, 10, 10, render_buffer.width + 30, render_buffer.height + 50, SWP_SHOWWINDOW)){
                global_running = true;

                f64 FPS = 0;
                f64 MSPF = 0;
                u64 frame_count = 0;

                clock.dt =  0.01;
                f64 accumulator = 0.0;
                s64 start_ticks = clock.get_ticks();
                f64 second_marker = clock.get_ticks();

                render_buffer.device_context = GetDC(window);
                while(global_running){
                    MSG message;
                    while(PeekMessageW(&message, window, 0, 0, PM_REMOVE)){
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }

                    s64 end_ticks = clock.get_ticks();
                    f64 frame_time = clock.get_seconds_elapsed(start_ticks, end_ticks);
                    MSPF = 1000/1000/((f64)clock.frequency / (f64)(end_ticks - start_ticks));
					start_ticks = end_ticks;

                    accumulator += frame_time;
                    while(accumulator >= clock.dt){
                        //update_game(&memory, &render_buffer, &controller, &clock, &thread_context);
                        accumulator -= clock.dt;
                    }

                    frame_count++;
                    f64 time_elapsed = clock.get_seconds_elapsed(second_marker, clock.get_ticks());
                    if(time_elapsed > 1){
                        FPS = (frame_count / time_elapsed);
                        second_marker = clock.get_ticks();
                        frame_count = 0;
                    }
                    print("FPS: %f - MSPF: %f - dt: %f\n", FPS, MSPF, clock.dt);

                    // THIS FUNCTION
                    update_window(window, render_buffer); // this is the function that calls StretchDIBits, and is causing problems
                    // THIS FUNCTION
                }
                ReleaseDC(window, render_buffer.device_context);
            }
            else{
                print("SetWindowPos failed\n");
            }
        }
        else{
            print("CreateWindowA failed\n");
        }
    }
    else{
        print("RegisterClassA failed\n");
    }
    return(0);
}
