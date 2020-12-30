#if !defined(VECTORS_H)

#include <math.h>

typedef union Vec2{
    f32 e[2];
    struct{
        f32 x; f32 y;
    };
    struct{
        f32 p0, p1;
    };
    struct{
        f32 w; f32 h;
    };
} Vec2;

typedef union Vec3{
    f32 e[3];
    struct{
        f32 x, y, z;
    };
    struct{
        f32 p0, p1, p2;
    };
    struct{
        f32 r, g, b;
    };
} Vec3;

typedef union Vec4{
    f32 e[4];
    struct{
        f32 x, y, z, w;
    };
    struct{
        f32 p0, p1, p2, p3;
    };
    struct{
        f32 r, g, b, a;
    };
} Vec4;

static Vec2
vec2(f32 x, f32 y){
    Vec2 result = {x, y};
    return(result);
}

static Vec3
vec3(f32 x, f32 y, f32 z){
    Vec3 result = {x, y, z};
    return(result);
}

static Vec4
vec4(f32 x, f32 y, f32 z, f32 w){
    Vec4 result = {x, y, z, w};
    return(result);
}

static void
swap_v2(Vec2 *a, Vec2 *b){
    Vec2 t = *a;
    *a = *b;
    *b = t;
}

static Vec2
add2(Vec2 a, Vec2 b){
    Vec2 result = {0};
    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return(result);
}

static Vec2
sub2(Vec2 a, Vec2 b){
    Vec2 result = {0};
    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return(result);
}

static Vec2
mul2(Vec2 a, Vec2 b){
    Vec2 result = {0};
    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return(result);
}

static Vec2
scale2(Vec2 a, f32 s){
    Vec2 result = {0};
    result.x = a.x * s;
    result.y = a.y * s;

    return(result);
}

static void
scale_pts(Vec2 *p, size_t count, f32 s){
    for(int i=0; i < count; ++i){
        *p++ = scale2(*p, s);
    }
}

static bool
cmp2(Vec2 a, Vec2 b){
    bool result = false;
    if(a.x == b.x && a.y == b.y){
        result = true;
    }
    return(result);
}

static f32
dot2(Vec2 a, Vec2 b){
    return((a.x * b.x) + (a.y * b.y));
}

static f32
magnitude2(Vec2 a){
    return(sqrtf((a.x * a.x) + (a.y * a.y)));
}

static f32
magnitude_sq2(Vec2 a){
    return((a.x * a.x) + (a.y * a.y));
}

static f32
distance2(Vec2 a, Vec2 b){
    Vec2 result = sub2(a, b);
    return(magnitude2(result));
}

static void
normalize2(Vec2 *a){
    f32 mag = magnitude2(*a);
    a->x = (a->x / mag);
    a->y = (a->y / mag);
}

static Vec2
normalized2(Vec2 a){
    Vec2 result = {0};

    f32 mag = magnitude2(a);
    result.x = (a.x / mag);
    result.y = (a.y / mag);

    return(result);
}

static f32
angle2(Vec2 a, Vec2 b){
    f32 result = 0;

    f32 mag = sqrtf(magnitude_sq2(a) * magnitude_sq2(b));
    result = (f32)acos(dot2(a, b) / mag);

    return(result);
}

static Vec2
project2(Vec2 a, Vec2 b){
    Vec2 result = {0};

    f32 n = dot2(a, b);
    f32 d = magnitude_sq2(a);
    result = scale2(b, (n/d));

    return(result);
}

static Vec2
perpendicular2(Vec2 a, Vec2 b){
    Vec2 result = {0};

    result = sub2(a, project2(a, b));
    return(result);
}

static Vec2
reflection2(Vec2 vec, Vec2 normal){
    Vec2 result = {0};

    f32 d = dot2(vec, normal);
    result.x = vec.x - normal.x * (d * 2.0f);
    result.y = vec.y - normal.y * (d * 2.0f);
    
    return(result);
}

#define VECTORS_H
#endif
