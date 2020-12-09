#include "game.h"
#include "math.h"
#include "vectors.h"
#include "matrices.h"


static Vec2
calc_center(Vec2 *p, ui32 count){
    Vec2 result = {0};

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
translate(Vec2 *p, ui32 count, Vec2 translation){
    for(ui32 i=0; i<count; ++i){
        *p = add2(*p, translation);
        p++;
    }
    p -= count;
}

static void
scale(Vec2 *p, ui32 count, f32 scalar, Vec2 origin){
    for(ui32 i=0; i<count; ++i){
        *p = add2(scale2(sub2(*p, origin), scalar), origin);
        p++;
    }
    p -= count;
}

static Vec2
rotate_pt(Vec2 p, f32 angle, Vec2 origin){
    Vec2 result = {0};
    result.x = (p.x - origin.x) * Cos(angle * 3.14f/180.0f) - (p.y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
    result.y = (p.x - origin.x) * Sin(angle * 3.14f/180.0f) + (p.y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
    return(result);
}

static void
rotate_pts(Vec2 *p, ui32 count, f32 angle, Vec2 origin){
    for(ui32 i=0; i<count; ++i){
        Vec2 result = {0};
        result = rotate_pt(*p, angle, origin);
        p->x = result.x;
        p->y = result.y;
        p++;
    }
    p -= count;
}

// TODO: REMOVE
static Color
get_color_test(GameMemory *memory, RenderBuffer *buffer, f32 x, f32 y){
    Color result = {0};

    ui8 *location = (ui8 *)memory->temporary_storage + ((buffer->height - 1 - (i32)y) * buffer->pitch) + ((i32)x * buffer->bytes_per_pixel);
    ui32 *pixel = (ui32 *)location;
    result.r = (f32)((*pixel >> 16) & 0xFF) / 255.0f;
    result.g = (f32)((*pixel >> 8) & 0xFF) / 255.0f;
    result.b = (f32)((*pixel >> 0) & 0xFF) / 255.0f;
    result.a = 1.0f;

    return(result);
}

static Color
get_color(RenderBuffer *buffer, f32 x, f32 y){
    Color result = {0};

    ui8 *location = (ui8 *)buffer->memory + ((buffer->height - 1 - (i32)y) * buffer->pitch) + ((i32)x * buffer->bytes_per_pixel);
    ui32 *pixel = (ui32 *)location;
    result.r = (f32)((*pixel >> 16) & 0xFF) / 255.0f;
    result.g = (f32)((*pixel >> 8) & 0xFF) / 255.0f;
    result.b = (f32)((*pixel >> 0) & 0xFF) / 255.0f;
    result.a = 1.0f;

    return(result);
}

static void
draw_pixel(RenderBuffer *buffer, f32 x, f32 y, Color c){
    x = round_ff(x);
    y = round_ff(y);

    if(x >= 0 && x < buffer->width && y >= 0 && y < buffer->height){
        ui8 *row = (ui8 *)buffer->memory + ((buffer->height - 1 - (i32)y) * buffer->pitch) + ((i32)x * buffer->bytes_per_pixel);
        ui32 *pixel = (ui32 *)row;

        f32 current_r = (f32)((*pixel >> 16) & 0xFF);
        f32 current_g = (f32)((*pixel >> 8) & 0xFF);
        f32 current_b = (f32)((*pixel >> 0) & 0xFF);

        f32 new_r = ((1.0f - c.a) * current_r + c.a * (c.r * 255.0f));
        f32 new_g = ((1.0f - c.a) * current_g + c.a * (c.g * 255.0f));
        f32 new_b = ((1.0f - c.a) * current_b + c.a * (c.b * 255.0f));

        ui32 color = (round_fi32(new_r) << 16 | round_fi32(new_g) << 8 | round_fi32(new_b) << 0);
        *pixel = color;
    }
}

static void
draw_ray(RenderBuffer *buffer, Vec2 point, Vec2 direction, Color c){
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
draw_line(RenderBuffer *buffer, Vec2 point, Vec2 direction, Color c){
    Vec2 point1 = round_v2(point);
    Vec2 point2 = point1;
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
draw_segment(RenderBuffer *buffer, Vec2 p0, Vec2 p1, Color c){
    p0 = round_v2(p0);
    p1 = round_v2(p1);

    if(p0.x > p1.x) { swap_v2(&p0, &p1); };

    f32 distance_x =  ABS(p1.x - p0.x);
    f32 distance_y = -ABS(p1.y - p0.y);
    f32 step_x = p0.x < p1.x ? 1.0f : -1.0f;
    f32 step_y = p0.y < p1.y ? 1.0f : -1.0f; 

    f32 error = distance_x + distance_y;

    for(;;){
        draw_pixel(buffer, p0.x, p0.y, c); // NOTE: before break, so you can draw a single point draw_segment (a pixel)
        if(p0.x == p1.x && p0.y == p1.y) break;

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
draw_flattop_triangle(RenderBuffer *buffer, Vec2 p0, Vec2 p1, Vec2 p2, Color c){
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
draw_flatbottom_triangle(RenderBuffer *buffer, Vec2 p0, Vec2 p1, Vec2 p2, Color c){
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
draw_triangle(RenderBuffer *buffer, Vec2 *points, Color c, bool fill){
    Vec2 p0 = (*points++);
    Vec2 p1 = (*points++);
    Vec2 p2 = (*points);

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
            Vec2 p3 = {x, p1.y};
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
draw_triangle_v2(RenderBuffer *buffer, Vec2 p0, Vec2 p1, Vec2 p2, Color c, bool fill){
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
            Vec2 p3 = {x, p1.y};
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
clear(RenderBuffer *buffer, Color c){
    for(f32 y=0; y < buffer->height; ++y){
        for(f32 x=0; x < buffer->width; ++x){
            draw_pixel(buffer, x, y, c);
        }
    }
}

static void
draw_rect(RenderBuffer *buffer, Vec2 *p, Color c, bool fill){
    Vec2 p0 = round_v2(*p++);
    Vec2 p1 = round_v2(*p++);
    Vec2 p2 = round_v2(*p++);
    Vec2 p3 = round_v2(*p);

    for(f32 y=p0.y; y <= p2.y; ++y){
        for(f32 x=p0.x; x <= p1.x; ++x){
            draw_pixel(buffer, x, y, c);
        }
    }
}

static void
draw_quad(RenderBuffer *buffer, Vec2 *p, Color c, bool fill){
    Vec2 p0 = (*p++);
    Vec2 p1 = (*p++);
    Vec2 p2 = (*p++);
    Vec2 p3 = (*p);

    draw_triangle_v2(buffer, p0, p1, p2, c, fill);
    draw_triangle_v2(buffer, p0, p2, p3, c, fill);
}

static void
draw_box(RenderBuffer *buffer, Rect rect, Color c){
    Vec2 p0 = {rect.x, rect.y};
    Vec2 p1 = {rect.x + rect.w, rect.y};
    Vec2 p2 = {rect.x + rect.w, rect.y + rect.h};
    Vec2 p3 = {rect.x, rect.y + rect.h};

    draw_segment(buffer, p0, p1, c);
    draw_segment(buffer, p1, p2, c);
    draw_segment(buffer, p2, p3, c);
    draw_segment(buffer, p3, p0, c);
}

static void
draw_quad_wire(RenderBuffer *buffer, Vec2 *p, Color c){
    Vec2 p0 = *p++;
    Vec2 p1 = *p++;
    Vec2 p2 = *p++;
    Vec2 p3 = *p;

    draw_triangle_v2(buffer, p0, p1, p2, c, false);
    draw_triangle_v2(buffer, p1, p2, p3, c, false);
}

static void
draw_polygon(RenderBuffer *buffer, Vec2 *points, ui32 count, Color c){
    Vec2 first = *points++;
    Vec2 prev = first;
    for(ui32 i=1; i<count; ++i){
        draw_segment(buffer, prev, *points, c);
        prev = *points++;
    }
    draw_segment(buffer, first, prev, c);
}

static void
copy_array(Vec2 *a, Vec2 *b, i32 count){
    for(i32 i=0; i < count; ++i){
        *a++ = *b++;
    }
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;
    
    if(!memory->initialized){
        memory->initialized = true;

        Vec2 box1[4] = {{100, 100}, {200, 100}, {200, 200}, {100, 200}};
        copy_array(game_state->box1, box1, array_count(box1));

        Vec2 box2[4] = {{100, 300}, {200, 300}, {200, 400}, {100, 400}};
        copy_array(game_state->box2, box2, array_count(box2));

        Vec2 background[4] = {{0.0f, 0.0f}, {20.0f, 0.0f}, {0.0f, 10.0f}, {20.0f, 10.0f}};
        copy_array(game_state->test_background, background, array_count(background));

        game_state->r1 = rect(vec2(100, 100), vec2(50, 50));
        game_state->r2 = rect(vec2(100, 300), vec2(100, 100));
    }


    for(ui32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_W){
                game_state->move.up = true;
            }
            if(event->key == KEY_S){
                game_state->move.down = true;
            }
            if(event->key == KEY_A){
                game_state->move.left = true;
            }
            if(event->key == KEY_D){
                game_state->move.right = true;
            }
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
            if(event->key == KEY_W){
                game_state->move.up = false;
            }
            if(event->key == KEY_S){
                game_state->move.down = false;
            }
            if(event->key == KEY_A){
                game_state->move.left = false;
            }
            if(event->key == KEY_D){
                game_state->move.right = false;
            }
        }
    }

    f32 speed = 4.0f;

    if(game_state->move.up){
        game_state->r1.y += speed;
    }
    if(game_state->move.down){
        game_state->r1.y -= speed;
    }
    if(game_state->move.left){
        game_state->r1.x -= speed;
    }
    if(game_state->move.right){
        game_state->r1.x += speed;
    }

    Color red = {1.0f, 0.0f, 0.0f, 0.5f};
    Color green = {0.0f, 1.0f, 0.0f, 0.5f};
    Color blue = {0.0f, 0.0f, 1.0f, 1.0f};
    Color magenta = {1.0f, 0.0f, 1.0f, 0.5f};
    Color pink = {1.0f, 0.30f, 0.50, 0.5f};
    Color yellow = {0.9f, 0.9f, 0.0f, 0.5f};
    Color teal = {0.0f, 1.0f, 1.0f, 0.5f};
    Color white = {1.0f, 1.0f, 1.0f, 0.5f};
    Color black = {0.0f, 0.0f, 0.0f, 1.0f};
    Color dgray = {0.5f, 0.5f, 0.5f, 0.5f};
    Color lgray = {0.8f, 0.8f, 0.8f, 0.5f};

    clear(render_buffer, black);
    
    draw_pixel(render_buffer, 300, 300, red);

    if(rect_contains_rect(game_state->r2, game_state->r1)){
        draw_box(render_buffer, game_state->r1, yellow);
    }
    else if(rect_collide_rect(game_state->r1, game_state->r2)){
        draw_box(render_buffer, game_state->r1, green);
    }
    else if(rect_collide_point(game_state->r1, vec2(300, 300))){
        draw_box(render_buffer, game_state->r1, pink);
    }
    else{
        draw_box(render_buffer, game_state->r1, blue);
    }
    draw_box(render_buffer, game_state->r2, magenta);

    Rect border = rect(vec2(0, 0), vec2((f32)render_buffer->width-1, (f32)render_buffer->height-1));
    draw_box(render_buffer, border, red);

    Vec3 t = {1, 2, 3};
}
