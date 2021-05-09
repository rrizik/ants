#if !defined(GAME_H)


#include "game_platform.h"
#include "random.h"
#include <time.h>
#include "vectors.h"

// get rid of this shit
#include <stdio.h>
#include <windows.h>
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

#pragma pack(push, 1)
typedef struct BitmapHeader {
    ui16 file_type;
	ui32 file_size;
	ui16 reserved1;
	ui16 reserved2;
	ui32 bitmap_offset;
	ui32 header_size;
	i32  width;
	i32  height;
	ui16 planes;
	ui16 bits_per_pixel;
    ui32 Compression;
	ui32 SizeOfBitmap;
	i32  HorzResolution;
	i32  VertResolution;
	ui32 ColorsUsed;
	ui32 ColorsImportant;
//    ui32 RedMask;
//    ui32 GreenMask;
//    ui32 BlueMask;
} BitmapHeader;
#pragma pack(pop)

typedef struct Bitmap{
    BitmapHeader *header; //maybe i dont want this here?
	ui32  width;
	ui32  height;
    ui32 *pixels;
} Bitmap;


typedef struct MemoryArena {
    ui8 *base;
    size_t size;
    size_t used;
} MemoryArena; 

static void
initialize_arena(MemoryArena *arena, ui8 *base, size_t size){
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

#define push_array(arena, count, type) (type *)push_size_(arena, count * sizeof(type));
#define push_struct(arena, type) (type *)push_size_(arena, sizeof(type));
#define push_size(arena, size) push_size_(arena, size);
static void*
push_size_(MemoryArena *arena, size_t size){
    assert((arena->used + size) < arena->size);
    void *result = arena->base + arena->used;
    arena->used += size;

    return(result);
}

#include "renderer.h"

typedef enum EntityFlags {EntityFlag_Movable} EntityFlags;
typedef enum EntityType {EntityType_None, EntityType_Player, EntityType_Object, EntityType_Pixel, EntityType_Line, EntityType_Ray, EntityType_Segment, EntityType_Triangle, EntityType_Rect, EntityType_Quad, EntityType_Box, EntityType_Circle, EntityType_Bitmap, EntityType_Food, EntityType_Ant, EntityType_Colony} EntityType;
typedef enum AntState {AntState_Wondering, AntState_Collecting, AntState_Depositing} AntState;

typedef struct Entity{
    int id;
    int index;
    struct Entity* first_child;
    struct Entity* next_child;
    int children_count;
    ui32 flags;
    EntityType type;
    v2 position;
    v2 dimension;
    v2 direction;
    v4 color;
    f32 x;
    f32 y;
    f32 w;
    f32 h;
    bool fill;
    bool draw_bounding_box;
    v4 outline_color;
    v2 p0;
    v2 p1;
    v2 p2;
    v2 p3;
    ui8 rad;
    f32 speed;
    AntState ant_state;
    struct Entity *ant_food;
    Bitmap image;
} Entity;



static bool
has_flags(Entity *e, ui32 flags){
    return(e->flags & flags);
}

static void
set_flags(Entity *e, ui32 flags){
    e->flags |= flags;
}

static void
clear_flags(Entity *e, ui32 flags){
    e->flags &= ~flags; 
}

typedef struct TranState{
    MemoryArena transient_arena;
    RenderCommands *render_commands;
} TranState;

typedef struct GameState{
    MemoryArena permanent_arena;

    ui32 colony_index;
    Entity* food[2048];// fthis
    ui32 food_count;// fthis
    Entity* ants[1024];// fthis
    ui32 ants_count;// fthis

    Entity entities[1024];
    ui32 entities_size;
    ui32 entity_at;
    ui32 entities_free;

    f32 ant_speed;

    ui32 clip_region_index;
    Controller controller;
    Bitmap test;
    Bitmap circle;
    Bitmap image;

    f32 ant_speed_max;
    f32 suck_speed;
    f32 suck_speed_max;

    ui32 c2_index;
    ui32 player_index;
} GameState;

static void
add_child(GameState *game_state, ui32 parent_index, ui32 child_index){
    Entity *parent = game_state->entities + parent_index;
    Entity *child = game_state->entities + child_index;
    child->next_child = parent->first_child;
    parent->first_child = child;
}

static ui32
add_entity(GameState *game_state, EntityType type){
    if(game_state->entities_free > 0){
        for(ui32 entity_index = 1; entity_index <= game_state->entity_at; ++entity_index){
            Entity *result = game_state->entities + entity_index;
            if(result->type == EntityType_None){
                result->index = entity_index;
                result->id = entity_index;
                result->type = type;
                game_state->entities_free--;
            }
        }
    }
    if(game_state->entity_at < game_state->entities_size){
        Entity *result = game_state->entities + game_state->entity_at;
        result->index = game_state->entity_at;
        result->id = game_state->entity_at;
        result->type = type;
        game_state->entity_at++;

        return(result->index);
    }
    return(0);
}

static ui32
add_pixel(GameState* game_state, f32 x, f32 y, v4 color){
    ui32 e_index = add_entity(game_state, EntityType_Pixel);
    Entity *e = game_state->entities + e_index;
    e->x = x;
    e->y = y;
    e->color = color;
    return(e->index);
}

static ui32
add_segment(GameState* game_state, v2 p0, v2 p1, v4 color){
    ui32 e_index = add_entity(game_state, EntityType_Segment);
    Entity *e = game_state->entities + e_index;
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    return(e->index);
}

static ui32
add_ray(GameState* game_state, v2 position, v2 direction, v4 color){
    ui32 e_index = add_entity(game_state, EntityType_Ray);
    Entity *e = game_state->entities + e_index;
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e->index);
}

static ui32
add_line(GameState* game_state, v2 position, v2 direction, v4 color){
    ui32 e_index = add_entity(game_state, EntityType_Line);
    Entity *e = game_state->entities + e_index;
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e->index);
}

static ui32
add_rect(GameState* game_state, v2 position, v2 dimension, v4 color){
    ui32 e_index = add_entity(game_state, EntityType_Rect);
    Entity *e = game_state->entities + e_index;
    e->position = position;
    e->dimension = dimension;
    e->w = dimension.w;
    e->h = dimension.h;
    e->color = color;
    return(e->index);
}

static ui32
add_box(GameState* game_state, v2 position, v2 dimension, v4 color){
    ui32 e_index = add_entity(game_state, EntityType_Box);
    Entity *e = game_state->entities + e_index;
    e->position = position;
    e->dimension = dimension;
    e->w = dimension.w;
    e->h = dimension.h;
    e->color = color;
    return(e->index);
}

static ui32
add_quad(GameState* game_state, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
    ui32 e_index = add_entity(game_state, EntityType_Quad);
    Entity *e = game_state->entities + e_index;
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->p3 = p3;
    e->fill = fill;
    return(e->index);
}

static ui32
add_triangle(GameState *game_state, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    ui32 e_index = add_entity(game_state, EntityType_Triangle);
    Entity *e = game_state->entities + e_index;
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->color = color;
    e->fill = fill;
    return(e->index);
}

static ui32
add_circle(GameState *game_state, v2 pos, ui8 rad, v4 color, bool fill){
    ui32 e_index = add_entity(game_state, EntityType_Circle);
    Entity *e = game_state->entities + e_index;
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    return(e->index);
}

static ui32
add_bitmap(GameState* game_state, v2 position, Bitmap image){
    ui32 e_index = add_entity(game_state, EntityType_Bitmap);
    Entity *e = game_state->entities + e_index;
    e->position = position;
    e->x = position.x;
    e->y = position.y;
    e->image = image;
    return(e->index);
}

static ui32
add_player(GameState *game_state, v2 position, v2 dimension, v4 color, Bitmap image){
    ui32 e_index = add_entity(game_state, EntityType_Player);
    Entity *e = game_state->entities + e_index;
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    e->image = image;
    return(e->index);
    //e->w = dimension.w;
    //e->h = dimension.h;
    //e->x = position.x;
    //e->y = position.y;
}

static ui32
add_food(GameState *game_state, v2 pos, ui8 rad, v4 color, bool fill){
    ui32 e_index = add_entity(game_state, EntityType_Food);
    Entity *e = game_state->entities + e_index;
    game_state->food[game_state->food_count++] = e;
    e->position = pos;
    e->color = color;

    e->rad = rad;
    e->draw_bounding_box = true;
    return(e->index);
}

static ui32
add_ant(GameState *game_state, v2 pos, ui8 rad, v4 color, bool fill){
    ui32 e_index = add_entity(game_state, EntityType_Ant);
    Entity *e = game_state->entities + e_index;
    game_state->ants[game_state->ants_count++] = e;
    e->ant_state = AntState_Wondering;
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    e->direction = vec2((f32)(range_random(100)+1), (f32)(range_random(100)+1));
    e->draw_bounding_box = true;
    e->speed = 250.0f;
    return(e->index);
}

static ui32
add_colony(GameState *game_state, v2 pos, ui8 rad, v4 color, bool fill){
    ui32 e_index = add_entity(game_state, EntityType_Colony);
    Entity *e = game_state->entities + e_index;
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    e->draw_bounding_box = true;
    e->speed = 250.0f;
    return(e->index);
}

//static ui32
//add_ant(GameState *game_state, v2 position, v2 dimension, v4 color, Bitmap image){
//    ui32 e_index = add_entity(game_state, EntityType_Player);
//    Entity *e = game_state->entities + e_index;
//    e->position = position;
//    e->dimension = dimension;
//    e->color = color;
//    e->image = image;
//    return(e->index);
//}

static v4
convert_ui32_v4_normalized(ui32 value){
	f32 alpha = ((f32)((value >> 24) & 0xFF) / 255.0f);
	f32 red =   ((f32)((value >> 16) & 0xFF) / 255.0f);
	f32 green = ((f32)((value >> 8) & 0xFF) / 255.0f);
	f32 blue =  ((f32)((value >> 0) & 0xFF) / 255.0f);
    v4 result = {red, green, blue, alpha};
    return result;
}

#define GAME_H
#endif
