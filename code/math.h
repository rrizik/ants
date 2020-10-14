#if !defined(MATH_H)

#include <math.h>

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

#define MATH_H
#endif
