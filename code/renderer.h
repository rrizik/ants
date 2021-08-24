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

#define RENDERER_H
#endif
