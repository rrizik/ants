#if !defined(GAME_H)

//#include <stdbool.h> //FUTURE: Use this later once your more comfortable with bool
#include <stdint.h>

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define array_count(value) (sizeof(value)/sizeof(value[0]))
#define ABS(n) ((n)<0 ? -(n) : (n))
#define PI 3.14159265f
#define RAD2DEG(n) ((180.0f/PI) * n);
#define DEG2RAD(n) ((PI/180.0f) * n);

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

//typedef float __attribute__((ext_vector_type(2))) Vec2;
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

typedef enum{PIXEL, SEGMENT, RAY, LINE, TRIANGLE, RECTANGLE, QUAD, BOX, CIRCLE} GeometryType;
typedef enum{MOUSE_NONE, MOUSE_LBUTTON, MOUSE_RBUTTON, MOUSE_MBUTTON, MOUSE_XBUTTON1, MOUSE_XBUTTON2,MOUSE_WHEEL} EventMouse;
typedef enum{PAD_NONE, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_BACK} EventPad;
typedef enum{KEY_NONE, KEY_W, KEY_A, KEY_S, KEY_D, KEY_L, KEY_P, KEY_ESCAPE, KEY_1, KEY_2, KEY_3} EventKey;
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

typedef struct Color{
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} Color;

#include "vectors.h"

typedef struct Move{
    bool up;
    bool down;
    bool right;
    bool left;
} Move;

typedef struct Rect{
    f32 x, y, w, h, width, height;
} Rect;

static Rect
rect(Vec2 pos, Vec2 dim){
    Rect result = {0};
    result.x = pos.x;
    result.y = pos.y;
    result.w = dim.x;
    result.h = dim.y;
    result.width = result.w;
    result.height = result.h;
    return(result);
}

typedef struct Tri{
    Vec2 p0;
    Vec2 p1;
    Vec2 p2;
    Rect rect;
} Tri;

static Tri
triangle(Vec2 p0, Vec2 p1, Vec2 p2){
    Tri result = {0};
    result.p0 = p0;
    result.p1 = p1;
    result.p2 = p2;
    
    if(result.p0.x > result.p1.x) { swap_v2(&result.p0, &result.p1); };
    if(result.p0.x > result.p2.x) { swap_v2(&result.p0, &result.p2); };
    if(result.p1.x > result.p2.x) { swap_v2(&result.p1, &result.p2); };

    f32 left_x = p0.x;
    f32 right_x = p2.x;

    if(result.p0.y > result.p1.y) { swap_v2(&result.p0, &result.p1); };
    if(result.p0.y > result.p2.y) { swap_v2(&result.p0, &result.p2); };
    if(result.p1.y > result.p2.y) { swap_v2(&result.p1, &result.p2); };

    f32 bottom_y = result.p0.y; 
    f32 top_y = result.p2.y; 

    Vec2 pos = {left_x, bottom_y};
    Vec2 dim = {ABS(right_x - left_x), ABS(top_y - bottom_y)};
    Rect r = rect(pos, dim);

    result.rect = r;
}

static bool
rect_collide_rect(Rect r1, Rect r2){
    if((r1.x < r2.x + r2.w) &&
       (r1.x + r1.w > r2.x) &&
       (r1.y < r2.y + r2.h) &&
       (r1.y + r1.h > r2.y)){
        return true;
    }
    return false;
}

static bool
rect_collide_point(Rect r1, Vec2 p){
    if((p.x < r1.x + r1.w) &&
       (p.x > r1.x) &&
       (p.y < r1.y + r1.h) &&
       (p.y > r1.y)){
        return true;
    }
    return false;
}

static bool
rect_contains_rect(Rect r1, Rect r2){
    if((r2.x > r1.x) &&
       (r2.x + r2.w < r1.x + r1.w) &&
       (r2.y > r1.y) &&
       (r2.y + r2.h < r1.y + r1.h)){
        return true;
    }
    return false;
}

typedef struct Entity{
    bool is_geometry; 
    GeometryType geometry_type;
} Entity;

typedef struct Entities{
    Entity *e;
    i32 i;
    size count;
} Entities;

static void
add_entity(Entities *entities, Entity e){
    entities->e[entities->i] = e;
    entities->i++;
}

static Entity
entity(bool is_geometry, GeometryType geometry_type){
    Entity result = {is_geometry, geometry_type};
    return(result);
}

typedef struct GameState{
    Move move;
    Vec2 test_background[4];
    Vec2 box1[4];
    Vec2 box2[4];
    Rect r1;
    Rect r2;
    Entities entities;
} GameState;


#define GAME_H
#endif
