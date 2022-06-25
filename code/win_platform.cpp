/*
TODO
WRONG
FIXME

@fixme

INCOMPLETE
STUDY
QUESTION
FUTURE
CONSIDER
UNCLEAR
UNTESTED

IMPORTANT
NOTE
*/

#include "base_include.h"
#include "win32_base_include.h"

#if DEBUG

enum{ DebugCycleCounter_update, DebugCycleCounter_draw_rect_fast, DebugCycleCounter_draw_rect_slow, DebugCycleCounter_frame, DebugCycleCounter_init, DebugCycleCounter_reset_sentinels, DebugCycleCounter_LL_setup, DebugCycleCounter_events, DebugCycleCounter_controller, DebugCycleCounter_degrade_pher, DebugCycleCounter_behavior, DebugCycleCounter_state_none, DebugCycleCounter_state_wondering, DebugCycleCounter_wondering, DebugCycleCounter_wondering_search, DebugCycleCounter_wondering_search_food, DebugCycleCounter_wondering_search_pher, DebugCycleCounter_state_collecting, DebugCycleCounter_state_depositing, DebugCycleCounter_depositing_search, DebugCycleCounter_allocate_commands, DebugCycleCounter_draw, DebugCycleCounter_count, };
enum{ DebugTickCounter_update, DebugTickCounter_draw_rect, DebugTickCounter_frame, DebugTickCounter_init, DebugTickCounter_reset_sentinels, DebugTickCounter_LL_setup, DebugTickCounter_events, DebugTickCounter_controller, DebugTickCounter_degrade_pher, DebugTickCounter_behavior, DebugTickCounter_state_none, DebugTickCounter_state_wondering, DebugTickCounter_wondering, DebugTickCounter_wondering_search, DebugTickCounter_wondering_search_food, DebugTickCounter_wondering_search_pher, DebugTickCounter_state_collecting, DebugTickCounter_state_depositing, DebugTickCounter_depositing_search, DebugTickCounter_allocate_commands, DebugTickCounter_draw, DebugTickCounter_count, };

typedef struct DebugCycleCounter{
    u64 cycle_count;
    u32 hit_count;
    char *name;
} DebugCycleCounter;

typedef struct DebugTickCounter{
    f64 tick_count;
    u32 hit_count;
    char *name;
} DebugTickCounter;

#define BEGIN_CYCLE_COUNTER(ID) u64 StartCycleCounter_##ID = __rdtsc();
#define BEGIN_TICK_COUNTER(ID) u64 StartTickCounter_##ID = clock->get_ticks()
#define BEGIN_TICK_COUNTER_L(ID) u64 StartTickCounter_##ID = clock.get_ticks()
#define END_CYCLE_COUNTER(ID) cycle_counters[DebugCycleCounter_##ID].cycle_count += __rdtsc() - StartCycleCounter_##ID; ++cycle_counters[DebugCycleCounter_##ID].hit_count; cycle_counters[DebugCycleCounter_##ID].name = #ID;
#define END_TICK_COUNTER(ID) tick_counters[DebugTickCounter_##ID].tick_count += clock->get_seconds_elapsed(StartTickCounter_##ID, clock->get_ticks()); ++tick_counters[DebugTickCounter_##ID].hit_count; tick_counters[DebugTickCounter_##ID].name = #ID;
#define END_TICK_COUNTER_L(ID) tick_counters[DebugTickCounter_##ID].tick_count += clock.get_seconds_elapsed(StartTickCounter_##ID, clock.get_ticks()); ++tick_counters[DebugTickCounter_##ID].hit_count; tick_counters[DebugTickCounter_##ID].name = #ID;

DebugCycleCounter cycle_counters[DebugCycleCounter_count];
DebugTickCounter tick_counters[DebugTickCounter_count];

#define handle_debug_counters() handle_debug_counters_()
static void handle_debug_counters_(){
    print("Debug Cycle Counters:\n");
    print("    Name:%-18s Cycles:%-3s Hits:%-3s C/H:%s\n", "", "", "", "");
    for(u32 counter_index=0; counter_index < (u32)ArrayCount(cycle_counters); ++counter_index){
        DebugCycleCounter *counter = cycle_counters + counter_index;
        if(counter->hit_count){
            print("    %-23s %-10u %-8u %u\n", counter->name, counter->cycle_count, counter->hit_count, (counter->cycle_count / counter->hit_count));
            counter->cycle_count = 0;
            counter->hit_count = 0;
        }
    }
    print("Debug Tick Counters:\n");
    print("    Name:%-18s Ticks:%-4s Hits:%-3s T/H:%s\n", "", "", "", "");
    for(u32 counter_index=0; counter_index < (u32)ArrayCount(tick_counters); ++counter_index){
        DebugTickCounter *counter = tick_counters + counter_index;
        if(counter->hit_count){
            print("    %-23s %-10f %-8u %f\n", counter->name, counter->tick_count, counter->hit_count, (counter->tick_count / counter->hit_count));
            counter->tick_count = 0;
            counter->hit_count = 0;
        }
    }
}

#else
#define BEGIN_CYCLE_COUNTER(ID)
#define END_CYCLE_COUNTER(ID)
#define BEGIN_TICK_COUNTER(ID)
#define END_TICK_COUNTER(ID)
#define BEGIN_TICK_COUNTER_L(ID)
#define END_TICK_COUNTER_L(ID)
#define handle_debug_counters()
#endif

typedef i64 GetTicks(void);
typedef f64 GetSecondsElapsed(i64 start, i64 end);
typedef f64 GetMsElapsed(i64 start, i64 end);

typedef struct Clock{
    f32 dt;
    i64 frequency;
    GetTicks* get_ticks;
    GetSecondsElapsed* get_seconds_elapsed;
    GetMsElapsed* get_ms_elapsed;
} Clock;

typedef struct Button{
    bool pressed;
    bool held;
} Button;

typedef struct Controller{
    Button up;
    Button down;
    Button left;
    Button right;
    Button one;
    Button two;
    Button three;
    Button four;
    Button m1;
    Button m2;
    Button m3;
    v2 mouse_pos;
} Controller;

typedef struct RenderBuffer{
    void* base;
    size_t size;

    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 stride;

    BITMAPINFO bitmap_info;
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
global Controller controller;
global Memory memory;
global Clock clock;


static i64 get_ticks(){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result.QuadPart);
}

static f64 get_seconds_elapsed(i64 start, i64 end){
    f64 result;
    result = ((f64)(end - start) / ((f64)clock.frequency));
    return(result);
}

static f64 get_ms_elapsed(i64 start, i64 end){
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
    clock->dt = 1.0/60.0;
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
init_render_buffer(RenderBuffer* render_buffer, i32 width, i32 height){
    render_buffer->width = width;
    render_buffer->height = height;

    render_buffer->bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    render_buffer->bitmap_info.bmiHeader.biWidth = width;
    render_buffer->bitmap_info.bmiHeader.biHeight = -height;
    render_buffer->bitmap_info.bmiHeader.biPlanes = 1;
    render_buffer->bitmap_info.bmiHeader.biBitCount = 32;
    render_buffer->bitmap_info.bmiHeader.biCompression = BI_RGB;

    i32 bytes_per_pixel = 4;
    render_buffer->bytes_per_pixel = bytes_per_pixel;
    render_buffer->stride = width * bytes_per_pixel;
    render_buffer->size = width * height * bytes_per_pixel;
    render_buffer->base = os_virtual_alloc(render_buffer->size);
}

static void
update_window(HWND window, RenderBuffer render_buffer){
    HDC device_context = GetDC(window);

    i32 x_offset = 10;
    i32 y_offset = 10;

    PatBlt(device_context, 0, 0, render_buffer.width + x_offset, y_offset, WHITENESS);
    PatBlt(device_context, 0, 0, x_offset, render_buffer.height + y_offset, WHITENESS);
    PatBlt(device_context, render_buffer.width + x_offset, 0, render_buffer.width + x_offset, render_buffer.height + y_offset, WHITENESS);
    PatBlt(device_context, 0, render_buffer.height + y_offset, render_buffer.width + x_offset, render_buffer.height + y_offset, WHITENESS);

    if(StretchDIBits(device_context,
                     x_offset, y_offset, render_buffer.width, render_buffer.height,
                     0, 0, render_buffer.width, render_buffer.height,
                     render_buffer.base, &render_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY))
    {
    }
    else{
        OutputDebugStringA("StrechDIBits failed\n");
    }
}

#include "game.h"

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
        case WM_MOUSEMOVE: {
            //QUESTION: ask about this being 8bytes but behaving like 4bytes
            controller.mouse_pos.x = (i32)(l_param & 0xFFFF);
            controller.mouse_pos.y = render_buffer.height - (i32)(l_param >> 16);
        } break;
            // CONSIDER TODO: maybe get rid of held, and just do what you did with m3 in the old rasterizer
        case WM_LBUTTONUP:{
            controller.m1.held = false;
        } break;
        case WM_MBUTTONUP:{
            controller.m2.held = false;
        } break;
        case WM_RBUTTONUP:{
            controller.m3.held = false;
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:{
            switch(w_param){
                case MK_LBUTTON:{
                    controller.m1.held = true;
                    controller.m1.pressed = true;
                } break;
                case MK_RBUTTON:{
                    controller.m2.held = true;
                    controller.m2.pressed = true;
                } break;
                case MK_MBUTTON:{
                    controller.m3.held = true;
                    controller.m3.pressed = true;
                } break;
            }
            bool was_down = ((l_param & (1 << 30)) != 0);
            bool is_down = ((l_param & (1 << 31)) == 0);
		} break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:{
            bool was_down = ((l_param & (1 << 30)) != 0);
            bool is_down = ((l_param & (1 << 31)) == 0);
            if(is_down != was_down){
                switch(w_param){
                    case 'W':{
                        controller.up.held = is_down;
                        controller.up.pressed = is_down;
                    } break;
                    case 'S':{
                        controller.down.held = is_down;
                        controller.down.pressed = is_down;
                    } break;
                    case 'A':{
                        controller.left.held = is_down;
                        controller.left.pressed = is_down;
                    } break;
                    case 'D':{
                        controller.right.held = is_down;
                        controller.right.pressed = is_down;
                    } break;
                    case '1':{
                        controller.one.held = is_down;
                        controller.one.pressed = is_down;
                    } break;
                    case '2':{
                        controller.two.held = is_down;
                        controller.two.pressed = is_down;
                    } break;
                    case '3':{
                        controller.three.held = is_down;
                        controller.three.pressed = is_down;
                    } break;
                    case '4':{
                        controller.four.held = is_down;
                        controller.four.pressed = is_down;
                    } break;
                    case VK_ESCAPE:{
                        OutputDebugStringA("quiting\n");
                        global_running = false;
                    } break;
                }
            }
        } break;
        default:{
            result = DefWindowProc(window, message, w_param, l_param);
        } break;
    }
    return(result);
}

i32 WinMain(HINSTANCE instance, HINSTANCE pinstance, LPSTR command_line, i32 window_type){
    WNDCLASSW window_class = {0};
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = win_message_handler_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"window class";

    init_memory(&memory);
    init_clock(&clock);
    init_render_buffer(&render_buffer, 1280, 720);

    if(RegisterClassW(&window_class)){
        HWND window = CreateWindowW(window_class.lpszClassName, L"Title", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

        if(window){
            // 30 50
            if(SetWindowPos(window, HWND_TOP, 10, 10, render_buffer.width + 30, render_buffer.height + 50, SWP_SHOWWINDOW)){
                global_running = true;

				u64 prev_cycles = __rdtsc();
                f32 prev_ticks = clock.get_ticks();
                while(global_running){
					u64 now_cycles = __rdtsc();
                    f32 now_ticks = clock.get_ticks();
#if DEBUG
					DebugCycleCounter frame_cycles = {(now_cycles - prev_cycles), 1, "frame"};
                    DebugTickCounter frame_ticks = {clock.get_seconds_elapsed(prev_cycles, now_cycles), 1, "frame"};
                    cycle_counters[DebugCycleCounter_frame] = frame_cycles;
                    tick_counters[DebugTickCounter_frame] = frame_ticks;
#endif

                    MSG message;
                    while(PeekMessageW(&message, window, 0, 0, PM_REMOVE)){
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }

                    //update_game(&memory, &render_buffer, &controller, &clock, &sound, &threads);
                    //BEGIN_CYCLE_COUNTER(update);
                    //BEGIN_TICK_COUNTER_L(update);
                    update_game(&memory, &render_buffer, &controller, &clock);
                    //END_CYCLE_COUNTER(update);
                    //END_TICK_COUNTER_L(update);

                    // NOTE: Debug
                    handle_debug_counters();

                    update_window(window, render_buffer);
                    controller.up.pressed = false;
                    controller.down.pressed = false;
                    controller.left.pressed = false;
                    controller.right.pressed = false;
                    controller.one.pressed = false;
                    controller.two.pressed = false;
                    controller.three.pressed = false;
                    controller.four.pressed = false;
                    controller.m1.pressed = false;
                    controller.m2.pressed = false;
                    controller.m3.pressed = false;

					prev_cycles = now_cycles;
					prev_ticks = now_ticks;
                }
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