#if !defined(MATRICES_H)

typedef union mat2{
    f32 e[4];
    struct{
        f32 _11, _12,
            _21, _22;
    };
} mat2;

typedef union mat3{
    f32 e[9];
    struct{
        f32 _11, _12, _13,
            _21, _22, _23,
            _31, _32, _33;
    };
} mat3;

typedef union mat4{
    f32 e[16];
    struct{
        f32 _11, _12, _13, _14,
            _21, _22, _23, _24,
            _31, _32, _33, _34,
            _41, _42, _43, _44;
    };
} mat4;

static bool
cmp2x2(mat2 a, mat2 b){
    bool result = false;

    if(a._11 == b._11 && a._12 == b._12 && 
       a._21 == b._21 && a._22 == b._22){
        result = true;
    }

    return(result);
}

static bool
cmp3x3(mat3 a, mat3 b){
    bool result = false;

    if(a._11 == b._11 && a._12 == b._12 && a._13 == b._13 &&
       a._21 == b._21 && a._22 == b._22 && a._23 == b._23 &&
       a._31 == b._31 && a._32 == b._32 && a._33 == b._33){
        result = true;
    }

    return(result);
}

static bool
cmp4x4(mat4 a, mat4 b){
    bool result = false;

    if(a._11 == b._11 && a._12 == b._12 && a._13 == b._13 && a._14 == b._14 &&
       a._21 == b._21 && a._22 == b._22 && a._23 == b._23 && a._24 == b._24 &&
       a._31 == b._31 && a._32 == b._32 && a._33 == b._33 && a._34 == b._34 &&
       a._41 == b._41 && a._42 == b._42 && a._43 == b._43 && a._44 == b._44){
        result = true;
    }

    return(result);
}

static void
transpose_any(f32 *source_mat, f32 *dest_mat, ui32 source_rows, ui32 source_cols){
    for(ui32 i = 0; i < source_rows * source_cols; ++i){
        i32 row = i / source_rows;
        i32 col = i % source_rows;
        dest_mat[i] = source_mat[source_cols * col + row];
    }
}

static mat2
transpose2(mat2 source_mat){
    mat2 result;
    transpose_any(source_mat.e, result.e, 2, 2);
    return(result);
}

static mat3
transpose3(mat3 source_mat){
    mat3 result;
    transpose_any(source_mat.e, result.e, 3, 3);
    return(result);
}

static mat4
transpose4(mat4 source_mat){
    mat4 result;
    transpose_any(source_mat.e, result.e, 4, 4);
    return(result);
}

static mat2
scale2x2(mat2 src, f32 scalar){
    mat2 result;
    for(i32 i=0; i < 4; ++i){
        result.e[i] = src.e[i] * scalar;
    }
    return(result);
}

static mat3
scale3x3(mat3 src, f32 scalar){
    mat3 result;
    for(i32 i=0; i < 9; ++i){
        result.e[i] = src.e[i] * scalar;
    }
    return(result);
}

static mat4
scale4x4(mat4 src, f32 scalar){
    mat4 result;
    for(i32 i=0; i < 16; ++i){
        result.e[i] = src.e[i] * scalar;
    }
    return(result);
}

static bool
mul_any(f32 *mat_a, ui32 a_rows, ui32 a_cols, f32 *mat_b, ui32 b_cols, ui32 b_rows, f32 *out){
    if(a_cols != b_rows){
        return(false);
    }

    for(ui32 i = 0; i < a_rows; ++i){
       for(ui32 j = 0; j < b_cols; ++j){
           out[b_cols * i + j] = 0.0f;
           for(ui32 k = 0; k < b_rows; ++k){
               i32 a = a_cols * i + k;
               i32 b = b_cols * k + j;
               out[b_cols * i + j] += mat_a[a] * mat_b[b];
           }
       }
    }
    return(true);
}

static mat2
mul2x2(mat2 a, mat2 b){
    mat2 result;
    mul_any(a.e, 2, 2, b.e, 2, 2, result.e);
    return(result);
}

static mat3
mul3x3(mat3 a, mat3 b){
    mat3 result;
    mul_any(a.e, 3, 3, b.e, 3, 3, result.e);
    return(result);
}

static mat4
mul4x4(mat4 a, mat4 b){
    mat4 result;
    mul_any(a.e, 4, 4, b.e, 4, 4, result.e);
    return(result);
}

static mat2
identity2x2(){
    mat2 result = {0};
    result._11 = result._22 = 1.0f;
    return(result);
}

static mat3
identity3x3(){
    mat3 result = {0};
    result._11 = result._22 = result._33 = 1.0f;
    return(result);
}

static mat4
identity4x4(){
    mat4 result = {0};
    result._11 = result._22 = result._33 = result._44 = 1.0f;
    return(result);
}

static f32
determinant2x2(mat2 a){
    f32 result;
    result = a._11 * a._22 - a._12 * a._21;
    return(result);
}

static mat2
m2(f32 a, f32 b, 
   f32 c, f32 d){
    mat2 result = {a, b, c, d};
    return(result);
}

static mat3
m3(f32 a, f32 b, f32 c, 
   f32 d, f32 e, f32 f, 
   f32 g, f32 h, f32 i){
    mat3 result = {a, b, c, d, e, f, g, h, i};
    return(result);
}

static mat4
m4(f32 a, f32 b, f32 c, f32 d, 
   f32 e, f32 f, f32 g, f32 h, 
   f32 i, f32 j, f32 k, f32 l,
   f32 m, f32 n, f32 o, f32 p){
    mat4 result = {a, b, c, d, e, f, g, h, 
                   i, j, k, l, m, n, o, p};
    return(result);
}

#define MATRICES_H
#endif
