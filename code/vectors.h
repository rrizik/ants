#if !defined(VECTORS_H)

#include <math.h>

typedef union v2{
    f32 e[2];
    struct{
        f32 x; f32 y;
    };
    struct{
        f32 w; f32 h;
    };
} v2;

typedef union v3{
    f32 e[3];
    struct{
        f32 x, y, z;
    };
    struct{
        f32 r, g, b;
    };
} v3;

typedef union v4{
    f32 e[4];
    struct{
        f32 x, y, w, h;
    };
    struct{
        f32 r, g, b, a;
    };
    struct{
        f32 top, bottom, right, left;
    };
} v4;

static v2
vec2(f32 x, f32 y){
    v2 result = {x, y};
    return(result);
}

static v3
vec3(f32 x, f32 y, f32 z){
    v3 result = {x, y, z};
    return(result);
}

static v4
vec4(f32 x, f32 y, f32 w, f32 h){
    v4 result = {x, y, w, h};
    return(result);
}

static void
swap_v2(v2 *a, v2 *b){
    v2 t = *a;
    *a = *b;
    *b = t;
}

static v2
add2(v2 a, v2 b){
    v2 result = {0};
    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return(result);
}

static v2
sub2(v2 a, v2 b){
    v2 result = {0};
    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return(result);
}

static v2
mul2(v2 a, v2 b){
    v2 result = {0};
    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return(result);
}

static v2
scale2(v2 a, f32 s){
    v2 result = {0};
    result.x = a.x * s;
    result.y = a.y * s;

    return(result);
}

// INCOMPLETE: LOOK AT SIZE_T VS INT DIFFERENCES
static void
scale_pts(v2 *p, size_t count, f32 s){
    for(int i=0; i < (int)count; ++i){
        *p = scale2(*p, s);
        p++;
    }
}

static bool
cmp2(v2 a, v2 b){
    bool result = false;
    if(a.x == b.x && a.y == b.y){
        result = true;
    }
    return(result);
}

static f32
dot2(v2 a, v2 b){
    return((a.x * b.x) + (a.y * b.y));
}

static v2
direction2(v2 a, v2 b){
    v2 result = {0};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return(result);
}

static f32
magnitude2(v2 a){
    return(sqrtf((a.x * a.x) + (a.y * a.y)));
}

static f32
magnitude_sq2(v2 a){
    return((a.x * a.x) + (a.y * a.y));
}

static f32
distance2(v2 a, v2 b){
    v2 result = sub2(a, b);
    return(magnitude2(result));
}

static void
normalize2(v2 *a){
    f32 mag = magnitude2(*a);
    a->x = (a->x / mag);
    a->y = (a->y / mag);
}

static v2
get_normalized2(v2 a){
    v2 result = {0};

    f32 mag = magnitude2(a);
    result.x = (a.x / mag);
    result.y = (a.y / mag);

    return(result);
}

static f32
angle2(v2 a, v2 b){
    f32 result = 0;

    f32 mag = sqrtf(magnitude_sq2(a) * magnitude_sq2(b));
    result = (f32)acos(dot2(a, b) / mag);

    return(result);
}

static v2
project2(v2 a, v2 b){
    v2 result = {0};

    f32 n = dot2(a, b);
    f32 d = magnitude_sq2(a);
    result = scale2(b, (n/d));

    return(result);
}

static v2
perpendicular2(v2 a, v2 b){
    v2 result = {0};

    result = sub2(a, project2(a, b));
    return(result);
}

static v2
reflection2(v2 vec, v2 normal){
    v2 result = {0};

    f32 d = dot2(vec, normal);
    result.x = vec.x - normal.x * (d * 2.0f);
    result.y = vec.y - normal.y * (d * 2.0f);
    
    return(result);
}

static bool
equal4(v4 a, v4 b){
    if(a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h){
        return true;
    }
    return false;
}

#define VECTORS_H
#endif
