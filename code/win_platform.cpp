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

#include "base_inc.h"
#include "win32_base_inc.h"

#define THREADED 1
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
#define END_TICK_COUNTER(ID) tick_counters[DebugTickCounter_##ID].tick_count += clock->get_ms_elapsed(StartTickCounter_##ID, clock->get_ticks()); ++tick_counters[DebugTickCounter_##ID].hit_count; tick_counters[DebugTickCounter_##ID].name = #ID;
#define END_TICK_COUNTER_L(ID) tick_counters[DebugTickCounter_##ID].tick_count += clock.get_ms_elapsed(StartTickCounter_##ID, clock.get_ticks()); ++tick_counters[DebugTickCounter_##ID].hit_count; tick_counters[DebugTickCounter_##ID].name = #ID;

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

    s32 width;
    s32 height;
    s32 bytes_per_pixel;
    s32 stride;

    BITMAPINFO bitmap_info;

    Arena* render_command_arena;
    Arena* render_command_arenas[5][5];
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

    render_buffer->render_command_arena = allocate_arena(MB(16));
    for(u32 y=0; y < 5; ++y){
        for(u32 x=0; x < 5; ++x){
            render_buffer->render_command_arenas[y][x] = allocate_arena(MB(16));
        }
    }
}

static void
update_window(HWND window, RenderBuffer render_buffer){
    HDC device_context = GetDC(window);

    s32 x_offset = 10;
    s32 y_offset = 10;

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
            controller.mouse_pos.x = (s32)(l_param & 0xFFFF);
            controller.mouse_pos.y = render_buffer.height - (s32)(l_param >> 16);
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

struct WorkQueue;
#define work_queue_callback(name) void name(WorkQueue* queue, void* data)
typedef work_queue_callback(WorkQueueCallback);

typedef void AddWorkEntry(WorkQueue* queue, WorkQueueCallback* callback, void* data);
typedef void CompleteAllWork(WorkQueue* queue);

typedef struct WorkQueueEntry{
    void* data;
    WorkQueueCallback* callback;
} WorkQueueEntry;

typedef struct WorkQueue{
    volatile u32 work_count;
    volatile u32 work_done;
    volatile u32 write_location;
    volatile u32 read_location;

    HANDLE semaphore_handle;
    WorkQueueEntry entries[256];
} WorkQueue;

typedef struct ThreadContext{
    WorkQueue* queue;
    u32 thread_count;
    AddWorkEntry* add_work_entry;
    CompleteAllWork* complete_all_work;
} ThreadContext;
global ThreadContext thread_context;

#include <intrin.h>
static void
add_work_entry(WorkQueue* queue, WorkQueueCallback* callback, void* data){
    u32 next_write_location = (queue->write_location + 1) % ArrayCount(queue->entries);
    Assert(next_write_location != queue->read_location);

    WorkQueueEntry* entry = queue->entries + queue->write_location;
    entry->callback = callback;
    entry->data = data;
    ++queue->work_count;
    _WriteBarrier(); // I dont think I need this since variables that are being modified are volatile.
    //queue->write_location = (queue->write_location + 1) % ArrayCount(queue->entries);
    //Assert(queue->write_location != queue->read_location);

    queue->write_location = next_write_location;
    ReleaseSemaphore(queue->semaphore_handle, 1, 0);
}

static bool
do_next_work_entry(WorkQueue* queue){
    bool sleep = false;

    u32 current_read_location = queue->read_location;
    u32 next_read_location = (current_read_location + 1) % ArrayCount(queue->entries);

    if(current_read_location != queue->write_location){
        u32 index = InterlockedCompareExchange((LONG volatile *)&queue->read_location, next_read_location, current_read_location);

        if(index == current_read_location){
            WorkQueueEntry entry = queue->entries[index];
            entry.callback(queue, entry.data);
            InterlockedIncrement((LONG volatile *)&queue->work_done);
        }

    }
    else{
        sleep = true;
    }
    return(sleep);
}

static void
complete_all_work(WorkQueue* queue){
    while(queue->work_done != queue->work_count){
        do_next_work_entry(queue);
    }

    queue->work_done = 0;
    queue->work_count = 0;

}

DWORD thread_proc(LPVOID param){
    WorkQueue* queue = (WorkQueue*)param;
    for(;;){
        if(do_next_work_entry(queue)){
            WaitForSingleObjectEx(queue->semaphore_handle, INFINITE, FALSE);
        }
    }
    return(0);
}

static work_queue_callback(print_string){
    char* string = (char*)data;
    print("Thread %i: %s\n", GetCurrentThreadId(), string);
}

#include "game.h"

s32 WinMain(HINSTANCE instance, HINSTANCE pinstance, LPSTR command_line, s32 window_type){

#if THREADED
    u32 thread_count = 23;
#else
    u32 thread_count = 0;
#endif
    WorkQueue queue = {};
    queue.semaphore_handle = CreateSemaphoreExW(0, 0, thread_count, 0, 0, SEMAPHORE_ALL_ACCESS);

    thread_context.queue = &queue;
    thread_context.thread_count = thread_count;
    thread_context.add_work_entry = add_work_entry;
    thread_context.complete_all_work = complete_all_work;

    for(u32 i=0; i<thread_count; ++i){
        DWORD thread_id;
        HANDLE thread_handle = CreateThread(0, 0, thread_proc, thread_context.queue, 0, &thread_id);
        CloseHandle(thread_handle);
    }

    //add_work_entry(&queue, print_string, (void*)"job A0");
    //add_work_entry(&queue, print_string, (void*)"job A1");
    //add_work_entry(&queue, print_string, (void*)"job A2");
    //add_work_entry(&queue, print_string, (void*)"job A3");
    //add_work_entry(&queue, print_string, (void*)"job A4");
    //add_work_entry(&queue, print_string, (void*)"job A5");
    //add_work_entry(&queue, print_string, (void*)"job A6");
    //add_work_entry(&queue, print_string, (void*)"job A7");
    //add_work_entry(&queue, print_string, (void*)"job A8");
    //add_work_entry(&queue, print_string, (void*)"job A9");

    //add_work_entry(&queue, print_string, (void*)"job B0");
    //add_work_entry(&queue, print_string, (void*)"job B1");
    //add_work_entry(&queue, print_string, (void*)"job B2");
    //add_work_entry(&queue, print_string, (void*)"job B3");
    //add_work_entry(&queue, print_string, (void*)"job B4");
    //add_work_entry(&queue, print_string, (void*)"job B5");
    //add_work_entry(&queue, print_string, (void*)"job B6");
    //add_work_entry(&queue, print_string, (void*)"job B7");
    //add_work_entry(&queue, print_string, (void*)"job B8");
    //add_work_entry(&queue, print_string, (void*)"job B9");

    // QUESTION: Is this necessary? can you not send the job through thread_proc on the main thread as well?
    //complete_all_work(&queue);

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
            if(SetWindowPos(window, HWND_TOP, 10, 10, render_buffer.width + 30, render_buffer.height + 50, SWP_SHOWWINDOW)){
                global_running = true;

                f64 FPS = 0;
                f64 MSPF = 0;
                u64 frame_count = 0;

                s64 prev_ticks = clock.get_ticks();
                s64 frame_time = clock.get_ticks();
				u64 prev_cycles = __rdtsc();

                //f64 dt =  0.05;
                //f64 accumulator = 0.0;

                while(global_running){
                    MSG message;
                    while(PeekMessageW(&message, window, 0, 0, PM_REMOVE)){
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }
					u64 now_cycles = __rdtsc();
					prev_cycles = now_cycles;

                    s64 now_ticks = clock.get_ticks();
                    MSPF = 1000/((f64)clock.frequency / (f64)(now_ticks - prev_ticks));
#if DEBUG
                    clock.dt = 1.0/60.0;
#else
                    clock.dt = 1.0/60.0;
                    //clock.dt = clock.get_seconds_elapsed(prev_ticks, now_ticks);
#endif
                    //f64 frame_time = clock.get_seconds_elapsed(prev_ticks, now_ticks);
					prev_ticks = now_ticks;

                    //accumulator += frame_time;
                    //f64 t1;
                    //f64 t3_start = clock.get_ticks();
                    //while(accumulator >= dt){
                    //    clock.dt = dt;
                    //    s64 t1_start = get_ticks();
                    //    update_game(&memory, &render_buffer, &controller, &clock, &thread_context);
                    //    t1 = clock.get_ms_elapsed(t1_start, clock.get_ticks());
                    //    accumulator -= dt;
                    //}
                    //f64 t3 = clock.get_ms_elapsed(t3_start, clock.get_ticks());

                    frame_count++;
                    f64 time_elapsed = clock.get_seconds_elapsed(frame_time, clock.get_ticks());
                    if(time_elapsed > 1){
                        FPS = (frame_count / time_elapsed);
                        frame_time = clock.get_ticks();
                        frame_count = 0;
                    }

                    s64 start_update = clock.get_ticks();
                    update_game(&memory, &render_buffer, &controller, &clock, &thread_context);
                    f64 update_time = clock.get_ms_elapsed(start_update, clock.get_ticks());
                    s64 start_draw = get_ticks();
                    for(u32 y=0; y < 5; ++y){
                        for(u32 x=0; x < 5; ++x){
                            draw_commands(&render_buffer, render_buffer.render_command_arenas[y][x]);
                        }
                    }
                    //draw_commands(&render_buffer, render_buffer.render_command_arena);
                    f64 draw_time = clock.get_ms_elapsed(start_draw, clock.get_ticks());
                    print("FPS: %f - MSPF: %f - update: %f - draw: %f\n", FPS, MSPF, update_time, draw_time);
                    //print("FPS: %f - MSPF: %f\n", FPS, MSPF);

                    //END_CYCLE_COUNTER(update);
                    //END_TICK_COUNTER_L(update);

                    // NOTE: Debug
                    //handle_debug_counters();

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
