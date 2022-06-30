#include "random.h"
#include "math.h"
#include "renderer.h"

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
    i32 children_count;
    u32 flags;
    v2 position;
    v2s32 dimension;
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

    f64 pher_home_decay_rate;
    f64 pher_food_decay_rate;
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

typedef struct PermanentMemory{
    Arena arena;
    Arena *assets_arena;
    String8 cwd; // CONSIDER: this might be something we want to be set on the platform side
    String8 dir_assets; // CONSIDER: this might be something we want to be set on the platform side
    i64 frame_ticks_now;
    i64 frame_ticks_prev;

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

    DLL ants;
    DLL misc;
    DLL food_cells     [16][16];
    DLL pher_cells     [16][16];
    DLL pher_food_cells[16][16];
    DLL pher_home_cells[16][16];

    u32 free_entities[110000];
    u32 free_entities_at;

    u32 generation[110000];

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
    Arena *render_commands_arena;
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
        i32 free_entity_index = pm->free_entities[pm->free_entities_at--];
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
add_pixel(PermanentMemory* pm, v2 position, v4 color){
    Entity* e = add_entity(pm, EntityType_Pixel);
    e->position = position;
    e->color = color;
    return(e);
}

static Entity*
add_segment(PermanentMemory* pm, v2 p0, v2 p1, v4 color){
    Entity* e = add_entity(pm, EntityType_Segment);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    return(e);
}

static Entity*
add_ray(PermanentMemory* pm, v2 position, v2 direction, v4 color){
    Entity* e = add_entity(pm, EntityType_Ray);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_line(PermanentMemory* pm, v2 position, v2 direction, v4 color){
    Entity* e = add_entity(pm, EntityType_Line);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_rect(PermanentMemory* pm, v2 position, v2s32 dimension, v4 color){
    Entity* e = add_entity(pm, EntityType_Rect);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    return(e);
}

static Entity*
add_box(PermanentMemory* pm, v2 position, v2s32 dimension, v4 color){
    Entity* e = add_entity(pm, EntityType_Box);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    return(e);
}

static Entity*
add_quad(PermanentMemory* pm, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
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
add_triangle(PermanentMemory *pm, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    Entity* e = add_entity(pm, EntityType_Triangle);
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->color = color;
    e->fill = fill;
    return(e);
}

static Entity*
add_circle(PermanentMemory *pm, v2 pos, u8 rad, v4 color, bool fill){
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
add_player(PermanentMemory *pm, v2 position, v2s32 dimension, v4 color, Bitmap image){
    Entity* e = add_entity(pm, EntityType_Player);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    e->image = image;
	e->speed = 50;
    return(e);
}

static Entity*
add_food(PermanentMemory *pm, v2 pos, u8 rad, v4 color, bool fill){
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
add_ant(PermanentMemory *pm, v2 pos, u8 rad, v4 color, bool fill){
    Entity* e = add_entity(pm, EntityType_Ant);

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
    e->speed = 20.0f; // variable dt
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
add_colony(PermanentMemory *pm, v2 pos, u8 rad, v4 color, bool fill){
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
add_to_home_pheromone(PermanentMemory *pm, v2 pos, u8 rad, v4 color){
    Entity* e = add_entity(pm, EntityType_ToHomePheromone);
    e->position = pos;
    e->pheromone_alpha_start = 0.25;
    e->color = color;
    e->color.a = e->pheromone_alpha_start;
    e->rad = rad;
    e->pher_home_decay_rate = 18.0f;
    return(e);
}

static Entity*
add_to_food_pheromone(PermanentMemory *pm, v2 pos, u8 rad, v4 color){
    Entity* e = add_entity(pm, EntityType_ToFoodPheromone);
    e->position = pos;
    e->pheromone_alpha_start = 0.25;
    e->color = color;
    e->color.a = e->pheromone_alpha_start;
    e->rad = rad;
    e->pher_food_decay_rate = 18.0f;
    return(e);
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
                clear(render_buffer, command->header.color);
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

static void update_game(Memory* memory, RenderBuffer* render_buffer, Controller* controller, Clock* clock){

    Assert(sizeof(PermanentMemory) < memory->permanent_size);
    Assert(sizeof(TransientMemory) < memory->transient_size);
    pm = (PermanentMemory*)memory->permanent_base;
    tm = (TransientMemory*)memory->transient_base;

    v4 RED =     {1.0f, 0.0f, 0.0f,  1.0f};
    v4 GREEN =   {0.0f, 1.0f, 0.0f,  1.0f};
    v4 BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    v4 MAGENTA = {1.0f, 0.0f, 1.0f,  0.1f};
    v4 TEAL =    {0.0f, 1.0f, 1.0f,  1.0f};
    v4 PINK =    {0.92f, 0.62f, 0.96f, 1.0f};
    v4 YELLOW =  {0.9f, 0.9f, 0.0f,  1.0f};
    v4 ORANGE =  {1.0f, 0.5f, 0.15f,  1.0f};
    v4 DGRAY =   {0.5f, 0.5f, 0.5f,  0.5f};
    v4 LGRAY =   {0.8f, 0.8f, 0.8f,  0.7f};
    v4 WHITE =   {1.0f, 1.0f, 1.0f,  1.0f};
    v4 BLACK =   {0.0f, 0.0f, 0.0f,  1.0f};

    if(!memory->initialized){
        //BEGIN_CYCLE_COUNTER(init);
        //BEGIN_TICK_COUNTER(init);

        arena_init(&pm->arena, (u8*)memory->permanent_base + sizeof(PermanentMemory), memory->permanent_size - sizeof(PermanentMemory));
        arena_init(&tm->arena, (u8*)memory->transient_base + sizeof(TransientMemory), memory->transient_size - sizeof(TransientMemory));
        pm->cwd = os_get_cwd(&pm->arena);
        pm->dir_assets = str8_concatenate(&pm->arena, pm->cwd, str8_literal("\\data\\"));

        pm->assets_arena = push_arena(&pm->arena, MB(1));
        tm->render_commands_arena = push_arena(&tm->arena, MB(16));
        tm->LL_arena = push_arena(&tm->arena, MB(800));

        pm->screen_width = render_buffer->width;
        pm->screen_height = render_buffer->height;
        memory->initialized = true;

        seed_random(1, 0);

        pm->cell_row_count = 16;
        pm->cell_width = render_buffer->width / pm->cell_row_count;
        pm->cell_height = render_buffer->height / pm->cell_row_count;

        // setup free entities array
        pm->free_entities_at = ArrayCount(pm->free_entities) - 1;
        pm->entities_count = ArrayCount(pm->entities) - 1;
        for(i32 i = ArrayCount(pm->free_entities) - 1; i >= 0; --i){
            pm->free_entities[i] = ArrayCount(pm->free_entities) - 1 - i;
        }

        Entity *zero_entity = add_entity(pm, EntityType_None);
        pm->colony = add_colony(pm, vec2(render_buffer->width/2, 100), 25, DGRAY, true);

        // add ants
        pm->ants_count = 1000;
        for(u32 i=0; i < pm->ants_count; ++i){
            //Entity* ant = add_ant(pm, pm->colony->position, 2, LGRAY, true);
            Entity* ant = add_ant(pm, (v2){(f32)(render_buffer->width/2), 100}, 2, LGRAY, true);
            ant->direction_change_timer = clock->get_ticks();
            ant->pheromone_spawn_timer = clock->get_ticks();
        }

        // add grid
        for(i32 i=1; i < pm->cell_row_count; ++i){
            // vertical
            add_segment(pm, {((f32)render_buffer->width - pm->cell_width * i), 0}, {((f32)render_buffer->width - pm->cell_width * i), (f32)render_buffer->height}, DGRAY);
            // horizonal
            add_segment(pm, {0, ((f32)render_buffer->height - pm->cell_height * i)}, {(f32)render_buffer->width, ((f32)render_buffer->height - pm->cell_height * i)}, DGRAY);
        }

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
        memory->initialized = true;
        //END_TICK_COUNTER(init);
        //END_CYCLE_COUNTER(init);
    }

    //BEGIN_CYCLE_COUNTER(reset_sentinels);
    //BEGIN_TICK_COUNTER(reset_sentinels);
    // reset LL
    reset_sentinel(&pm->ants);
    reset_sentinel(&pm->misc);
    for(i32 j=0; j < pm->cell_row_count; ++j){
        for(i32 k=0; k < pm->cell_row_count; ++k){
            reset_sentinel(&pm->food_cells[j][k]);
            reset_sentinel(&pm->pher_cells[j][k]);
            reset_sentinel(&pm->pher_food_cells[j][k]);
            reset_sentinel(&pm->pher_home_cells[j][k]);
        }
    }
    tm->LL_arena->used = 0;
    //END_TICK_COUNTER(reset_sentinels);
    //END_CYCLE_COUNTER(reset_sentinels);

    //BEGIN_CYCLE_COUNTER(LL_setup);
    //BEGIN_TICK_COUNTER(LL_setup);
    // setup LL
    for(u32 entity_index = 0; entity_index < (pm->entities_count - pm->free_entities_at); ++entity_index){
        Entity *e = pm->entities + entity_index;
        switch(e->type){
            case EntityType_Box:{
                remove_entity(pm, e);
            }break;
            case EntityType_Segment:{
                Node* node = push_node(tm->LL_arena);
                node->data = e;
                dll_push_front(&pm->misc, node);
            }break;
            case EntityType_Ant:{
                Node* node = push_node(tm->LL_arena);
                node->data = e;
                dll_push_front(&pm->ants, node);
            }break;
            case EntityType_Food:{
                if(!e->food_targeted && !e->food_collected){
                    v2 cell_coord = vec2(floor(e->position.x / pm->cell_width), floor(e->position.y / pm->cell_height));
                    cell_coord.x = clamp_f32(0, cell_coord.x, pm->cell_row_count - 1);
                    cell_coord.y = clamp_f32(0, cell_coord.y, pm->cell_row_count - 1);
                    DLL* cell = &pm->food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                    Node* node = push_node(tm->LL_arena);
                    node->data = e;
                    dll_push_front(cell, node);
                }
            }break;
            case EntityType_ToHomePheromone:{
                v2 cell_coord = vec2(floor(e->position.x / pm->cell_width), floor(e->position.y / pm->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, pm->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, pm->cell_row_count - 1);

                DLL* cell = &pm->pher_home_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                Node* node = push_node(tm->LL_arena);
                node->data = e;
                dll_push_front(cell, node);

                DLL* pher_cell = &pm->pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                Node* pher_node = push_node(tm->LL_arena);
                pher_node->data = e;
                dll_push_front(pher_cell, pher_node);
            }break;
            case EntityType_ToFoodPheromone:{
                v2 cell_coord = vec2(floor(e->position.x / pm->cell_width), floor(e->position.y / pm->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, pm->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, pm->cell_row_count - 1);

                DLL* cell = &pm->pher_food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                Node* node = push_node(tm->LL_arena);
                node->data = e;
                dll_push_front(cell, node);

                DLL* pher_cell = &pm->pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                Node* pher_node = push_node(tm->LL_arena);
                pher_node->data = e;
                dll_push_front(pher_cell, pher_node);
            }break;
        }
    }

    //END_TICK_COUNTER(LL_setup);
    //END_CYCLE_COUNTER(LL_setup);

    //BEGIN_CYCLE_COUNTER(controller);
    //BEGIN_TICK_COUNTER(controller);
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
    if(controller->m3.held){
        print("add food pher\n");
        Entity* pher = add_to_food_pheromone(pm, controller->mouse_pos, 1, TEAL);
        pher->food_decay_timer = clock->get_ticks();
    }
    if(controller->m2.held){
        print("add home pher\n");
        Entity* pher = add_to_home_pheromone(pm, controller->mouse_pos, 1, MAGENTA);
        pher->home_decay_timer = clock->get_ticks();
    }
    if(controller->m1.pressed){
        if(!pm->food_added){
            pm->food_added = true;
            v2 cell_coord = vec2(floor(controller->mouse_pos.x / pm->cell_width), floor(controller->mouse_pos.y / pm->cell_height));
            cell_coord.x = clamp_f32(0, cell_coord.x, pm->cell_row_count - 1);
            cell_coord.y = clamp_f32(0, cell_coord.y, pm->cell_row_count - 1);
            i32 x_start = cell_coord.x * pm->cell_width;
            i32 y_start = cell_coord.y * pm->cell_height;
            for(i32 x = x_start; x < (i32)(x_start + pm->cell_width); x+=4){
                for(i32 y = y_start; y < (i32)(y_start + pm->cell_height); y+=4){
                    add_food(pm, vec2(x, y), 1, GREEN, true);
                }
            }
        }
    }
    else{
        pm->food_added = false;
    }
    //END_TICK_COUNTER(controller);
    //END_CYCLE_COUNTER(controller);

    //BEGIN_CYCLE_COUNTER(degrade_pher);
    //BEGIN_TICK_COUNTER(degrade_pher);
    // pheromone degradation
    for(i32 y=0; y<pm->cell_row_count; ++y){
        for(i32 x=0; x<pm->cell_row_count; ++x){
            DLL *pher_cell = &pm->pher_cells[y][x];
            for(Node* node=pher_cell->next; node != pher_cell; node=node->next){
                Entity *e = (Entity*)node->data;
                switch(e->type){
                    case EntityType_ToFoodPheromone:{
                        u64 now = clock->get_ticks();
                        f64 seconds_elapsed = clock->get_seconds_elapsed(e->food_decay_timer, now);
                        f64 t = seconds_elapsed / e->pher_food_decay_rate;
                        e->color.a = lerp(e->pheromone_alpha_start, t, 0.0f);

                        if(e->color.a <= 0){
                            e->color.a = e->pheromone_alpha_start;
                            remove_entity(pm, e);
                        }
                    }break;
                    case EntityType_ToHomePheromone:{
                        u64 now = clock->get_ticks();
                        f64 seconds_elapsed = clock->get_seconds_elapsed(e->home_decay_timer, now);
                        f64 t = seconds_elapsed / e->pher_home_decay_rate;
                        e->color.a = lerp(e->pheromone_alpha_start, t, 0.0f);

                        if(e->color.a <= 0.0f){
                            e->color.a = e->pheromone_alpha_start;
                            remove_entity(pm, e);
                        }
                    }break;
                }
            }
        }
    }
    //END_TICK_COUNTER(degrade_pher);
    //END_CYCLE_COUNTER(degrade_pher);

    //BEGIN_CYCLE_COUNTER(behavior);
    //BEGIN_TICK_COUNTER(behavior);
    // ant behavior
    for(Node* node=pm->ants.next; node != &pm->ants; node=node->next){
        //BEGIN_CYCLE_COUNTER(state_none);
        //BEGIN_TICK_COUNTER(state_none);
        u64 behavior_none_stated_ticks = clock->get_ticks();

        Entity *ant = (Entity*)node->data;

        // reset sensor density
        ant->right_sensor_density = 0.0f;
        ant->forward_sensor_density = 0.0f;
        ant->left_sensor_density = 0.0f;

        // to home/to food pheromones
        f64 seconds_elapsed = clock->get_seconds_elapsed(ant->pheromone_spawn_timer, clock->get_ticks());
        if(seconds_elapsed >= ant->pheromone_spawn_timer_max){
            if(ant->ant_state == AntState_Wondering || ant->ant_state == AntState_Collecting){
                Entity* pher = add_to_home_pheromone(pm, ant->position, 1, MAGENTA);
                pher->home_decay_timer = clock->get_ticks();
            }
            if(ant->ant_state == AntState_Depositing){
                Entity* pher = add_to_food_pheromone(pm, ant->position, 1, TEAL);
                pher->food_decay_timer = clock->get_ticks();
            }
            ant->pheromone_spawn_timer = clock->get_ticks();
        }

        // rotate towards target_direction
        f32 ant_direction_radian = dir_to_rad(ant->direction);
        f32 target_direction_radian = dir_to_rad(ant->target_direction);
        v2 new_lerped_direction = rad_to_dir(lerp_rad(ant_direction_radian, target_direction_radian, ant->rot_percent));
        ant->direction = new_lerped_direction;
        ant->rot_percent += ant->rotate_speed;
        if(ant->rot_percent > 1.0f){
            ant->rotation_complete = true;
            ant->rot_percent = 1.0f;
        }
        else{
            ant->rotation_complete = false;
        }

        // move forward direction
        ant->position.x += (ant->speed * ant->direction.x) * clock->dt;
        ant->position.y += (ant->speed * ant->direction.y) * clock->dt;

        // X wall collision
        if(ant->position.x < pm->border_size){
            ant->target_direction.x = 1;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        else if(ant->position.x > render_buffer->width - pm->border_size){
            ant->target_direction.x = -1;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        // Y wall collision
        else if(ant->position.y < pm->border_size){
            ant->target_direction.y = 1;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else if(ant->position.y > render_buffer->height - pm->border_size){
            ant->target_direction.y = -1;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else{
            ant->out_of_bounds_x = false;
            ant->out_of_bounds_y = false;
        }

        // center cell and bottom left cell coords
        v2 center_cell_coord = vec2(floor(ant->position.x / pm->cell_width), floor(ant->position.y / pm->cell_height));
        center_cell_coord.x = clamp_f32(0, center_cell_coord.x, pm->cell_row_count - 1);
        center_cell_coord.y = clamp_f32(0, center_cell_coord.y, pm->cell_row_count - 1);
        v2 bottom_left_cell_coord = vec2(center_cell_coord.x - 1, center_cell_coord.y - 1);

        // all three sensor rects
        f32 forward_rad = dir_to_rad(ant->direction);
        f32 right_rad = forward_rad + (RAD * ant->sensor_angle);
        f32 left_rad = forward_rad - (RAD * ant->sensor_angle);

        v2 right_direction = rad_to_dir(right_rad);
        v2 left_direction = rad_to_dir(left_rad);

        ant->right_sensor = vec2(ant->position.x + (ant->sensor_distance * right_direction.x), ant->position.y + (ant->sensor_distance * right_direction.y));
        ant->mid_sensor = vec2(ant->position.x + (ant->sensor_distance * ant->direction.x), ant->position.y + (ant->sensor_distance * ant->direction.y));
        ant->left_sensor = vec2(ant->position.x + (ant->sensor_distance * left_direction.x), ant->position.y + (ant->sensor_distance * left_direction.y));

        Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2s32(ant->sensor_radius * 2, ant->sensor_radius * 2));
        Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2s32(ant->sensor_radius * 2, ant->sensor_radius * 2));
        Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2s32(ant->sensor_radius * 2, ant->sensor_radius * 2));
        //END_TICK_COUNTER(state_none);
        //END_CYCLE_COUNTER(state_none);

        //BEGIN_CYCLE_COUNTER(state_wondering);
        //BEGIN_TICK_COUNTER(state_wondering);
        if(ant->ant_state == AntState_Wondering){
            //BEGIN_CYCLE_COUNTER(wondering_search);
            //BEGIN_TICK_COUNTER(wondering_search);

            Entity *food = 0;
            Entity *pher = 0;
            for(i32 y=bottom_left_cell_coord.y; y<=center_cell_coord.y+1; ++y){
                for(i32 x=bottom_left_cell_coord.x; x<=center_cell_coord.x+1; ++x){
                    if(x >= 0 && x < pm->cell_row_count && y >= 0 && y < pm->cell_row_count){
                        //BEGIN_CYCLE_COUNTER(wondering_search_food);
                        //BEGIN_TICK_COUNTER(wondering_search_food);
                        //add_box(pm, vec2(pm->cell_width * x, pm->cell_height * y), vec2(pm->cell_width, pm->cell_height), ORANGE);
                        DLL* food_cell = &pm->food_cells[y][x];
                        for(Node* node=food_cell->next; node != food_cell; node=node->next){
                            food = (Entity*)node->data;
                            if(!food->food_targeted){
                                f32 distance = distance2(ant->position, food->position);
                                if(distance < 20){
                                    //ant->color = ORANGE;
                                    ant->ant_state = AntState_Collecting;
                                    food->food_targeted = true;
                                    ant->ant_food = handle_from_entity(pm, food);
                                    food->food_ant = handle_from_entity(pm, ant);

                                    ant->target_direction = direction2(ant->position, food->position);
                                    ant->rot_percent = 0.0f;
                                    goto end_of_behavior;
                                }
                            }
                        }
                        //END_TICK_COUNTER(wondering_search_food);
                        //END_CYCLE_COUNTER(wondering_search_food);

                        //BEGIN_CYCLE_COUNTER(wondering_search_pher);
                        //BEGIN_TICK_COUNTER(wondering_search_pher);
                        DLL* pher_cell = &pm->pher_food_cells[y][x];
                        for(Node *node=pher_cell->next; node != pher_cell; node=node->next){
                            pher = (Entity*)node->data;
                            if(rect_collides_point(right_rect, pher->position)){
                                ant->right_sensor_density += pher->color.a;
                            }
                            if(rect_collides_point(mid_rect, pher->position)){
                                ant->forward_sensor_density += pher->color.a;
                            }
                            if(rect_collides_point(left_rect, pher->position)){
                                ant->left_sensor_density += pher->color.a;
                            }
                        }
                        //END_TICK_COUNTER(wondering_search_pher);
                        //END_CYCLE_COUNTER(wondering_search_pher);
                    }
                }
            }
            //END_TICK_COUNTER(wondering_search);
            //END_CYCLE_COUNTER(wondering_search);

            if(ant->right_sensor_density > 0 || ant->forward_sensor_density > 0 || ant->left_sensor_density > 0){
                if(ant->forward_sensor_density > MAX(ant->right_sensor_density, ant->left_sensor_density)){
                    ant->target_direction = ant->direction;
                    if(ant->forward == false){
                        ant->rot_percent = 0.0f;
                    }
                    ant->right = false;
                    ant->forward = true;
                    ant->left = false;
                }
                else if(ant->right_sensor_density > ant->left_sensor_density){
                    ant->target_direction = right_direction;
                    if(ant->right == false){
                        ant->rot_percent = 0.0f;
                    }
                    ant->right = true;
                    ant->forward = false;
                    ant->left = false;
                }
                else if(ant->left_sensor_density > ant->right_sensor_density){
                    ant->target_direction = left_direction;
                    if(ant->left == false){
                        ant->rot_percent = 0.0f;
                    }
                    ant->right = false;
                    ant->forward = false;
                    ant->left = true;
                }
            }
            else{
                // random target_direction on timer
                f64 seconds_elapsed = clock->get_seconds_elapsed(ant->direction_change_timer, clock->get_ticks());
                //print("seconds elapsed: (%f, %f)\n", ant->direction_change_timer_max, seconds_elapsed);
                if(seconds_elapsed >= ant->direction_change_timer_max){
                    ant->random_vector.x = ((i32)(random_range(201)-100)/100.0f);
                    ant->random_vector.y = ((i32)(random_range(201)-100)/100.0f);
                    ant->rot_percent = 0.0f;
                    ant->direction_change_timer_max = (random_range(3) + 1);
                    ant->direction_change_timer = clock->get_ticks();
                    ant->target_direction = ant->random_vector;
                }
            }
            //END_TICK_COUNTER(state_wondering);
            //END_CYCLE_COUNTER(state_wondering);
        }

        else if(ant->ant_state == AntState_Collecting){
            //BEGIN_CYCLE_COUNTER(state_collecting);
            //BEGIN_TICK_COUNTER(state_collecting);

            Entity* targeted_food = entity_from_handle(pm, ant->ant_food);
            ant->target_direction = direction2(ant->position, targeted_food->position);
            f32 distance = distance2(ant->position, targeted_food->position);

            // consume food once close enough
            if(distance < 2){
                ant->ant_state = AntState_Depositing;
                targeted_food->food_collected = true;
                ant->target_direction.x = -ant->direction.x;
                ant->target_direction.y = -ant->direction.y;
                ant->rot_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = clock->get_ticks();
                ant->color = RED;
            }
            //END_TICK_COUNTER(state_collecting);
            //END_CYCLE_COUNTER(state_collecting);
        }

        else if(ant->ant_state == AntState_Depositing){
            //BEGIN_CYCLE_COUNTER(state_depositing);
            //BEGIN_TICK_COUNTER(state_depositing);

            Entity* collected_food = entity_from_handle(pm, ant->ant_food);
            collected_food->position.x = ant->position.x + (5 * ant->direction.x);
            collected_food->position.y = ant->position.y + (5 * ant->direction.y);

            if(!ant->colony_targeted){
                //BEGIN_CYCLE_COUNTER(depositing_search);
                //BEGIN_TICK_COUNTER(depositing_search);

                Entity *pher = NULL;
                for(i32 y=bottom_left_cell_coord.y; y<=center_cell_coord.y+1; ++y){
                    for(i32 x=bottom_left_cell_coord.x; x<=center_cell_coord.x+1; ++x){
                        if(x >= 0 && x < pm->cell_row_count && y >= 0 && y < pm->cell_row_count){
                            DLL* pher_cell = &pm->pher_home_cells[y][x];
                            for(Node* node=pher_cell->next; node != pher_cell; node=node->next){
                                pher = (Entity*)node->data;
                                if(rect_collides_point(right_rect, pher->position)){
                                    ant->right_sensor_density += pher->color.a;
                                }
                                if(rect_collides_point(mid_rect, pher->position)){
                                    ant->forward_sensor_density += pher->color.a;
                                }
                                if(rect_collides_point(left_rect, pher->position)){
                                    ant->left_sensor_density += pher->color.a;
                                }
                            }
                        }
                    }
                }
                //END_TICK_COUNTER(depositing_search);
                //END_CYCLE_COUNTER(depositing_search);

                if(ant->right_sensor_density > 0 || ant->forward_sensor_density > 0 || ant->left_sensor_density > 0){
                    if(ant->forward_sensor_density > MAX(ant->right_sensor_density, ant->left_sensor_density)){
                        ant->target_direction = ant->direction;
                        if(ant->forward == false){
                            ant->rot_percent = 0.0f;
                        }
                        ant->right = false;
                        ant->forward = true;
                        ant->left = false;
                    }
                    else if(ant->right_sensor_density > ant->left_sensor_density){
                        ant->target_direction = right_direction;
                        if(ant->right == false){
                            ant->rot_percent = 0.0f;
                        }
                        ant->right = true;
                        ant->forward = false;
                        ant->left = false;
                    }
                    else if(ant->left_sensor_density > ant->right_sensor_density){
                        ant->target_direction = left_direction;
                        if(ant->left == false){
                            ant->rot_percent = 0.0f;
                        }
                        ant->right = false;
                        ant->forward = false;
                        ant->left = true;
                    }
                }
                else{
                    // random target_direction on timer
                    f64 seconds_elapsed = clock->get_seconds_elapsed(ant->direction_change_timer, clock->get_ticks());
                    if(seconds_elapsed >= ant->direction_change_timer_max){
                        ant->random_vector.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->random_vector.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->rot_percent = 0.0f;
                        ant->direction_change_timer_max = (random_range(3) + 1);
                        ant->direction_change_timer = clock->get_ticks();
                        ant->target_direction = ant->random_vector;
                    }
                }
            }

            // target colony when close enough
            f32 colony_distance = distance2(ant->position, pm->colony->position);
            if(colony_distance < 75){
                ant->target_direction = direction2(ant->position, pm->colony->position);
                if(!ant->colony_targeted){
                    ant->rot_percent = 0;
                    ant->colony_targeted = true;
                }
            }

            // deposite if close to colony
            if(colony_distance < 2){
                remove_entity(pm, collected_food);

                ant->ant_state = AntState_Wondering;
                ant->target_direction.x = -ant->direction.x;
                ant->target_direction.y = -ant->direction.y;
                ant->colony_targeted = false;
                ant->rot_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = clock->get_ticks();
                collected_food->food_targeted = false;
                collected_food->food_collected = false;
                collected_food->food_ant = zero_entity_handle();
                ant->ant_food = zero_entity_handle();
                ant->color = LGRAY;
            }
            //END_TICK_COUNTER(state_depositing);
            //END_CYCLE_COUNTER(state_depositing);
        }
        end_of_behavior:;
    }
    //END_TICK_COUNTER(behavior);
    //END_CYCLE_COUNTER(behavior);

    //BEGIN_CYCLE_COUNTER(allocate_commands);
    //BEGIN_TICK_COUNTER(allocate_commands);
    tm->render_commands_arena->used = 0;
    push_clear_color(tm->render_commands_arena, BLACK);
    for(u32 entity_index = 0; entity_index <= ArrayCount(pm->entities); ++entity_index){
        Entity *e = pm->entities + entity_index;
        switch(e->type){
            case EntityType_Player:{
                //allocate_bitmap(tm->render_commands_arena, e->position, e->image);
                //if(e->draw_bounding_box){
                //    allocate_box(tm->render_commands_arena, e->position, e->dimension, e->color);
                //}
                //allocate_rect(tm->render_commands_arena, e->position, e->dimension, e->color);
                push_box(tm->render_commands_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Pixel:{
                push_pixel(tm->render_commands_arena, e->position, e->color);
            }break;
            case EntityType_Segment:{
                push_segment(tm->render_commands_arena, e->p0, e->p1, e->color);
            }break;
            case EntityType_Line:{
                push_line(tm->render_commands_arena, e->position, e->direction, e->color);
            }break;
            case EntityType_Ray:{
                push_ray(tm->render_commands_arena, e->position, e->direction, e->color);
            }break;
            case EntityType_Rect:{
                push_rect(tm->render_commands_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Box:{
                push_box(tm->render_commands_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Quad:{
                push_quad(tm->render_commands_arena, e->p0, e->p1, e->p2, e->p3, e->color, e->fill);
            }break;
            case EntityType_Triangle:{
                push_triangle(tm->render_commands_arena, e->p0, e->p1, e->p2, e->color, e->fill);
            }break;
            case EntityType_Circle:{
                push_circle(tm->render_commands_arena, e->position, e->rad, e->color, e->fill);
                v2s32 dimension = {e->rad*2, e->rad*2};
                v2 position = vec2(e->position.x - e->rad, e->position.y - e->rad);
                if(e->draw_bounding_box){
                    push_box(tm->render_commands_arena, position, dimension, e->color);
                }
            }break;
            case EntityType_Bitmap:{
                push_bitmap(tm->render_commands_arena, e->position, e->image);
            }break;
            case EntityType_None:{
            }break;
            case EntityType_Object:{
            }break;
            case EntityType_Food:{
                if(!e->food_collected){
                    push_rect(tm->render_commands_arena, e->position, (v2s32){4, 4}, e->color);
                }
                if(pm->draw_depositing_ants){
                    if(e->food_collected){
                        push_rect(tm->render_commands_arena, e->position, (v2s32){4, 4}, e->color);
                    }
                }
            }break;
            case EntityType_Ant:{
				if(pm->draw_wondering_ants && (e->color == LGRAY)){
					push_rect(tm->render_commands_arena, e->position, (v2s32){8, 8}, e->color);
				}
                if(pm->draw_depositing_ants && (e->color == RED)){
					push_rect(tm->render_commands_arena, e->position, (v2s32){8, 8}, e->color);
				}
                if((e->color == ORANGE)){
					push_rect(tm->render_commands_arena, e->position, (v2s32){8, 8}, e->color);
                }

                //f32 forward_rad = dir_to_rad(e->direction);
                //f32 right_rad = forward_rad + (RAD * e->sensor_angle);
                //f32 left_rad = forward_rad - (RAD * e->sensor_angle);

                //v2 right_direction = rad_to_dir(right_rad);
                //v2 left_direction = rad_to_dir(left_rad);

                //e->right_sensor = vec2(e->position.x + (e->sensor_distance * right_direction.x), e->position.y + (e->sensor_distance * right_direction.y));
                //e->mid_sensor = vec2(e->position.x + (e->sensor_distance * e->direction.x), e->position.y + (e->sensor_distance * e->direction.y));
                //e->left_sensor = vec2(e->position.x + (e->sensor_distance * left_direction.x), e->position.y + (e->sensor_distance * left_direction.y));

                //Rect right_rect = rect(vec2(e->right_sensor.x - e->sensor_radius, e->right_sensor.y - e->sensor_radius), vec2s32(e->sensor_radius * 2, e->sensor_radius * 2));
                //Rect mid_rect = rect(vec2(e->mid_sensor.x - e->sensor_radius, e->mid_sensor.y - e->sensor_radius), vec2s32(e->sensor_radius * 2, e->sensor_radius * 2));
                //Rect left_rect = rect(vec2(e->left_sensor.x - e->sensor_radius, e->left_sensor.y - e->sensor_radius), vec2s32(e->sensor_radius * 2, e->sensor_radius * 2));

                //push_box(tm->render_commands_arena, right_rect.pos, right_rect.dim, YELLOW);
                //push_box(tm->render_commands_arena, mid_rect.pos, mid_rect.dim, GREEN);
                //push_box(tm->render_commands_arena, left_rect.pos, left_rect.dim, ORANGE);

                //push_segment(tm->render_commands_arena, e->position, e->right_sensor, YELLOW);
                //push_segment(tm->render_commands_arena, e->position, e->mid_sensor, GREEN);
                //push_segment(tm->render_commands_arena, e->position, e->left_sensor, ORANGE);
                
                //allocate_ray(tm->render_commands_arena, e->position, e->target_direction, BLUE);
            }break;
            case EntityType_Colony:{
                push_circle(tm->render_commands_arena, e->position, e->rad, e->color, e->fill);
            }break;
            case EntityType_ToHomePheromone:{
                if(pm->draw_home_phers){
                    push_rect(tm->render_commands_arena, e->position, (v2s32){4, 4}, e->color);
                }
            }break;
            case EntityType_ToFoodPheromone:{
                if(pm->draw_food_phers){
                    push_rect(tm->render_commands_arena, e->position, (v2s32){4, 4}, e->color);
                }
            }break;
        }
    }
    //END_TICK_COUNTER(allocate_commands);
    //END_CYCLE_COUNTER(allocate_commands);


    //BEGIN_CYCLE_COUNTER(draw);
    //BEGIN_TICK_COUNTER(draw);
    draw_commands(render_buffer, tm->render_commands_arena);
    draw_rect_fast(render_buffer, (v2){300.0f, 0.0f}, (v2s32){15, 2}, RED);
    draw_rect_fast(render_buffer, (v2){(f32)(render_buffer->width - 100), 000.0f}, (v2s32){100, 100}, RED);
    draw_rect_fast(render_buffer, (v2){(f32)(render_buffer->width - 101), 100.0f}, (v2s32){100, 100}, GREEN);
    draw_rect_fast(render_buffer, (v2){(f32)(render_buffer->width - 102), 200.0f}, (v2s32){100, 100}, YELLOW);
    draw_rect_fast(render_buffer, (v2){(f32)(render_buffer->width - 103), 300.0f}, (v2s32){100, 100}, TEAL);
    draw_rect_fast(render_buffer, (v2){(f32)(render_buffer->width - 104), 400.0f}, (v2s32){100, 100}, PINK);
    draw_rect_fast(render_buffer, (v2){(f32)(render_buffer->width - 2), 500.0f}, (v2s32){8, 8}, ORANGE);
    // color from srgb to linear srgb, then math, then linear srgb to srgb
    // 
    //END_TICK_COUNTER(draw);
    //END_CYCLE_COUNTER(draw);
    pm->frame_index++;
}
