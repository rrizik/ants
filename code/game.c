#include "game.h"
#include "math.h"


static v2
calc_center(v2 *p, u32 count){
    v2 result = {0};

    for(u32 i=0; i<count; ++i){
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
translate(v2 *p, u32 count, v2 translation){
    for(u32 i=0; i<count; ++i){
        *p = add2(*p, translation);
        p++;
    }
    p -= count;
}

static void
scale(v2 *p, u32 count, f32 scalar, v2 origin){
    for(u32 i=0; i<count; ++i){
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
rotate_pts(v2 *p, u32 count, f32 angle, v2 origin){
    for(u32 i=0; i<count; ++i){
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

    u8 *location = (u8 *)buffer->memory +
                    ((buffer->height - (i32)y - 1) * buffer->pitch) +
                    ((i32)x * buffer->bytes_per_pixel);
    u32 *pixel = (u32 *)location;
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
        u8 *row = (u8 *)buffer->memory +
                   ((buffer->height - y - 1) * buffer->pitch) +
                   (x * buffer->bytes_per_pixel);

        u32 *pixel = (u32 *)row;

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

        //u32 new_color = (round_fi32(color.a * 255.0f) << 24 | round_fi32(color.r) << 16 | round_fi32(color.g) << 8 | round_fi32(color.b) << 0);
        u32 new_color = (round_fi32(color.a * 255.0f) << 24 | round_fi32(new_r) << 16 | round_fi32(new_g) << 8 | round_fi32(new_b) << 0);
        *pixel = new_color;
    }
}

static void
draw_line(RenderBuffer *buffer, v2 position, v2 direction, v4 c){
    v2 point1 = round_v2(position);
    v2 point2 = point1;
    v2 non_normalized_direction = vec2(position.x + direction.x, position.y + direction.y);
    direction = round_v2(non_normalized_direction);

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
draw_ray(RenderBuffer *buffer, v2 position, v2 direction, v4 c){
    position = round_v2(position);
    v2 non_normalized_direction = round_v2(vec2((direction.x * 100000), (direction.y * 100000)));
    v2 new_direction = vec2(position.x + non_normalized_direction.x, position.y + non_normalized_direction.y);

    f32 distance_x =  ABS(new_direction.x - position.x);
    f32 distance_y = -ABS(new_direction.y - position.y);
    f32 step_x = position.x < new_direction.x ? 1.0f : -1.0f;
    f32 step_y = position.y < new_direction.y ? 1.0f : -1.0f;

    f32 error = distance_x + distance_y;

    for(;;){
        draw_pixel(buffer, position.x, position.y, c); // NOTE: before break, so you can draw a single position draw_segment (a pixel)
        if(position.x < 0 || position.x > buffer->width || position.y < 0 || position.y > buffer->height)break;

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y;
            position.x += step_x;
        }
        if (error2 <= distance_x){
            error += distance_x;
            position.y += step_y;
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
draw_triangle_outlined(RenderBuffer *buffer, v2 p0, v2 p1, v2 p2, v4 c, v4 c_outlined, bool fill){
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
draw_triangle(RenderBuffer *buffer, v2 p0, v2 p1, v2 p2, v4 c, bool fill){
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
draw_bitmap_clip(RenderBuffer *buffer, v2 position, Bitmap image, v4 clip_region){
    v4 cr = {100, 300, 200, 200};
    if(equal4(clip_region, vec4(0,0,0,0))){
        f32 rounded_x = round_ff(position.x);
        f32 rounded_y = round_ff(position.y);
        for(f32 y=rounded_y; y < rounded_y + image.height; ++y){
            for(f32 x=rounded_x; x < rounded_x + image.width; ++x){
                v4 color = convert_u32_v4_normalized(*image.pixels++);
                draw_pixel(buffer, x, y, color);
            }
        }
    }
    else{
        v4 result = {position.x, position.y, image.width, image.height};

        if(position.x < cr.x){
            result.x = cr.x;
            result.w = image.width - (cr.x - position.x);
        }
        if((result.x + result.w) > (cr.x + cr.w)){
            result.w = result.w - ((result.x + result.w) - (cr.x + cr.w));
        }

        if(position.y < cr.y){
            result.y = cr.y;
            result.h = image.height - (cr.y - position.y);
        }
        if((result.y + result.h) > (cr.y + cr.h)){
            result.h = result.h - ((result.y + result.h) - (cr.y + cr.h));
        }

        u32 rounded_x = round_fu32(result.x);
        u32 rounded_y = round_fu32(result.y);
        
        u32 x_shift = (u32)clamp_f32(0, (cr.x - position.x), 100000);
        u32 y_shift = (u32)clamp_f32(0, (cr.y - position.y), 100000);

        u32 iy = 0;
        for(u32 y = rounded_y; y < rounded_y + result.h; ++y){
            u32 ix = 0;
            for(u32 x = rounded_x; x < rounded_x + result.w; ++x){
                u8 *byte = (u8 *)image.pixels + ((y_shift + iy) * image.width * 4) + ((x_shift + ix) * 4);
                u32 *c = (u32 *)byte;
                v4 color = convert_u32_v4_normalized(*c);
                draw_pixel(buffer, x, y, color);
                ix++;
            }
            iy++;
        }
    }
}

static void
draw_bitmap(RenderBuffer *buffer, v2 position, Bitmap image){
    draw_bitmap_clip(buffer, position, image, vec4(0,0,0,0));
}

static void
draw_rect(RenderBuffer *buffer, v2 position, v2 dimension, v4 c){
    v2 p0 = {position.x, position.y};
    v2 p1 = {position.x + dimension.w, position.y};
    v2 p2 = {position.x, position.y + dimension.h};
    v2 p3 = {position.x + dimension.w, position.y + dimension.h};

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
draw_box(RenderBuffer *buffer, v2 position, v2 dimension, v4 c){
    v2 p0 = {position.x, position.y};
    v2 p1 = {position.x + dimension.w, position.y};
    v2 p2 = {position.x + dimension.w, position.y + dimension.h};
    v2 p3 = {position.x, position.y + dimension.h};

    draw_segment(buffer, p0, p1, c);
    draw_segment(buffer, p1, p2, c);
    draw_segment(buffer, p2, p3, c);
    draw_segment(buffer, p3, p0, c);
}

static void
draw_quad(RenderBuffer *buffer, v2 p0_, v2 p1_, v2 p2_, v2 p3_, v4 c, bool fill){
    v2 p0 = p0_;
    v2 p1 = p1_;
    v2 p2 = p2_;
    v2 p3 = p3_;

    if(p0.x > p1.x){ swap_v2(&p0, &p1); }
    if(p0.x > p2.x){ swap_v2(&p0, &p2); }
    if(p0.y > p2.y){ swap_v2(&p0, &p2); }
    if(p0.x > p3.x){ swap_v2(&p0, &p3); }
    if(p0.y > p3.y){ swap_v2(&p0, &p3); }

    if(p1.x < p0.x){ swap_v2(&p1, &p0); }
    if(p1.x < p3.x){ swap_v2(&p1, &p3); }
    if(p1.y > p2.y){ swap_v2(&p1, &p2); }
    if(p1.y > p3.y){ swap_v2(&p1, &p3); }

    if(p2.x < p0.x){ swap_v2(&p2, &p0); }
    if(p2.x < p3.x){ swap_v2(&p2, &p3); }
    if(p2.y < p0.y){ swap_v2(&p2, &p0); }
    if(p2.y < p1.y){ swap_v2(&p2, &p1); }

    if(p3.x > p1.x){ swap_v2(&p3, &p1); }
    if(p3.x > p2.x){ swap_v2(&p3, &p2); }
    if(p3.y < p0.y){ swap_v2(&p3, &p0); }
    if(p3.y < p1.y){ swap_v2(&p3, &p1); }

    draw_triangle(buffer, p0, p1, p2, c, fill);
    draw_triangle(buffer, p0, p2, p3, c, fill);
}

static void
draw_polygon(RenderBuffer *buffer, v2 *points, u32 count, v4 c){
    v2 first = *points++;
    v2 prev = first;
    for(u32 i=1; i<count; ++i){
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
        result.pixels = (u32 *)((u8 *)bitmap_file.content + header->bitmap_offset);
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
        BaseCommand* base_command = (BaseCommand*)at;

        switch(base_command->type){
            case RenderCommand_ClearColor:{
                ClearColorCommand *command = (ClearColorCommand*)base_command;
                clear(render_buffer, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Pixel:{
                PixelCommand *command = (PixelCommand*)base_command;
                draw_pixel(render_buffer, command->x, command->y, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Segment:{
                SegmentCommand *command = (SegmentCommand*)base_command;
                draw_segment(render_buffer, command->p0, command->p1, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Ray:{
                RayCommand *command = (RayCommand*)base_command;
                draw_ray(render_buffer, command->base.position, command->direction, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Line:{
                LineCommand *command = (LineCommand*)base_command;
                draw_line(render_buffer, command->base.position, command->direction, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Rect:{
                RectCommand *command = (RectCommand*)base_command;
                draw_rect(render_buffer, command->base.position, command->dimension, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Box:{
                BoxCommand *command = (BoxCommand*)base_command;
                draw_box(render_buffer, command->base.position, command->dimension, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Quad:{
                QuadCommand *command = (QuadCommand*)base_command;
                draw_quad(render_buffer, command->p0, command->p1, command->p2, command->p3, command->base.color, command->base.fill);
                at = command + 1;
            } break;
            case RenderCommand_Triangle:{
                TriangleCommand *command = (TriangleCommand*)base_command;
                draw_triangle(render_buffer, command->p0, command->p1, command->p2, base_command->color, base_command->fill);
                at = command + 1;
            } break;
            case RenderCommand_Circle:{
                CircleCommand *command = (CircleCommand*)base_command;
                draw_circle(render_buffer, command->base.position.x, command->base.position.y, command->rad, command->base.color, command->base.fill);
                at = command + 1;
            } break;
            case RenderCommand_Bitmap:{
                BitmapCommand *command = (BitmapCommand*)base_command;
                draw_bitmap(render_buffer, command->base.position, command->image);
                at = command + 1;
            } break;
        }
    }
}

MAIN_GAME_LOOP(main_game_loop){

    v4 RED =     {1.0f, 0.0f, 0.0f,  0.5f};
    v4 GREEN =   {0.0f, 1.0f, 0.0f,  0.5f};
    v4 BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    v4 MAGENTA = {1.0f, 0.0f, 1.0f,  1.0f};
    v4 PINK =    {0.92f, 0.62f, 0.96f, 0.5f};
    v4 YELLOW =  {0.9f, 0.9f, 0.0f,  0.5f};
    v4 TEAL =    {0.0f, 1.0f, 1.0f,  0.5f};
    v4 ORANGE =  {1.0f, 0.5f, 0.15f,  0.5f};
    v4 DGRAY =   {0.5f, 0.5f, 0.5f,  0.5f};
    v4 LGRAY =   {0.8f, 0.8f, 0.8f,  0.5f};
    v4 WHITE =   {1.0f, 1.0f, 1.0f,  1.0f};
    v4 BLACK =   {0.0f, 0.0f, 0.0f,  1.f};

    assert(sizeof(GameState) <= memory->permanent_storage_size);
    assert(sizeof(TranState) <= memory->transient_storage_size);
    GameState *game_state = (GameState *)memory->permanent_storage;
    TranState *transient_state = (TranState *)memory->transient_storage;


    if(!memory->initialized){
        game_state->start = clock->get_ticks();
        game_state->fc = 0;
        game_state->wc = 0;
        game_state->cc = 0;
        game_state->dc = 0;
        game_state->tc = 0;
        game_state->ntc = 0;
        game_state->ptc = 0;

        //game_state->ants_count = 3000;
        //game_state->ants_count = 1000;
        game_state->ants_count = 300;
        //game_state->ants_count = 1;
        game_state->screen_width = render_buffer->width;
        game_state->screen_height = render_buffer->height;
        game_state->ant_speed = 400.0f;
        game_state->ant_speed_max = 400.0f;
        game_state->suck_speed = 0.0f;
        game_state->suck_speed_max = 2400.0f;

        seed_random(time(NULL), 0);

        game_state->added = false;
        game_state->entities_size = 100000;
        add_entity(game_state, EntityType_None);

        game_state->test = load_bitmap(memory, "test.bmp");
        game_state->circle = load_bitmap(memory, "circle.bmp");
        game_state->image = load_bitmap(memory, "image.bmp");

        game_state->colony_index = add_colony(game_state, vec2(render_buffer->width/2, render_buffer->height/2), 25, DGRAY, true);

        for(u32 i=0; i < game_state->ants_count; ++i){
            // BUT
            u32 index = add_ant(game_state, vec2(render_buffer->width/2, render_buffer->height/2), 2, LGRAY, true);
            Entity *ant = game_state->entities + index;
            ant->t = clock->get_ticks();
            ant->tp = clock->get_ticks();
            //add_ant(game_state, vec2(render_buffer->width/2, 50), 2, LGRAY, true);
        }

        //add_pixel(game_state, 1, 1, red);
        //game_state->player_index = add_player(game_state, vec2(50, 250), vec2(100, 100), green, game_state->circle);
        //Entity* player = game_state->entities + game_state->player_index;
        //player->draw_bounding_box = true;
        //game_state->clip_region_index = add_box(game_state, vec2(100, 300), vec2(200, 200), blue);
        //add_child(game_state, game_state->clip_region_index, game_state->player_index);
        //add_circle(game_state, vec2(400, 400), 2, green, true);

        //add_segment(game_state, vec2(300, 300), vec2(400, 300), magenta);
        //add_segment(game_state, vec2(300, 300), vec2(400, 400), magenta);
        //add_segment(game_state, vec2(300, 300), vec2(200, 350), magenta);
        //add_ray(game_state, vec2(20, 100), vec2(1, 1), teal);
        //add_line(game_state, vec2(40, 120), vec2(21, 21), pink);
        //add_quad(game_state, vec2(0, 400), vec2(20, 400), vec2(20, 500), vec2(0, 500), green, false);
        //add_quad(game_state, vec2(40, 400), vec2(60, 400), vec2(60, 500), vec2(40, 500), green, true);
        //add_rect(game_state, vec2(100, 100), vec2(50, 50), orange);

        //add_triangle(game_state, vec2(400, 100), vec2(500, 100), vec2(450, 200), lgray, true);
        //add_circle(game_state, vec2(450, 400), 50, dgray, false);

        //u32 c1_index = add_bitmap(game_state, vec2(20, 20), game_state->circle);
        //game_state->c2_index = add_bitmap(game_state, vec2(500, 50), game_state->image);
        //add_child(game_state, game_state->player_index, c1_index);
        //add_child(game_state, game_state->player_index, game_state->c2_index);

        initialize_arena(&game_state->permanent_arena, 
                         (u8*)memory->permanent_storage + sizeof(GameState), 
                         memory->permanent_storage_size - sizeof(GameState));
        initialize_arena(&transient_state->transient_arena, 
                         (u8*)memory->transient_storage + sizeof(TranState), 
                         memory->transient_storage_size - sizeof(TranState));

        transient_state->render_commands = allocate_render_commands(&transient_state->transient_arena, Megabytes(16));
        memory->initialized = true;
    }

    f32 seconds_elapsed = clock->get_seconds_elapsed(game_state->start, clock->get_ticks());
    f32 t = seconds_elapsed / 4.0f;
    t = clamp_f32(1.0f, t, 0.0f);
    f32 result = lerp(100, 0, t);
    //print("Result: %.05f - seconds_elapsed: %.05f - t: %.05f\n", result, seconds_elapsed, t);

    for(u32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_MOUSEMOTION){
            game_state->controller.mouse_pos = vec2(event->mouse_x, render_buffer->height - event->mouse_y);
        }
        if(event->type == EVENT_MOUSEDOWN){
            if(event->mouse == MOUSE_LBUTTON){
                game_state->controller.m1 = true;
            }
            if(event->mouse == MOUSE_RBUTTON){
                game_state->controller.m2 = true;
            }
        }
        if(event->type == EVENT_MOUSEUP){
            if(event->mouse == MOUSE_LBUTTON){
                game_state->controller.m1 = false;
            }
            if(event->mouse == MOUSE_RBUTTON){
                game_state->controller.m2 = false;
            }
        }
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

    f32 speed = 250.0f;

    if(game_state->player_index != 0){
        Entity *player = game_state->entities + game_state->player_index;
        if(game_state->controller.up){
            player->position.y += speed * clock->dt;
        }
        if(game_state->controller.down){
            player->position.y -= speed * clock->dt;
        }
        if(game_state->controller.left){
            player->position.x -= speed * clock->dt;
        }
        if(game_state->controller.right){
            player->position.x += speed * clock->dt;
        }
        for (Entity *child = player->first_child; child; child = child->next_child){
            if(game_state->controller.up){
                child->position.y += speed * clock->dt;
            }
            if(game_state->controller.down){
                child->position.y -= speed * clock->dt;
            }
            if(game_state->controller.left){
                child->position.x -= speed * clock->dt;
            }
            if(game_state->controller.right){
                child->position.x += speed * clock->dt;
            }
        }
    }


    if(game_state->controller.m1){
        game_state->fc++;
        add_food(game_state, game_state->controller.mouse_pos, 2, GREEN, true);
        //add_to_food_pheromone(game_state, game_state->controller.mouse_pos, 1, TEAL);
    }
    if(game_state->controller.m2){
        u32 index = add_to_home_pheromone(game_state, game_state->controller.mouse_pos, 1, MAGENTA);
        //if(!game_state->added){
        //    for(u32 i=0; i < 10000; ++i){
        //        add_food(game_state, game_state->controller.mouse_pos, 2, GREEN, true);
        //    }
        //    game_state->fc += 10000;
        //    //add_food(game_state, game_state->controller.mouse_pos, vec2(2,2), GREEN, true);
        //    //game_state->fc++;
        //    game_state->added = true;
        //}
    }
    else{
        game_state->added = false;
    }

    f32 damp = 10;
    game_state->fc = 0;
    game_state->wc = 0;
    game_state->cc = 0;
    game_state->dc = 0;
    game_state->tc = 0;
    game_state->ntc = 0;
    game_state->ptc = 0;
    game_state->c = 0;
    for(u32 entity_index = 0; entity_index <= game_state->entity_at; ++entity_index){
        Entity *entity = game_state->entities + entity_index;

        switch(entity->type){
            case EntityType_ToFoodPheromone:{
                u64 now = clock->get_ticks();
                f32 seconds_elapsed = clock->get_seconds_elapsed(entity->food_decay_timer, now);
                f32 t = seconds_elapsed / entity->pher_food_decay_rate;
                t = clamp_f32(1.0f, t, 0.0f);
                entity->color.a = lerp(1.0f, 0.0f, t);
                if(entity->color.a <= 0){
                    entity->color.a = 1.0f;
                    entity->type = EntityType_None;
                    game_state->entities_free++;
                }
            } break;
            case EntityType_ToHomePheromone:{
                u64 now = clock->get_ticks();
                f32 seconds_elapsed = clock->get_seconds_elapsed(entity->home_decay_timer, now);
                f32 t = seconds_elapsed / entity->pher_home_decay_rate;
                t = clamp_f32(1.0f, t, 0.0f);
                entity->color.a = lerp(1.0f, 0.0f, t);

                if(entity->color.a <= 0.0f){
                    entity->color.a = 1.0f;
                    entity->type = EntityType_None;
                    game_state->entities_free++;
                }
            } break;
            case EntityType_Ant:{
                Entity *ant = entity;

                if(ant->ant_state == AntState_Wondering){
                    ant->color = LGRAY;
                    game_state->wc++;
                    ant->right_sensor_density = 0.0f;
                    ant->middle_sensor_density = 0.0f;
                    ant->left_sensor_density = 0.0f;
                    //ant->right = false;
                    //ant->middle = false;
                    //ant->left = false;
                    for(u32 entity_index = game_state->ants_count; entity_index <= game_state->entity_at; ++entity_index){
                        Entity *entity = game_state->entities + entity_index;
                        switch(entity->type){
                            case EntityType_ToFoodPheromone:{
                                Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                if(rect_collide_point(right_rect, entity->position)){
                                    ant->right_sensor_density += entity->color.a;
                                }
                                if(rect_collide_point(mid_rect, entity->position)){
                                    ant->middle_sensor_density += entity->color.a;
                                }
                                if(rect_collide_point(left_rect, entity->position)){
                                    ant->left_sensor_density += entity->color.a;
                                }
                            } break;
                            case EntityType_Food:{
                                Entity *food = entity;
                                if(!food->targeted){
                                    Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                    Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                    Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                    if(rect_collide_point(right_rect, food->position) || rect_collide_point(mid_rect, food->position) || rect_collide_point(left_rect, food->position)){
                                        ant->ant_state = AntState_Collecting;
                                        ant->ant_food = food;
                                        food->targeted = true;
                                        ant->changing_state = true;
                                        //ant->color = RED;
                                        entity_index = game_state->entity_at;
                                        break;
                                    }
                                }
                            }break;
                        }
                    }

                    if(ant->ant_state != AntState_Collecting){
                        if(ant->right_sensor_density > 0 || ant->middle_sensor_density > 0 || ant->left_sensor_density > 0){
                            //ant->color = YELLOW;
                            f32 mid_rad = dir_rad(ant->direction);
                            f32 right_rad = mid_rad + (RAD * ant->sensor_angle);
                            f32 left_rad = mid_rad - (RAD * ant->sensor_angle);

                            v2 right_direction = rad_dir(right_rad);
                            v2 left_direction = rad_dir(left_rad);
                            if(ant->middle_sensor_density > MAXf32(ant->right_sensor_density, ant->left_sensor_density)){
                                ant->target_direction = ant->direction;
                                if(ant->middle == false){
                                    ant->rot_percent = 0.0f;
                                }
                                ant->right = false;
                                ant->middle = true;
                                ant->left = false;
                            }
                            else if(ant->right_sensor_density > ant->left_sensor_density){
                                ant->target_direction = right_direction;
                                if(ant->right == false){
                                    ant->rot_percent = 0.0f;
                                }
                                ant->right = true;
                                ant->middle = false;
                                ant->left = false;
                            }
                            else if(ant->left_sensor_density > ant->right_sensor_density){
                                ant->target_direction = left_direction;
                                if(ant->left == false){
                                    ant->rot_percent = 0.0f;
                                }
                                ant->right = false;
                                ant->middle = false;
                                ant->left = true;
                            }
                        }
                        else{
                            if(ant->timer >= ant->max_timer){
                                ant->timer = 0.0f;
                                ant->change_direction = true;
                            }
                            //print("seconds_elapsed: %.05f - max_timer: %i - timer: %i\n", seconds_elapsed, ant->t_max, ant->t);
                            //u64 now = clock->get_ticks();
                            //f32 seconds_elapsed = clock->get_seconds_elapsed(ant->t, now);
                            //if(seconds_elapsed >= ant->t_max){
                            //    ant->t = clock->get_ticks();
                            //    ant->change_direction = true;
                            //}
                            if(ant->change_direction){
                                ant->random_vector.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                                ant->random_vector.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                                ant->rot_percent = 0.0f;
                                ant->max_timer = (random_range(10) + 1);
                                ant->t_max = (random_range(3) + 1);
                                ant->change_direction = false;
                                //ant->target_direction = get_normalized2(add2(ant->direction, ant->random_vector));
                                ant->target_direction = ant->random_vector;
                            }
                        }
                        //print("rd: %.05f - md: %.05f - ld: %.05f - R: %i - M: %i - L: %i\n", ant->right_sensor_density, ant->middle_sensor_density, ant->left_sensor_density, ant->right, ant->middle, ant->left);
                        //print("rd: %.05f - md: %.05f - ld: %.05f - R: %i - M: %i - L: %i - RP: %.05f\n", ant->right_sensor_density, ant->middle_sensor_density, ant->left_sensor_density, ant->right, ant->middle, ant->left, ant->rot_percent);

                        ant->timer += 0.1f;
                        if(ant->changing_state){
                            ant->timer = 0.0f;
                            ant->t = clock->get_ticks();
                            ant->rot_percent = 0.0f;
                            ant->changing_state = false;
                        }

                        f32 ant_angle = dir_rad(ant->direction);
                        f32 target_angle = dir_rad(ant->target_direction);
                        ant->direction = rad_dir(lerp_rad(ant_angle, target_angle, ant->rot_percent));
                        ant->rot_percent += ant->rotate_speed;
                        if(ant->rot_percent > 1.0f){
                            ant->rot_percent = 1.0f;
                        }

                        u64 now = clock->get_ticks();
                        f32 seconds_elapsed = clock->get_seconds_elapsed(ant->tp, now);
                        if(seconds_elapsed >= ant->tp_max){
                            ant->tp = clock->get_ticks();
                            u32 index = add_to_home_pheromone(game_state, ant->position, 1, MAGENTA);
                            Entity *pher = game_state->entities + index;
                            pher->home_decay_timer = clock->get_ticks();
                        }
                            
                        if((ant->position.x + (game_state->ant_speed/damp * ant->direction.x) * clock->dt) < 0.0f ||
                           (ant->position.x + (game_state->ant_speed/damp * ant->direction.x) * clock->dt) > render_buffer->width){ 
                            ant->direction.x = -ant->direction.x;
                            ant->random_vector.x = -ant->random_vector.x;
                            ant->target_direction.x = -ant->target_direction.x;
                        }
                        else if((ant->position.y + (game_state->ant_speed/damp * ant->direction.y) * clock->dt) < 0 ||
                                (ant->position.y + (game_state->ant_speed/damp * ant->direction.y) * clock->dt) > render_buffer->height){
                            ant->direction.y = -ant->direction.y;
                            ant->random_vector.y = -ant->random_vector.y;
                            ant->target_direction.y = -ant->target_direction.y;
                        }
                        else{
                            ant->position.x += (game_state->ant_speed/damp * ant->direction.x) * clock->dt;
                            ant->position.y += (game_state->ant_speed/damp * ant->direction.y) * clock->dt;
                        }
                    }
                }
                if(ant->ant_state == AntState_Collecting){
                    if(ant->changing_state){
                        ant->rot_percent = 0.0f;
                        ant->changing_state = false;
                    }

                    ant->target_direction = direction2(ant->position, ant->ant_food->position);
                    f32 ant_angle = dir_rad(ant->direction);
                    f32 target_angle = dir_rad(ant->target_direction);
                    ant->direction = rad_dir(lerp_rad(ant_angle, target_angle, ant->rot_percent));
                    ant->rot_percent += ant->rotate_speed;
                    if(ant->rot_percent > 1.0f){
                        ant->rot_percent = 1.0f;
                    }

                    ant->position.x += (game_state->ant_speed/damp * ant->direction.x) * clock->dt;
                    ant->position.y += (game_state->ant_speed/damp * ant->direction.y) * clock->dt;

                    f32 distance = distance2(ant->position, ant->ant_food->position);
                    if(distance < 10){
                        ant->ant_state = AntState_Depositing;
                        ant->changing_state = true;
                        ant->ant_food->type = EntityType_None;
                        ant->ant_food->targeted = false;
                        game_state->entities_free++;
                        game_state->fc--;
                        ant->target_direction.x = -ant->target_direction.x;
                        ant->target_direction.y = -ant->target_direction.y;
                    }
                }
                if(ant->ant_state == AntState_Depositing){
                    game_state->dc++;
                    ant->color = RED;
                    ant->right_sensor_density = 0.0f;
                    ant->middle_sensor_density = 0.0f;
                    ant->left_sensor_density = 0.0f;
                    for(u32 entity_index = game_state->ants_count; entity_index <= game_state->entity_at; ++entity_index){
                        Entity *entity = game_state->entities + entity_index;
                        switch(entity->type){
                            case EntityType_ToHomePheromone:{
                                Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                                if(rect_collide_point(right_rect, entity->position)){
                                    ant->right_sensor_density += entity->color.a;
                                }
                                if(rect_collide_point(mid_rect, entity->position)){
                                    ant->middle_sensor_density += entity->color.a;
                                }
                                if(rect_collide_point(left_rect, entity->position)){
                                    ant->left_sensor_density += entity->color.a;
                                }
                            } break;
                        }
                    }
                    if(ant->right_sensor_density > 0 || ant->middle_sensor_density > 0 || ant->left_sensor_density > 0){
                        f32 mid_rad = dir_rad(ant->direction);
                        f32 right_rad = mid_rad + (RAD * ant->sensor_angle);
                        f32 left_rad = mid_rad - (RAD * ant->sensor_angle);

                        v2 right_direction = rad_dir(right_rad);
                        v2 left_direction = rad_dir(left_rad);
                        if(ant->middle_sensor_density > MAXf32(ant->right_sensor_density, ant->left_sensor_density)){
                            ant->target_direction = ant->direction;
                            if(ant->middle == false){
                                ant->rot_percent = 0.0f;
                            }
                            ant->right = false;
                            ant->middle = true;
                            ant->left = false;
                        }
                        else if(ant->right_sensor_density > ant->left_sensor_density){
                            ant->target_direction = right_direction;
                            if(ant->right == false){
                                ant->rot_percent = 0.0f;
                            }
                            ant->right = true;
                            ant->middle = false;
                            ant->left = false;
                        }
                        else if(ant->left_sensor_density > ant->right_sensor_density){
                            ant->target_direction = left_direction;
                            if(ant->left == false){
                                ant->rot_percent = 0.0f;
                            }
                            ant->right = false;
                            ant->middle = false;
                            ant->left = true;
                        }
                    }
                    else{
                        if(ant->t >= ant->t_max){
                            ant->t = clock->get_ticks();
                            ant->change_direction = true;
                        }
                        if(ant->change_direction){
                            ant->random_vector.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                            ant->random_vector.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                            ant->rot_percent = 0.0f;
                            ant->max_timer = (random_range(10) + 1);
                            ant->t_max = (random_range(3) + 1);
                            ant->change_direction = false;
                            ant->target_direction = get_normalized2(add2(ant->direction, ant->random_vector));
                        }
                    }
                    ant->timer += 0.1f;
                    if(ant->changing_state){
                        ant->timer = 0.0f;
                        ant->t = clock->get_ticks();
                        ant->rot_percent = 0.0f;
                        ant->changing_state = false;
                    }


                    //ant->target_direction = direction2(ant->position, colony->position);
                    f32 ant_angle = dir_rad(ant->direction);
                    f32 target_angle = dir_rad(ant->target_direction);
                    ant->direction = rad_dir(lerp_rad(ant_angle, target_angle, ant->rot_percent));
                    ant->rot_percent += ant->rotate_speed;
                    if(ant->rot_percent > 1.0f){
                        ant->rot_percent = 1.0f;
                    }

                    u64 now = clock->get_ticks();
                    f32 seconds_elapsed = clock->get_seconds_elapsed(ant->tp, now);
                    if(seconds_elapsed >= ant->tp_max){
                        ant->tp = clock->get_ticks();
                        u32 index = add_to_food_pheromone(game_state, ant->position, 1, TEAL);
                        Entity *pher = game_state->entities + index;
                        pher->food_decay_timer = clock->get_ticks();
                    }

                    ant->position.x += (game_state->ant_speed/damp * ant->direction.x) * clock->dt;
                    ant->position.y += (game_state->ant_speed/damp * ant->direction.y) * clock->dt;

                    Entity* colony = game_state->entities + game_state->colony_index;
                    f32 colony_distance = distance2(ant->position, colony->position);
                    if(colony_distance < 20){
                        //ant->color = LGRAY;
                        ant->ant_state = AntState_Wondering;
                        ant->changing_state = true;
                        ant->ant_food = NULL;
                        ant->target_direction.x = -ant->target_direction.x;
                        ant->target_direction.y = -ant->target_direction.y;
                    }
                }
            } break;
        }
    }


    transient_state->render_commands->used_bytes = 0;
    push_clear_color(transient_state->render_commands, BLACK);
    for(u32 entity_index = 1; entity_index <= game_state->entity_at; ++entity_index){
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type){
            case EntityType_Player:{
                push_bitmap(transient_state->render_commands, entity->position, entity->image);
                if(entity->draw_bounding_box){
                    push_box(transient_state->render_commands, entity->position, entity->dimension, entity->color);
                }
                //push_rect(transient_state->render_commands, entity->position, entity->dimension, entity->color);
            }break;
            case EntityType_Pixel:{
                push_pixel(transient_state->render_commands, entity->position, entity->color);
            }break;
            case EntityType_Segment:{
                push_segment(transient_state->render_commands, entity->p0, entity->p1, entity->color);
            }break;
            case EntityType_Line:{
                push_line(transient_state->render_commands, entity->position, entity->direction, entity->color);
            }break;
            case EntityType_Ray:{
                push_ray(transient_state->render_commands, entity->position, entity->direction, entity->color);
            }break;
            case EntityType_Rect:{
                push_rect(transient_state->render_commands, entity->position, entity->dimension, entity->color);
            }break;
            case EntityType_Box:{
                push_box(transient_state->render_commands, entity->position, entity->dimension, entity->color);
            }break;
            case EntityType_Quad:{
                push_quad(transient_state->render_commands, entity->p0, entity->p1, entity->p2, entity->p3, entity->color, entity->fill);
            }break;
            case EntityType_Triangle:{
                push_triangle(transient_state->render_commands, entity->p0, entity->p1, entity->p2, entity->color, entity->fill);
            }break;
            case EntityType_Circle:{
                push_circle(transient_state->render_commands, entity->position, entity->rad, entity->color, entity->fill);
                v2 dimenstion = vec2(entity->rad*2, entity->rad*2);
                v2 position = vec2(entity->position.x - entity->rad, entity->position.y - entity->rad);
                if(entity->draw_bounding_box){
                    push_box(transient_state->render_commands, position, dimenstion, entity->color);
                }
            }break;
            case EntityType_Bitmap:{
                push_bitmap(transient_state->render_commands, entity->position, entity->image);
            }break;
            case EntityType_None:{
            }break; 
            case EntityType_Object:{
            }break; 
            case EntityType_Food:{
                push_circle(transient_state->render_commands, entity->position, entity->rad, entity->color, entity->fill);
            }break; 
            case EntityType_Ant:{
                //HERE
                // TODO: segments seem to crash the game, figure out why
                //push_segment(transient_state->render_commands, entity->position, game_state->controller.mouse_pos, RED);

                
                push_circle(transient_state->render_commands, entity->position, entity->rad, entity->color, entity->fill);

                f32 mid_rad = dir_rad(entity->direction);
                f32 right_rad = mid_rad + (RAD * entity->sensor_angle);
                f32 left_rad = mid_rad - (RAD * entity->sensor_angle);

                v2 right_direction = rad_dir(right_rad);
                v2 left_direction = rad_dir(left_rad);

                entity->right_sensor = vec2(entity->position.x + (entity->sensor_distance * right_direction.x), entity->position.y + (entity->sensor_distance * right_direction.y));
                entity->mid_sensor = vec2(entity->position.x + (entity->sensor_distance * entity->direction.x), entity->position.y + (entity->sensor_distance * entity->direction.y));
                entity->left_sensor = vec2(entity->position.x + (entity->sensor_distance * left_direction.x), entity->position.y + (entity->sensor_distance * left_direction.y));

                Rect right_rect = rect(vec2(entity->right_sensor.x - entity->sensor_radius, entity->right_sensor.y - entity->sensor_radius), vec2(entity->sensor_radius * 2, entity->sensor_radius * 2));
                Rect mid_rect = rect(vec2(entity->mid_sensor.x - entity->sensor_radius, entity->mid_sensor.y - entity->sensor_radius), vec2(entity->sensor_radius * 2, entity->sensor_radius * 2));
                Rect left_rect = rect(vec2(entity->left_sensor.x - entity->sensor_radius, entity->left_sensor.y - entity->sensor_radius), vec2(entity->sensor_radius * 2, entity->sensor_radius * 2));


                //push_box(transient_state->render_commands, vec2(entity->right_sensor.x - entity->sensor_radius, entity->right_sensor.y - entity->sensor_radius), vec2(entity->sensor_radius * 2, entity->sensor_radius * 2), YELLOW);
                //push_box(transient_state->render_commands, vec2(entity->mid_sensor.x - entity->sensor_radius, entity->mid_sensor.y - entity->sensor_radius), vec2(entity->sensor_radius * 2, entity->sensor_radius * 2), GREEN);
                //push_box(transient_state->render_commands, vec2(entity->left_sensor.x - entity->sensor_radius, entity->left_sensor.y - entity->sensor_radius), vec2(entity->sensor_radius * 2, entity->sensor_radius * 2), ORANGE);

                //push_circle(transient_state->render_commands, entity->right_sensor, entity->sensor_radius, YELLOW, false);
                //push_circle(transient_state->render_commands, entity->mid_sensor, entity->sensor_radius, GREEN, false);
                //push_circle(transient_state->render_commands, entity->left_sensor, entity->sensor_radius, ORANGE, false);

                //push_segment(transient_state->render_commands, entity->position, entity->right_sensor, YELLOW);
                //push_segment(transient_state->render_commands, entity->position, entity->mid_sensor, GREEN);
                //push_segment(transient_state->render_commands, entity->position, entity->left_sensor, ORANGE);
                //push_ray(transient_state->render_commands, entity->position, entity->target_direction, BLUE);
            }break; 
            case EntityType_Colony:{
                push_circle(transient_state->render_commands, entity->position, entity->rad, entity->color, entity->fill);
            }break; 
            case EntityType_ToHomePheromone:{
                push_circle(transient_state->render_commands, entity->position, entity->rad, entity->color, true);
            }break; 
            case EntityType_ToFoodPheromone:{
                push_circle(transient_state->render_commands, entity->position, entity->rad, entity->color, true);
            }break; 
        }
    }
    //print("DT: %.05f - X: %.02f - Y: %.02f\n", clock->dt, game_state->c2->position.x, game_state->c2->position.y);
    //print("fc: %i - wc: %i - cc: %i - dc: %i - tc: %i - ntc: %i - ptc: %i\n", game_state->fc, game_state->wc, game_state->cc, game_state->dc, game_state->tc, game_state->ntc, game_state->ptc);
    draw_commands(render_buffer, transient_state->render_commands);
}
