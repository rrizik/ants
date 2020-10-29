#include "game.h"
#include "math.h"


static v2
calc_centroid(v2 p1, v2 p2, v2 p3){
    v2 result = {0};

    result.x = (p1.x + p2.x + p3.x) / 3.0f;
    result.y = (p1.y + p2.y + p3.y) / 3.0f;

    return(result);
}

static tri
translate(tri t1, v2 translation){
    t1.p1 = v2_add(t1.p1, translation);
    t1.p2 = v2_add(t1.p2, translation);
    t1.p3 = v2_add(t1.p3, translation);

    return(t1);
}

static tri
scale(tri t, v2 scalar, v2 center){
    t.p1.x = ((t.p1.x - center.x) * scalar.x) + center.x; 
    t.p1.y = ((t.p1.y - center.y) * scalar.y) + center.y; 
    t.p2.x = ((t.p2.x - center.x) * scalar.x) + center.x; 
    t.p2.y = ((t.p2.y - center.y) * scalar.y) + center.y; 
    t.p3.x = ((t.p3.x - center.x) * scalar.x) + center.x; 
    t.p3.y = ((t.p3.y - center.y) * scalar.y) + center.y; 

    return(t);
}

static tri
rotate(tri t1, i16 angle, v2 center){
    float x1 = (t1.p1.x - center.x) * Cos(angle * 3.14f/180.0f) - (t1.p1.y - center.y) * Sin(angle * 3.14f/180.0f) + center.x;
    float y1 = (t1.p1.x - center.x) * Sin(angle * 3.14f/180.0f) + (t1.p1.y - center.y) * Cos(angle * 3.14f/180.0f) + center.y;
    float x2 = (t1.p2.x - center.x) * Cos(angle * 3.14f/180.0f) - (t1.p2.y - center.y) * Sin(angle * 3.14f/180.0f) + center.x;
    float y2 = (t1.p2.x - center.x) * Sin(angle * 3.14f/180.0f) + (t1.p2.y - center.y) * Cos(angle * 3.14f/180.0f) + center.y;
    float x3 = (t1.p3.x - center.x) * Cos(angle * 3.14f/180.0f) - (t1.p3.y - center.y) * Sin(angle * 3.14f/180.0f) + center.x;
    float y3 = (t1.p3.x - center.x) * Sin(angle * 3.14f/180.0f) + (t1.p3.y - center.y) * Cos(angle * 3.14f/180.0f) + center.y;
    t1.p1.x = x1;
    t1.p1.y = y1;
    t1.p2.x = x2;
    t1.p2.y = y2;
    t1.p3.x = x3;
    t1.p3.y = y3;

    return(t1);
}

// QUESTION: i dont think this should be floats, should be ints, ask about it
static void
set_pixel(RenderBuffer *buffer, f32 x, f32 y, Color *c){
    x = round_ff(x);
    y = round_ff(y);

    if(x >= 0 && x < buffer->width && y >= 0 && y < buffer->height){
        ui32 color = (round_fi32(c->r * 255) << 16 | round_fi32(c->g * 255) << 8 | round_fi32(c->b * 255));
        ui8 *row = (ui8 *)buffer->memory + ((buffer->height - 1 - (i32)y) * buffer->pitch) + ((i32)x * buffer->bytes_per_pixel);
        ui32 *pixel = (ui32 *)row;
        *pixel = color;
    }
}

static void 
line(RenderBuffer *buffer, v2 p1, v2 p2, Color *c){
    p1.x = round_ff(p1.x);
    p1.y = round_ff(p1.y);
    p2.x = round_ff(p2.x);
    p2.y = round_ff(p2.y);

    f32 distance_x =  ABS(p2.x - p1.x);
    f32 distance_y = -ABS(p2.y - p1.y);
    f32 step_x = p1.x < p2.x ? 1.0f : -1.0f;
    f32 step_y = p1.y < p2.y ? 1.0f : -1.0f; 

    f32 error = distance_x + distance_y;

    for(;;){
        set_pixel(buffer, p1.x, p1.y, c); // NOTE: before break, so you can draw a single point line (a pixel)
        if(p1.x == p2.x && p1.y == p2.y) break;

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y; 
            p1.x += step_x; 
        }
        if (error2 <= distance_x){ 
            error += distance_x; 
            p1.y += step_y; 
        }
    }
}

static void 
fill_flatbottom_triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color *c){
    f32 slope_1 = (p2.x - p1.x) / (p2.y - p1.y);
    f32 slope_2 = (p3.x - p1.x) / (p3.y - p1.y);
    
    f32 x1_inc = p1.x;
    f32 x2_inc = p1.x;

    for(f32 y=p1.y; y >= p2.y; --y){
        v2 new_p1 = {x1_inc, y};
        v2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc -= slope_1;
        x2_inc -= slope_2;
    }
}

static void
fill_flattop_triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color *c){
    f32 slope_1 = (p1.x - p3.x) / (p1.y - p3.y);
    f32 slope_2 = (p2.x - p3.x) / (p2.y - p3.y);
    
    f32 x1_inc = p3.x;
    f32 x2_inc = p3.x;

    for(f32 y=p3.y; y < p1.y; ++y){
        v2 new_p1 = {x1_inc, y};
        v2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc += slope_1;
        x2_inc += slope_2;
    }
}

static void 
triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color *c){
    p1.x = round_ff(p1.x);
    p1.y = round_ff(p1.y);
    p2.x = round_ff(p2.x);
    p2.y = round_ff(p2.y);
    p3.x = round_ff(p3.x);
    p3.y = round_ff(p3.y);

    if(p1.y < p2.y){ swapv2(&p1, &p2); }
    if(p1.y < p3.y){ swapv2(&p1, &p3); }
    if(p2.y < p3.y){ swapv2(&p2, &p3); }
    if(p2.y == p3.y){
        fill_flatbottom_triangle(buffer, p1, p2, p3, c);
    }
    if(p1.y == p2.y){
        fill_flattop_triangle(buffer, p1, p2, p3, c);
    }
    else{
        f32 x = p1.x + ((p2.y - p1.y) / (p3.y - p1.y)) * (p3.x - p1.x);
        v2 p4 = {x, p2.y};
        fill_flatbottom_triangle(buffer, p1, p2, p4, c);
        fill_flattop_triangle(buffer, p2, p4, p3, c);
    }
}

static void
trianglew(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color *c){
    if(p1.y < p2.y){ swapv2(&p1, &p2); }
    if(p1.y < p3.y){ swapv2(&p1, &p3); }
    if(p2.y < p3.y){ swapv2(&p2, &p3); }
    line(buffer, p1, p2, c);
    line(buffer, p2, p3, c);
    line(buffer, p3, p1, c);
}

static void
rect(RenderBuffer *buffer, v2 pos, v2 dim, Color *c){
    pos.x = round_ff(pos.x);
    pos.y = round_ff(pos.y);
    dim.x = round_ff(dim.x);
    dim.y = round_ff(dim.y);

    v2 p1 = {(f32)pos.x, (f32)pos.y};
    v2 p2 = {(f32)(pos.x + dim.x), (f32)pos.y};
    v2 p3 = {(f32)pos.x, (f32)(pos.y + dim.y)};
    v2 p4 = {(f32)(pos.x + dim.x), (f32)(pos.y + dim.y)};
    //triangle(buffer, p1, p2, p3, c);
    //triangle(buffer, p2, p3, p4, c);
    for(f32 y = pos.y; y < pos.y + dim.y; ++y){
        for(f32 x = pos.x; x < pos.x + dim.x; ++x){
            set_pixel(buffer, x, y, c);
        }
    }
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;
    
    if(!memory->initialized){
        memory->initialized = true;

        game_state->t1.p1.x = 400.0f;
        game_state->t1.p1.y = 160.0f;
        game_state->t1.p2.x = 450.0f;
        game_state->t1.p2.y = 70.0f;
        game_state->t1.p3.x = 500.0f;
        game_state->t1.p3.y = 160.0f;

        //game_state->t1.p1.x = 10.0f;
        //game_state->t1.p1.y = 10.0f;
        //game_state->t1.p2.x = 20.0f;
        //game_state->t1.p2.y = 10.0f;
        //game_state->t1.p3.x = 15.0f;
        //game_state->t1.p3.y = 20.0f;
    }


    for(ui32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_KEYDOWN){
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
        }
    }

    Color red = {1.0f, 0.0f, 0.0f};
    Color green = {0.0f, 1.0f, 0.0f};
    Color blue = {0.0f, 0.0f, 1.0f};
    Color white = {1.0f, 1.0f, 1.0f};
    Color black = {0.0f, 0.0f, 0.0f};

    v2 pos = {0, 0};
    v2 dim = {(f32)render_buffer->width, (f32)render_buffer->height};
    rect(render_buffer, pos, dim, &black);

    v2 t0[3] = {{10.0f, 70.0f}, {50.0f, 160.0f}, {70.0f, 80.0f}};
    v2 t1[3] = {{180.0f, 50.0f}, {150.0f, 1.0f}, {70.0f, 180.0f}};
    v2 t2[3] = {{180.0f, 150.0f}, {120.0f, 160.0f}, {130.0f, 180.0f}};
    //triangle(render_buffer, t0[0], t0[1], t0[2], &red);
    //triangle(render_buffer, t1[0], t1[1], t1[2], &blue);
    //triangle(render_buffer, t2[0], t2[1], t2[2], &green);
    //set_pixel(render_buffer, 0, 0, &red);
    //set_pixel(render_buffer, 959, 0, &red);
    //set_pixel(render_buffer, 0, 539, &red);
    //set_pixel(render_buffer, 959, 539, &red);

    v2 l1[2] = {{10, 300}, {100, 300}};
    //line(render_buffer, l1[0], l1[1], &red);

    v2 l2[2] = {{100, 300}, {100, 200}};
    //line(render_buffer, l2[0], l2[1], &red);

    // flat bottom
    v2 t3[3] = {{200.0f, 70.0f}, {250.0f, 160.0f}, {300.0f, 70.0f}};
    //triangle(render_buffer, t3[2], t3[1], t3[0], &red);
    // flat top
    v2 t4[3] = {{400.0f, 160.0f}, {450.0f, 70.0f}, {500.0f, 160.0f}};
    v2 translation = {0.01f, 0.01f};
    //v2 scalar = {3.0f, 3.0f};
    v2 scalar = {2.01f, 2.01f};
    tri new = {0};
    //game_state->t1 = translate(game_state->t1, translation);
    v2 center = calc_centroid(game_state->t1.p1, game_state->t1.p2, game_state->t1.p3);
    //game_state->t1 = scale(game_state->t1, scalar, center);
    game_state->t1 = rotate(game_state->t1, 1, center);
    triangle(render_buffer, game_state->t1.p1, game_state->t1.p2, game_state->t1.p3, &green);
    //new = scale(game_state->t1, scalar, center);
    //new = scale(game_state->t1, scalar, center);
    new = rotate(game_state->t1, 20, center);
    //game_state->t1 = rotate(new, 1, center);
    //triangle(render_buffer, new.p1, new.p2, new.p3, &green);

    v2 t5[3] = {{650.0f, 160.0f}, {670.0f, 80.0f}, {614.0f, 80.0f}};
    //triangle(render_buffer, t5[2], t5[1], t5[0], &red);
}
