#if !defined(GAME_H)

//#include <stdbool.h> //FUTURE: Use this later once your more comfortable for bool
#include <stdint.h>

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define ArrayCount(value) (sizeof(value)/sizeof(value[0]))

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define local_static static
#define global static

#include <windows.h>
#include <stdio.h>
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
typedef size_t size;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef float f32;
typedef double f64;

typedef int32_t bool;
enum{false, true};

typedef enum{MOUSE_NONE, MOUSE_LBUTTON, MOUSE_RBUTTON, MOUSE_MBUTTON, MOUSE_XBUTTON1, MOUSE_XBUTTON2,MOUSE_WHEEL} EventMouse;
typedef enum{PAD_NONE, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_BACK} EventPad;
typedef enum{KEY_NONE, KEY_W, KEY_A, KEY_S, KEY_D, KEY_L, KEY_P, KEY_ESCAPE} EventKey;
typedef enum{EVENT_NONE, EVENT_KEYDOWN, EVENT_KEYUP, EVENT_MOUSEWHEEL, EVENT_MOUSEDOWN, EVENT_MOUSEUP, EVENT_MOUSEMOTION, EVENT_TEXT, EVENT_PADDOWN, EVENT_PADUP} EventType;

typedef struct Event{
    EventType type;
    EventKey key;
    EventPad pad;
    EventMouse mouse;
    i32 mouse_x;
    i32 mouse_y;
    i8 wheel_y;
    i8 wheel_x;
    // QUESTION: ask about ui16 vs WCHAR
    ui16 text;
} Event;

typedef struct Events{
    Event event[256];
    ui32 size;
    ui32 index;
} Events;

typedef struct RenderBuffer{
    void *memory;
    int memory_size;

    int bytes_per_pixel;
    int width;
    int height;
    int pitch;
} RenderBuffer;

typedef struct FileData{
    ui32 size;
    void* content;
} FileData;

typedef struct Controller{
    bool up;
    bool down;
    bool left;
    bool right;

    f32 dt;
} Controller;

#define READ_ENTIRE_FILE(name) FileData name(char *filename)
typedef READ_ENTIRE_FILE(ReadEntireFile);

#define WRITE_ENTIRE_FILE(name) bool name(char *filename, void *memory, ui32 memory_size)
typedef WRITE_ENTIRE_FILE(WriteEntireFile);

#define FREE_FILE_MEMORY(name) void name(void *memory)
typedef FREE_FILE_MEMORY(FreeFileMemory);

typedef struct GameMemory{
	bool running;
    bool initialized;

    ui64 total_size;
    ui64 permanent_storage_size;
    ui64 temporary_storage_size;

    void *total_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup
    void *temporary_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup
    void *permanent_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup

    ReadEntireFile *read_entire_file;
    WriteEntireFile *write_entire_file;
    FreeFileMemory *free_file_memory;
} GameMemory;

#define MAIN_GAME_LOOP(name) void name(GameMemory *memory, RenderBuffer *render_buffer, Events *events, Controller *controller)
typedef MAIN_GAME_LOOP(MainGameLoop);

static int
string_length(char* s){
    int count = 0;
    while(*s++){
        ++count;
    }

    return(count);
}

static char *
string_point_at_last(char s[], char t, int count){
    char *result = NULL;
    char *found[10];
    ui32 i = 0;

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
    int left_size = string_length(left);
    int right_size = string_length(right);

    for(int i=0; i < left_size; ++i){
        *dest++ = *left++;
    }
    for(int i=0; i < right_size; ++i){
        *dest++ = *right++;
    }
     
    *dest++ = 0;
}

static void
get_root_dir(char *left, i64 length, char *full_path){
    int i=0;
    for(;i < length; ++i){
        *left++ = *full_path++;
    }
    *left++ = 0;
}

typedef struct GameState{
    i32 player_tilemap_x;
    i32 player_tilemap_y;

    f32 player_x;
    f32 player_y;
} GameState;

typedef struct TileMap{
    // QUESTION: why uint32
    ui32 *tiles;

} TileMap;

typedef struct World{
    i32 col_count;
    i32 row_count;
    i32 tile_col_count;
    i32 tile_row_count;

    f32 x;
    f32 y;
    f32 tile_w;
    f32 tile_h;

    TileMap *tilemaps;
} World;

typedef struct Position{
    i32 tilemap_x;
    i32 tilemap_y;

    i32 tile_x;
    i32 tile_y;

    f32 x;
    f32 y;
} Position;

#define GAME_H
#endif
