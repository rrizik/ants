#if !defined(RENDERER_H)

typedef struct Rect{
    f32 x, y, w, h, width, height;
} Rect;

static Rect
rect(v2 pos, v2 dim){
    Rect result = {0};
    result.x = pos.x;
    result.y = pos.y;
    result.w = dim.x;
    result.h = dim.y;
    result.width = result.w;
    result.height = result.h;
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
rect_collide_point(Rect r1, v2 p){
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
    ui8 rad;
} CircleCommand;

typedef struct BitmapCommand{
    BaseCommand base; 
    Bitmap image;
} BitmapCommand;

typedef struct RenderCommands{
    size_t max_bytes;
    size_t used_bytes;
    void* buffer;
} RenderCommands;

static RenderCommands* 
allocate_render_commands(MemoryArena *arena, size_t max_bytes){
    RenderCommands* result = push_struct(arena, RenderCommands);
    result->max_bytes = max_bytes;
    result->buffer = push_size(arena, max_bytes);
    return(result);
}

#define push_command(commands, type) (type*)push_command_(commands, sizeof(type))
static void*
push_command_(RenderCommands* commands, size_t size){
    void* command = (char*)commands->buffer + commands->used_bytes;
    commands->used_bytes += size;
    assert(commands->used_bytes < commands->max_bytes);
    return(command);
}

static void
push_clear_color(RenderCommands *commands, v4 color){
    ClearColorCommand* command = push_command(commands, ClearColorCommand);
    command->base.color = color;
}

static void
push_pixel(RenderCommands *commands, v2 position, v4 color){
    PixelCommand* command = push_command(commands, PixelCommand);
    command->base.type = RenderCommand_Pixel;
    command->base.position = position;
    command->base.color = color;
}

static void
push_segment(RenderCommands *commands, v2 p0, v2 p1, v4 color){
    SegmentCommand* command = push_command(commands, SegmentCommand);
    command->base.type = RenderCommand_Segment;
    command->base.color = color;
    command->p0 = p0;
    command->p1 = p1;
}

static void
push_ray(RenderCommands *commands, v2 position, v2 direction, v4 color){
    RayCommand* command = push_command(commands, RayCommand);
    command->base.type = RenderCommand_Ray;
    command->base.position = position;
    command->base.color = color;
    command->direction = direction;
}

static void
push_line(RenderCommands *commands, v2 position, v2 direction, v4 color){
    LineCommand* command = push_command(commands, LineCommand);
    command->base.type = RenderCommand_Line;
    command->base.position = position;
    command->base.color = color;
    command->direction = direction;
}

static void
push_rect(RenderCommands *commands, v2 position, v2 dimension, v4 color){
    RectCommand* command = push_command(commands, RectCommand);
    command->base.type = RenderCommand_Rect;
    command->base.color = color;
    command->base.position = position;
    command->dimension = dimension;
}

static void
push_box(RenderCommands *commands, v2 position, v2 dimension, v4 color){
    BoxCommand* command = push_command(commands, BoxCommand);
    command->base.type = RenderCommand_Box;
    command->base.color = color;
    command->base.position = position;
    command->dimension = dimension;
}

static void
push_quad(RenderCommands *commands, v2 p0, v2 p1, v2 p2, v2 p3, v4 color, bool fill){
    QuadCommand* command = push_command(commands, QuadCommand);
    command->base.type = RenderCommand_Quad;
    command->base.color = color;
    command->base.fill = fill;
    command->p0 = p0;
    command->p1 = p1;
    command->p2 = p2;
    command->p3 = p3;
}

static void
push_triangle(RenderCommands *commands, v2 p0, v2 p1, v2 p2, v4 color, bool fill){
    TriangleCommand* command = push_command(commands, TriangleCommand);
    command->base.type = RenderCommand_Triangle;
    command->base.color = color;
    command->base.fill = fill;
    command->p0 = p0;
    command->p1 = p1;
    command->p2 = p2;
}

static void
push_circle(RenderCommands *commands, v2 pos, ui8 rad, v4 color, bool fill){
    CircleCommand* command = push_command(commands, CircleCommand);
    command->base.type = RenderCommand_Circle;
    command->base.position = pos;
    command->base.color = color;
    command->base.fill = fill;
    command->rad = rad;
}

static void
push_bitmap(RenderCommands *commands, v2 position, Bitmap image){
    BitmapCommand* command = push_command(commands, BitmapCommand);
    command->base.type = RenderCommand_Bitmap;
    command->base.position = position;
    command->image = image;
}

#define RENDERER_H
#endif
