#if !defined(GAME_H)


#include "game_platform.h"
#include "random.h"
#include <time.h>
#include "vectors.h"
#include "linked_list.h"
#include "math.h"

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
    u16 file_type;
	u32 file_size;
	u16 reserved1;
	u16 reserved2;
	u32 bitmap_offset;
	u32 header_size;
	i32  width;
	i32  height;
	u16 planes;
	u16 bits_per_pixel;
    u32 Compression;
	u32 SizeOfBitmap;
	i32  HorzResolution;
	i32  VertResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;
//    u32 RedMask;
//    u32 GreenMask;
//    u32 BlueMask;
} BitmapHeader;
#pragma pack(pop)

typedef struct Bitmap{
    BitmapHeader *header; //maybe i dont want this here?
	u32  width;
	u32  height;
    u32 *pixels;
} Bitmap;


typedef struct Arena {
    void *base;
    size_t size;
    size_t used;
} Arena;

static void
initialize_arena(Arena *arena, void *base, size_t size){
    arena->base = base;
    arena->size = size;
    arena->used = 0;
}

#define allocate_array(arena, count, type) (type *)allocate_size_(arena, count * sizeof(type));
#define allocate_struct(arena, type) (type *)allocate_size_(arena, sizeof(type));
#define allocate_size(arena, size) allocate_size_(arena, size);
static void*
allocate_size_(Arena *arena, size_t size){
    void *result = (u8*)arena->base + arena->used;
    arena->used += size;
    assert(arena->used < arena->size);
    return(result);
}

typedef enum {EntityFlag_Movable} EntityFlags;
typedef enum {EntityType_None, EntityType_Player, EntityType_Object, EntityType_Pixel, EntityType_Line, EntityType_Ray, EntityType_Segment, EntityType_Triangle, EntityType_Rect, EntityType_Quad, EntityType_Box, EntityType_Circle, EntityType_Bitmap, EntityType_Food, EntityType_Ant, EntityType_Colony, EntityType_ToHomePheromone, EntityType_ToFoodPheromone} EntityType;
typedef enum {AntState_Wondering, AntState_Collecting, AntState_Depositing} AntState;

typedef struct EntityHandle{
    u32 index;
    u32 generation;
} EntityHandle;

typedef struct Entity{
    u32 index;
    u32 generation;

    EntityType type;
    struct Entity* first_child;
    struct Entity* next_child;
    i32 children_count;
    u32 flags;
    v2 position;
    v2 dimension;
    v2 direction;
    v4 color;
    bool fill;
    bool draw_bounding_box;
    v4 outline_color;
    v2 p0;
    v2 p1;
    v2 p2;
    v2 p3;
    u8 rad;
    f32 speed;

    bool out_of_bounds_x;
    bool out_of_bounds_y;

    AntState ant_state;

    EntityHandle ant_food;
    EntityHandle food_ant;

    v2 random_vector;
    i64 direction_change_timer;
    f64 direction_change_timer_max;
	f32 rot_percent;
    f32 rotate_speed;
    bool rotation_complete;

    bool food_targeted;
    bool food_collected;

    v2 right_sensor;
    v2 left_sensor;
    v2 mid_sensor;
    u8 sensor_radius;
    f32 sensor_angle;
    f32 sensor_distance;
    i64 pheromone_spawn_timer;
    f64 pheromone_spawn_timer_max;
	f64 pheromone_alpha_start;

    f32 right_sensor_density;
    f32 forward_sensor_density;
    f32 left_sensor_density;
    bool forward;
    bool right;
    bool left;

    bool colony_targeted;
    v2 target_direction;

    f32 pher_home_decay_rate;
    f32 pher_food_decay_rate;
    u64 food_decay_timer;
    u64 home_decay_timer;

    Bitmap image;
    bool render;
} Entity;

static bool
has_flags(Entity *e, u32 flags){
    return(e->flags & flags);
}

static void
set_flags(Entity *e, u32 flags){
    e->flags |= flags;
}

static void
clear_flags(Entity *e, u32 flags){
    e->flags &= ~flags;
}


static void
reset_LL_sentinel(LinkedList *sentinel){
    sentinel->count = 0;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    sentinel->data = NULL;
}


#include "renderer.h"

typedef struct TransientState{
    Arena transient_arena;
    Arena *render_commands_arena;
    Arena *LL_arena;
} TransientState;

typedef struct PermanentState{
    i64 frame_ticks_now;
    i64 frame_ticks_prev;
    Arena permanent_arena;

    f32 screen_width;
    f32 screen_height;

    u64 start;
    bool added;

    bool draw_home_phers;
    bool draw_food_phers;
    bool draw_depositing_ants;
    bool draw_wondering_ants;

    i32 cell_row_count;
    i32 cell_width;
    i32 cell_height;
    i32 border_size;

    LinkedList ants;
    LinkedList misc;
    LinkedList food_cells     [16][16];
    LinkedList pher_cells     [16][16];
    LinkedList pher_food_cells[16][16];
    LinkedList pher_home_cells[16][16];

    u32 free_entities[110000];
    i32 free_entities_at;

    u32 generation[110000];

    Entity entities[110000];

    Entity* colony;
    Entity* player;

    u32 ants_count;
    f32 ant_speed;

    u32 clip_region_index;
    Controller controller;
    Bitmap test;
    Bitmap circle;
    Bitmap image;

    bool add_food;
    u64 frame_index;
} PermanentState;

static EntityHandle
zero_entity_handle(){
    EntityHandle result = {0};
    return(result);
}

static Entity*
entity_from_handle(PermanentState* state, EntityHandle handle){
    Entity *result = NULL;
    if(handle.index < array_count(state->entities)){
        Entity *e = state->entities + handle.index;
        if(e->generation == handle.generation){
            result = e;
        }
    }
    return(result);
}

static EntityHandle
handle_from_entity(PermanentState* state, Entity *e){
    assert(e != NULL);
    EntityHandle result = {0};
    if((e >= (state->entities + 0)) && (e < (state->entities + array_count(state->entities)))){
        result.index = e->index;
        result.generation = e->generation;
    }
    return(result);
}

static void
add_child(PermanentState *state, u32 parent_index, u32 child_index){
    Entity *parent = state->entities + parent_index;
    Entity *child = state->entities + child_index;
    child->next_child = parent->first_child;
    parent->first_child = child;
}

static void
remove_entity(PermanentState* state, Entity* e){
    // TODO: make sure im not supposed to do [free_entities_at + 1]
    e->type = EntityType_None;
    state->free_entities_at++;
    state->free_entities[state->free_entities_at] = e->index;
    e->index = 0;
    e->generation = 0;
}

static Entity*
add_entity(PermanentState *state, EntityType type){
    if(state->free_entities_at >= 0){
        i32 free_entity_index = state->free_entities[state->free_entities_at--];
        Entity *e = state->entities + free_entity_index;
        e->index = free_entity_index;
        state->generation[e->index] += 1;
        e->generation = state->generation[e->index];
        e->type = type;

        return(e);
    }
    //if(state->entities_at < state->entities_size){
    //    Entity *result = state->entities + state->entities_at;
    //    result->index = state->entities_at;
    //    result->type = type;
    //    state->entities_at++;

    //    return(result->index);
    //}
    return(0);
}

static Entity*
add_pixel(PermanentState* state, v2 position, v4 color){
    Entity* e = add_entity(state, EntityType_Pixel);
    e->position = position;
    e->color = color;
    return(e);
}

static Entity*
add_segment(PermanentState* state, v2 p0, v2 p1, v4 color){
    Entity* e = add_entity(state, EntityType_Segment);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    return(e);
}

static Entity*
add_ray(PermanentState* state, v2 position, v2 direction, v4 color){
    Entity* e = add_entity(state, EntityType_Ray);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_line(PermanentState* state, v2 position, v2 direction, v4 color){
    Entity* e = add_entity(state, EntityType_Line);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_rect(PermanentState* state, v2 position, v2 dimension, v4 color){
    Entity* e = add_entity(state, EntityType_Rect);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    return(e);
}

static Entity*
add_box(PermanentState* state, v2 position, v2 dimension, v4 color){
    Entity* e = add_entity(state, EntityType_Box);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    return(e);
}

static Entity*
add_quad(PermanentState* state, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
    Entity* e = add_entity(state, EntityType_Quad);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->p3 = p3;
    e->fill = fill;
    return(e);
}

static Entity*
add_triangle(PermanentState *state, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    Entity* e = add_entity(state, EntityType_Triangle);
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->color = color;
    e->fill = fill;
    return(e);
}

static Entity*
add_circle(PermanentState *state, v2 pos, u8 rad, v4 color, bool fill){
    Entity* e = add_entity(state, EntityType_Circle);
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    return(e);
}

static Entity*
add_bitmap(PermanentState* state, v2 position, Bitmap image){
    Entity* e = add_entity(state, EntityType_Bitmap);
    e->position = position;
    e->image = image;
    return(e);
}

static Entity*
add_player(PermanentState *state, v2 position, v2 dimension, v4 color, Bitmap image){
    Entity* e = add_entity(state, EntityType_Player);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    e->image = image;
	e->speed = 50;
    return(e);
}

static Entity*
add_food(PermanentState *state, v2 pos, u8 rad, v4 color, bool fill){
    Entity* e = add_entity(state, EntityType_Food);

    e->position = pos;
    e->color = color;
    e->food_targeted = false;
    e->food_collected = false;
    //e->dimension = dimension;
    e->draw_bounding_box = true;
    e->rad = rad;
    e->fill = true;
    return(e);
}

static Entity*
add_ant(PermanentState *state, v2 pos, u8 rad, v4 color, bool fill){
    Entity* e = add_entity(state, EntityType_Ant);

    e->ant_state = AntState_Wondering;
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    e->direction.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
    e->direction.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
    e->random_vector.x = e->direction.x;
    e->random_vector.y = e->direction.y;
    e->target_direction.x = e->direction.x;
    e->target_direction.y = e->direction.y;
    e->rot_percent = 0.0f;
    e->direction_change_timer_max = 0;
    e->pheromone_spawn_timer_max = 0.25f;
    e->rot_percent = 1.0f;
#if DEBUG
    e->speed = 140.0f; // constant dt
#else
    e->speed = 80.0f; // variable dt
#endif
    e->draw_bounding_box = true;
    e->sensor_radius = 10;
    e->sensor_angle = 60;
    e->sensor_distance = 20;
    e->right_sensor_density = 0.0f;
    e->forward_sensor_density = 0.0f;
    e->left_sensor_density = 0.0f;
    e->rotate_speed = 0.05f;
    e->rotation_complete = true;

    return(e);
}

static Entity*
add_colony(PermanentState *state, v2 pos, u8 rad, v4 color, bool fill){
    Entity* e = add_entity(state, EntityType_Colony);
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    e->draw_bounding_box = true;
    e->speed = 250.0f;
    return(e);
}

static Entity*
add_to_home_pheromone(PermanentState *state, v2 pos, u8 rad, v4 color){
    Entity* e = add_entity(state, EntityType_ToHomePheromone);
    e->position = pos;
    e->pheromone_alpha_start = 0.25;
    e->color = color;
    e->color.a = e->pheromone_alpha_start;
    e->rad = rad;
    e->pher_home_decay_rate = 18.0f;
    return(e);
}

static Entity*
add_to_food_pheromone(PermanentState *state, v2 pos, u8 rad, v4 color){
    Entity* e = add_entity(state, EntityType_ToFoodPheromone);
    e->position = pos;
    e->pheromone_alpha_start = 0.25;
    e->color = color;
    e->color.a = e->pheromone_alpha_start;
    e->rad = rad;
    e->pher_food_decay_rate = 18.0f;
    return(e);
}

static Bitmap
load_bitmap(GameMemory *memory, char *filename){
    Bitmap result = {0};

    char full_path[256];
    cat_strings(memory->data_dir, filename, full_path);

    FileData bitmap_file = memory->read_entire_file(full_path);
    if(bitmap_file.size > 0){
        BitmapHeader *header = (BitmapHeader *)bitmap_file.content;
        result.pixels = (u32 *)((u8 *)bitmap_file.content + header->bitmap_offset);
        result.width = header->width;
        result.height = header->height;
        // IMPORTANT: depending on compression type you might have to re order the bytes to make it AA RR GG BB
        // as well as consider the masks for each color included in the header
    }
    return(result);
}
#define GAME_H
#endif
