#include "game.h"
#include "math.h"
#include "vectors.h"
#include "matrices.h"
#include "renderer.h"


static v2
calc_center(v2 *p, ui32 count){
    v2 result = {0};

    for(ui32 i=0; i<count; ++i){
        result.x += p->x;
        result.y += p->y;
        p++;
    }
    p -= count;

    result.x /= count;
    result.y /= count;

    return(result);
}

static void
translate(v2 *p, ui32 count, v2 translation){
    for(ui32 i=0; i<count; ++i){
        *p = add2(*p, translation);
        p++;
    }
    p -= count;
}

static void
scale(v2 *p, ui32 count, f32 scalar, v2 origin){
    for(ui32 i=0; i<count; ++i){
        *p = add2(scale2(sub2(*p, origin), scalar), origin);
        p++;
    }
    p -= count;
}

static v2
rotate_pt(v2 p, f32 angle, v2 origin){
    v2 result = {0};
    result.x = (p.x - origin.x) * Cos(angle * 3.14f/180.0f) - (p.y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
    result.y = (p.x - origin.x) * Sin(angle * 3.14f/180.0f) + (p.y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
    return(result);
}

static void
rotate_pts(v2 *p, ui32 count, f32 angle, v2 origin){
    for(ui32 i=0; i<count; ++i){
        v2 result = {0};
        result = rotate_pt(*p, angle, origin);
        p->x = result.x;
        p->y = result.y;
        p++;
    }
    p -= count;
}

static v4
get_color_at(RenderBuffer *buffer, f32 x, f32 y){
    v4 result = {0};

    ui8 *location = (ui8 *)buffer->memory +
                    ((buffer->height - (i32)y - 1) * buffer->pitch) +
                    ((i32)x * buffer->bytes_per_pixel);
    ui32 *pixel = (ui32 *)location;
    result.a = (f32)((*pixel >> 24) & 0xFF) / 255.0f;
    result.r = (f32)((*pixel >> 16) & 0xFF) / 255.0f;
    result.g = (f32)((*pixel >> 8) & 0xFF) / 255.0f;
    result.b = (f32)((*pixel >> 0) & 0xFF) / 255.0f;

    return(result);
}

static void
draw_pixel(RenderBuffer *buffer, f32 float_x, f32 float_y, v4 color){
    i32 x = round_fi32(float_x);
    i32 y = round_fi32(float_y);

    if(x >= 0 && x < buffer->width && y >= 0 && y < buffer->height){
        ui8 *row = (ui8 *)buffer->memory +
                   ((buffer->height - y - 1) * buffer->pitch) +
                   (x * buffer->bytes_per_pixel);

        ui32 *pixel = (ui32 *)row;

        f32 current_a = ((f32)((*pixel >> 24) & 0xFF) / 255.0f);
        f32 current_r = (f32)((*pixel >> 16) & 0xFF);
        f32 current_g = (f32)((*pixel >> 8) & 0xFF);
        f32 current_b = (f32)((*pixel >> 0) & 0xFF);

        color.r *= 255.0f;
        color.g *= 255.0f;
        color.b *= 255.0f;

        f32 new_r = (1 - color.a) * current_r + (color.a * color.r);
        f32 new_g = (1 - color.a) * current_g + (color.a * color.g);
        f32 new_b = (1 - color.a) * current_b + (color.a * color.b);

        ui32 new_color = (round_fi32(color.a * 255.0f) << 24 | round_fi32(new_r) << 16 | round_fi32(new_g) << 8 | round_fi32(new_b) << 0);
        *pixel = new_color;
    }
}

static void
draw_ray(RenderBuffer *buffer, v2 point, v2 direction, v4 c){
    point = round_v2(point);
    direction = round_v2(direction);

    f32 distance_x =  ABS(direction.x - point.x);
    f32 distance_y = -ABS(direction.y - point.y);
    f32 step_x = point.x < direction.x ? 1.0f : -1.0f;
    f32 step_y = point.y < direction.y ? 1.0f : -1.0f;

    f32 error = distance_x + distance_y;

    for(;;){
        draw_pixel(buffer, point.x, point.y, c); // NOTE: before break, so you can draw a single point draw_segment (a pixel)
        if(point.x < 0 || point.x > buffer->width || point.y < 0 || point.y > buffer->height)break;

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y;
            point.x += step_x;
        }
        if (error2 <= distance_x){
            error += distance_x;
            point.y += step_y;
        }
    }
}

static void
draw_line(RenderBuffer *buffer, v2 point, v2 direction, v4 c){
    v2 point1 = round_v2(point);
    v2 point2 = point1;
    direction = round_v2(direction);

    f32 distance_x =  ABS(direction.x - point1.x);
    f32 distance_y = -ABS(direction.y - point1.y);
    f32 d1_step_x = point1.x < direction.x ? 1.0f : -1.0f;
    f32 d1_step_y = point1.y < direction.y ? 1.0f : -1.0f;
    f32 d2_step_x = -(d1_step_x);
    f32 d2_step_y = -(d1_step_y);

    f32 error = distance_x + distance_y;

    for(;;){
        draw_pixel(buffer, point1.x, point1.y, c); // NOTE: before break, so you can draw a single point draw_segment (a pixel)
        if(point1.x < 0 || point1.x > buffer->width || point1.y < 0 || point1.y > buffer->height)break;

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y;
            point1.x += d1_step_x;
        }
        if (error2 <= distance_x){
            error += distance_x;
            point1.y += d1_step_y;
        }
    }
    for(;;){
        draw_pixel(buffer, point2.x, point2.y, c);
        if(point2.x < 0 || point2.x > buffer->width || point2.y < 0 || point2.y > buffer->height)break;

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y;
            point2.x += d2_step_x;
        }
        if (error2 <= distance_x){
            error += distance_x;
            point2.y += d2_step_y;
        }
    }
}

static void
draw_segment(RenderBuffer *buffer, v2 p0, v2 p1, v4 c){
    p0 = round_v2(p0);
    p1 = round_v2(p1);

    f32 distance_x =  ABS(p1.x - p0.x);
    f32 distance_y = -ABS(p1.y - p0.y);
    f32 step_x = p0.x < p1.x ? 1.0f : -1.0f;
    f32 step_y = p0.y < p1.y ? 1.0f : -1.0f;

    f32 error = distance_x + distance_y;

    for(;;){
        if(p0.x == p1.x && p0.y == p1.y) break;
        draw_pixel(buffer, p0.x, p0.y, c); // NOTE: before break, so you can draw a single point draw_segment (a pixel)

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y;
            p0.x += step_x;
        }
        if (error2 <= distance_x){
            error += distance_x;
            p0.y += step_y;
        }
    }
}

static void
draw_flattop_triangle(RenderBuffer *buffer, v2 p0, v2 p1, v2 p2, v4 c){
    f32 left_slope = (p0.x - p2.x) / (p0.y - p2.y);
    f32 right_slope = (p1.x - p2.x) / (p1.y - p2.y);

    int start_y = (int)round(p2.y);
    int end_y = (int)round(p0.y);

    for(int y=start_y; y < end_y; ++y){
        f32 x0 = left_slope * (y + 0.5f - p0.y) + p0.x;
        f32 x1 = right_slope * (y + 0.5f - p1.y) + p1.x;
        int start_x = (int)ceil(x0 - 0.5f);
        int end_x = (int)ceil(x1 - 0.5f);
        for(int x=start_x; x < end_x; ++x){
            draw_pixel(buffer, (f32)x, (f32)y, c);
        }
    }
}

static void
draw_flatbottom_triangle(RenderBuffer *buffer, v2 p0, v2 p1, v2 p2, v4 c){
    f32 left_slope = (p1.x - p0.x) / (p1.y - p0.y);
    f32 right_slope = (p2.x - p0.x) / (p2.y - p0.y);

    int start_y = (int)round(p0.y);
    int end_y = (int)round(p1.y);

    for(int y=start_y; y >= end_y; --y){
        f32 x0 = left_slope * (y + 0.5f - p0.y) + p0.x;
        f32 x1 = right_slope * (y + 0.5f - p0.y) + p0.x;
        int start_x = (int)ceil(x0 - 0.5f);
        int end_x = (int)ceil(x1 - 0.5f);
        for(int x=start_x; x < end_x; ++x){
            draw_pixel(buffer, (f32)x, (f32)y, c);
        }
    }
}

static void
draw_triangle_outlined(RenderBuffer *buffer, Triangle tri, v4 c, v4 c_outlined, bool fill){
    v2 p0 = tri.p0;
    v2 p1 = tri.p1;
    v2 p2 = tri.p2;

    if(p0.y < p1.y){ swap_v2(&p0, &p1); }
    if(p0.y < p2.y){ swap_v2(&p0, &p2); }
    if(p1.y < p2.y){ swap_v2(&p1, &p2); }

    if(fill){
        if(p0.y == p1.y){
            if(p0.x > p1.x){ swap_v2(&p0, &p1); }
            draw_flattop_triangle(buffer, p0, p1, p2, c);
        }
        else if(p1.y == p2.y){
            if(p1.x > p2.x){ swap_v2(&p1, &p2); }
            draw_flatbottom_triangle(buffer, p0, p1, p2, c);
        }
        else{
            f32 x = p0.x + ((p1.y - p0.y) / (p2.y - p0.y)) * (p2.x - p0.x);
            v2 p3 = {x, p1.y};
            if(p1.x > p3.x){ swap_v2(&p1, &p3); }
            draw_flattop_triangle(buffer, p1, p3, p2, c);
            draw_flatbottom_triangle(buffer, p0, p1, p3, c);
        }
    }
    draw_segment(buffer, p0, p1, c_outlined);
    draw_segment(buffer, p1, p2, c_outlined);
    draw_segment(buffer, p2, p0, c_outlined);
}

static void
draw_triangle(RenderBuffer *buffer, v2 p0_, v2 p1_, v2 p2_, v4 c, bool fill){
    v2 p0 = p0_;
    v2 p1 = p1_;
    v2 p2 = p2_;

    if(p0.y < p1.y){ swap_v2(&p0, &p1); }
    if(p0.y < p2.y){ swap_v2(&p0, &p2); }
    if(p1.y < p2.y){ swap_v2(&p1, &p2); }

    if(fill){
        if(p0.y == p1.y){
            if(p0.x > p1.x){ swap_v2(&p0, &p1); }
            draw_flattop_triangle(buffer, p0, p1, p2, c);
        }
        else if(p1.y == p2.y){
            if(p1.x > p2.x){ swap_v2(&p1, &p2); }
            draw_flatbottom_triangle(buffer, p0, p1, p2, c);
        }
        else{
            f32 x = p0.x + ((p1.y - p0.y) / (p2.y - p0.y)) * (p2.x - p0.x);
            v2 p3 = {x, p1.y};
            if(p1.x > p3.x){ swap_v2(&p1, &p3); }
            draw_flattop_triangle(buffer, p1, p3, p2, c);
            draw_flatbottom_triangle(buffer, p0, p1, p3, c);
        }
    }
    else{
        draw_segment(buffer, p0, p1, c);
        draw_segment(buffer, p1, p2, c);
        draw_segment(buffer, p2, p0, c);
    }
}

static void
clear(RenderBuffer *buffer, v4 c){
    for(f32 y=0; y < buffer->height; ++y){
        for(f32 x=0; x < buffer->width; ++x){
            draw_pixel(buffer, x, y, c);
        }
    }
}

static void
draw_bitmap(RenderBuffer *buffer, v2 position, Bitmap image){
    f32 rounded_x = round_ff(position.x);
    f32 rounded_y = round_ff(position.y);
    for(f32 y=rounded_y; y < rounded_y + image.height; ++y){
        for(f32 x=rounded_x; x < rounded_x + image.width; ++x){
            v4 color = convert_ui32_v4_normalized(*image.pixels++);
            draw_pixel(buffer, x, y, color);
        }
    }
}

static void
draw_rect(RenderBuffer *buffer, Rect r, v4 c){
    v2 p0 = {r.x, r.y};
    v2 p1 = {r.x + r.w, r.y};
    v2 p2 = {r.x, r.y + r.h};
    v2 p3 = {r.x + r.w, r.y + r.h};

    for(f32 y=p0.y; y <= p2.y; ++y){
        for(f32 x=p0.x; x <= p1.x; ++x){
            draw_pixel(buffer, x, y, c);
        }
    }
}

static void
draw_rect_pts(RenderBuffer *buffer, v2 *p, v4 c){
    v2 p0 = round_v2(*p++);
    v2 p1 = round_v2(*p++);
    v2 p2 = round_v2(*p++);
    v2 p3 = round_v2(*p);

    for(f32 y=p0.y; y <= p2.y; ++y){
        for(f32 x=p0.x; x <= p1.x; ++x){
            draw_pixel(buffer, x, y, c);
        }
    }
}

static void
draw_quad(RenderBuffer *buffer, v2 p0_, v2 p1_, v2 p2_, v2 p3_, v4 c, bool fill){
    v2 p0 = p0_;
    v2 p1 = p1_;
    v2 p2 = p2_;
    v2 p3 = p3_;

    Triangle t1 = triangle(p0, p1, p2);
    Triangle t2 = triangle(p0, p2, p3);

    draw_triangle(buffer, p0, p1, p2, c, fill);
    draw_triangle(buffer, p0, p1, p2, c, fill);
}

static void
draw_box(RenderBuffer *buffer, Rect rect, v4 c){
    v2 p0 = {rect.x, rect.y};
    v2 p1 = {rect.x + rect.w, rect.y};
    v2 p2 = {rect.x + rect.w, rect.y + rect.h};
    v2 p3 = {rect.x, rect.y + rect.h};

    draw_segment(buffer, p0, p1, c);
    draw_segment(buffer, p1, p2, c);
    draw_segment(buffer, p2, p3, c);
    draw_segment(buffer, p3, p0, c);
}

static void
draw_polygon(RenderBuffer *buffer, v2 *points, ui32 count, v4 c){
    v2 first = *points++;
    v2 prev = first;
    for(ui32 i=1; i<count; ++i){
        draw_segment(buffer, prev, *points, c);
        prev = *points++;
    }
    draw_segment(buffer, first, prev, c);
}

static void
draw_circle(RenderBuffer *buffer, f32 xm, f32 ym, f32 r, v4 c, bool fill) {
   f32 x = -r;
   f32 y = 0;
   f32 err = 2-2*r;
   do {
      draw_pixel(buffer, (xm - x), (ym + y), c);
      draw_pixel(buffer, (xm - y), (ym - x), c);
      draw_pixel(buffer, (xm + x), (ym - y), c);
      draw_pixel(buffer, (xm + y), (ym + x), c);
      r = err;
      if (r <= y){
          if(fill){
              if(ym + y == ym - y){
                  draw_segment(buffer, vec2((xm - x - 1), (ym + y)), vec2((xm + x), (ym + y)), c);
              }
              else{
                  draw_segment(buffer, vec2((xm - x - 1), (ym + y)), vec2((xm + x), (ym + y)), c);
                  draw_segment(buffer, vec2((xm - x - 1), (ym - y)), vec2((xm + x), (ym - y)), c);
              }
          }
          y++;
          err += y * 2 + 1;
      }
      if (r > x || err > y){
          x++;
          err += x * 2 + 1;
      }
   } while (x < 0);
}

static void
copy_array(v2 *a, v2 *b, i32 count){
    for(i32 i=0; i < count; ++i){
        *a++ = *b++;
    }
}

static Bitmap
load_bitmap(GameMemory *memory, char *filename){
    Bitmap result = {0};

    char full_path[256];
    cat_strings(memory->data_dir, filename, full_path);

    FileData bitmap_file = memory->read_entire_file(full_path);
    if(bitmap_file.size > 0){
        BitmapHeader *header = (BitmapHeader *)bitmap_file.content;
        result.pixels = (ui32 *)((ui8 *)bitmap_file.content + header->bitmap_offset);
        result.width = header->width;
        result.height = header->height;
        // IMPORTANT: depending on compression type you might have to re order the bytes to make it AA RR GG BB
        // as well as consider the masks for each color included in the header
    }
    return(result);
}

static void
draw_commands(RenderBuffer *render_buffer, RenderCommands *commands){
    size_t bytes_to_move_forward = 0;
    void* at = commands->buffer;
    void* end = (char*)commands->buffer + commands->used_bytes;
    while(at != end){
        BaseCommand* command = (BaseCommand*)at;

        switch(command->type){
            case RenderCommand_Circle:{
                CircleCommand *circle = (CircleCommand*)command;
                draw_circle(render_buffer, command->position.x, command->position.y, circle->rad, command->color, command->fill);
                at = circle + 1;
            } break;
            case RenderCommand_Triangle:{
                TriangleCommand *triangle = (TriangleCommand*)command;
                draw_triangle(render_buffer, triangle->p0, triangle->p1, triangle->p2, command->color, command->fill);
                at = triangle + 1;
            } break;
            case RenderCommand_Pixel:{
                PixelCommand *pixel = (PixelCommand*)command;
                at = pixel + 1;
            } break;
            case RenderCommand_Box:{
                BoxCommand *box = (BoxCommand*)command;
                at = box + 1;
            } break;
            case RenderCommand_Bitmap:{
                BitmapCommand *bitmap = (BitmapCommand*)command;
                at = bitmap + 1;
            } break;
    //draw_box(render_buffer, game_state->r2, orange);
    //draw_pixel(render_buffer, 300, 300, orange);
    //draw_circle(render_buffer, 400, 400, 50, green, true);
    //draw_bitmap(render_buffer, 0, 0, game_state->test);
    //draw_bitmap(render_buffer, 500, 100, game_state->image);
    //draw_bitmap(render_buffer, 100, 100, game_state->circle);
        }
    }
}

MAIN_GAME_LOOP(main_game_loop){

    v4 red =     {1.0f, 0.0f, 0.0f,  0.5f};
    v4 green =   {0.0f, 1.0f, 0.0f,  0.5f};
    v4 blue =    {0.0f, 0.0f, 1.0f,  0.5f};
    v4 magenta = {1.0f, 0.0f, 1.0f,  0.5f};
    v4 pink =    {0.92f, 0.62f, 0.96f, 0.5f};
    v4 yellow =  {0.9f, 0.9f, 0.0f,  0.5f};
    v4 teal =    {0.0f, 1.0f, 1.0f,  0.5f};
    v4 orange =  {1.0f, 0.5f, 0.15f,  0.5f};
    v4 dgray =   {0.5f, 0.5f, 0.5f,  0.5f};
    v4 lgray =   {0.8f, 0.8f, 0.8f,  0.5f};
    v4 white =   {1.0f, 1.0f, 1.0f,  1.0f};
    v4 black =   {0.0f, 0.0f, 0.0f,  1.f};

    assert(sizeof(GameState) <= memory->permanent_storage_size);
    assert(sizeof(TranState) <= memory->transient_storage_size);
    GameState *game_state = (GameState *)memory->permanent_storage;
    TranState *transient_state = (TranState *)memory->transient_storage;

    if(!memory->initialized){
        add_entity(game_state, EntityType_None);
        v2 box1[4] = {{100, 250}, {200, 250}, {200, 350}, {100, 350}};
        copy_array(game_state->box1, box1, array_count(box1));

        game_state->test = load_bitmap(memory, "test.bmp");
        game_state->circle = load_bitmap(memory, "circle.bmp");
        game_state->image = load_bitmap(memory, "image.bmp");

        v2 box2[4] = {{100, 300}, {200, 300}, {200, 400}, {100, 400}};
        copy_array(game_state->box2, box2, array_count(box2));

        v2 background[4] = {{0.0f, 0.0f}, {16.0f, 0.0f}, {0.0f, 10.0f}, {16.0f, 10.0f}};
        copy_array(game_state->test_background, background, array_count(background));

        add_pixel(game_state, vec2(1, 1), red);
        add_segment(game_state, vec2(300, 300), vec2(200, 200), magenta);
        add_line(game_state, vec2(250, 300), vec2(200, 200), pink);
        add_ray(game_state, vec2(400, 400), vec2(50, 50), teal);
        //add_rect(game_state, rect(vec2(100, 100), vec2(50, 50)), orange);
        //add_box(game_state, rect(vec2(100, 300), vec2(100, 100)), blue);
        add_quad(game_state, vec2(0, 500), vec2(20, 500), vec2(0, 400), vec2(20, 400), green);
        add_triangle(game_state, vec2(400, 100), vec2(500, 100), vec2(450, 200), lgray);
        add_circle(game_state, vec2(400, 400), 50, dgray);
        //add_bitmap(game_state, vec2(20, 20), game_state->circle);
        //add_bitmap(game_state, vec2(0, 300), game_state->image);
        game_state->player_index = add_player(game_state, vec2(100, 100), red)->index;


        initialize_arena(&game_state->permanent_arena, 
                         (ui8*)memory->permanent_storage + sizeof(GameState), 
                         memory->permanent_storage_size - sizeof(GameState));
        initialize_arena(&transient_state->transient_arena, 
                         (ui8*)memory->transient_storage + sizeof(TranState), 
                         memory->transient_storage_size - sizeof(TranState));

        transient_state->render_commands = allocate_render_commands(&transient_state->transient_arena, 4096);
        memory->initialized = true;
    }

    for(ui32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_W){
                game_state->controller.up = true;
            }
            if(event->key == KEY_S){
                game_state->controller.down = true;
            }
            if(event->key == KEY_A){
                game_state->controller.left = true;
            }
            if(event->key == KEY_D){
                game_state->controller.right = true;
            }
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
            if(event->key == KEY_W){
                game_state->controller.up = false;
            }
            if(event->key == KEY_S){
                game_state->controller.down = false;
            }
            if(event->key == KEY_A){
                game_state->controller.left = false;
            }
            if(event->key == KEY_D){
                game_state->controller.right = false;
            }
        }
    }

    f32 speed = 150.0f;

    //if(game_state->player_index != 0){
    //    Entity *player = game_state->entities + game_state->player_index;
    //    if(game_state->controller.up){
    //        player->rect.y += speed * clock->dt;
    //    }
    //    if(game_state->controller.down){
    //        player->rect.y -= speed * clock->dt;
    //    }
    //    if(game_state->controller.left){
    //        player->rect.x -= speed * clock->dt;
    //    }
    //    if(game_state->controller.right){
    //        player->rect.x += speed * clock->dt;
    //    }
    //}

    clear(render_buffer, black);

    transient_state->render_commands->used_bytes = 0;
    for(ui32 entity_index = 1; entity_index <= game_state->entity_count; ++entity_index){
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type){
            case EntityType_Player:{
                push_bitmap(transient_state->render_commands, entity->position, entity->color);
            }break;
            case EntityType_Triangle:{
                push_triangle(transient_state->render_commands, entity->p0, entity->p1, entity->p2, entity->color, true);
            }break;
            case EntityType_Circle:{
                push_circle(transient_state->render_commands, vec2(400, 400), 50, green, true);
            }break;
        }
    }

    draw_commands(render_buffer, transient_state->render_commands);

    //draw_box(render_buffer, game_state->r2, orange);
    //draw_pixel(render_buffer, 300, 300, orange);
    //draw_circle(render_buffer, 400, 400, 50, green, true);
    //draw_bitmap(render_buffer, 0, 0, game_state->test);
    //draw_bitmap(render_buffer, 500, 100, game_state->image);
    //draw_bitmap(render_buffer, 100, 100, game_state->circle);
}
