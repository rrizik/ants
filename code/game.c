#include "game.h"

static void
render_player(RenderBuffer *buffer, int player_x, int player_y){
    uint8 *end_of_buffer = (uint8 *)buffer->memory + buffer->pitch*buffer->height;
    uint32 color = 0xFFFFFFFF;

    if(player_y < 0){
        player_y = 0;
    }
    if(player_x < 0){
        player_x = 0;
    }

    for(int x = player_x; x < player_x+10; ++x){
        // QUESTION: ask about what this line means
        uint8 *pixel = ((uint8 *)buffer->memory + x*buffer->bytes_per_pixel + player_y*buffer->pitch);
        for(int y = player_y; y < player_y+10; ++y){
            if((pixel >= (uint8 *)buffer->memory) && (pixel < end_of_buffer)){
                *(uint32 *)pixel = color;
            }
            pixel += buffer->pitch;
        }
    }
}

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

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;

    if(!memory->initialized){
        memory->initialized = true;

        game_state->xoffset = 0;
        game_state->move = false;
        game_state->player_x = 100;
        game_state->player_y = 100;

        // NOTE: fileIO test
        char *filename = __FILE__;
        FileData file = memory->read_entire_file(filename);
        if(file.content){
            memory->write_entire_file("c:/sh1tz/apesticks/dx11/rafikmade/code/test.out", file.content, file.size);
            memory->free_file_memory(file.content);
        }
    }

    for(uint32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_W){
                controller->up = true;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_S){
                print("true\n");
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
                print("false\n");
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

    render_gradient(render_buffer, game_state->xoffset);
    render_player(render_buffer, game_state->player_x, game_state->player_y);
}
