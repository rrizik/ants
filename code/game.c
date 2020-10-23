#include "game.h"

static i32
round_fi32(f32 value){
    i32 result = (i32)(value + 0.5f);
    return(result);
}

static void
swapf(f32 *a, f32 *b){
    f32 t = *a;
    *a = *b;
    *b = t;
}

static void
swapv2(v2 *a, v2 *b){
    v2 t = *a;
    *a = *b;
    *b = t;
}

static void
set_pixel(RenderBuffer *buffer, f32 x, f32 y, Color c){
    i32 rounded_x = round_fi32(x);
    i32 rounded_y = round_fi32(y);

    ui32 color = (round_fi32(c.r * 255) << 16 | round_fi32(c.g * 255) << 8 | round_fi32(c.b * 255));
    ui8 *row = (ui8 *)buffer->memory + ((buffer->height - 1 - rounded_y) * buffer->pitch) + (rounded_x * buffer->bytes_per_pixel);
    ui32 *pixel = (ui32 *)row;
    *pixel = color;
}

static void 
line(RenderBuffer *buffer, v2 p0, v2 p1, Color c){
    // NOTE: independent from drawing a line
    //i32 rounded_x0 = round_fi32(p0.x);
    //i32 rounded_y0 = round_fi32(p0.y);
    //i32 rounded_x1 = round_fi32(p1.x);
    //i32 rounded_y1 = round_fi32(p1.y);

    // NOTE: STEP1: get the distance of x1 - x0, y1 - y0
    f32 distance_x =  ABS(p1.x - p0.x);
    f32 distance_y = -ABS(p1.y - p0.y);
    // NOTE: STEP2: get step direction in which to draw the line
    f32 step_x = p0.x < p1.x ? 1.0f : -1.0f;
    f32 step_y = p0.y < p1.y ? 1.0f : -1.0f; 

    // NOTE: STEP3: get the error threshold
    f32 error = distance_x + distance_y;

    do{
        // NOTE: STEP4: draw atleast 1 pixel. p0 == p1.
        set_pixel(buffer, p0.x, p0.y, c);

        // NOTE: STEP5: determine in which direction to increment x,y based on error
        if ((error * 2) >= distance_y){ 
            error += distance_y; 
            p0.x += step_x; 
        }
        if ((error * 2) <= distance_x){ 
            error += distance_x; 
            p0.y += step_y; 
        }
    } while(p0.x != p1.x && p0.y != p1.y);
}

static void
interpolate(i32 x0, i32 y0, i32 x1, i32 y1){
}

static void
fill_flatbottom_triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color c){
    f32 slope_p12 = (p2.x - p1.x) / (p2.y - p1.y);
    f32 slope_p13 = (p3.x - p1.x) / (p3.y - p1.y);

    f32 p2_x = p3.y;
    f32 p3_x = p3.y;

    for(f32 y=p3.y; y < p1.y; ++y){
        v2 new_p1 = {p2_x, y};
        v2 new_p2 = {p3_x, y};
        line(buffer, p1, p2, c);
        p2_x += slope_p12;
        p3_x += slope_p13;
    }
}

static void
triangle(RenderBuffer *buffer, v2 p0, v2 p1, v2 p2, Color c){
    // TODO: REMOVE COLORS
    Color red = {1.0f, 0.0f, 0.0f};
    Color blue = {0.0f, 0.0f, 1.0f};
    Color green = {0.0f, 1.0f, 0.0f};

    i32 rounded_x0 = round_fi32(p0.x);
    i32 rounded_y0 = round_fi32(p0.y);
    i32 rounded_x1 = round_fi32(p1.x);
    i32 rounded_y1 = round_fi32(p1.y);
    i32 rounded_x2 = round_fi32(p2.x);
    i32 rounded_y2 = round_fi32(p2.y);

    if(p0.y > p1.y){ swapv2(&p0, &p1); }
    if(p0.y > p2.y){ swapv2(&p0, &p2); }
    if(p1.y > p2.y){ swapv2(&p1, &p2); }

    //i32 x01 = interpolate(rounded

    line(buffer, p0, p1, green);
    line(buffer, p1, p2, green);
    line(buffer, p2, p0, red);
}

static void
rect(RenderBuffer *buffer, v2 pos, v2 dim, Color c){
    i32 rounded_x = round_fi32(pos.x);
    i32 rounded_y = round_fi32(pos.y);
    i32 rounded_w = round_fi32(dim.x);
    i32 rounded_h = round_fi32(dim.y);

    if(rounded_x < 0) { rounded_x = 0; }
    if(rounded_y < 0) { rounded_y = 0; }
    if(rounded_x + rounded_w > buffer->width){ rounded_x = buffer->width - rounded_w; }
    if(rounded_y + rounded_h > buffer->height){ rounded_y = buffer->height - rounded_h; }

    for(int y = rounded_y; y < rounded_y + rounded_h; ++y){
        for(int x = rounded_x; x < rounded_x + rounded_w; ++x){
            set_pixel(buffer, (f32)x, (f32)y, c);
        }
    }
}

MAIN_GAME_LOOP(main_game_loop){
    //GameState *game_state = (GameState *)memory->permanent_storage;
    
    if(!memory->initialized){
        memory->initialized = true;
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
    Color blue = {0.0f, 0.0f, 1.0f};
    Color green = {0.0f, 1.0f, 0.0f};
    Color white = {1.0f, 1.0f, 1.0f};

    v2 pos = {0, 0};
    v2 dim = {(f32)render_buffer->width, (f32)render_buffer->height};
    //rect(render_buffer, pos, dim, white);

    v2 t0[3] = {{10.0f, 70.0f}, {50.0f, 160.0f}, {70.0f, 80.0f}};
    v2 t1[3] = {{180.0f, 50.0f}, {150.0f, 1.0f}, {70.0f, 180.0f}};
    v2 t2[3] = {{180.0f, 150.0f}, {120.0f, 160.0f}, {130.0f, 180.0f}};
    triangle(render_buffer, t0[0], t0[1], t0[2], red);
    triangle(render_buffer, t1[0], t1[1], t1[2], blue);
    triangle(render_buffer, t2[0], t2[1], t2[2], green);

    v2 p0 = {10, 300};
    v2 p1 = {10, 200};
    line(render_buffer, p1, p0, red);
}
