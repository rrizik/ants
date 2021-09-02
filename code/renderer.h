#if !defined(RENDERER_H)

typedef struct Rect{
    v2 position;
    v2 dimension;
    f32 x; 
    f32 y;
    f32 w; 
    f32 h;
    f32 x1;
    f32 y1;
} Rect;

static Rect
rect(v2 pos, v2 dim){
    Rect result = {0};
    result.position = pos;
    result.dimension = dim;
    result.x = pos.x;
    result.y = pos.y;
    result.w = dim.x;
    result.h = dim.y;
    result.x1 = pos.x + dim.x;
    result.y1 = pos.y + dim.y;
    return(result);
}

static bool
rect_collides_rect(Rect r1, Rect r2){
    if((r1.x < r2.x + r2.w) &&
       (r1.x + r1.w > r2.x) &&
       (r1.y < r2.y + r2.h) &&
       (r1.y + r1.h > r2.y)){
        return true;
    }
    return false;
}

static bool
rect_collides_point(Rect r1, v2 p){
    if((p.x < r1.x + r1.w) &&
       (p.x > r1.x) &&
       (p.y < r1.y + r1.h) &&
       (p.y > r1.y)){
        return true;
    }
    return false;
}

static bool
rect_contains_rect(Rect r1, Rect r2){
    if((r2.x > r1.x) &&
       (r2.x + r2.w < r1.x + r1.w) &&
       (r2.y > r1.y) &&
       (r2.y + r2.h < r1.y + r1.h)){
        return true;
    }
    return false;
}

typedef enum RenderCommandType{
    RenderCommand_ClearColor,
    RenderCommand_Pixel,
    RenderCommand_Segment,
    RenderCommand_Line,
    RenderCommand_Ray,
    RenderCommand_Rect,
    RenderCommand_Box,
    RenderCommand_Quad,
    RenderCommand_Triangle,
    RenderCommand_Circle,
    RenderCommand_Bitmap,
} RenderCommandType;

typedef struct BaseCommand{
    RenderCommandType type;
    v2 position;
    v4 color;
    v4 clip_region;
    bool fill;
} BaseCommand;

typedef struct ClearColorCommand{
    BaseCommand base;
} ClearColorCommand;

typedef struct PixelCommand{
    BaseCommand base;
    f32 x;
    f32 y;
} PixelCommand;

typedef struct SegmentCommand{
    BaseCommand base;
    v2 p0;
    v2 p1;
} SegmentCommand;

typedef struct RayCommand{
    BaseCommand base;
    v2 direction;
} RayCommand;

typedef struct LineCommand{
    BaseCommand base;
    v2 direction;
} LineCommand;

typedef struct RectCommand{
    BaseCommand base;
    v2 dimension;
} RectCommand;

typedef struct BoxCommand{
    BaseCommand base;
    v2 dimension;
} BoxCommand;

typedef struct QuadCommand{
    BaseCommand base;
    v2 p0;
    v2 p1;
    v2 p2;
    v2 p3;
} QuadCommand;

typedef struct TriangleCommand{
    BaseCommand base;
    v2 p0;
    v2 p1;
    v2 p2;
} TriangleCommand;

typedef struct CircleCommand{
    BaseCommand base;
    u8 rad;
} CircleCommand;

typedef struct BitmapCommand{
    BaseCommand base;
    Bitmap image;
} BitmapCommand;

typedef struct RenderCommandBuffer{
    size_t max_bytes;
    size_t used_bytes;
    void* base;
} RenderCommandBuffer;

typedef struct LinkedListBuffer{
    size_t max_bytes;
    size_t used_bytes;
    void* base;
} LinkedListBuffer;

static RenderCommandBuffer*
allocate_render_commands_buffer(MemoryArena *arena, size_t max_bytes){
    RenderCommandBuffer* result = allocate_struct(arena, RenderCommandBuffer);
    result->base = allocate_size(arena, max_bytes);
    result->max_bytes = max_bytes;
    return(result);
}

static LinkedListBuffer*
allocate_LL_buffer(MemoryArena *arena, size_t max_bytes){
    LinkedListBuffer* result = allocate_struct(arena, LinkedListBuffer);
    result->base = allocate_size(arena, max_bytes);
    result->max_bytes = max_bytes;
    return(result);
}

static void*
allocate_LL_node(LinkedListBuffer* buffer){
    void* result = (char*)buffer->base + buffer->used_bytes;
    buffer->used_bytes += sizeof(LinkedList);
    assert(buffer->used_bytes < buffer->max_bytes);
    return(result);
}

#define allocate_command(commands, type) (type*)allocate_command_(commands, sizeof(type))
static void*
allocate_command_(RenderCommandBuffer* commands, size_t size){
    void* command = (char*)commands->base + commands->used_bytes;
    commands->used_bytes += size;
    assert(commands->used_bytes < commands->max_bytes);
    return(command);
}

static void
allocate_clear_color(RenderCommandBuffer *commands, v4 color){
    void* command = (char*)commands->base + commands->used_bytes;
    commands->used_bytes += sizeof(ClearColorCommand);
    assert(commands->used_bytes < commands->max_bytes);

    ClearColorCommand* new_command = command;
    new_command->base.color = color;
}

static void
allocate_pixel(RenderCommandBuffer *commands, v2 position, v4 color){
    PixelCommand* command = allocate_command(commands, PixelCommand);
    command->base.type = RenderCommand_Pixel;
    command->base.position = position;
    command->base.color = color;
}

static void
allocate_segment(RenderCommandBuffer *commands, v2 p0, v2 p1, v4 color){
    SegmentCommand* command = allocate_command(commands, SegmentCommand);
    command->base.type = RenderCommand_Segment;
    command->base.color = color;
    command->p0 = p0;
    command->p1 = p1;
}

static void
allocate_ray(RenderCommandBuffer *commands, v2 position, v2 direction, v4 color){
    RayCommand* command = allocate_command(commands, RayCommand);
    command->base.type = RenderCommand_Ray;
    command->base.position = position;
    command->base.color = color;
    command->direction = direction;
}

static void
allocate_line(RenderCommandBuffer *commands, v2 position, v2 direction, v4 color){
    LineCommand* command = allocate_command(commands, LineCommand);
    command->base.type = RenderCommand_Line;
    command->base.position = position;
    command->base.color = color;
    command->direction = direction;
}

static void
allocate_rect(RenderCommandBuffer *commands, v2 position, v2 dimension, v4 color){
    RectCommand* command = allocate_command(commands, RectCommand);
    command->base.type = RenderCommand_Rect;
    command->base.color = color;
    command->base.position = position;
    command->dimension = dimension;
}

static void
allocate_box(RenderCommandBuffer *commands, v2 position, v2 dimension, v4 color){
    BoxCommand* command = allocate_command(commands, BoxCommand);
    command->base.type = RenderCommand_Box;
    command->base.color = color;
    command->base.position = position;
    command->dimension = dimension;
}

static void
allocate_quad(RenderCommandBuffer *commands, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
    QuadCommand* command = allocate_command(commands, QuadCommand);
    command->base.type = RenderCommand_Quad;
    command->base.color = color;
    command->base.fill = fill;
    command->p0 = p0;
    command->p1 = p1;
    command->p2 = p2;
    command->p3 = p3;
}

static void
allocate_triangle(RenderCommandBuffer *commands, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    TriangleCommand* command = allocate_command(commands, TriangleCommand);
    command->base.type = RenderCommand_Triangle;
    command->base.color = color;
    command->base.fill = fill;
    command->p0 = p0;
    command->p1 = p1;
    command->p2 = p2;
}

static void
allocate_circle(RenderCommandBuffer *commands, v2 pos, u8 rad, v4 color, bool fill){
    CircleCommand* command = allocate_command(commands, CircleCommand);
    command->base.type = RenderCommand_Circle;
    command->base.position = pos;
    command->base.color = color;
    command->base.fill = fill;
    command->rad = rad;
}

static void
allocate_bitmap(RenderCommandBuffer *commands, v2 position, Bitmap image){
    BitmapCommand* command = allocate_command(commands, BitmapCommand);
    command->base.type = RenderCommand_Bitmap;
    command->base.position = position;
    command->image = image;
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

        // clamp is wrong here, was 1 n 0 now 0 n 1
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
#define RENDERER_H
#endif
