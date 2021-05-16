#if !defined(MATH_H)

#include <math.h>

//lerp(0, 100, .5) 50
//0 + .5(100 - 0)
//a + t(2 * (b - a))
//a + t(b - a)

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

static ui32
round_fui32(f32 value){
    ui32 result = (ui32)(value + 0.5);
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
    if(value < left){
        value = left;
    }
    if(value > right){
        value = right;
    }
    return(value);
}

static v2
angle_direction(f32 angle){
    return vec2(cosf(angle), sinf(angle));
}

static f32
direction_angle(v2 direction){
    return atan2f(direction.y, direction.x);
}

static f32 
lerp_rad(f32 a, f32 b, f32 t) {
    float difference = fmodf(b - a, 2*PI),
        distance = fmodf(2.0f * difference, 2*PI) - difference;
    return a + distance * t;
}

#define MATH_H
#endif
