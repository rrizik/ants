#include "game.h"

static float 
round_ff(float value){
    float result = (float)((int32)(value + 0.5));
    return(result);
}

static int32 
round_fi(float value){
    int32 result = (int32)(value + 0.5);
    return(result);
}

static uint32 
round_fui(float value){
    uint32 result = (uint32)(value + 0.5);
    return(result);
}

static void
draw_rect(RenderBuffer *buffer, float _x, float _y, float _w, float _h, float r, float g, float b){
    uint8 *end_of_buffer = (uint8 *)buffer->memory + buffer->pitch*buffer->height;

    int32 rounded_x = round_fi(_x);
    int32 rounded_y = round_fi(_y); 
    int32 rounded_w = round_fi(_w); 
    int32 rounded_h = round_fi(_h); 

    if(rounded_x < 0){
        rounded_x = 0;
    }
    if(rounded_y < 0){
        rounded_y = 0;
    }
    if(rounded_x+rounded_w > buffer->width){
        rounded_x = buffer->width - rounded_w;
    }
    if(rounded_y+rounded_h > buffer->height){
        rounded_y = buffer->height - rounded_h;
    }

    uint8 *row = ((uint8 *)buffer->memory + rounded_y*buffer->pitch + rounded_x*buffer->bytes_per_pixel);
    for(int y = rounded_y; y < rounded_y+rounded_h; ++y){
        // QUESTION: ask about what this line means
        uint32 *pixel = (uint32 *)row;
        for(int x = rounded_x; x < rounded_x+rounded_w; ++x){
            *pixel++ = (uint32)(round_fui(r * 255.0f) << 16 | round_fui(g * 255.0f) << 8 | round_fui(b * 255.0f));
        }
        row += buffer->pitch;
    }
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;

    if(!memory->initialized){
        memory->initialized = true;

        game_state->xoffset = 0;
        game_state->move = false;
        game_state->player_x = 100;
        game_state->player_y = 100;

        // NOTE: fileIO test
        //char *filename = __FILE__;
        //FileData file = memory->read_entire_file(filename);
        //if(file.content){
        //    memory->write_entire_file("c:/sh1tz/apesticks/dx11/rafikmade/code/test.out", file.content, file.size);
        //    memory->free_file_memory(file.content);
        //}
    }

    for(uint32 i=0; i < events->index; ++i){
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

    if(controller->up){
        game_state->player_y -= 5;
    }
    if(controller->down){
        game_state->player_y += 5;
    }
    if(controller->right){
        game_state->player_x += 5;
    }
    if(controller->left){
        game_state->player_x -= 5;
    }

    uint32 tile_map[9][16] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };

    draw_rect(render_buffer, 0, 0, (float)render_buffer->width, (float)render_buffer->height, 1.0f, 0.0f, 0.0f);
    float ratio = 60.0f;
    for(int x=0; x<16; ++x){
        for(int y=0; y<9; ++y){
            float x_offset = x * ratio;
            float y_offset = y * ratio;
            if(tile_map[y][x] == 1){
                draw_rect(render_buffer, x_offset, y_offset, ratio, ratio, 0.4f, 0.0f, 0.8f);
            }
            else{
                draw_rect(render_buffer, x_offset, y_offset, ratio, ratio, 0.5f, 0.5f, 0.5f);
            }
        }
    }

                           
    draw_rect(render_buffer, (float)game_state->player_x, (float)game_state->player_y, 30, 30, 0.8f, 1.0f, 0.0f);
}


/*
static void
render_gradient(RenderBuffer *buffer, int xoffset){
    // QUESTION: ask about what this line means
    uint8 *row = (uint8 *)buffer->memory;

    for(int y = 0; y < buffer->height; ++y){
        // QUESTION: ask about what this line means
        uint32 *pixel = (uint32 *)row;
        for(int x = 0; x < buffer->width; ++x){
            uint8 blue = (uint8)(x + xoffset);
            uint8 green = (uint8)y;
            uint8 red = 0;
            *pixel++ = (0 | (red << 16) | (green << 8) | blue);
        }
        row += buffer->pitch;
    }
}
*/
