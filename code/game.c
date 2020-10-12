#include "game.h"

static float
round_ff(float value){
    float result = (float)((i32)(value + 0.5));
    return(result);
}

static i32
round_fi32(float value){
    i32 result = (i32)(value + 0.5);
    return(result);
}

static ui32
round_fui32(float value){
    ui32 result = (ui32)(value + 0.5);
    return(result);
}

static i32
trunc_fi32(float value){
    i32 result = (i32)value;
    return result;
}

static void
draw_rect(RenderBuffer *buffer, float _x, float _y, float _w, float _h, float r, float g, float b){
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

static bool
is_tilemap_point_empty(TileMap tilemap, float point_x, float point_y){
    bool result = false;

    i32 player_tile_x = trunc_fi32((point_x - tilemap.pos_x) / tilemap.tile_w);
    i32 player_tile_y = trunc_fi32((point_y - tilemap.pos_y) / tilemap.tile_h);

    if((player_tile_x >= 0) && (player_tile_x < tilemap.col_count) &&
       (player_tile_y >= 0) && (player_tile_y < tilemap.row_count)){
        ui32 tile_value = tilemap.tiles[tilemap.col_count * player_tile_y + player_tile_x];
        result = (tile_value == 0); 
    }

    return(result);
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;

    if(!memory->initialized){
        memory->initialized = true;

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

    TileMap tilemap = {0};
    tilemap.col_count = TILEMAP_COL;
    tilemap.row_count = TILEMAP_ROW;
    tilemap.pos_x = -30;
    tilemap.pos_y = 0;
    tilemap.tile_w = 60.0f;
    tilemap.tile_h = 60.0f;

    TileMap tilemaps[2][2] = {0};
    tilemaps[0][0] = tilemap;
    tilemaps[0][0].tiles = (ui32 *)tiles00;
    tilemaps[0][1] = tilemap;
    tilemaps[0][1].tiles = (ui32 *)tiles01;
    tilemaps[1][0] = tilemap;
    tilemaps[1][0].tiles = (ui32 *)tiles10;
    tilemaps[1][1] = tilemap;
    tilemaps[1][1].tiles = (ui32 *)tiles11;

    World world = {0};
    world.tilemaps = (TileMap *)tilemaps;

    float speed = 256.0f;
    float direction_x = 0;
    float direction_y = 0;
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

    float new_player_x = game_state->player_x + (direction_x * controller->dt);
    float new_player_y = game_state->player_y + (direction_y * controller->dt);

    if(is_tilemap_point_empty(tilemaps[0][0], new_player_x, new_player_y) &&
        is_tilemap_point_empty(tilemaps[0][0], new_player_x + 30, new_player_y) &&
        is_tilemap_point_empty(tilemaps[0][0], new_player_x, new_player_y + 30)){
        game_state->player_x = new_player_x;
        game_state->player_y = new_player_y;
    }

    draw_rect(render_buffer, 0, 0, (float)render_buffer->width, (float)render_buffer->height, 1.0f, 0.0f, 0.0f);
    for(int row=0; row<TILEMAP_ROW; ++row){
        for(int col=0; col<TILEMAP_COL; ++col){
            float gray = 0.5f;
            if(tilemaps[0][0].tiles[(tilemaps[0][0].col_count * row) + col] == 1){
                gray = 1.0f;
            }

            float x_offset = tilemaps[0][0].pos_x + ((float)col * tilemaps[0][0].tile_w);
            float y_offset = tilemaps[0][0].pos_y + ((float)row * tilemaps[0][0].tile_h);
            draw_rect(render_buffer, x_offset, y_offset, tilemaps[0][0].tile_w, tilemaps[0][0].tile_h, gray, gray, gray); 
        }
    }

    draw_rect(render_buffer, (float)game_state->player_x, (float)game_state->player_y, 30, 30, 0.8f, 1.0f, 0.0f);
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
