
typedef struct Arena {
    void *base;
    size_t size;
    size_t used;
} Arena;

#define allocate_array(arena, count, type) (type *)allocate_size_(arena, count * sizeof(type));
#define allocate_struct(arena, type) (type *)allocate_size_(arena, sizeof(type));
#define allocate_size(arena, size) allocate_size_(arena, size);
static void*
allocate_size_(Arena *arena, size_t size){
    assert((arena->used + size) < arena->size);
    void *result = (u8*)arena->base + arena->used;
    arena->used = arena->used + size;
    return(result);
}

static Arena*
allocate_arena(Arena *arena, size_t size){
    Arena* result = allocate_struct(arena, Arena);
    result->base = allocate_size(arena, size);
    result->size = size;
    return(result);
}

static void
initialize_arena(Arena *arena, void *base, size_t size){
    arena->base = base;
    arena->size = size;
    arena->used = 0;
}
