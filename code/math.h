#if !defined(MATH_H)
#define MATH_H
// Eventually get rid of this file and everything in it


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
        *p = *p + translation;
        p++;
    }
    p -= count;
}

static void
scale(v2 *p, u32 count, f32 scalar, v2 origin){
    for(u32 i=0; i<count; ++i){
        *p = ((*p - origin) * scalar) + origin;
        p++;
    }
    p -= count;
}

static v2
rotate_point(v2 p, f32 angle, v2 origin){
    v2 result = {0};
    result.x = (p.x - origin.x) * cos(angle * 3.14f/180.0f) - (p.y - origin.y) * sin(angle * 3.14f/180.0f) + origin.x;
    result.y = (p.x - origin.x) * sin(angle * 3.14f/180.0f) + (p.y - origin.y) * cos(angle * 3.14f/180.0f) + origin.y;
    return(result);
}

static void
rotate_points(v2 *p, u32 count, f32 angle, v2 origin){
    for(u32 i=0; i<count; ++i){
        v2 result = {0};
        result = rotate_point(*p, angle, origin);
        p->x = result.x;
        p->y = result.y;
        p++;
    }
    p -= count;
}

static void
swap_v2(v2 *a, v2 *b){
    v2 t = *a;
    *a = *b;
    *b = t;
}


// INCOMPLETE: LOOK AT SIZE_T VS INT DIFFERENCES
static void
scale_pts(v2 *p, size_t count, f32 s){
    for(i32 i=0; i < (int)count; ++i){
        *p = (*p * s);
        p++;
    }
}

static f32
dot2(v2 a, v2 b){
    return((a.x * b.x) + (a.y * b.y));
}

static f32
magnitude2(v2 a){
    return(sqrtf((a.x * a.x) + (a.y * a.y)));
}

static f32
magnitude_sq2(v2 a){
    return((a.x * a.x) + (a.y * a.y));
}

static v2
normalize2(v2 a){
    v2 result = {0};

    f32 mag = magnitude2(a);
    result.x = (a.x / mag);
    result.y = (a.y / mag);

    return(result);
}

static v2
direction2(v2 a, v2 b){
    v2 result = {0};
    result.x = b.x - a.x;
    result.y = b.y - a.y;
    normalize2(result);
    return(result);
}

static f32
distance2(v2 a, v2 b){
    v2 result = a - b;
    return(magnitude2(result));
}

static f32
angle2(v2 a, v2 b){
    f32 result = 0;

    f32 mag = sqrtf(magnitude_sq2(a) * magnitude_sq2(b));
    result = (f32)acos(dot2(a, b) / mag);

    return(result);
}

static f32
full_angle2(v2 dir){
    return(atan2(dir.y, dir.x));
}

static v2
project2(v2 a, v2 b){
    v2 result = {0};

    f32 n = dot2(a, b);
    f32 d = magnitude_sq2(a);
    result = (b * (n/d));

    return(result);
}

static v2
perpendicular_v2(v2 a, v2 b){
    v2 result = {0};

    result = (a - project2(a, b));
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

#endif
