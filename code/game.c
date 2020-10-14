#include "game.h"

static f32
round_ff(f32 value){
    f32 result = (f32)((i32)(value + 0.5));
    return(result);
}

static i32
round_fi32(f32 value){
    i32 result = (i32)(value + 0.5);
    return(result);
}

static ui32
round_fui32(f32 value){
    ui32 result = (ui32)(value + 0.5);
    return(result);
}

static i32
trunc_fi32(f32 value){
    i32 result = (i32)value;
    return result;
}

#include <math.h>
static i32
floor_fi32(f32 value){
    i32 result = (i32)floorf(value);
    return result;
}

static void
draw_rect(RenderBuffer *buffer, f32 _x, f32 _y, f32 _w, f32 _h, f32 r, f32 g, f32 b){
    // QUESTION: why round
    i32 rounded_x = round_fi32(_x);
    i32 rounded_y = round_fi32(_y);
    i32 rounded_w = round_fi32(_w);
    i32 rounded_h = round_fi32(_h);

    // NOTE: bounds check
    if(rounded_x < 0){ rounded_x = 0; }
    if(rounded_y < 0){ rounded_y = 0; }
    if(rounded_x+rounded_w > buffer->width){ rounded_w = buffer->width - rounded_x; }
    if(rounded_y+rounded_h > buffer->height){ rounded_h = buffer->height - rounded_y; }
    ui32 color = (ui32)(round_fui32(r * 255.0f) << 16 | 
                        round_fui32(g * 255.0f) << 8 | 
                        round_fui32(b * 255.0f));

    ui8 *row = ((ui8 *)buffer->memory + rounded_y*buffer->pitch + rounded_x*buffer->bytes_per_pixel);
    for(int y = rounded_y; y < rounded_y+rounded_h; ++y){
        ui32 *pixel = (ui32 *)row;
        for(int x = rounded_x; x < rounded_x+rounded_w; ++x){
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

static ui32
get_tile_value(World *world, TileMap *tilemap , i32 tile_x, i32 tile_y){
    Assert(tilemap);
    Assert((tile_x >= 0) && (tile_x < world->tile_col_count) &&
           (tile_y >= 0) && (tile_y < world->tile_row_count));

    ui32 result = tilemap->tiles[(world->tile_col_count * tile_y) + tile_x];
    return(result);
}

static bool
is_tile_empty(World *world, TileMap *tilemap, i32 tile_x, i32 tile_y){
    bool result = false;

    if(tilemap){
        if((tile_x >= 0) && (tile_x < world->tile_col_count) &&
           (tile_y >= 0) && (tile_y < world->tile_row_count)){
            ui32 tile_value = get_tile_value(world, tilemap, tile_x, tile_y);
            result = (tile_value == 0); 
        }
    }

    return(result);
}

static TileMap *
get_tilemap(World *world , i32 col, i32 row){
    TileMap *result = 0;

    if(row >= 0 && row < world->row_count && col >= 0 && col < world->col_count){
        result = &world->tilemaps[(world->col_count * row) + col];
    }
    return(result);
}

static Position
get_position_based_tilemap(World *world, Position pos){
    Position result = {0};

    result.tilemap_x = pos.tilemap_x;
    result.tilemap_y = pos.tilemap_y;

    f32 x = pos.x - world->x;
    f32 y = pos.y - world->y;
    result.tile_x = floor_fi32(x / world->tile_w);
    result.tile_y = floor_fi32(y / world->tile_h);

    result.x = x - result.tile_x * world->tile_w;
    result.y = y - result.tile_y * world->tile_h;

    Assert(result.x >= 0);
    Assert(result.y >= 0);
    Assert(result.x < world->tile_w);
    Assert(result.y < world->tile_h);

    if(result.tile_x < 0){
        result.tile_x = world->tile_col_count + result.tile_x;
        --result.tilemap_x;
    }
    if(result.tile_x >= world->tile_col_count){
        result.tile_x = result.tile_x - world->tile_col_count;
        ++result.tilemap_x;
    }
    if(result.tile_y < 0){
        result.tile_y = world->tile_row_count + result.tile_y;
        --result.tilemap_y;
    }
    if(result.tile_y >= world->tile_row_count){
        result.tile_y = result.tile_y - world->tile_row_count;
        ++result.tilemap_y;
    }
    
    return(result);
}

static bool
is_world_point_empty(World *world, Position pos){
    bool result = false;

    //i32 player_tile_x = trunc_fi32((pos.x - world->x) / world->tile_w);
    //i32 player_tile_y = trunc_fi32((pos.y - world->y) / world->tile_h);

    //if(player_tile_x < 0){
    //    player_tile_x = world->tile_col_count + player_tile_x;
    //    --map_x;
    //}
    //if(player_tile_x >= world->tile_col_count){
    //    player_tile_x = player_tile_x - world->tile_col_count;
    //    ++map_x;
    //}
    //if(player_tile_y < 0){
    //    player_tile_y = world->tile_row_count + player_tile_y;
    //    --map_y;
    //}
    //if(player_tile_y >= world->tile_row_count){
    //    player_tile_y = player_tile_y - world->tile_row_count;
    //    ++map_y;
    //}

    Position new_pos = get_position_based_tilemap(world, pos);

    TileMap *tilemap = get_tilemap(world, new_pos.tilemap_x, new_pos.tilemap_y);
    result = is_tile_empty(world, tilemap, new_pos.tile_x, new_pos.tile_y);

    return(result);
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;

    if(!memory->initialized){
        memory->initialized = true;

        game_state->player_tilemap_x = 0;
        game_state->player_tilemap_y = 0;
        game_state->player_x = 200;
        game_state->player_y = 150;

        // NOTE: fileIO test
        //char *filename = __FILE__;
        //FileData file = memory->read_entire_file(filename);
        //if(file.content){
        //    memory->write_entire_file("c:/sh1tz/apesticks/dx11/rafikmade/code/test.out", file.content, file.size);
        //    memory->free_file_memory(file.content);
        //}
    }

    for(ui32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_W){
                controller->up = true;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_S){
                controller->down = true;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_D){
                controller->right = true;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_A){
                controller->left = true;
                event->key = KEY_NONE;
            }
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_W){
                controller->up = false;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_S){
                controller->down = false;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_D){
                controller->right = false;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_A){
                controller->left = false;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
        }
    }


#define TILEMAP_COL 17
#define TILEMAP_ROW 9
    // QUESTION: if i have a ui32 2d array, does that mean each element in that array is represented as a ui32?
    // meaning that if i do something like this "ui32 *t = (ui32 *)tiles;" t will start at the beginning of the
    // 2d array and then if i do t++, it will move to the next element [0][1], [0][2], ... and eventually get
    // to [max_row][max_col]?
    ui32 tiles00[TILEMAP_ROW][TILEMAP_COL] = {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1}
    };
    ui32 tiles01[TILEMAP_ROW][TILEMAP_COL] = {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1}
    };
    ui32 tiles10[TILEMAP_ROW][TILEMAP_COL] = {
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1}
    };
    ui32 tiles11[TILEMAP_ROW][TILEMAP_COL] = {
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1}
    };

    World world = {0};
    world.tile_col_count = TILEMAP_COL;
    world.tile_row_count = TILEMAP_ROW;
    world.col_count = 2;
    world.row_count = 2;
    world.x = -30;
    world.y = 0;
    world.tile_w = 60.0f;
    world.tile_h = 60.0f;

    TileMap tilemaps[2][2] = {0};
    tilemaps[0][0].tiles = (ui32 *)tiles00;
    tilemaps[0][1].tiles = (ui32 *)tiles01;
    tilemaps[1][0].tiles = (ui32 *)tiles10;
    tilemaps[1][1].tiles = (ui32 *)tiles11;

    world.tilemaps = (TileMap *)tilemaps;
    TileMap *current_tilemap = get_tilemap(&world, game_state->player_tilemap_x, game_state->player_tilemap_y);

    f32 speed = 256.0f;
    f32 direction_x = 0;
    f32 direction_y = 0;
    if(controller->left){
        direction_x = -1;
    }
    if(controller->right){
        direction_x = 1;
    }
    if(controller->up){
        direction_y = -1;
    }
    if(controller->down){
        direction_y = 1;
    }
    direction_x *= speed;
    direction_y *= speed;

    f32 new_player_x = game_state->player_x + (direction_x * controller->dt);
    f32 new_player_y = game_state->player_y + (direction_y * controller->dt);
    
    Position player_pos = {
        .tilemap_x = game_state->player_tilemap_x, 
        .tilemap_y = game_state->player_tilemap_y, 
        .x = new_player_x, 
        .y = new_player_y}; 
    Position player_pos_left = player_pos;
    player_pos_left.x += 30;
    Position player_pos_right = player_pos;
    player_pos_right.y += 30;

    if(is_world_point_empty(&world, player_pos) &&
       is_world_point_empty(&world, player_pos_left) &&
       is_world_point_empty(&world, player_pos_right)){
        Position new_pos = get_position_based_tilemap(&world, player_pos);
        game_state->player_tilemap_x = new_pos.tilemap_x;
        game_state->player_tilemap_y = new_pos.tilemap_y;
        //game_state->player_x = new_player_x;
        //game_state->player_y = new_player_y;
        game_state->player_x = world.x + world.tile_w * new_pos.tile_x + new_pos.x;
        game_state->player_y = world.y + world.tile_h * new_pos.tile_y + new_pos.y;
    }

    draw_rect(render_buffer, 0, 0, (f32)render_buffer->width, (f32)render_buffer->height, 1.0f, 0.0f, 0.0f);
    for(int row=0; row<TILEMAP_ROW; ++row){
        for(int col=0; col<TILEMAP_COL; ++col){
            f32 gray = 0.5f;
            if(get_tile_value(&world, current_tilemap, col, row)){
                gray = 1.0f;
            }

            f32 x_offset = world.x + ((f32)col * world.tile_w);
            f32 y_offset = world.y + ((f32)row * world.tile_h);
            draw_rect(render_buffer, x_offset, y_offset, world.tile_w, world.tile_h, gray, gray, gray); 
        }
    }

    draw_rect(render_buffer, (f32)game_state->player_x, (f32)game_state->player_y, 30, 30, 0.8f, 1.0f, 0.0f);
}


/*
static void
render_gradient(RenderBuffer *buffer){
    // QUESTION: ask about what this line means
    ui8 *row = (ui8 *)buffer->memory;

    for(int y = 0; y < buffer->height; ++y){
        // QUESTION: ask about what this line means
        ui32 *pixel = (ui32 *)row;
        for(int x = 0; x < buffer->width; ++x){
            ui8 blue = (ui8)(x);
            ui8 green = (ui8)y;
            ui8 red = 0;
            *pixel++ = (0 | (red << 16) | (green << 8) | blue);
        }
        row += buffer->pitch;
    }
}
*/
