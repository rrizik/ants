#ifndef PCG_BASIC_H_INCLUDED
#define PCG_BASIC_H_INCLUDED 1

#include <inttypes.h>

#if __cplusplus
extern "C" {
#endif

typedef struct pcg_state_setseq_64{  // Internals are *Private*.
    ui64 state;                  // RNG state.  All values are possible.
    ui64 inc;                    // Controls which RNG sequence (stream) is
                                     // selected. Must *always* be odd.
} pcg_state_setseq_64;
typedef struct pcg_state_setseq_64 pcg32_random_t;

// If you *must* statically initialize it, here's one.
#define PCG32_INITIALIZER   { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

// state for global RNGs
static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

// Generate a uniformly distributed 32-bit random number
static ui32 
random_r(pcg32_random_t* rng){
    ui64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    ui32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    ui32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
static ui32 
random(){
    return random_r(&pcg32_global);
}

// Seed the rng.  Specified in two parts, state initializer and a
// sequence selection constant (a.k.a. stream id)
static void 
srandom_r(pcg32_random_t* rng, ui64 initstate, ui64 initseq){
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    random_r(rng);
    rng->state += initstate;
    random_r(rng);
}
static void 
srandom(ui64 seed, ui64 seq){
    srandom_r(&pcg32_global, seed, seq);
}

// Generate a uniformly distributed number, r, where 0 <= r < bound
static ui32 boundedrand_r(pcg32_random_t* rng, ui32 bound)
{
    ui32 threshold = -bound % bound;

    for (;;) {
        ui32 r = random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}
static ui32 boundedrand(ui32 bound)
{
    return boundedrand_r(&pcg32_global, bound);
}

#if __cplusplus
}
#endif

#endif 
