#if !defined(MATH_H)

#include <math.h>

static void
swapf(f32 *a, f32 *b){
    f32 t = *a;
    *a = *b;
    *b = t;
}

static void
swapv2(v2 *a, v2 *b){
    v2 t = *a;
    *a = *b;
    *b = t;
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

static v2
v2_add(v2 p1, v2 p2){
    v2 result = {0};
    result.x = p1.x + p2.x;
    result.y = p1.y + p2.y;

    return(result);
}

static v2
v2_sub(v2 p1, v2 p2){
    v2 result = {0};
    result.x = p1.x - p2.x;
    result.y = p1.y - p2.y;

    return(result);
}

static v2
v2_mul(v2 p1, v2 p2){
    v2 result = {0};
    result.x = p1.x * p2.x;
    result.y = p1.y * p2.y;

    return(result);
}

static v2
v2_div(v2 p1, v2 p2){
    v2 result = {0};
    result.x = p1.x / p2.x;
    result.y = p1.y / p2.y;

    return(result);
}

#define MATH_H
#endif
