#if !defined(GAME_PLATFORM_H)

#include <stdint.h>
#include <stdbool.h>

//typedef float __attribute__((ext_vector_type(2))) v2;
typedef size_t size;
typedef int8_t i8;
typedef int16_t i16; 
typedef int32_t i32;
typedef int64_t i64;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;


#define assert(expresion) if(!(expresion)) __debugbreak();
#define array_count(value) (sizeof(value)/sizeof(value[0]))
#define array_length(value) (sizeof(value)/sizeof(value[0]))
#define array_size(value) (sizeof(value)/sizeof(value[0]))
#define ABS(n) ((n)<0 ? -(n) : (n))
#define PI 3.14159265f
#define RAD 0.0174533f
#define RAD2DEG(n) ((180.0f/PI) * (n))
#define DEG2RAD(n) ((PI/180.0f) * (n))

#define MAX(a,b) ((a) >= (b) ? a : b)
#define MIN(a,b) ((a) <= (b) ? a : b)

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define local_static static
#define global static

#define RDTSC 1
#define TICKS 1

typedef enum EventMouse{MOUSE_NONE, MOUSE_LBUTTON, MOUSE_RBUTTON, MOUSE_MBUTTON, MOUSE_XBUTTON1, MOUSE_XBUTTON2, MOUSE_WHEEL} EventMouse;
typedef enum EventPad{PAD_NONE, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_BACK} EventPad;
typedef enum EventKey{KEY_NONE, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_ESCAPE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0} EventKey;
typedef enum EventType{EVENT_NONE, EVENT_KEYDOWN, EVENT_KEYUP, EVENT_MOUSEWHEEL, EVENT_MOUSEDOWN, EVENT_MOUSEUP, EVENT_MOUSEMOTION, EVENT_TEXT, EVENT_PADDOWN, EVENT_PADUP} EventType;


typedef struct Event{
    EventType type;
    EventKey key;
    EventPad pad;
    EventMouse mouse;
    i32 mouse_x;
    i32 mouse_y;
    i8 wheel_y;
    i8 wheel_x;
    // QUESTION: ask about u16 vs WCHAR
    u16 text;
} Event;

typedef struct Events{
    Event event[256];
    u32 size;
    u32 index;
} Events;

typedef struct RenderBuffer{
    void *memory;
    i32 memory_size;

    i32 bytes_per_pixel;
    i32 width;
    i32 height;
    i32 pitch;
} RenderBuffer;

typedef struct FileData{
    u32 size;
    void* content;
} FileData;

#include "vectors.h"
typedef struct Controller{
    bool up;
    bool down;
    bool left;
    bool right;
    bool m1;
    bool m2;
    bool m3;
    v2 mouse_pos;
} Controller;

#define WIN_GET_TICKS(name) i64 name(void)
typedef WIN_GET_TICKS(GetTicks);

#define WIN_GET_SECONDS_ELAPSED(name) f32 name(i64 start, i64 end)
typedef WIN_GET_SECONDS_ELAPSED(GetSecondsElapsed);

#define WIN_GET_MS_ELAPSED(name) f32 name(u64 start, u64 end)
typedef WIN_GET_MS_ELAPSED(GetMsElapsed);

typedef struct Clock{
    GetTicks *get_ticks;
    GetSecondsElapsed *get_seconds_elapsed;
    GetMsElapsed *get_ms_elapsed;
    i64 frequency;
    f32 dt;

    u64 cpu_prev;
    u64 cpu_now;
} Clock;

//static f32 
//get_cycles_elapsed(u64 end, u64 start){
//    f32 result = (f32)(end - start) / (1000 * 1000);
//    return(result);
//}

static f64 
get_cycles_elapsed(u64 end, u64 start){
    f64 result = (f64)(end - start);
    return(result);
}

//static f32
//get_seconds_elapsed(u64 start, u64 end, u64 frequency){
//    f32 result;
//    result = ((f32)(end- start) / ((f32)frequency));
//
//    return(result);
//}

static i32
string_length(char* s){
    i32 count = 0;
    while(*s++){
        ++count;
    }

    return(count);
}

static char *
string_point_at_last(char s[], char t, i32 count){
    char *result = NULL;
    char *found[10];
    u32 i = 0;

    while(*s++){
        if(*s == t){
            found[i] = s + 1;
            i++;
        }
    }
    result = found[i - count];

    return(result);
}

static void
cat_strings(char *left, char *right, char *dest){
    i32 left_size = string_length(left);
    i32 right_size = string_length(right);

    for(i32 i=0; i < left_size; ++i){
        *dest++ = *left++;
    }
    for(i32 i=0; i < right_size; ++i){
        *dest++ = *right++;
    }

    *dest++ = 0;
}

static char*
end_of_string(char *str){
    while(*str++){
    }
    return(str);
}

static void
get_root_dir(char *left, i64 length, char *full_path){
    i32 i=0;
    for(;i < length; ++i){
        *left++ = *full_path++;
    }
    *left++ = 0;
}

static void
copy_string(char *src, char *dst, size_t size){
    for(u32 i=0; i < size; ++i){
        *dst++ = *src++;
    }
}

static void
copy_array(v2 *a, v2 *b, i32 count){
    for(i32 i=0; i < count; ++i){
        *a++ = *b++;
    }
}

static f32
f32_max(f32 a, f32 b){
    if(a >= b){
        return(a);
    }
    return(b);
}

static f32
f32_min(f32 a, f32 b){
    if(a <= b){
        return(a);
    }
    return(b);
}

#define READ_ENTIRE_FILE(name) FileData name(char *filename)
typedef READ_ENTIRE_FILE(ReadEntireFile);

#define WRITE_ENTIRE_FILE(name) bool name(char *filename, void *memory, u32 memory_size)
typedef WRITE_ENTIRE_FILE(WriteEntireFile);

#define FREE_FILE_MEMORY(name) void name(void *memory)
typedef FREE_FILE_MEMORY(FreeFileMemory);
    

enum{
    DebugCycleCounter_frame,
    DebugCycleCounter_init,
    DebugCycleCounter_reset_sentinels,
    DebugCycleCounter_LL_setup,
    DebugCycleCounter_events,
    DebugCycleCounter_controller,
    DebugCycleCounter_degrade_pher,
    DebugCycleCounter_behavior,
    DebugCycleCounter_state_none,
    DebugCycleCounter_state_wondering,
    DebugCycleCounter_wondering,
    DebugCycleCounter_wondering_search,
    DebugCycleCounter_wondering_search_food,
    DebugCycleCounter_wondering_search_pher,
    DebugCycleCounter_state_collecting,
    DebugCycleCounter_state_depositing,
    DebugCycleCounter_depositing_search,
    DebugCycleCounter_allocate_commands,
    DebugCycleCounter_draw,
    DebugCycleCounter_count,
};

enum{
    DebugTickCounter_frame,
    DebugTickCounter_init,
    DebugTickCounter_reset_sentinels,
    DebugTickCounter_LL_setup,
    DebugTickCounter_events,
    DebugTickCounter_controller,
    DebugTickCounter_degrade_pher,
    DebugTickCounter_behavior,
    DebugTickCounter_state_none,
    DebugTickCounter_state_wondering,
    DebugTickCounter_wondering,
    DebugTickCounter_wondering_search,
    DebugTickCounter_wondering_search_food,
    DebugTickCounter_wondering_search_pher,
    DebugTickCounter_state_collecting,
    DebugTickCounter_state_depositing,
    DebugTickCounter_depositing_search,
    DebugTickCounter_allocate_commands,
    DebugTickCounter_draw,
    DebugTickCounter_count,
};

typedef struct DebugCycleCounter{
    u64 cycle_count;
    u32 hit_count;
    char *name;
} DebugCycleCounter;

typedef struct DebugTickCounter{
    f32 tick_count;
    u32 hit_count;
    char *name;
} DebugTickCounter;

struct GameMemory *debug_global_memory;


#if RDTSC
#define BEGIN_CYCLE_COUNTER(ID) u64 StartCycleCounter_##ID = __rdtsc();
#define END_CYCLE_COUNTER(ID) debug_global_memory->cycle_counters[DebugCycleCounter_##ID].cycle_count += __rdtsc() - StartCycleCounter_##ID; ++debug_global_memory->cycle_counters[DebugCycleCounter_##ID].hit_count; debug_global_memory->cycle_counters[DebugCycleCounter_##ID].name = #ID;
#else
#define BEGIN_CYCLE_COUNTER(ID)
#define END_CYCLE_COUNTER(ID)
#endif
#if TICKS
#define BEGIN_TICK_COUNTER(ID) u64 StartTickCounter_##ID = clock->get_ticks()
#define END_TICK_COUNTER(ID) debug_global_memory->tick_counters[DebugTickCounter_##ID].tick_count += ((f32)(clock->get_ticks() - StartTickCounter_##ID) / ((f32)clock->frequency)); ++debug_global_memory->tick_counters[DebugTickCounter_##ID].hit_count; debug_global_memory->tick_counters[DebugTickCounter_##ID].name = #ID;
#else
#define BEGIN_TICK_COUNTER(ID)
#define END_TICK_COUNTER(ID)
#endif


typedef struct GameMemory{
	bool running;
    bool initialized;

    void *total_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup
    void *permanent_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup
    void *transient_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup

    u64 total_size;
    u64 permanent_storage_size;
    u64 transient_storage_size;

    char root_dir[256];
    char data_dir[256];

    ReadEntireFile *read_entire_file;
    WriteEntireFile *write_entire_file;
    FreeFileMemory *free_file_memory;

    DebugCycleCounter cycle_counters[DebugCycleCounter_count];
    DebugTickCounter tick_counters[DebugTickCounter_count];
} GameMemory;

#define MAIN_GAME_LOOP(name) void name(GameMemory *memory, RenderBuffer *render_buffer, Events *events, Clock *clock)
typedef MAIN_GAME_LOOP(MainGameLoop);


#define GAME_PLATFORM_H
#endif
