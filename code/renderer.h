#if !defined(RENDERER_H)

#include <emmintrin.h>

#pragma pack(push, 1)
typedef struct BitmapHeader {
    u16 file_type;
	u32 file_size;
	u16 reserved1;
	u16 reserved2;
	u32 bitmap_offset;
	u32 header_size;
	s32 width;
	s32 height;
	u16 planes;
	u16 bits_per_pixel;
    u32 Compression;
	u32 SizeOfBitmap;
	s32 HorzResolution;
	s32 VertResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;
//    u32 RedMask;
//    u32 GreenMask;
//    u32 BlueMask;
} BitmapHeader;
#pragma pack(pop)

typedef struct Bitmap{
    BitmapHeader* header; //maybe i dont want this here?
	u32  width;
	u32  height;
    u32 *pixels;
} Bitmap;

// UNTESTED: test this again, changed how os_file_read() works
static Bitmap
load_bitmap(Arena *arena, String8 dir, String8 file_name){
    Bitmap result = {0};

    FileData bitmap_file = os_file_read(arena, dir, file_name);
    if(bitmap_file.size > 0){
        BitmapHeader *header = (BitmapHeader *)bitmap_file.base;
        result.pixels = (u32 *)((u8 *)bitmap_file.base + header->bitmap_offset);
        result.width = header->width;
        result.height = header->height;
        result.header = header;
        // IMPORTANT: depending on compression type you might have to re order the bytes to make it AA RR GG BB
        // as well as consider the masks for each color included in the header
    }
    return(result);
}

static v4
u32_to_rgba(u32 value){
	f32 alpha = ((f32)((value >> 24) & 0xFF) / 255.0f);
	f32 red =   ((f32)((value >> 16) & 0xFF) / 255.0f);
	f32 green = ((f32)((value >> 8) & 0xFF) / 255.0f);
	f32 blue =  ((f32)((value >> 0) & 0xFF) / 255.0f);
    v4 result = {red, green, blue, alpha};
    return result;
}

typedef struct Rect{
    v2 pos;
    v2s32 dim;
    f32 x; 
    f32 y;
    f32 w; 
    f32 h;
    f32 x1;
    f32 y1;
} Rect;

static Rect
rect(v2 pos, v2s32 dim){
    Rect result = {0};
    result.pos = pos;
    result.dim = dim;
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

typedef struct CommandHeader{
    RenderCommandType type;
    v2 position;
    v4 color;
    v4 clip_region;
    bool fill;
    size_t arena_used;
} CommandHeader;

typedef struct ClearColorCommand{
    CommandHeader header;
} ClearColorCommand;

typedef struct PixelCommand{
    CommandHeader header;
} PixelCommand;

typedef struct SegmentCommand{
    CommandHeader header;
    v2 p0;
    v2 p1;
} SegmentCommand;

typedef struct RayCommand{
    CommandHeader header;
    v2 direction;
} RayCommand;

typedef struct LineCommand{
    CommandHeader header;
    v2 direction;
} LineCommand;

typedef struct RectCommand{
    CommandHeader header;
    v2s32 dimension;
} RectCommand;

typedef struct BoxCommand{
    CommandHeader header;
    v2s32 dimension;
} BoxCommand;

typedef struct QuadCommand{
    CommandHeader header;
    v2 p0;
    v2 p1;
    v2 p2;
    v2 p3;
} QuadCommand;

typedef struct TriangleCommand{
    CommandHeader header;
    v2 p0;
    v2 p1;
    v2 p2;
} TriangleCommand;

typedef struct CircleCommand{
    CommandHeader header;
    u8 rad;
} CircleCommand;

typedef struct BitmapCommand{
    CommandHeader header;
    Bitmap image;
} BitmapCommand;

static void
push_clear_color(Arena *arena, v4 color){
    ClearColorCommand* command = push_struct(arena, ClearColorCommand);
    command->header.type = RenderCommand_ClearColor,
    command->header.color = color;
    command->header.arena_used = arena->used;
}

static void
push_pixel(Arena *arena, v2 position, v4 color){
    PixelCommand* command = push_struct(arena, PixelCommand);
    command->header.type = RenderCommand_Pixel;
    command->header.arena_used = arena->used;
    command->header.position = position;
    command->header.color = color;
}

static void
push_segment(Arena *arena, v2 p0, v2 p1, v4 color){
    SegmentCommand* command = push_struct(arena, SegmentCommand);
    command->header.type = RenderCommand_Segment;
    command->header.arena_used = arena->used;
    command->header.color = color;
    command->p0 = p0;
    command->p1 = p1;
}

static void
push_ray(Arena *arena, v2 position, v2 direction, v4 color){
    RayCommand* command = push_struct(arena, RayCommand);
    command->header.type = RenderCommand_Ray;
    command->header.arena_used = arena->used;
    command->header.position = position;
    command->header.color = color;
    command->direction = direction;
}

static void
push_line(Arena *arena, v2 position, v2 direction, v4 color){
    LineCommand* command = push_struct(arena, LineCommand);
    command->header.type = RenderCommand_Line;
    command->header.arena_used = arena->used;
    command->header.position = position;
    command->header.color = color;
    command->direction = direction;
}

static void
push_rect(Arena *arena, v2 position, v2s32 dimension, v4 color){
    RectCommand* command = push_struct(arena, RectCommand);
    command->header.type = RenderCommand_Rect;
    command->header.arena_used = arena->used;
    command->header.color = color;
    command->header.position = position;
    command->dimension = dimension;
}

static void
push_box(Arena *arena, v2 position, v2s32 dimension, v4 color){
    BoxCommand* command = push_struct(arena, BoxCommand);
    command->header.type = RenderCommand_Box;
    command->header.arena_used = arena->used;
    command->header.color = color;
    command->header.position = position;
    command->dimension = dimension;
}

static void
push_quad(Arena *arena, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
    QuadCommand* command = push_struct(arena, QuadCommand);
    command->header.type = RenderCommand_Quad;
    command->header.arena_used = arena->used;
    command->header.color = color;
    command->header.fill = fill;
    command->p0 = p0;
    command->p1 = p1;
    command->p2 = p2;
    command->p3 = p3;
}

static void
push_triangle(Arena *arena, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    TriangleCommand* command = push_struct(arena, TriangleCommand);
    command->header.type = RenderCommand_Triangle;
    command->header.arena_used = arena->used;
    command->header.color = color;
    command->header.fill = fill;
    command->p0 = p0;
    command->p1 = p1;
    command->p2 = p2;
}

static void
push_circle(Arena *arena, v2 pos, u8 rad, v4 color, bool fill){
    CircleCommand* command = push_struct(arena, CircleCommand);
    command->header.type = RenderCommand_Circle;
    command->header.arena_used = arena->used;
    command->header.position = pos;
    command->header.color = color;
    command->header.fill = fill;
    command->rad = rad;
}

static void
push_bitmap(Arena *arena, v2 position, Bitmap image){
    BitmapCommand* command = push_struct(arena, BitmapCommand);
    command->header.type = RenderCommand_Bitmap;
    command->header.arena_used = arena->used;
    command->header.position = position;
    command->image = image;
}

static void
draw_pixel(RenderBuffer *buffer, v2 position, v4 color){
    v2s32 pos = round_v2_v2s32(position);

    if(pos.x >= 0 && pos.x < buffer->width && pos.y >= 0 && pos.y < buffer->height){
        u8 *row = (u8 *)buffer->base +
                   ((buffer->height - pos.y - 1) * buffer->stride) +
                   (pos.x * buffer->bytes_per_pixel);
        u32 *pixel = (u32 *)row;

        if(color.a == 1){
            u32 new_color = (round_f32_s32(color.a * 255.0f) << 24 | round_f32_s32(color.r*255.0f) << 16 | round_f32_s32(color.g*255.0f) << 8 | round_f32_s32(color.b*255.0f) << 0);
            *pixel = new_color;
        }
        else if(color.a > 0){
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

            u32 new_color = (round_f32_s32(color.a * 255.0f) << 24 | round_f32_s32(new_r) << 16 | round_f32_s32(new_g) << 8 | round_f32_s32(new_b) << 0);
            *pixel = new_color;
        }
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
        draw_pixel(buffer, point1, c); // NOTE: before break, so you can draw a single point draw_segment (a pixel)
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
        draw_pixel(buffer, point2, c);
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
        draw_pixel(buffer, position, c); // NOTE: before break, so you can draw a single position draw_segment (a pixel)
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
        draw_pixel(buffer, p0, c); // NOTE: before break, so you can draw a single point draw_segment (a pixel)

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

    //s32 start_y = (int)round(p2.y);
    //s32 end_y = (int)round(p0.y);
    s32 start_y = (s32)round_f32_s32(p2.y);
    s32 end_y = (s32)round_f32_s32(p0.y);

    for(s32 y=start_y; y < end_y; ++y){
        f32 x0 = left_slope * (y + 0.5f - p0.y) + p0.x;
        f32 x1 = right_slope * (y + 0.5f - p1.y) + p1.x;
        s32 start_x = (s32)ceil(x0 - 0.5f);
        s32 end_x = (s32)ceil(x1 - 0.5f);
        for(s32 x=start_x; x < end_x; ++x){
            draw_pixel(buffer, (v2){(f32)x, (f32)y}, c);
        }
    }
}

static void
draw_flatbottom_triangle(RenderBuffer *buffer, v2 p0, v2 p1, v2 p2, v4 c){
    f32 left_slope = (p1.x - p0.x) / (p1.y - p0.y);
    f32 right_slope = (p2.x - p0.x) / (p2.y - p0.y);

    //s32 start_y = (int)round(p0.y);
    //s32 end_y = (int)round(p1.y);
    s32 start_y = (s32)round_f32_s32(p0.y);
    s32 end_y = (s32)round_f32_s32(p1.y);

    for(s32 y=start_y; y >= end_y; --y){
        f32 x0 = left_slope * (y + 0.5f - p0.y) + p0.x;
        f32 x1 = right_slope * (y + 0.5f - p0.y) + p0.x;
        s32 start_x = (s32)ceil(x0 - 0.5f);
        s32 end_x = (s32)ceil(x1 - 0.5f);
        for(s32 x=start_x; x < end_x; ++x){
            draw_pixel(buffer, (v2){(f32)x, (f32)y}, c);
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
    for(s32 i=0; i < (buffer->width * buffer->height); ++i){
        u32 *pixel = (u32 *)((u8 *)(buffer->base) + (i * buffer->bytes_per_pixel));
        u32 new_color = (round_f32_s32(c.a * 255.0f) << 24 | round_f32_s32(c.r*255.0f) << 16 | round_f32_s32(c.g*255.0f) << 8 | round_f32_s32(c.b*255.0f) << 0);
        *pixel = new_color;
    }
}

static void
draw_bitmap_clip(RenderBuffer *buffer, v2 position, Bitmap image, v4 clip_region){
    //v4 cr = {100, 300, 200, 200};
    Rect cr = rect((v2){100, 300}, (v2s32){200, 200});
    if(clip_region == vec4(0,0,0,0)){
        f32 rounded_x = round_f32(position.x);
        f32 rounded_y = round_f32(position.y);
        for(f32 y=rounded_y; y < rounded_y + image.height; ++y){
            for(f32 x=rounded_x; x < rounded_x + image.width; ++x){
                v4 color = u32_to_rgba(*image.pixels++);
                draw_pixel(buffer, (v2){x, y}, color);
            }
        }
    }
    else{
        //v4 result = {position.x, position.y, image.width, image.height};
        Rect result = rect(position, vec2s32(image.width, image.height));

        if(position.x < cr.pos.x){
            result.pos.x = cr.pos.x;
            result.dim.w = image.width - (cr.pos.x - position.x);
        }
        if((result.pos.x + result.dim.w) > (cr.pos.x + cr.dim.w)){
            result.dim.w = result.dim.w - ((result.pos.x + result.dim.w) - (cr.pos.x + cr.dim.w));
        }

        if(position.y < cr.pos.y){
            result.pos.y = cr.pos.y;
            result.dim.h = image.height - (cr.pos.y - position.y);
        }
        if((result.pos.y + result.dim.h) > (cr.pos.y + cr.dim.h)){
            result.dim.h = result.dim.h - ((result.pos.y + result.dim.h) - (cr.pos.y + cr.dim.h));
        }

        u32 rounded_x = round_f32_u32(result.pos.x);
        u32 rounded_y = round_f32_u32(result.pos.y);

        // clamp is wrong here, was 1 n 0 now 0 n 1
        u32 x_shift = (u32)clamp_f32(0, (cr.pos.x - position.x), 100000);
        u32 y_shift = (u32)clamp_f32(0, (cr.pos.y - position.y), 100000);

        u32 iy = 0;
        for(u32 y = rounded_y; y < rounded_y + result.dim.h; ++y){
            u32 ix = 0;
            for(u32 x = rounded_x; x < rounded_x + result.dim.w; ++x){
                u8 *byte = (u8 *)image.pixels + ((y_shift + iy) * image.width * 4) + ((x_shift + ix) * 4);
                u32 *c = (u32 *)byte;
                v4 color = u32_to_rgba(*c);
                draw_pixel(buffer, (v2){(f32)x, (f32)y}, color);
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
draw_rect_slow(RenderBuffer *buffer, v2 position, v2s32 dimension, v4 c){
    v2 p0 = {position.x, position.y};
    v2 p1 = {position.x + dimension.w, position.y};
    v2 p2 = {position.x, position.y + dimension.h};
    v2 p3 = {position.x + dimension.w, position.y + dimension.h};

    BEGIN_CYCLE_COUNTER(draw_rect_slow)
    for(f32 y=p0.y; y <= p2.y; ++y){
        for(f32 x=p0.x; x <= p1.x; ++x){
            draw_pixel(buffer, (v2){x, y}, c);
        }
    }
    END_CYCLE_COUNTER(draw_rect_slow)
}

static void 
draw_rect_fast_no_blend(RenderBuffer *buffer, v2 position, v2s32 dimension, v4 color){
    v2s32 bottom_left = round_v2_v2s32(position);
    v2s32 bottom_right = {bottom_left.x + dimension.w, bottom_left.y};
    v2s32 top_left = {bottom_left.x, bottom_left.y + dimension.h};

    __m128 color_255_4x = _mm_set_ps1(255.0f);
    __m128 half_increment = _mm_set_ps1(0.5f);

    __m128 color_a_4x = _mm_set_ps1(color.a);
    __m128 color_r_4x = _mm_set_ps1(color.r);
    __m128 color_g_4x = _mm_set_ps1(color.g);
    __m128 color_b_4x = _mm_set_ps1(color.b);

    u8 *row = (u8 *)buffer->base + 
              ((buffer->height - 1 - bottom_left.y) * buffer->stride) + 
              (bottom_left.x * buffer->bytes_per_pixel);

#define M(a, i) ((f32*)&(a))[i]
#define MM(a, i) ((u32*)&(a))[i]

    for(s32 y=bottom_left.y; y < top_left.y; ++y){
        u32 *pixel = (u32 *)row;
        for(s32 x=bottom_left.x; x < bottom_right.x; x+=4){
            __m128 scaled_a_4x = _mm_mul_ps(color_a_4x, color_255_4x);
            __m128 scaled_r_4x = _mm_mul_ps(color_r_4x, color_255_4x);
            __m128 scaled_g_4x = _mm_mul_ps(color_g_4x, color_255_4x);
            __m128 scaled_b_4x = _mm_mul_ps(color_b_4x, color_255_4x);

            __m128 incremented_a_4x = _mm_min_ps(_mm_add_ps(scaled_a_4x, half_increment), color_255_4x);
            __m128 incremented_r_4x = _mm_min_ps(_mm_add_ps(scaled_r_4x, half_increment), color_255_4x);
            __m128 incremented_g_4x = _mm_min_ps(_mm_add_ps(scaled_g_4x, half_increment), color_255_4x);
            __m128 incremented_b_4x = _mm_min_ps(_mm_add_ps(scaled_b_4x, half_increment), color_255_4x);

            __m128i rounded_a_4x = _mm_cvtps_epi32(incremented_a_4x);
            __m128i rounded_r_4x = _mm_cvtps_epi32(incremented_r_4x);
            __m128i rounded_g_4x = _mm_cvtps_epi32(incremented_g_4x);
            __m128i rounded_b_4x = _mm_cvtps_epi32(incremented_b_4x);

            for(s32 i=0; i < 4; ++i){

                //*(pixel + i) = (((u32)(M(incremented_a_4x, i) + 0.5f) << 24) |
                //                ((u32)(M(incremented_r_4x, i) + 0.5f) << 16) |
                //                ((u32)(M(incremented_g_4x, i) + 0.5f) << 8) |
                //                ((u32)(M(incremented_b_4x, i) + 0.5f) << 0));

                *(pixel + i) = (((u32)(MM(rounded_a_4x, i)) << 24) |
                                ((u32)(MM(rounded_r_4x, i)) << 16) |
                                ((u32)(MM(rounded_g_4x, i)) << 8) |
                                ((u32)(MM(rounded_b_4x, i)) << 0));
            }
            pixel += 4;
        }
        row -= buffer->stride;
    }
}

static void 
draw_rect_fast(RenderBuffer *buffer, v2 position, v2s32 dimension, v4 color){
#define MF(a, i) ((f32*)&(a))[i]
#define MI(a, i) ((u32*)&(a))[i]

    v2s32 bottom_left = round_v2_v2s32(position);

    s32 min_x = bottom_left.x;
    s32 max_x = bottom_left.x + dimension.w;
    s32 min_y = bottom_left.y;
    s32 max_y = bottom_left.y + dimension.h;

    if(min_x < 0) { min_x = 0; }
    if(min_y < 0) { min_y = 0; }
    if(max_x > buffer->width - 1) { min_x = buffer->width - 1; }
    if(max_y > buffer->height - 1) { min_y = buffer->height - 1; }

    v2s32 bottom_right = {bottom_left.x + dimension.w, bottom_left.y};
    v2s32 top_left = {bottom_left.x, bottom_left.y + dimension.h};

    __m128 color_255_4x = _mm_set_ps1(255.0f);
    __m128 color_one_4x = _mm_set_ps1(1.0f);
    __m128 color_zero_4x = _mm_set_ps1(0.0f);

    __m128 color_a_4x = _mm_set_ps1(color.a);
    __m128 color_r_4x = _mm_set_ps1(color.r);
    __m128 color_g_4x = _mm_set_ps1(color.g);
    __m128 color_b_4x = _mm_set_ps1(color.b);

    //u8 *row = (u8 *)buffer->base + 
    //          ((buffer->height - 1 - bottom_left.y) * buffer->stride) + 
    //          (bottom_left.x * buffer->bytes_per_pixel);
    u8 *row = (u8 *)buffer->base + 
              ((buffer->height - 1 - min_y) * buffer->stride) + 
              (min_x * buffer->bytes_per_pixel);

    BEGIN_CYCLE_COUNTER(draw_rect_fast)
    //for(s32 y=bottom_left.y; y < top_left.y; ++y){
    for(s32 y=min_y; y < max_y; ++y){
        u32 *pixel = (u32 *)row;
        //for(s32 x=bottom_left.x; x < bottom_right.x; x+=4){
        for(s32 x=min_x; x < max_x; x+=4){
            __m128 current_a_4x = _mm_set_ps1(0.0f);
            __m128 current_r_4x = _mm_set_ps1(0.0f);
            __m128 current_g_4x = _mm_set_ps1(0.0f);
            __m128 current_b_4x = _mm_set_ps1(0.0f);

            __m128 new_a_4x = _mm_set_ps1(0.0f);
            __m128 new_r_4x = _mm_set_ps1(0.0f);
            __m128 new_g_4x = _mm_set_ps1(0.0f);
            __m128 new_b_4x = _mm_set_ps1(0.0f);

            bool should_fill[4];
            __m128i loaded_current_4x = _mm_loadu_si128((__m128i * )pixel);
            __m128i write_mask = _mm_set1_epi32(0);

            for(s32 i=0; i < 4; ++i){
                should_fill[i] = (((x + i) >= 0) && 
                                  ((x + i) < buffer->width) && 
                                   (y >= 0) && 
                                   (y < buffer->height));

                if(should_fill[i]){
                    MF(current_a_4x, i) = ((f32)((*(pixel + i) >> 24) & 0xFF) / 255.0f);
                    MF(current_r_4x, i) =  (f32)((*(pixel + i) >> 16) & 0xFF);
                    MF(current_g_4x, i) =  (f32)((*(pixel + i) >> 8)  & 0xFF);
                    MF(current_b_4x, i) =  (f32)((*(pixel + i) >> 0)  & 0xFF);

                    MI(write_mask, i) = 0xFFFFFFFF;
                }
            }
            
            __m128 current_r_percent_4x = _mm_sub_ps(color_one_4x, color_a_4x);
            __m128 current_g_percent_4x = _mm_sub_ps(color_one_4x, color_a_4x);
            __m128 current_b_percent_4x = _mm_sub_ps(color_one_4x, color_a_4x);

            __m128 new_current_r_4x = _mm_mul_ps(current_r_percent_4x, current_r_4x);
            __m128 new_current_g_4x = _mm_mul_ps(current_g_percent_4x, current_g_4x);
            __m128 new_current_b_4x = _mm_mul_ps(current_b_percent_4x, current_b_4x);

            __m128 scaled_color_r_4x = _mm_mul_ps(color_r_4x, color_255_4x);
            __m128 scaled_color_g_4x = _mm_mul_ps(color_g_4x, color_255_4x);
            __m128 scaled_color_b_4x = _mm_mul_ps(color_b_4x, color_255_4x);

            __m128 new_color_r_4x = (color_a_4x * scaled_color_r_4x);
            __m128 new_color_g_4x = (color_a_4x * scaled_color_g_4x);
            __m128 new_color_b_4x = (color_a_4x * scaled_color_b_4x);

            new_r_4x = _mm_add_ps(new_current_r_4x, new_color_r_4x);
            new_g_4x = _mm_add_ps(new_current_g_4x, new_color_g_4x);
            new_b_4x = _mm_add_ps(new_current_b_4x, new_color_b_4x);
            new_a_4x = _mm_mul_ps(color_a_4x, color_255_4x);

            new_r_4x = _mm_min_ps(_mm_max_ps(new_r_4x, color_zero_4x), color_255_4x);
            new_g_4x = _mm_min_ps(_mm_max_ps(new_g_4x, color_zero_4x), color_255_4x);
            new_b_4x = _mm_min_ps(_mm_max_ps(new_b_4x, color_zero_4x), color_255_4x);
            new_a_4x = _mm_min_ps(_mm_max_ps(color_a_4x, color_255_4x), color_255_4x);

            __m128i int_a_4x = _mm_cvtps_epi32(new_a_4x);
            __m128i int_r_4x = _mm_cvtps_epi32(new_r_4x);
            __m128i int_g_4x = _mm_cvtps_epi32(new_g_4x);
            __m128i int_b_4x = _mm_cvtps_epi32(new_b_4x);

            __m128i shifted_int_a_4x = _mm_slli_epi32(int_a_4x, 24);
            __m128i shifted_int_r_4x = _mm_slli_epi32(int_r_4x, 16);
            __m128i shifted_int_g_4x = _mm_slli_epi32(int_g_4x, 8);
            __m128i shifted_int_b_4x = int_b_4x;

            __m128i out = _mm_or_si128(shifted_int_a_4x,
                          _mm_or_si128(shifted_int_r_4x,
                          _mm_or_si128(shifted_int_g_4x, 
                                       shifted_int_b_4x)));
            
            __m128i masked_out = _mm_or_si128(_mm_and_si128(write_mask, out),
                                              _mm_andnot_si128(write_mask, loaded_current_4x));
            
            _mm_storeu_si128((__m128i * )pixel, masked_out);

            //for(s32 i=0; i < 4; ++i){
            //    if(should_fill[i]){
            //        *(pixel + i) = ((u32)MI(int_a_4x, i) << 24 |
            //                        (u32)MI(int_r_4x, i) << 16 |
            //                        (u32)MI(int_g_4x, i) << 8  |
            //                        (u32)MI(int_b_4x, i) << 0  );
            //    }
            //}
            pixel += 4;
        }
        row -= buffer->stride;
    }
    END_CYCLE_COUNTER(draw_rect_fast)
}

static void
draw_box(RenderBuffer *buffer, v2 position, v2s32 dimension, v4 c){
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
        draw_pixel(buffer, (v2){(xm - x), (ym + y)}, c);
        draw_pixel(buffer, (v2){(xm - y), (ym - x)}, c);
        draw_pixel(buffer, (v2){(xm + x), (ym - y)}, c);
        draw_pixel(buffer, (v2){(xm + y), (ym + x)}, c);
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
