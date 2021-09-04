#if !defined(MATH_H)

#include <math.h>

//lerp(0, 100, .5) 50
//0 + .5(100 - 0)
//a + t(2 * (b - a))
//a + t(b - a)

static f32
MAXf32(f32 a, f32 b){
    if(a > b){
        return(a);
    }
    return(b);
}

static void
swapf(f32 *a, f32 *b){
    f32 t = *a;
    *a = *b;
    *b = t;
}

static v2
ceil_v2(v2 value){
    v2 result = {0};
    result.x = (f32)ceil(value.x);
    result.y = (f32)ceil(value.y);
    return(result);
}

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
Sin(f32 angle){
    f32 result = sinf(angle);
    return(result);
}

static f32
Cos(f32 angle){
    f32 result = cosf(angle);
    return(result);
}

static f32
Atan(f32 y, f32 x){
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

static f32 
lerp_rad(f32 a, f32 b, f32 t) {
    float difference = fmodf(b - a, 2*PI),
        distance = fmodf(2.0f * difference, 2*PI) - difference;
    return a + distance * t;
}

static f32 lerp(f32 a, f32 b, f32 t){
    f32 clamped_t = clamp_f32(0.0f, t, 1.0f); 
    f32 result = a + (clamped_t * (b - a));
    return(result);
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
        *p = add2(*p, translation);
        p++;
    }
    p -= count;
}

static void
scale(v2 *p, u32 count, f32 scalar, v2 origin){
    for(u32 i=0; i<count; ++i){
        *p = add2(scale2(sub2(*p, origin), scalar), origin);
        p++;
    }
    p -= count;
}

static v2
rotate_pt(v2 p, f32 angle, v2 origin){
    v2 result = {0};
    result.x = (p.x - origin.x) * Cos(angle * 3.14f/180.0f) - (p.y - origin.y) * Sin(angle * 3.14f/180.0f) + origin.x;
    result.y = (p.x - origin.x) * Sin(angle * 3.14f/180.0f) + (p.y - origin.y) * Cos(angle * 3.14f/180.0f) + origin.y;
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


#define MATH_H
#endif
