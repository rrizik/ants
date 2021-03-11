#if !defined(ENTITY_H)

typedef enum EntityFlags {EntityFlag_Movable} EntityFlags;
typedef enum EntityType {EntityType_None, EntityType_Player, EntityType_Object, EntityType_Pixel, EntityType_Line, EntityType_Ray, EntityType_Segment, EntityType_Triangle, EntityType_Rect, EntityType_Quad, EntityType_Box, EntityType_Circle, EntityType_Bitmap} EntityType;

typedef struct Entity{
    int id;
    int index;
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
    v4 outline_color;
    v2 p0;
    v2 p1;
    v2 p2;
    v2 p3;
    ui8 rad;
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

static Entity*
add_entity(GameState *game_state, EntityType type){
    Entity *result = game_state->entities + game_state->entity_count;
    result->index = game_state->entity_count;
    result->type = type;
    game_state->entity_count++;

    return(result);
}

static Entity*
add_pixel(GameState* game_state, f32 x, f32 y, v4 color){
    Entity *e = add_entity(game_state, EntityType_Pixel);
    e->x = x;
    e->y = y;
    e->color = color;
    return(e);
}

static Entity*
add_segment(GameState* game_state, v2 p0, v2 p1, v4 color){
    Entity *e = add_entity(game_state, EntityType_Segment);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    return(e);
}

static Entity*
add_ray(GameState* game_state, v2 position, v2 direction, v4 color){
    Entity *e = add_entity(game_state, EntityType_Ray);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_line(GameState* game_state, v2 position, v2 direction, v4 color){
    Entity *e = add_entity(game_state, EntityType_Line);
    e->color = color;
    e->position = position;
    e->direction = direction;
    return(e);
}

static Entity*
add_rect(GameState* game_state, v2 position, v2 dimension, v4 color){
    Entity *e = add_entity(game_state, EntityType_Rect);
    e->position = position;
    e->dimension = dimension;
    e->w = dimension.w;
    e->h = dimension.h;
    e->color = color;
    return(e);
}

static Entity*
add_box(GameState* game_state, v2 position, v2 dimension, v4 color){
    Entity *e = add_entity(game_state, EntityType_Box);
    e->position = position;
    e->dimension = dimension;
    e->w = dimension.w;
    e->h = dimension.h;
    e->color = color;
    return(e);
}

static Entity*
add_quad(GameState* game_state, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
    Entity *e = add_entity(game_state, EntityType_Quad);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->p3 = p3;
    e->fill = fill;
    return(e);
}

static Entity*
add_triangle(GameState *game_state, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    Entity *e = add_entity(game_state, EntityType_Triangle);
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->color = color;
    e->fill = fill;
    return(e);
}

static Entity*
add_circle(GameState *game_state, v2 pos, ui8 rad, v4 color, bool fill){
    Entity *e = add_entity(game_state, EntityType_Circle);
    e->position = pos;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    return(e);
}

static Entity*
add_bitmap(GameState* game_state, v2 position, Bitmap image){
    Entity *e = add_entity(game_state, EntityType_Bitmap);
    e->position = position;
    e->image = image;
    return(e);
}


static Entity*
add_player(GameState *game_state, v2 position, v2 dimension, v4 color, Bitmap image){
    Entity *e = add_entity(game_state, EntityType_Player);
    e->position = position;
    e->dimension = dimension;
    e->color = color;
    e->image = image;
    return(e);
}

#define ENTITY_H
#endif
