#include "game.h"

static void
render_gradient(RenderBuffer buffer, int xoffset){
    // QUESTION: ask about what this line means
    uint8 *row = (uint8 *)buffer.memory;

    for(int y = 0; y < buffer.height; ++y){
        // QUESTION: ask about what this line means
        uint32 *pixel = (uint32 *)row;
        for(int x = 0; x < buffer.width; ++x){
            uint8 blue = (uint8)(x + xoffset);
            uint8 green = (uint8)y;
            uint8 red = 0;
            *pixel++ = (0 | (red << 16) | (green << 8) | blue);
        }
        row += buffer.pitch;
    }
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;

    if(!memory->initialized){
        memory->initialized = true;

        game_state->xoffset = 0;
        game_state->move = false;

        // NOTE: fileIO test
        char *filename = __FILE__;
        FileData file = memory->read_entire_file(filename);
        if(file.content){
            memory->write_entire_file("c:/sh1tz/apesticks/dx11/rafikmade/code/test.out", file.content, file.size);
            memory->free_file_memory(file.content);
        }
    }

    for(uint32 i=0; i < event_i; ++i){
        Event *event = &events[i];
        if(event->type == EVENT_TEXT){
            //print("%c\n", event->text);
        }
        if(event->type == EVENT_PADUP){
            if(event->pad == PAD_LEFT){
                game_state->move = false;
                event->pad = PAD_NONE;
            }
        }
        if(event->type == EVENT_PADDOWN){
            if(event->pad == PAD_LEFT){
                game_state->move = true;
                event->pad = PAD_NONE;
            }
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_A){
                game_state->move = false;
                event->key = KEY_NONE;
            }
        }
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_A){
                game_state->move = true;
                event->key = KEY_NONE;
            }
            if(event->key == KEY_ESCAPE){
                *running = false;
            }
        }
    }
    if(game_state->move == true){
        game_state->xoffset++;
    }

    render_gradient(*render_buffer, game_state->xoffset);
}
