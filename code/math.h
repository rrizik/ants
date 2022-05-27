#if !defined(MATH_H)

#include <math.h>


static v2
round_v2(v2 value){
    v2 result;
    result.x = (f32)((i32)(value.x + 0.5f));
    result.y = (f32)((i32)(value.y + 0.5f));
    return(result);
}

static f32
round_ff(f32 value){
    f32 result = (f32)((i32)(value + 0.5));
    return(result);
}

static i32
round_fi32(f32 value){
    i32 result = (i32)(value + 0.5);
    return(result);
}

static u32
round_fu32(f32 value){
    u32 result = (u32)(value + 0.5);
    return(result);
}

static i32
trunc_fi32(f32 value){
    i32 result = (i32)value;
    return result;
}

static f32
trunc_ff(f32 value){
    f32 result = (f32)(i32)value;
    return result;
}

static i32
floor_fi32(f32 value){
    i32 result = (i32)floorf(value);
    return result;
}

static f32
sin(f32 angle){
    f32 result = sinf(angle);
    return(result);
}

static f32
cos(f32 angle){
    f32 result = cosf(angle);
    return(result);
}

static f32
atan(f32 y, f32 x){
    f32 result = atan2f(y, x);
    return(result);
}

static f32
clamp_f32(f32 left, f32 value, f32 right){
    if(value < left){ value = left; }
    if(value > right){ value = right; }
    return(value);
}

static v2
rad_to_dir(f32 rad){
    return vec2(cosf(rad), sinf(rad));
}

static f32
dir_to_rad(v2 dir){
    return atan2f(dir.y, dir.x);
}

// NOTE: idk wtf is going on here, probably copy pasta from www.bullshit.com
static f32 
lerp_rad(f32 a, f32 b, f32 t) {
    float difference = fmodf(b - a, 2*PI),
        distance = fmodf(2.0f * difference, 2*PI) - difference;
    return a + distance * t;
}


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
        //*p = add2(*p, translation);
        *p = *p + translation;
        p++;
    }
    p -= count;
}

static void
scale(v2 *p, u32 count, f32 scalar, v2 origin){
    for(u32 i=0; i<count; ++i){
        //*p = add2(scale2(sub2(*p, origin), scalar), origin);
        *p = ((*p - origin) * scalar) + origin;
        p++;
    }
    p -= count;
}

static v2
rotate_pt(v2 p, f32 angle, v2 origin){
    v2 result = {0};
    result.x = (p.x - origin.x) * cos(angle * 3.14f/180.0f) - (p.y - origin.y) * sin(angle * 3.14f/180.0f) + origin.x;
    result.y = (p.x - origin.x) * sin(angle * 3.14f/180.0f) + (p.y - origin.y) * cos(angle * 3.14f/180.0f) + origin.y;
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
convert_u32_v4_normalized(u32 value){
	f32 alpha = ((f32)((value >> 24) & 0xFF) / 255.0f);
	f32 red =   ((f32)((value >> 16) & 0xFF) / 255.0f);
	f32 green = ((f32)((value >> 8) & 0xFF) / 255.0f);
	f32 blue =  ((f32)((value >> 0) & 0xFF) / 255.0f);
    v4 result = {red, green, blue, alpha};
    return result;
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
        //*p = scale2(*p, s);
        *p = (*p * s);
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

static f32
magnitude2(v2 a){
    return(sqrtf((a.x * a.x) + (a.y * a.y)));
}

static f32
magnitude_sq2(v2 a){
    return((a.x * a.x) + (a.y * a.y));
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

static v2
direction2(v2 a, v2 b){
    v2 result = {0};
    result.x = b.x - a.x;
    result.y = b.y - a.y;
    normalize2(&result);
    return(result);
}

static v2
direction(v2 a, v2 b){
    v2 result = {0};
    result.x = b.x - a.x;
    result.y = b.y - a.y;
    normalize2(&result);
    return(result);
}

static f32
distance2(v2 a, v2 b){
    //v2 result = sub2(a, b);
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
    //result = scale2(b, (n/d));
    result = (b * (n/d));

    return(result);
}

static v2
perpendicular2(v2 a, v2 b){
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

#define MATH_H
#endif
