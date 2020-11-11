#include "game.h"
#include "math.h"
#include "vectors.h"
#include "matrices.h"


__attribute__((overloadable)) static vec2
calc_centroid(vec2 p1, vec2 p2, vec2 p3){
    vec2 result = {0};

    result.x = (p1.x + p2.x + p3.x) / 3.0f;
    result.y = (p1.y + p2.y + p3.y) / 3.0f;

    return(result);
}

__attribute__((overloadable)) static vec2
calc_centroid(Points p){
    vec2 result = {0};

    for(ui32 i=0; i<p.count; ++i){
        result.x += p.at->x;
        result.y += p.at->y;
        p.at++;
    }

    result.x /= p.count;
    result.y /= p.count;

    return(result);
}

__attribute__((overloadable)) static tri
translate(tri t, vec2 translation){
    t.p1 = add2(t.p1, translation);
    t.p2 = add2(t.p2, translation);
    t.p3 = add2(t.p3, translation);

    return(t);
}

__attribute__((overloadable)) static void
translate(Points *p, vec2 translation){
    for(ui32 i=0; i<p->count; ++i){
        *p->at = add2(*p->at, translation);
        p->at++;
    }
    p->at -= p->count;
}

__attribute__((overloadable)) static tri
scale(tri t, f32 scalar, vec2 origin){
    t.p1 = add2(scale2(sub2(t.p1, origin), scalar), origin);
    t.p2 = add2(scale2(sub2(t.p2, origin), scalar), origin);
    t.p3 = add2(scale2(sub2(t.p3, origin), scalar), origin);
    return(t);
}

__attribute__((overloadable)) static void
scale(Points *p, f32 scalar, vec2 origin){
    for(ui32 i=0; i<p->count; ++i){
        *p->at = add2(scale2(sub2(*p->at, origin), scalar), origin);
        p->at++;
    }
    p->at -= p->count;
}

__attribute__((overloadable)) static tri
rotate(tri t, i16 angle, vec2 origin){
    float x1 = (t.p1.x - origin.x) * Cos(angle * 3.14f/180.0f) - (t.p1.y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
    float y1 = (t.p1.x - origin.x) * Sin(angle * 3.14f/180.0f) + (t.p1.y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
    float x2 = (t.p2.x - origin.x) * Cos(angle * 3.14f/180.0f) - (t.p2.y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
    float y2 = (t.p2.x - origin.x) * Sin(angle * 3.14f/180.0f) + (t.p2.y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
    float x3 = (t.p3.x - origin.x) * Cos(angle * 3.14f/180.0f) - (t.p3.y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
    float y3 = (t.p3.x - origin.x) * Sin(angle * 3.14f/180.0f) + (t.p3.y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
    t.p1.x = x1;
    t.p1.y = y1;
    t.p2.x = x2;
    t.p2.y = y2;
    t.p3.x = x3;
    t.p3.y = y3;

    return(t);
}

__attribute__((overloadable)) static void
rotate(Points *p, i16 angle, vec2 origin){
    for(ui32 i=0; i<p->count; ++i){
        float x = (p->at->x - origin.x) * Cos(angle * 3.14f/180.0f) - (p->at->y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
        float y = (p->at->x - origin.x) * Sin(angle * 3.14f/180.0f) + (p->at->y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
        p->at->x = x;
        p->at->y = y;
        p->at++;
    }
    p->at -= p->count;
}

__attribute__((overloadable)) static void
point(RenderBuffer *buffer, f32 x, f32 y, Color c){
    x = round_ff(x);
    y = round_ff(y);

    if(x >= 0 && x < buffer->width && y >= 0 && y < buffer->height){
        ui32 color = (round_fi32(c.r * 255) << 16 | round_fi32(c.g * 255) << 8 | round_fi32(c.b * 255));
        ui8 *row = (ui8 *)buffer->memory + ((buffer->height - 1 - (i32)y) * buffer->pitch) + ((i32)x * buffer->bytes_per_pixel);
        ui32 *pixel = (ui32 *)row;
        *pixel = color;
    }
}

__attribute__((overloadable)) static void
point(RenderBuffer *buffer, vec2 p, Color c){
    p.x = round_ff(p.x);
    p.y = round_ff(p.y);

    if(p.x >= 0 && p.x < buffer->width && p.y >= 0 && p.y < buffer->height){
        ui32 color = (round_fi32(c.r * 255) << 16 | round_fi32(c.g * 255) << 8 | round_fi32(c.b * 255));
        ui8 *row = (ui8 *)buffer->memory + ((buffer->height - 1 - (i32)p.y) * buffer->pitch) + ((i32)p.x * buffer->bytes_per_pixel);
        ui32 *pixel = (ui32 *)row;
        *pixel = color;
    }
}

static void 
line(RenderBuffer *buffer, vec2 p1, vec2 p2, Color c){
    p1 = round_v2(p1);
    p2 = round_v2(p2);

    f32 distance_x =  ABS(p2.x - p1.x);
    f32 distance_y = -ABS(p2.y - p1.y);
    f32 step_x = p1.x < p2.x ? 1.0f : -1.0f;
    f32 step_y = p1.y < p2.y ? 1.0f : -1.0f; 

    f32 error = distance_x + distance_y;

    for(;;){
        point(buffer, p1.x, p1.y, c); // NOTE: before break, so you can draw a single point line (a pixel)
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

__attribute__((overloadable)) static void
fill_flatbottom_triangle(RenderBuffer *buffer, vec2 p1, vec2 p2, vec2 p3, Color c){
    f32 slope_1 = (p2.x - p1.x) / (p2.y - p1.y);
    f32 slope_2 = (p3.x - p1.x) / (p3.y - p1.y);
    
    f32 x1_inc = p1.x;
    f32 x2_inc = p1.x;

    for(f32 y=p1.y; y >= p2.y; --y){
        vec2 new_p1 = {x1_inc, y};
        vec2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc -= slope_1;
        x2_inc -= slope_2;
    }
}

__attribute__((overloadable)) static void
fill_flatbottom_triangle(RenderBuffer *buffer, tri t, Color c){
    f32 slope_1 = (t.p2.x - t.p1.x) / (t.p2.y - t.p1.y);
    f32 slope_2 = (t.p3.x - t.p1.x) / (t.p3.y - t.p1.y);
    
    f32 x1_inc = t.p1.x;
    f32 x2_inc = t.p1.x;

    for(f32 y=t.p1.y; y >= t.p2.y; --y){
        vec2 new_p1 = {x1_inc, y};
        vec2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc -= slope_1;
        x2_inc -= slope_2;
    }
}

__attribute__((overloadable)) static void
fill_flattop_triangle(RenderBuffer *buffer, vec2 p1, vec2 p2, vec2 p3, Color c){
    f32 slope_1 = (p1.x - p3.x) / (p1.y - p3.y);
    f32 slope_2 = (p2.x - p3.x) / (p2.y - p3.y);
    
    f32 x1_inc = p3.x;
    f32 x2_inc = p3.x;

    for(f32 y=p3.y; y < p1.y; ++y){
        vec2 new_p1 = {x1_inc, y};
        vec2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc += slope_1;
        x2_inc += slope_2;
    }
}

__attribute__((overloadable)) static void
fill_flattop_triangle(RenderBuffer *buffer, tri t, Color c){
    f32 slope_1 = (t.p1.x - t.p3.x) / (t.p1.y - t.p3.y);
    f32 slope_2 = (t.p2.x - t.p3.x) / (t.p2.y - t.p3.y);
    
    f32 x1_inc = t.p3.x;
    f32 x2_inc = t.p3.x;

    for(f32 y=t.p3.y; y < t.p1.y; ++y){
        vec2 new_p1 = {x1_inc, y};
        vec2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc += slope_1;
        x2_inc += slope_2;
    }
}

__attribute__((overloadable)) static void
triangle(RenderBuffer *buffer, vec2 p1, vec2 p2, vec2 p3, Color c, bool wire){
    p1 = round_v2(p1);
    p2 = round_v2(p2);
    p3 = round_v2(p3);

    if(p1.y < p2.y){ swapvec2(&p1, &p2); }
    if(p1.y < p3.y){ swapvec2(&p1, &p3); }
    if(p2.y < p3.y){ swapvec2(&p2, &p3); }

    if(wire){
        line(buffer, p1, p2, c);
        line(buffer, p2, p3, c);
        line(buffer, p3, p1, c);
    }
    else{
        if(p2.y == p3.y){
            fill_flatbottom_triangle(buffer, p1, p2, p3, c);
        }
        if(p1.y == p2.y){
            fill_flattop_triangle(buffer, p1, p2, p3, c);
        }
        else{
            f32 x = p1.x + ((p2.y - p1.y) / (p3.y - p1.y)) * (p3.x - p1.x);
            vec2 p4 = {x, p2.y};
            fill_flatbottom_triangle(buffer, p1, p2, p4, c);
            fill_flattop_triangle(buffer, p2, p4, p3, c);
        }
    }
}

__attribute__((overloadable)) static void
triangle(RenderBuffer *buffer, tri t, Color c, bool wire){
    t.p1 = round_v2(t.p1);
    t.p2 = round_v2(t.p2);
    t.p3 = round_v2(t.p3);

    if(t.p1.y < t.p2.y){ swapvec2(&t.p1, &t.p2); }
    if(t.p1.y < t.p3.y){ swapvec2(&t.p1, &t.p3); }
    if(t.p2.y < t.p3.y){ swapvec2(&t.p2, &t.p3); }

    if(wire){
        line(buffer, t.p1, t.p2, c);
        line(buffer, t.p2, t.p3, c);
        line(buffer, t.p3, t.p1, c);
    }
    else{
        if(t.p2.y == t.p3.y){
            fill_flatbottom_triangle(buffer, t, c);
        }
        if(t.p1.y == t.p2.y){
            fill_flattop_triangle(buffer, t, c);
        }
        else{
            f32 x = t.p1.x + ((t.p2.y - t.p1.y) / (t.p3.y - t.p1.y)) * (t.p3.x - t.p1.x);
            vec2 p4 = {x, t.p2.y};
            fill_flatbottom_triangle(buffer, t.p1, t.p2, p4, c);
            fill_flattop_triangle(buffer, t.p2, p4, t.p3, c);
        }
    }
}

__attribute__((overloadable)) static void
triangle(RenderBuffer *buffer, tri t, Color c){
    triangle(buffer, t, c, false);
}

__attribute__((overloadable)) static void
triangle(RenderBuffer *buffer, vec2 p1, vec2 p2, vec2 p3, Color c){
    triangle(buffer, p1, p2, p3, c, false);
}

static void
rect(RenderBuffer *buffer, vec2 pos, vec2 dim, Color c){
    pos.x = round_ff(pos.x);
    pos.y = round_ff(pos.y);
    dim.x = round_ff(dim.x);
    dim.y = round_ff(dim.y);

    vec2 p1 = {(f32)pos.x, (f32)pos.y};
    vec2 p2 = {(f32)(pos.x + dim.x), (f32)pos.y};
    vec2 p3 = {(f32)pos.x, (f32)(pos.y + dim.y)};
    vec2 p4 = {(f32)(pos.x + dim.x), (f32)(pos.y + dim.y)};
    triangle(buffer, p1, p2, p3, c);
    triangle(buffer, p2, p3, p4, c);
}

static void
polygon(RenderBuffer *buffer, Points points, Color c){
    vec2 first = *points.at++;
    vec2 prev = first;
    for(ui32 i=1; i<points.count; ++i){
        line(buffer, prev, *points.at, c);
        prev = *points.at++;
    }
    line(buffer, first, prev, c);
}

static void
matmul(Points points, Matrix matrix, Points *new_points){
    for(ui32 i=0; i<points.count; ++i){
        new_points->at->x = (points.at->x * *(matrix.at + 0)) + (points.at->y * *(matrix.at + 1));
        new_points->at->y = (points.at->y * *(matrix.at + 2)) + (points.at->y * *(matrix.at + 3));
        points.at++;
        new_points->at++;
    }
    new_points->at -= new_points->count;
}

MAIN_GAME_LOOP(main_game_loop){
    GameState *game_state = (GameState *)memory->permanent_storage;
    
    vec2 t6[5] = {{100.0f, 100.0f}, {300.0f, 100.0f}, {300.0f, 200.0f}, {200.0f, 300.0f}, {100.0f, 300.0f}};
    if(!memory->initialized){
        memory->initialized = true;

        game_state->t1.p1.x = 400.0f;
        game_state->t1.p1.y = 160.0f;
        game_state->t1.p2.x = 450.0f;
        game_state->t1.p2.y = 70.0f;
        game_state->t1.p3.x = 500.0f;
        game_state->t1.p3.y = 160.0f;

        game_state->t2.p1.x = 600.0f;
        game_state->t2.p1.y = 160.0f;
        game_state->t2.p2.x = 650.0f;
        game_state->t2.p2.y = 70.0f;
        game_state->t2.p3.x = 700.0f;
        game_state->t2.p3.y = 160.0f;
        vec2 center = calc_centroid(game_state->t1.p1, game_state->t1.p2, game_state->t1.p3);
        game_state->t2 = rotate(game_state->t2, 45, center);

        game_state->p.at = t6;
        game_state->p.count = 5;
        game_state->angle = 0;
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

    vec2 pos = {0, 0};
    vec2 dim = {(f32)render_buffer->width, (f32)render_buffer->height};
    rect(render_buffer, pos, dim, black);

    tri t0 = {{10.0f, 70.0f}, {50.0f, 160.0f}, {70.0f, 80.0f}};

    vec2 translation = {0.01f, 0.01f};
    f32 scalar = 2.0f;
    tri new = {0};
    //game_state->t1 = translate(game_state->t1, translation);
    vec2 center = calc_centroid(game_state->t1.p1, game_state->t1.p2, game_state->t1.p3);
    //game_state->t1 = scale(game_state->t1, scalar, center);
    game_state->t1 = rotate(game_state->t1, 1, center);
    triangle(render_buffer, game_state->t1, green);
    game_state->t2 = rotate(game_state->t2, 1, center);
    triangle(render_buffer, game_state->t2, red);

    vec2 t5[3] = {{650.0f, 160.0f}, {670.0f, 80.0f}, {614.0f, 80.0f}};

    //f32 m[2][2] = {{2,0}, {0,2}};
    //Matrix matrix = {0};
    //matrix.at = *m;
    //matrix.col = 2;
    //matrix.row = 2;

    //vec2 t6[5] = {{100.0f, 100.0f}, {300.0f, 100.0f}, {300.0f, 200.0f}, {200.0f, 300.0f}, {100.0f, 300.0f}};
    //Points points = {0};
    //points.at = t6;
    //points.count = ArrayCount(t6);

    //vec2 t7[5];
    //Points new_points = {0};
    //new_points.at = t7;
    //new_points.count = 5;
    //matmul(points, matrix, &new_points);
    
    vec2 poly_center = calc_centroid(game_state->p);
    translate(&game_state->p, translation);
    rotate(&game_state->p, game_state->angle, poly_center);
    game_state->angle++;
    if(game_state->angle >= 360){
        game_state->angle = 0;
    }
    scale(&game_state->p, scalar, poly_center);
    polygon(render_buffer, game_state->p, red);
    vec2 a = {3.0f, 3.0f};
    vec2 r = {0.0f, 1.0f};
    vec2 b = {-3.0f, -3.0f};
    vec2 c = {-3.0f, 3.0f};
    vec2 d = {3.0f, 3.0f};
    vec2 e = {3.0f, 0.0f};
    vec2 f = {0.0f, 3.0f};
    f32 r1 = dot2(a, b);
    f32 r2 = dot2(a, c);
    f32 r3 = dot2(a, d);
    f32 r4 = dot2(e, e);

    f32 result = angle2(e, f);
    f32 deg = RAD2DEG(result);
    f32 rad = DEG2RAD(deg);
    vec2 res = reflection2(a, r);

    mat2 m1 = {1.0f, 2.0f, 3.0f, 4.0f};
    mat2 m2 = {1.0f, 2.0f, 3.0f, 4.0f};
    mat2 m3 = mul2x2(m1, m2);
    mat2 m4 = identity2x2();
    mat3 m5 = identity3x3();
    mat4 m6 = identity4x4();
    mat2 m7 = mul2x2(m1, m4);
    vec2 v1 = {1.0f};

    //vec2 v1 = {3.0f, 4.0f};
    //vec2 v2 = normalized2(v1);
    //f32 length1 = magnitude_sq2(v2);
    //f32 length2 = magnitude2(v2);
    //mat2 m2 = transpose2(m1);
    //m2.e[0] = 100.0f;
    //mat2 sm2 = scale2x2(m2, 4);

    //polygon(render_buffer, new_points, white);
}
