#if !defined(VECTORS_H)

#include <math.h>

typedef union vec2{
    f32 e[2];
    struct{
        f32 x;
        f32 y;
    };
} vec2;

typedef union vec3{
    f32 e[3];
    struct{
        f32 x, y, z;
    };
    struct{
        f32 r, g, b;
    };
} vec3;

typedef union vec4{
    f32 e[4];
    struct{
        f32 x, y, z, w;
    };
    struct{
        f32 r, g, b, a;
    };
} vec4;

static vec2
Vec2(f32 x, f32 y){
    vec2 result = {x, y};
    return(result);
}

static vec3
Vec3(f32 x, f32 y, f32 z){
    vec3 result = {x, y, z};
    return(result);
}

static vec4
Vec4(f32 x, f32 y, f32 z, f32 w){
    vec4 result = {x, y, z, w};
    return(result);
}

static void
swapvec2(vec2 *a, vec2 *b){
    vec2 t = *a;
    *a = *b;
    *b = t;
}

static vec2
add2(vec2 a, vec2 b){
    vec2 result = {0};
    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return(result);
}

static vec2
sub2(vec2 a, vec2 b){
    vec2 result = {0};
    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return(result);
}

static vec2
mul2(vec2 a, vec2 b){
    vec2 result = {0};
    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return(result);
}

static vec2
scale2(vec2 a, f32 s){
    vec2 result = {0};
    result.x = a.x * s;
    result.y = a.y * s;

    return(result);
}

static bool
cmp2(vec2 a, vec2 b){
    bool result = false;
    if(a.x == b.x && a.y == b.y){
        result = true;
    }
    return(result);
}

static f32
dot2(vec2 a, vec2 b){
    return((a.x * b.x) + (a.y * b.y));
}

static f32
magnitude2(vec2 a){
    return(sqrtf((a.x * a.x) + (a.y * a.y)));
}

static f32
magnitude_sq2(vec2 a){
    return((a.x * a.x) + (a.y * a.y));
}

static f32
distance2(vec2 a, vec2 b){
    vec2 result = sub2(a, b);
    return(magnitude2(result));
}

static void
normalize2(vec2 *a){
    f32 mag = magnitude2(*a);
    a->x = (a->x / mag);
    a->y = (a->y / mag);
}

static vec2
normalized2(vec2 a){
    vec2 result = {0};

    f32 mag = magnitude2(a);
    result.x = (a.x / mag);
    result.y = (a.y / mag);

    return(result);
}

static f32
angle2(vec2 a, vec2 b){
    f32 result = 0;

    f32 mag = sqrtf(magnitude_sq2(a) * magnitude_sq2(b));
    result = acos(dot2(a, b) / mag);

    return(result);
}

static vec2
project2(vec2 a, vec2 b){
    vec2 result = {0};

    f32 n = dot2(a, b);
    f32 d = magnitude_sq2(a);
    result = scale2(b, (n/d));

    return(result);
}

static vec2
perpendicular2(vec2 a, vec2 b){
    vec2 result = {0};

    result = sub2(a, project2(a, b));
    return(result);
}

static vec2
reflection2(vec2 vec, vec2 normal){
    vec2 result = {0};

    f32 d = dot2(vec, normal);
    result.x = vec.x - normal.x * (d * 2.0f);
    result.y = vec.y - normal.y * (d * 2.0f);
    
    return(result);
}

#define VECTORS_H
#endif
