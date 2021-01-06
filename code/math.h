#if !defined(MATH_H)

#include <math.h>

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

#define MATH_H
#endif
