#include "random.h"
#include "math.h"
#include "renderer.h"

f64 dt;
s32 cell_width;
s32 cell_height;
s32 cell_row_count;
s32 border_size;

typedef enum {EntityFlag_Movable} EntityFlags;
typedef enum {EntityType_None, EntityType_Player, EntityType_Object, EntityType_Pixel, EntityType_Line, EntityType_Ray, EntityType_Segment, EntityType_Triangle, EntityType_Rect, EntityType_Quad, EntityType_Box, EntityType_Circle, EntityType_Bitmap, EntityType_Food, EntityType_Ant, EntityType_Colony, EntityType_ToHomePheromone, EntityType_ToFoodPheromone} EntityType;
typedef enum {AntState_Wondering, AntState_Collecting, AntState_Depositing} AntState;

typedef struct EntityHandle{
    u32 index;
    u32 generation;
} EntityHandle;

static EntityHandle
zero_entity_handle(){
    EntityHandle result = {0};
    return(result);
}

typedef struct Entity{
    u32 index;
    u32 generation;

    EntityType type;
    struct Entity* first_child;
    struct Entity* next_child;
    s32 children_count;
    u32 flags;
    v2 position;
    v2s32 dimension;
    v2 direction;
    RGBA color;
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
    bool out_of_bounds;

    AntState ant_state;

    EntityHandle ant_food;
    EntityHandle food_ant;

    v2 random_vector;
    f64 direction_change_timer;
    f64 direction_change_timer_max;
	f64 rot_percent;
    f32 rotate_speed;
    bool rotation_complete;

    bool food_targeted;
    bool food_collected;

    s32 sensor_radius;
    f32 sensor_angle;
    f32 sensor_distance;
    f64 pheromone_spawn_timer;
    f64 pheromone_spawn_timer_max;
	f64 pheromone_alpha_start;

    bool forward;
    bool right;
    bool left;

    bool colony_targeted;
    v2 target_direction;

    f64 pher_decay_max;
    f64 pher_decay;

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

typedef struct Array{
    Entity* element[40000];
    u32 count = 40000;
    u32 at = 0;
} Array;

typedef struct PermanentMemory{
    size_t size;
    Arena arena;
    Arena *assets_arena;
    String8 cwd; // CONSIDER: this might be something we want to be set on the platform side
    String8 dir_assets; // CONSIDER: this might be something we want to be set on the platform side
    s64 frame_ticks_now;
    s64 frame_ticks_prev;

    f32 screen_width;
    f32 screen_height;

    u64 start;
    bool added;

    bool draw_home_phers;
    bool draw_food_phers;
    bool draw_depositing_ants;
    bool draw_wondering_ants;

    s32 border_size;
    s32 cell_row_count;
    s32 cell_width;
    s32 cell_height;

    s32 cell_row_count_r;
    s32 cell_width_r;
    s32 cell_height_r;


    Entity* ants_list[1000];
    Array food_cells       [16][16];
    Array pher_food_cells  [16][16];
    Array pher_home_cells  [16][16];

    u32 generation[110000];
    u32 free_entities[110000];
    u32 free_entities_at;

    Entity entities[110000];
    u32 entities_count;

    Entity* colony;
    Entity* player;

    u32 ants_count;
    f32 ant_speed;

    u32 clip_region_index;
    Bitmap test;
    Bitmap circle;
    Bitmap image;

    bool food_added;
    u64 frame_index;
} PermanentMemory;

typedef struct TransientMemory{
    Arena arena;
    Arena *frame_arena;
    Arena *render_command_arena;
    Arena *LL_arena;
} TransientMemory;

static Entity*
entity_from_handle(PermanentMemory* pm, EntityHandle handle){
    Entity *result = NULL;
    if(handle.index < ArrayCount(pm->entities)){
        Entity *e = pm->entities + handle.index;
        if(e->generation == handle.generation){
            result = e;
        }
    }
    return(result);
}

static EntityHandle
handle_from_entity(PermanentMemory* pm, Entity *e){
    Assert(e != NULL);
    EntityHandle result = {0};
    if((e >= (pm->entities + 0)) && (e < (pm->entities + ArrayCount(pm->entities)))){
        result.index = e->index;
        result.generation = e->generation;
    }
    return(result);
}

static void
add_child(PermanentMemory *pm, u32 parent_index, u32 child_index){
    Entity *parent = pm->entities + parent_index;
    Entity *child = pm->entities + child_index;
    child->next_child = parent->first_child;
    parent->first_child = child;
}

static void
remove_entity(PermanentMemory* pm, Entity* e){
    // TODO: make sure im not supposed to do [free_entities_at + 1]
    e->type = EntityType_None;
    pm->free_entities_at++;
    pm->free_entities[pm->free_entities_at] = e->index;
    e->index = 0;
    e->generation = 0;
}

static Entity*
add_entity(PermanentMemory *pm, EntityType type){
    if(pm->free_entities_at >= 0){
        s32 free_entity_index = pm->free_entities[pm->free_entities_at--];
        Entity *e = pm->entities + free_entity_index;
        e->index = free_entity_index;
        pm->generation[e->index] += 1;
        e->generation = pm->generation[e->index];
        e->type = type;

        return(e);
    }
    //if(pm->entities_at < pm->entities_size){
    //    Entity *result = pm->entities + pm->entities_at;
    //    result->index = pm->entities_at;
    //    result->type = type;
    //    pm->entities_at++;

    //    return(result->index);
    //}
    return(0);
}

static Entity*
add_pixel(PermanentMemory* pm, v2 position, RGBA color){
    Entity* e = add_entity(pm, EntityType_Pixel);
    e->position = position;
    e->color = color;
    return(e);
}

static Entity*
add_segment(PermanentMemory* pm, v2 p0, v2 p1, RGBA color){
    Entity* e = add_entity(pm, EntityType_Segment);
    e->color = color;
    e->position = p0;
    e->p0 = p0;
    e->p1 = p1;
    return(e);
}

static Entity*
add_ray(PermanentMemory* pm, v2 position, v2 direction, RGBA color){
    Entity* e = add_entity(pm, EntityType_Ray);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_line(PermanentMemory* pm, v2 position, v2 direction, RGBA color){
    Entity* e = add_entity(pm, EntityType_Line);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_rect(PermanentMemory* pm, v2 position, v2s32 dimension, RGBA color){
    Entity* e = add_entity(pm, EntityType_Rect);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    return(e);
}

static Entity*
add_box(PermanentMemory* pm, v2 position, v2s32 dimension, RGBA color){
    Entity* e = add_entity(pm, EntityType_Box);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    return(e);
}

static Entity*
add_quad(PermanentMemory* pm, v2 p0, v2 p1, v2 p2, v2 p3, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Quad);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->p3 = p3;
    e->fill = fill;
    return(e);
}

static Entity*
add_triangle(PermanentMemory *pm, v2 p0, v2 p1, v2 p2, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Triangle);
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->color = color;
    e->fill = fill;
    return(e);
}

static Entity*
add_circle(PermanentMemory *pm, v2 pos, u8 rad, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Circle);
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    return(e);
}

static Entity*
add_bitmap(PermanentMemory* pm, v2 position, Bitmap image){
    Entity* e = add_entity(pm, EntityType_Bitmap);
    e->position = position;
    e->image = image;
    return(e);
}

static Entity*
add_player(PermanentMemory *pm, v2 position, v2s32 dimension, RGBA color, Bitmap image){
    Entity* e = add_entity(pm, EntityType_Player);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    e->image = image;
	e->speed = 50;
    return(e);
}

static Entity*
add_food(PermanentMemory *pm, v2 pos, u8 rad, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Food);

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
add_ant(PermanentMemory *pm, v2 pos, u8 rad, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Ant);

    e->ant_state = AntState_Wondering;
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    e->direction.x = (((s32)(random_range((2 * 100) + 1)) - 100)/100.0f);
    e->direction.y = (((s32)(random_range((2 * 100) + 1)) - 100)/100.0f);
    e->random_vector.x = e->direction.x;
    e->random_vector.y = e->direction.y;
    e->target_direction.x = e->direction.x;
    e->target_direction.y = e->direction.y;
    e->rot_percent = 0.0f;
    e->direction_change_timer_max = (random_range(3) + 1);
    e->direction_change_timer = e->direction_change_timer_max;
    e->pheromone_spawn_timer_max = 0.25;
    e->pheromone_spawn_timer = e->pheromone_spawn_timer_max;
    e->speed = 140.0f;
    e->draw_bounding_box = true;
    e->sensor_radius = 10;
    e->sensor_angle = 60;
    e->sensor_distance = 25;
    //e->right_sensor_density = 0.0f;
    //e->forward_sensor_density = 0.0f;
    //e->left_sensor_density = 0.0f;
    e->rotate_speed = 0.05f;
    e->rotation_complete = true;

    return(e);
}

static Entity*
add_colony(PermanentMemory *pm, v2 pos, u8 rad, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Colony);
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    e->draw_bounding_box = true;
    e->speed = 250.0f;
    return(e);
}

static Entity* 
add_to_home_pheromone(PermanentMemory *pm, v2 pos, RGBA color){
    Entity* e = add_entity(pm, EntityType_ToHomePheromone);
    e->position = pos;
    e->color = color;
    e->pheromone_alpha_start = 0.25;
    e->color.a = e->pheromone_alpha_start;
    e->pher_decay_max = 10.0f;
    e->pher_decay = e->pher_decay_max;
    return(e);
}

static Entity*
add_to_food_pheromone(PermanentMemory *pm, v2 pos, RGBA color){
    Entity* e = add_entity(pm, EntityType_ToFoodPheromone);
    e->position = pos;
    e->color = color;
    e->pheromone_alpha_start = 0.25;
    e->color.a = e->pheromone_alpha_start;
    e->pher_decay_max = 10.0f;
    e->pher_decay = e->pher_decay_max;
    return(e);
}

typedef struct BehaviorWork{
    RenderBuffer* render_buffer;
    u32 start;
    u32 end;
} BehaviorWork;

typedef struct DrawWork{
    RenderBuffer* render_buffer;
    Arena* arena;
} DrawWork;


static work_queue_callback(do_draw_commands){
    BEGIN_CYCLE_COUNTER(draw);
    BEGIN_TICK_COUNTER_L(draw);
    DrawWork* work = (DrawWork*)data;
    RenderBuffer* render_buffer = work->render_buffer;
    Arena* commands = work->arena;

    void* at = commands->base;
    void* end = (u8*)commands->base + commands->used;
    while(at != end){
        CommandHeader* base_command = (CommandHeader*)at;

        switch(base_command->type){
            case RenderCommand_ClearColor:{
                ClearColorCommand *command = (ClearColorCommand*)base_command;
                clear(render_buffer, command->header.position, command->header.size, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Pixel:{
                PixelCommand *command = (PixelCommand*)base_command;
                draw_pixel(render_buffer, command->header.position, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Segment:{
                SegmentCommand *command = (SegmentCommand*)base_command;
                draw_segment(render_buffer, command->p0, command->p1, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Ray:{
                RayCommand *command = (RayCommand*)base_command;
                draw_ray(render_buffer, command->header.position, command->direction, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Line:{
                LineCommand *command = (LineCommand*)base_command;
                draw_line(render_buffer, command->header.position, command->direction, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Rect:{
                RectCommand *command = (RectCommand*)base_command;
                if(command->header.color.g == 1.0f){
                    s32 i = 1;
                }
                //draw_rect_slow(render_buffer, command->header.position, command->dimension, command->header.color);
                draw_rect_fast(render_buffer, command->header.position, command->dimension, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Box:{
                BoxCommand *command = (BoxCommand*)base_command;
                draw_box(render_buffer, command->header.position, command->dimension, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Quad:{
                QuadCommand *command = (QuadCommand*)base_command;
                draw_quad(render_buffer, command->p0, command->p1, command->p2, command->p3, command->header.color, command->header.fill);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Triangle:{
                TriangleCommand *command = (TriangleCommand*)base_command;
                draw_triangle(render_buffer, command->p0, command->p1, command->p2, base_command->color, base_command->fill);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Circle:{
                CircleCommand *command = (CircleCommand*)base_command;
                draw_circle(render_buffer, command->header.position.x, command->header.position.y, command->rad, command->header.color, command->header.fill);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Bitmap:{
                BitmapCommand *command = (BitmapCommand*)base_command;
                draw_bitmap(render_buffer, command->header.position, command->image);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
        }
    }
    END_TICK_COUNTER_L(draw);
    END_CYCLE_COUNTER(draw);
}


static void
draw_commands(RenderBuffer *render_buffer, Arena *commands){
    void* at = commands->base;
    void* end = (u8*)commands->base + commands->used;
    while(at != end){
        CommandHeader* base_command = (CommandHeader*)at;

        switch(base_command->type){
            case RenderCommand_ClearColor:{
                ClearColorCommand *command = (ClearColorCommand*)base_command;
                clear(render_buffer, command->header.position, command->header.size, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Pixel:{
                PixelCommand *command = (PixelCommand*)base_command;
                draw_pixel(render_buffer, command->header.position, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Segment:{
                SegmentCommand *command = (SegmentCommand*)base_command;
                draw_segment(render_buffer, command->p0, command->p1, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Ray:{
                RayCommand *command = (RayCommand*)base_command;
                draw_ray(render_buffer, command->header.position, command->direction, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Line:{
                LineCommand *command = (LineCommand*)base_command;
                draw_line(render_buffer, command->header.position, command->direction, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Rect:{
                RectCommand *command = (RectCommand*)base_command;
                if(command->header.color.g == 1.0f){
                    s32 i = 1;
                }
                //draw_rect_slow(render_buffer, command->header.position, command->dimension, command->header.color);
                draw_rect_fast(render_buffer, command->header.position, command->dimension, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Box:{
                BoxCommand *command = (BoxCommand*)base_command;
                draw_box(render_buffer, command->header.position, command->dimension, command->header.color);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Quad:{
                QuadCommand *command = (QuadCommand*)base_command;
                draw_quad(render_buffer, command->p0, command->p1, command->p2, command->p3, command->header.color, command->header.fill);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Triangle:{
                TriangleCommand *command = (TriangleCommand*)base_command;
                draw_triangle(render_buffer, command->p0, command->p1, command->p2, base_command->color, base_command->fill);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Circle:{
                CircleCommand *command = (CircleCommand*)base_command;
                draw_circle(render_buffer, command->header.position.x, command->header.position.y, command->rad, command->header.color, command->header.fill);
                //draw_circle_fast(render_buffer, command->header.position.x, command->header.position.y, command->rad, command->header.color, command->header.fill);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
            case RenderCommand_Bitmap:{
                BitmapCommand *command = (BitmapCommand*)base_command;
                draw_bitmap(render_buffer, command->header.position, command->image);
                at = (u8*)commands->base + command->header.arena_used;
            } break;
        }
    }
}

PermanentMemory* pm;
TransientMemory* tm;

#if 1
static work_queue_callback(do_ant_behavior){

    BEGIN_CYCLE_COUNTER(behavior);
    BEGIN_TICK_COUNTER_L(behavior);
    BehaviorWork* work = (BehaviorWork*)data;

    RGBA MAGENTA = {1.0f, 0.0f, 1.0f,  0.1f};
    RGBA TEAL =    {0.0f, 1.0f, 1.0f,  1.0f};

    for(u32 i=work->start; i<work->end; ++i){
        BEGIN_CYCLE_COUNTER(state_none);
        BEGIN_TICK_COUNTER_L(state_none);

        Entity *ant = pm->ants_list[i];
    }
}
#endif
    

static void
fill_cell_with_food(u32 x_start, u32 y_start){
    for(u32 x = x_start; x < (u32)(x_start + pm->cell_width); x+=4){
        for(u32 y = y_start; y < (u32)(y_start + pm->cell_height); y+=4){
            add_food(pm, vec2(x, y), 1, (RGBA){0.0f, 1.0f, 0.0f,  1.0f}, true);
        }
    }
}

static void 
update_game(Memory* memory, RenderBuffer* render_buffer, Controller* controller, Clock* clock, ThreadContext* thread_context){
    Assert(sizeof(PermanentMemory) < memory->permanent_size);
    Assert(sizeof(TransientMemory) < memory->transient_size);
    pm = (PermanentMemory*)memory->permanent_base;
    tm = (TransientMemory*)memory->transient_base;

    RGBA RED =     {1.0f, 0.0f, 0.0f,  1.0f};
    RGBA GREEN =   {0.0f, 1.0f, 0.0f,  1.0f};
    RGBA BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    RGBA MAGENTA = {1.0f, 0.0f, 1.0f,  0.1f};
    RGBA TEAL =    {0.0f, 1.0f, 1.0f,  1.0f};
    RGBA PINK =    {0.92f, 0.62f, 0.96f, 1.0f};
    RGBA YELLOW =  {0.9f, 0.9f, 0.0f,  1.0f};
    RGBA ORANGE =  {1.0f, 0.5f, 0.15f,  1.0f};
    RGBA DGRAY =   {0.5f, 0.5f, 0.5f,  1.0f};
    RGBA LGRAY =   {0.8f, 0.8f, 0.8f,  1.0f};
    RGBA WHITE =   {1.0f, 1.0f, 1.0f,  1.0f};
    RGBA BLACK =   {0.0f, 0.0f, 0.0f,  1.0f};



    if(!memory->initialized){

        Entity a = {0};
        pm->size = sizeof(&a);
        init_arena(&pm->arena, (u8*)memory->permanent_base + sizeof(PermanentMemory), memory->permanent_size - sizeof(PermanentMemory));
        init_arena(&tm->arena, (u8*)memory->transient_base + sizeof(TransientMemory), memory->transient_size - sizeof(TransientMemory));
        pm->cwd = os_get_cwd(&pm->arena);
        pm->dir_assets = str8_concatenate(&pm->arena, pm->cwd, str8_literal("\\data\\"));

        pm->assets_arena = push_arena(&pm->arena, MB(1));
        tm->render_command_arena = push_arena(&tm->arena, MB(16));

        tm->LL_arena = push_arena(&tm->arena, MB(800));
        tm->frame_arena = push_arena(&tm->arena, MB(100));

        pm->screen_width = render_buffer->width;
        pm->screen_height = render_buffer->height;

        seed_random(1, 0);

        pm->cell_row_count = 16;
        pm->cell_width = render_buffer->width / pm->cell_row_count;
        pm->cell_height = render_buffer->height / pm->cell_row_count;

        pm->cell_row_count_r = 5;
        pm->cell_width_r = render_buffer->width / pm->cell_row_count_r;
        pm->cell_height_r = render_buffer->height / (thread_context->thread_count + 1);

        // setup free entities array
        pm->free_entities_at = ArrayCount(pm->free_entities) - 1;
        pm->entities_count = ArrayCount(pm->entities) - 1;
        for(s32 i = ArrayCount(pm->free_entities) - 1; i >= 0; --i){
            pm->free_entities[i] = ArrayCount(pm->free_entities) - 1 - i;
        }
        
        Entity *zero_entity = add_entity(pm, EntityType_None);
        pm->colony = add_colony(pm, vec2(render_buffer->width/2, 100), 25, DGRAY, true);

        // add ants
        //pm->ants_count = 800;
        for(u32 i=0; i < ArrayCount(pm->ants_list); ++i){
            //Entity* ant = add_ant(pm, pm->colony->position, 2, LGRAY, true);
            Entity* ant = add_ant(pm, (v2){(f32)(render_buffer->width/2) - 4, 100 - 4}, 2, LGRAY, true);
            pm->player = ant;
            pm->ants_list[i] = ant;
        }

        // add grid
        for(s32 i=1; i < pm->cell_row_count; ++i){
            // vertical
            add_segment(pm, {((f32)render_buffer->width - pm->cell_width * i), 0}, {((f32)render_buffer->width - pm->cell_width * i), (f32)render_buffer->height}, DGRAY);
            // horizonal
            add_segment(pm, {0, ((f32)render_buffer->height - pm->cell_height * i)}, {(f32)render_buffer->width, ((f32)render_buffer->height - pm->cell_height * i)}, DGRAY);
        }
        //fill_cell_with_food(10*pm->cell_width, 10*pm->cell_height);

        pm->border_size = 10;
        add_segment(pm, vec2(pm->border_size, pm->border_size), vec2(render_buffer->width - pm->border_size, pm->border_size), RED);
        add_segment(pm, vec2(pm->border_size, pm->border_size), vec2(pm->border_size, render_buffer->height - pm->border_size), RED);
        add_segment(pm, vec2(render_buffer->width - pm->border_size, render_buffer->height - pm->border_size), vec2(pm->border_size, render_buffer->height - pm->border_size), RED);
        add_segment(pm, vec2(render_buffer->width - pm->border_size, render_buffer->height - pm->border_size), vec2(render_buffer->width - pm->border_size, pm->border_size), RED);


        pm->food_added = false;
		pm->draw_home_phers = true;
		pm->draw_food_phers = true;
		pm->draw_depositing_ants = true;
		pm->draw_wondering_ants = true;

        dt = clock->dt;
        cell_width = pm->cell_width;
        cell_height = pm->cell_height;
        cell_row_count = pm->cell_row_count;
        border_size = pm->border_size;

        memory->initialized = true;
    }

    BEGIN_CYCLE_COUNTER(reset_sentinels);
    BEGIN_TICK_COUNTER(reset_sentinels);
    // reset LL
    for(s32 y=0; y < cell_row_count; ++y){
        for(s32 x=0; x < cell_row_count; ++x){
            pm->food_cells[y][x].at = 0;
            pm->pher_food_cells[y][x].at = 0;
            pm->pher_home_cells[y][x].at = 0;
        }
    }
    tm->LL_arena->used = 0;
    END_TICK_COUNTER(reset_sentinels);
    END_CYCLE_COUNTER(reset_sentinels);

    BEGIN_CYCLE_COUNTER(LL_setup);
    BEGIN_TICK_COUNTER(LL_setup);
    // Array setup
    u32 count = 0;
    for(u32 entity_index = pm->free_entities_at; entity_index < ArrayCount(pm->entities); ++entity_index){
        count++;
        Entity *e = pm->entities + pm->free_entities[entity_index];
        switch(e->type){
            case EntityType_Food:{
                if(!e->food_targeted && !e->food_collected){
                    v2 cell_coord = vec2(floor(e->position.x / cell_width), floor(e->position.y / cell_height));
                    cell_coord.x = clamp_f32(0, cell_coord.x, cell_row_count - 1);
                    cell_coord.y = clamp_f32(0, cell_coord.y, cell_row_count - 1);
                    
                    u32 at = pm->food_cells[(s32)cell_coord.y][(s32)cell_coord.x].at;
                    pm->food_cells[(s32)cell_coord.y][(s32)cell_coord.x].element[at] = e;
                    pm->food_cells[(s32)cell_coord.y][(s32)cell_coord.x].at++;
                }
            }break;
            case EntityType_ToHomePheromone:{
                e->pher_decay -= dt;
                if(e->pher_decay < 0){
                    e->pher_decay = e->pher_decay_max;
                    remove_entity(pm, e);
                }
                else{
                    f64 t = e->pher_decay / e->pher_decay_max;
                    e->color.a = lerp(0.0f, t, e->pheromone_alpha_start);

                    v2 cell_coord = vec2(floor(e->position.x / cell_width), floor(e->position.y / cell_height));
                    cell_coord.x = clamp_f32(0, cell_coord.x, cell_row_count - 1);
                    cell_coord.y = clamp_f32(0, cell_coord.y, cell_row_count - 1);

                    u32 at = pm->pher_home_cells[(s32)cell_coord.y][(s32)cell_coord.x].at;
                    pm->pher_home_cells[(s32)cell_coord.y][(s32)cell_coord.x].element[at] = e;
                    pm->pher_home_cells[(s32)cell_coord.y][(s32)cell_coord.x].at++;
                }
            }break;
            case EntityType_ToFoodPheromone:{
                e->pher_decay -= dt;
                if(e->pher_decay < 0){
                    e->pher_decay = e->pher_decay_max;
                    remove_entity(pm, e);
                }
                else{
                    f64 t = e->pher_decay / e->pher_decay_max;
                    e->color.a = lerp(0.0f, t, e->pheromone_alpha_start);
                    
                    v2 cell_coord = vec2(floor(e->position.x / cell_width), floor(e->position.y / cell_height));
                    cell_coord.x = clamp_f32(0, cell_coord.x, cell_row_count - 1);
                    cell_coord.y = clamp_f32(0, cell_coord.y, cell_row_count - 1);

                    u32 at = pm->pher_food_cells[(s32)cell_coord.y][(s32)cell_coord.x].at;
                    pm->pher_food_cells[(s32)cell_coord.y][(s32)cell_coord.x].element[at] = e;
                    pm->pher_food_cells[(s32)cell_coord.y][(s32)cell_coord.x].at++;
                }
            }break;
            case EntityType_Box:{
                remove_entity(pm, e);
            }break;
        }
    }
    END_TICK_COUNTER(LL_setup);
    END_CYCLE_COUNTER(LL_setup);

    BEGIN_CYCLE_COUNTER(controller);
    BEGIN_TICK_COUNTER(controller);
    u32 speed = 200;
    if(controller->up.held){
        pm->player->position.y += speed * dt;
    }
    if(controller->down.held){
        pm->player->position.y -= speed * dt;
    }
    if(controller->left.held){
        pm->player->position.x -= speed * dt;
    }
    if(controller->right.held){
        pm->player->position.x += speed * dt;
    }
    //pm->player->direction = direction2(pm->player->position, controller->mouse_pos);
    //print("dx: %f - dy: %f\n", pm->player->direction.x, pm->player->direction.y);

        // center cell and bottom left cell coords
    v2 center_cell_coord = (v2){floor_f32(pm->player->position.x / cell_width), floor_f32(pm->player->position.y / cell_height)};
    center_cell_coord.x = clamp_f32(0, center_cell_coord.x, cell_row_count - 1);
    center_cell_coord.y = clamp_f32(0, center_cell_coord.y, cell_row_count - 1);

    v2 nine_pos = (v2){(center_cell_coord.x + 1) * cell_width, (center_cell_coord.y + 1) * cell_height};
    v2 eight_pos = (v2){(center_cell_coord.x) * cell_width, (center_cell_coord.y + 1) * cell_height};
    v2 seven_pos = (v2){(center_cell_coord.x - 1) * cell_width, (center_cell_coord.y + 1) * cell_height};
    v2 six_pos = (v2){(center_cell_coord.x + 1) * cell_width, (center_cell_coord.y) * cell_height};
    v2 five_pos = (v2){(center_cell_coord.x) * cell_width, (center_cell_coord.y) * cell_height};
    v2 four_pos = (v2){(center_cell_coord.x - 1) * cell_width, (center_cell_coord.y) * cell_height};
    v2 three_pos = (v2){(center_cell_coord.x + 1) * cell_width, (center_cell_coord.y - 1) * cell_height};
    v2 two_pos = (v2){(center_cell_coord.x) * cell_width, (center_cell_coord.y - 1) * cell_height};
    v2 one_pos = (v2){(center_cell_coord.x - 1) * cell_width, (center_cell_coord.y - 1) * cell_height};

    v2s32 cell_dim = (v2s32){cell_width, cell_height};
    Rect nine_rect = rect(nine_pos, cell_dim);
    Rect eight_rect = rect(eight_pos, cell_dim);
    Rect seven_rect = rect(seven_pos, cell_dim);
    Rect six_rect = rect(six_pos, cell_dim);
    Rect five_rect = rect(five_pos, cell_dim);
    Rect four_rect = rect(four_pos, cell_dim);
    Rect three_rect = rect(three_pos, cell_dim);
    Rect two_rect = rect(two_pos, cell_dim);
    Rect one_rect = rect(one_pos, cell_dim);

    if(controller->one.pressed){
        pm->draw_home_phers = !pm->draw_home_phers;
    }
    if(controller->two.pressed){
        pm->draw_food_phers = !pm->draw_food_phers;
    }
    if(controller->three.pressed){
        pm->draw_depositing_ants = !pm->draw_depositing_ants;
    }
    if(controller->four.pressed){
        pm->draw_wondering_ants = !pm->draw_wondering_ants;
    }
    if(controller->m_right.held){
        Entity* pher = add_to_home_pheromone(pm, controller->mouse_pos, MAGENTA);
    }
    if(controller->m_middle.held){
        //Entity* pher = add_to_food_pheromone(pm, controller->mouse_pos, TEAL);
    }

    if(controller->m_left.pressed){
        if(!pm->food_added){
            pm->food_added = true;
            v2 cell_coord = vec2(floor(controller->mouse_pos.x / cell_width), floor(controller->mouse_pos.y / cell_height));
            cell_coord.x = clamp_f32(0, cell_coord.x, cell_row_count - 1);
            cell_coord.y = clamp_f32(0, cell_coord.y, cell_row_count - 1);
            u32 x_start = cell_coord.x * cell_width;
            u32 y_start = cell_coord.y * cell_height;
            fill_cell_with_food(x_start, y_start);
        }
    }
    else{
        pm->food_added = false;
    }
    END_TICK_COUNTER(controller);
    END_CYCLE_COUNTER(controller);

#if 0
    u32 length = ArrayCount(pm->ants_list) / (thread_context->thread_count + 1);
    for(u32 i=0; i < (thread_context->thread_count + 1); ++i){
        BehaviorWork* behavior_work = push_array(tm->frame_arena, BehaviorWork, 1);
        behavior_work->render_buffer = render_buffer;
        behavior_work->start = length * i;
        behavior_work->end = (length * (i + 1));
        
        thread_context->add_work_entry(thread_context->queue, do_ant_behavior, behavior_work);
    }
    thread_context->complete_all_work(thread_context->queue);
#else
    // TODO: put this back at the bottom with the rest of the render command stuff
    free_arena(render_buffer->render_command_arena);
    push_clear_color(render_buffer->render_command_arena, (v2){0, 0}, (v2){(f32)render_buffer->width, (f32)render_buffer->height}, BLACK);

    BEGIN_CYCLE_COUNTER(behavior);
    BEGIN_TICK_COUNTER(behavior);
    // bbb
    for(u32 i=0; i<ArrayCount(pm->ants_list); ++i){
        BEGIN_CYCLE_COUNTER(state_none);
        BEGIN_TICK_COUNTER(state_none);

        Entity *ant = pm->ants_list[i];

        // local ant data
        f32 right_sensor_density = 0;
        f32 forward_sensor_density = 0;
        f32 left_sensor_density = 0;
        AntState state = ant->ant_state;
        v2 target_direction = ant->target_direction;
        v2 direction = ant->direction;
        v2 position = ant->position;
        f32 rotation_percent = ant->rot_percent;
        f32 colony_distance = distance2(position, pm->colony->position);
        v2 colony_direction = direction2(position, pm->colony->position);
        bool out_of_bounds = ant->out_of_bounds;
        //if(controller->m_middle.held){
        //    target_direction = direction2(ant->position, (v2){controller->mouse_pos.x, controller->mouse_pos.y});
        //}

        // calc rect for all 9 cells
        v2 center_cell_coord = (v2){floor_f32(position.x / cell_width), floor_f32(position.y / cell_height)};
        center_cell_coord.x = clamp_f32(0, center_cell_coord.x, cell_row_count - 1);
        center_cell_coord.y = clamp_f32(0, center_cell_coord.y, cell_row_count - 1);

        v2 nine_pos = (v2){(center_cell_coord.x + 1) * cell_width, (center_cell_coord.y + 1) * cell_height};
        v2 eight_pos = (v2){(center_cell_coord.x) * cell_width, (center_cell_coord.y + 1) * cell_height};
        v2 seven_pos = (v2){(center_cell_coord.x - 1) * cell_width, (center_cell_coord.y + 1) * cell_height};
        v2 six_pos = (v2){(center_cell_coord.x + 1) * cell_width, (center_cell_coord.y) * cell_height};
        v2 five_pos = (v2){(center_cell_coord.x) * cell_width, (center_cell_coord.y) * cell_height};
        v2 four_pos = (v2){(center_cell_coord.x - 1) * cell_width, (center_cell_coord.y) * cell_height};
        v2 three_pos = (v2){(center_cell_coord.x + 1) * cell_width, (center_cell_coord.y - 1) * cell_height};
        v2 two_pos = (v2){(center_cell_coord.x) * cell_width, (center_cell_coord.y - 1) * cell_height};
        v2 one_pos = (v2){(center_cell_coord.x - 1) * cell_width, (center_cell_coord.y - 1) * cell_height};

        v2s32 cell_dim = (v2s32){cell_width, cell_height};
        Rect nine_rect = rect(nine_pos, cell_dim);
        Rect eight_rect = rect(eight_pos, cell_dim);
        Rect seven_rect = rect(seven_pos, cell_dim);
        Rect six_rect = rect(six_pos, cell_dim);
        Rect five_rect = rect(five_pos, cell_dim);
        Rect four_rect = rect(four_pos, cell_dim);
        Rect three_rect = rect(three_pos, cell_dim);
        Rect two_rect = rect(two_pos, cell_dim);
        Rect one_rect = rect(one_pos, cell_dim);

        // all three sensor rects
        f32 forward_rad = dir_to_rad(direction);
        f32 right_rad = forward_rad + (RAD * ant->sensor_angle);
        f32 left_rad = forward_rad - (RAD * ant->sensor_angle);

        v2 right_direction = rad_to_dir(right_rad);
        v2 left_direction = rad_to_dir(left_rad);

        v2 right_sensor = (v2){position.x + (ant->sensor_distance * right_direction.x), position.y + (ant->sensor_distance * right_direction.y)};
        v2 mid_sensor = (v2){position.x + (ant->sensor_distance * direction.x), position.y + (ant->sensor_distance * direction.y)};
        v2 left_sensor = (v2){position.x + (ant->sensor_distance * left_direction.x), position.y + (ant->sensor_distance * left_direction.y)};
        s32 half_width = ant->sensor_radius; 

        Rect right_rect = rect((v2){right_sensor.x - half_width, right_sensor.y - half_width}, (v2s32){half_width * 2, half_width * 2});
        Rect mid_rect = rect((v2){mid_sensor.x - half_width, mid_sensor.y - half_width}, (v2s32){half_width * 2, half_width * 2});
        Rect left_rect = rect((v2){left_sensor.x - half_width, left_sensor.y - half_width}, (v2s32){half_width * 2, half_width * 2});

        // to home/to food pheromones
        // QUESTION: (Devon) phermone_spawn_timer is example of something not worth caching/localizing/initialize locally?
        ant->pheromone_spawn_timer -= dt;
        if(ant->pheromone_spawn_timer <= 0){
            if(state == AntState_Wondering || state == AntState_Collecting){
                Entity* pher = add_to_home_pheromone(pm, position, MAGENTA);
            }
            if(state == AntState_Depositing){
                Entity* pher = add_to_food_pheromone(pm, position, TEAL);
            }
            ant->pheromone_spawn_timer = ant->pheromone_spawn_timer_max;
        }

        // rotate towards target_direction
        f32 ant_direction_radian = dir_to_rad(direction);
        f32 target_direction_radian = dir_to_rad(target_direction);
        v2 lerped_direction = rad_to_dir(lerp_rad(ant_direction_radian, rotation_percent, target_direction_radian));
        direction = lerped_direction;
        rotation_percent += dt;
        if(rotation_percent > 1.0f){
            rotation_percent = 1.0f;
        }

        // QUESTION: (Devon) speed has only 2 access, is it worth caching?
        // move forward direction
        position.x += (ant->speed * direction.x) * dt;
        position.y += (ant->speed * direction.y) * dt;

        // QUESTION: (Devon) border_size, can anything be initialized locally?
        // out of bounds check
        if(position.x < border_size){
            target_direction.x = 1;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = ant->direction_change_timer_max;
                rotation_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        else if(position.x > render_buffer->width - border_size){
            target_direction.x = -1;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = ant->direction_change_timer_max;
                rotation_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        else if(position.y < border_size){
            target_direction.y = 1;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = ant->direction_change_timer_max;
                rotation_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else if(position.y > render_buffer->height - border_size){
            target_direction.y = -1;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = ant->direction_change_timer_max;
                rotation_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else{
            ant->out_of_bounds_x = false;
            ant->out_of_bounds_y = false;
        }
        
        v2 valid_cell_coords[9];
        u32 at = 0;
        
        if(rect_collides_rect(right_rect, one_rect) || rect_collides_rect(mid_rect, one_rect) || rect_collides_rect(left_rect, one_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x - 1, center_cell_coord.y - 1};
            //push_rect(render_buffer->render_command_arena, one_rect.pos, one_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, two_rect) || rect_collides_rect(mid_rect, two_rect) || rect_collides_rect(left_rect, two_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x, center_cell_coord.y - 1};
            //push_rect(render_buffer->render_command_arena, two_rect.pos, two_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, three_rect) || rect_collides_rect(mid_rect, three_rect) || rect_collides_rect(left_rect, three_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x + 1, center_cell_coord.y - 1};
            //push_rect(render_buffer->render_command_arena, three_rect.pos, three_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, four_rect) || rect_collides_rect(mid_rect, four_rect) || rect_collides_rect(left_rect, four_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x - 1, center_cell_coord.y};
            //push_rect(render_buffer->render_command_arena, four_rect.pos, four_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, five_rect) || rect_collides_rect(mid_rect, five_rect) || rect_collides_rect(left_rect, five_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x, center_cell_coord.y};
            //push_rect(render_buffer->render_command_arena, five_rect.pos, five_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, six_rect) || rect_collides_rect(mid_rect, six_rect) || rect_collides_rect(left_rect, six_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x + 1, center_cell_coord.y};
            //push_rect(render_buffer->render_command_arena, six_rect.pos, six_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, seven_rect) || rect_collides_rect(mid_rect, seven_rect) || rect_collides_rect(left_rect, seven_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x - 1, center_cell_coord.y + 1};
            //push_rect(render_buffer->render_command_arena, seven_rect.pos, seven_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, eight_rect) || rect_collides_rect(mid_rect, eight_rect) || rect_collides_rect(left_rect, eight_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x, center_cell_coord.y + 1};
            //push_rect(render_buffer->render_command_arena, eight_rect.pos, eight_rect.dim, BLUE);
        }
        if(rect_collides_rect(right_rect, nine_rect) || rect_collides_rect(mid_rect, nine_rect) || rect_collides_rect(left_rect, nine_rect)){
            valid_cell_coords[at++] = (v2){center_cell_coord.x + 1, center_cell_coord.y + 1};
            //push_rect(render_buffer->render_command_arena, nine_rect.pos, nine_rect.dim, BLUE);
        }

        Entity *food = 0;
        Entity *pher_food = 0;
        Entity *pher_home = 0;
        for(s32 i=0; i <at; ++i){
            v2 cell_coord = valid_cell_coords[i];

            if(cell_coord.x >= 0 && cell_coord.x < cell_row_count && cell_coord.y >= 0 && cell_coord.y < cell_row_count){
                Array* food_cell = &pm->food_cells[(s32)cell_coord.y][(s32)cell_coord.x];
                Array* pher_food_cell = &pm->pher_food_cells[(s32)cell_coord.y][(s32)cell_coord.x];
                Array* pher_home_cell = &pm->pher_home_cells[(s32)cell_coord.y][(s32)cell_coord.x];
                
                if(state == AntState_Wondering){
                    BEGIN_CYCLE_COUNTER(wondering_search_food);
                    BEGIN_TICK_COUNTER(wondering_search_food);
                    for(u32 i=0; i < food_cell->at; ++i){
                        food = food_cell->element[i];
                        if(!food->food_targeted){
                            f32 distance = distance2(position, food->position);
                            if(distance < 40){
                                state = AntState_Collecting;
                                food->food_targeted = true;
                                ant->ant_food = handle_from_entity(pm, food);
                                food->food_ant = handle_from_entity(pm, ant);

                                target_direction = direction2(position, food->position);
                                rotation_percent = 0.0f;
                                goto end_of_behavior;
                            }
                        }
                    }
                    END_TICK_COUNTER(wondering_search_food);
                    END_CYCLE_COUNTER(wondering_search_food);

                    BEGIN_CYCLE_COUNTER(wondering_search_pher);
                    BEGIN_TICK_COUNTER(wondering_search_pher);
                    // QUESTION: (Devon) is pher_food->position, a random jump in memory? 
                    // Maybe because, even though I allocate a linked list node from Arena, its not gauranteed that I allocate after previous node.
                    for(u32 i=0; i < pher_food_cell->at; ++i){
                        pher_food = pher_food_cell->element[i];
                        if(rect_collides_point(right_rect, pher_food->position)){
                            right_sensor_density += pher_food->color.a;
                        }
                        if(rect_collides_point(mid_rect, pher_food->position)){
                            forward_sensor_density += pher_food->color.a;
                        }
                        if(rect_collides_point(left_rect, pher_food->position)){
                            left_sensor_density += pher_food->color.a;
                        }
                    }
                    END_TICK_COUNTER(wondering_search_pher);
                    END_CYCLE_COUNTER(wondering_search_pher);
                }
                if(state == AntState_Depositing){
                    Entity *pher_home = NULL;
                    BEGIN_CYCLE_COUNTER(depositing_search);
                    BEGIN_TICK_COUNTER(depositing_search);
                    for(u32 i=0; i < pher_home_cell->at; ++i){
                        pher_home = pher_home_cell->element[i];
                        if(rect_collides_point(right_rect, pher_home->position)){
                            right_sensor_density += pher_home->color.a;
                        }
                        if(rect_collides_point(mid_rect, pher_home->position)){
                            forward_sensor_density += pher_home->color.a;
                        }
                        if(rect_collides_point(left_rect, pher_home->position)){
                            left_sensor_density += pher_home->color.a;
                        }
                    }
                    END_TICK_COUNTER(depositing_search);
                    END_CYCLE_COUNTER(depositing_search);
                }
            }
        }
        END_TICK_COUNTER(state_none);
        END_CYCLE_COUNTER(state_none);

        if(state == AntState_Wondering){
            BEGIN_CYCLE_COUNTER(state_wondering);
            BEGIN_TICK_COUNTER(state_wondering);

            if(right_sensor_density > 0 || forward_sensor_density > 0 || left_sensor_density > 0){
                if(forward_sensor_density > MAX(right_sensor_density, left_sensor_density)){
                    target_direction = direction;
                    if(ant->forward == false){
                        rotation_percent = 0.0f;
                    }
                    ant->right = false;
                    ant->forward = true;
                    ant->left = false;
                }
                else if(right_sensor_density > left_sensor_density){
                    target_direction = right_direction;
                    if(ant->right == false){
                        rotation_percent = 0.0f;
                    }
                    ant->right = true;
                    ant->forward = false;
                    ant->left = false;
                }
                else if(left_sensor_density > right_sensor_density){
                    target_direction = left_direction;
                    if(ant->left == false){
                        rotation_percent = 0.0f;
                    }
                    ant->right = false;
                    ant->forward = false;
                    ant->left = true;
                }
            }
            else{
                // random target_direction on timer
                ant->direction_change_timer -= dt;
                if(ant->direction_change_timer <= 0){;
                    ant->random_vector.x = ((s32)(random_range(201)-100)/100.0f);
                    ant->random_vector.y = ((s32)(random_range(201)-100)/100.0f);
                    rotation_percent = 0.0f;
                    ant->direction_change_timer_max = (random_range(3) + 1);
                    ant->direction_change_timer = ant->direction_change_timer_max;
                    target_direction = ant->random_vector;
                }
            }
            END_TICK_COUNTER(state_wondering);
            END_CYCLE_COUNTER(state_wondering);
        }

        else if(state == AntState_Collecting){
            BEGIN_CYCLE_COUNTER(state_collecting);
            BEGIN_TICK_COUNTER(state_collecting);

            Entity* targeted_food = entity_from_handle(pm, ant->ant_food);
            target_direction = direction2(position, targeted_food->position);
            f32 distance = distance2(position, targeted_food->position);

            // consume food once close enough
            if(distance < 10){
                state = AntState_Depositing;
                targeted_food->food_collected = true;
                target_direction.x = -direction.x;
                target_direction.y = -direction.y;
                rotation_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = ant->direction_change_timer_max;
                ant->color = (RGBA){1.0f, 0.0f, 0.0f,  1.0f};
            }
            END_TICK_COUNTER(state_collecting);
            END_CYCLE_COUNTER(state_collecting);
        }

        else if(state == AntState_Depositing){
            BEGIN_CYCLE_COUNTER(state_depositing);
            BEGIN_TICK_COUNTER(state_depositing);

            Entity* collected_food = entity_from_handle(pm, ant->ant_food);
            collected_food->position.x = position.x + (5 * direction.x);
            collected_food->position.y = position.y + (5 * direction.y);

            // target colony when close enough
            if(ant->colony_targeted){
                target_direction = colony_direction;
                if(colony_distance < 2){
                    remove_entity(pm, collected_food);

                    state = AntState_Wondering;
                    target_direction.x = -direction.x;
                    target_direction.y = -direction.y;
                    ant->colony_targeted = false;
                    rotation_percent = 0.0f;
                    ant->direction_change_timer_max = (random_range(3) + 1);
                    ant->direction_change_timer = ant->direction_change_timer_max;
                    collected_food->food_targeted = false;
                    collected_food->food_collected = false;
                    collected_food->food_ant = zero_entity_handle();
                    ant->ant_food = zero_entity_handle();
                    ant->color = (RGBA){0.8f, 0.8f, 0.8f,  0.7f};
                }
            }
            else{
                if(colony_distance < 75){
                    rotation_percent = 0;
                    ant->colony_targeted = true;
                }

                if(right_sensor_density > 0 || forward_sensor_density > 0 || left_sensor_density > 0){
                    if(forward_sensor_density > MAX(right_sensor_density, left_sensor_density)){
                        target_direction = direction;
                        if(ant->forward == false){
                            rotation_percent = 0.0f;
                        }
                        ant->right = false;
                        ant->forward = true;
                        ant->left = false;
                    }
                    else if(right_sensor_density > left_sensor_density){
                        target_direction = right_direction;
                        if(ant->right == false){
                            rotation_percent = 0.0f;
                        }
                        ant->right = true;
                        ant->forward = false;
                        ant->left = false;
                    }
                    else if(left_sensor_density > right_sensor_density){
                        target_direction = left_direction;
                        if(ant->left == false){
                            rotation_percent = 0.0f;
                        }
                        ant->right = false;
                        ant->forward = false;
                        ant->left = true;
                    }
                }
                else{
                    // random target_direction on timer
                    ant->direction_change_timer -= dt;
                    if(ant->direction_change_timer <= 0){
                        ant->random_vector.x = (((s32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->random_vector.y = (((s32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        rotation_percent = 0.0f;
                        ant->direction_change_timer_max = (random_range(3) + 1);
                        ant->direction_change_timer = ant->direction_change_timer_max;
                        target_direction = ant->random_vector;
                    }
                }
            }
            END_TICK_COUNTER(state_depositing);
            END_CYCLE_COUNTER(state_depositing);
        }
        end_of_behavior:;
        ant->ant_state = state;
        ant->target_direction = target_direction;
        ant->direction = direction;
        ant->position = position;
        ant->rot_percent = rotation_percent;
        ant->out_of_bounds = out_of_bounds;
    }
    END_TICK_COUNTER(behavior);
    END_CYCLE_COUNTER(behavior);
#endif

    BEGIN_CYCLE_COUNTER(allocate_commands);
    BEGIN_TICK_COUNTER(allocate_commands);
    //for(u32 i=0; i < (thread_context->thread_count + 1); ++i){
    //    free_arena(render_buffer->render_command_arenas[i]);
    //    //push_clear_color(render_buffer->render_command_arenas[y][x], (v2){0, 0}, (v2){(f32)cell_width_r, (f32)cell_height_r}, BLACK);
    //}
    //memset(render_buffer->base, 0, (render_buffer->width * render_buffer->height) * render_buffer->bytes_per_pixel);

    for(u32 entity_index = pm->free_entities_at; entity_index < ArrayCount(pm->entities); ++entity_index){
        Entity *e = pm->entities + pm->free_entities[entity_index];

        //f32 cell = (e->position.y / cell_height_r);
        //f32 floor_cell = floor_f32(cell);
        //s32 floor_index = clamp_f32_s32(0, floor_cell, thread_context->thread_count);
        //Arena* render_command_arena = render_buffer->render_command_arenas[floor_index];
        Arena* render_command_arena = render_buffer->render_command_arena;

        switch(e->type){
            case EntityType_Pixel:{
                push_pixel(render_command_arena, e->position, e->color);
            }break;
            case EntityType_Segment:{
                push_segment(render_command_arena, e->p0, e->p1, e->color);
                push_segment(render_buffer->render_command_arena, e->p0, e->p1, e->color);
            }break;
            case EntityType_Line:{
                push_line(render_command_arena, e->position, e->direction, e->color);
            }break;
            case EntityType_Ray:{
                push_ray(render_command_arena, e->position, e->direction, e->color);
            }break;
            case EntityType_Rect:{
                push_rect(render_command_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Box:{
                push_box(render_command_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Quad:{
                push_quad(render_command_arena, e->p0, e->p1, e->p2, e->p3, e->color, e->fill);
            }break;
            case EntityType_Triangle:{
                push_triangle(render_command_arena, e->p0, e->p1, e->p2, e->color, e->fill);
            }break;
            case EntityType_Circle:{
                push_circle(render_command_arena, e->position, e->rad, e->color, e->fill);
                v2s32 dimension = {e->rad*2, e->rad*2};
                v2 position = vec2(e->position.x - e->rad, e->position.y - e->rad);
                if(e->draw_bounding_box){
                    push_box(render_command_arena, position, dimension, e->color);
                }
            }break;
            case EntityType_Bitmap:{
                push_bitmap(render_command_arena, e->position, e->image);
            }break;
            case EntityType_None:{
            }break;
            case EntityType_Object:{
            }break;
            case EntityType_Food:{
                if(!e->food_collected){
                    push_rect(render_command_arena, e->position, (v2s32){4, 4}, e->color);
                }
                if(pm->draw_depositing_ants){
                    if(e->food_collected){
                        push_rect(render_command_arena, e->position, (v2s32){4, 4}, e->color);
                    }
                }
            }break;
            case EntityType_Ant:{
                if(e->ant_state == AntState_Wondering || e->ant_state == AntState_Collecting){
                    if(pm->draw_wondering_ants){
                        push_rect(render_command_arena, e->position, (v2s32){8, 8}, e->color);
                    }
                }
                if(e->ant_state == AntState_Depositing){
                    if(pm->draw_depositing_ants){
                        push_rect(render_command_arena, e->position, (v2s32){8, 8}, e->color);
                    }
                }
#if 0
                f32 forward_rad = dir_to_rad(e->direction);
                f32 right_rad = forward_rad + (RAD * e->sensor_angle);
                f32 left_rad = forward_rad - (RAD * e->sensor_angle);

                v2 right_direction = rad_to_dir(right_rad);
                v2 left_direction = rad_to_dir(left_rad);

                v2 right_sensor = vec2(e->position.x + (e->sensor_distance * right_direction.x), e->position.y + (e->sensor_distance * right_direction.y));
                v2 mid_sensor = vec2(e->position.x + (e->sensor_distance * e->direction.x), e->position.y + (e->sensor_distance * e->direction.y));
                v2 left_sensor = vec2(e->position.x + (e->sensor_distance * left_direction.x), e->position.y + (e->sensor_distance * left_direction.y));

                Rect right_rect = rect(vec2(right_sensor.x - e->sensor_radius, right_sensor.y - e->sensor_radius), vec2s32(e->sensor_radius * 2, e->sensor_radius * 2));
                Rect mid_rect = rect(vec2(mid_sensor.x - e->sensor_radius, mid_sensor.y - e->sensor_radius), vec2s32(e->sensor_radius * 2, e->sensor_radius * 2));
                Rect left_rect = rect(vec2(left_sensor.x - e->sensor_radius, left_sensor.y - e->sensor_radius), vec2s32(e->sensor_radius * 2, e->sensor_radius * 2));

                push_box(render_command_arena, right_rect.pos, right_rect.dim, YELLOW);
                push_box(render_command_arena, mid_rect.pos, mid_rect.dim, GREEN);
                push_box(render_command_arena, left_rect.pos, left_rect.dim, ORANGE);

                push_segment(render_command_arena, e->position, right_sensor, YELLOW);
                push_segment(render_command_arena, e->position, mid_sensor, GREEN);
                push_segment(render_command_arena, e->position, left_sensor, ORANGE);
                
                push_ray(render_command_arena, e->position, e->direction, RED);
#endif
            }break;
            case EntityType_Colony:{
                push_circle(render_command_arena, e->position, e->rad, e->color, e->fill);
            }break;
            case EntityType_ToHomePheromone:{
                if(pm->draw_home_phers){
                    push_rect(render_command_arena, e->position, (v2s32){4, 4}, e->color);
                }
            }break;
            case EntityType_ToFoodPheromone:{
                if(pm->draw_food_phers){
                    push_rect(render_command_arena, e->position, (v2s32){4, 4}, e->color);
                }
            }break;
        }
    }
    END_TICK_COUNTER(allocate_commands);
    END_CYCLE_COUNTER(allocate_commands);

#if 0
    u32 highest_home_count = 0;
    u32 highest_food_count = 0;
    for(u32 y=0; y < 16; ++y){
        for(u32 x=0; x < 16; ++x){
            u32 current_home_count = pm->pher_home_cells[y][x].at;
            u32 current_food_count = pm->pher_food_cells[y][x].at;
            if(highest_home_count < current_home_count){
                highest_home_count = current_home_count;
            }
            if(highest_food_count < current_food_count){
                highest_food_count = current_food_count;
            }
        }
    }
    print("home_count: %i - food_count: %i\n", highest_home_count, highest_food_count);
#endif
    //for(u32 i=0; i < (thread_context->thread_count + 1); ++i){
    //    DrawWork* draw_work = push_array(tm->frame_arena, DrawWork, 1);
    //    draw_work->render_buffer = render_buffer;
    //    draw_work->arena = render_buffer->render_command_arenas[i];

    //    thread_context->add_work_entry(thread_context->queue, do_draw_commands, draw_work);
    //}
    //thread_context->complete_all_work(thread_context->queue);

    //free_arena(tm->frame_arena);

    //BEGIN_CYCLE_COUNTER(draw);
    //BEGIN_TICK_COUNTER(draw);
    //draw_commands(render_buffer, render_command_arena);
    //END_TICK_COUNTER(draw);
    //END_CYCLE_COUNTER(draw);
}
