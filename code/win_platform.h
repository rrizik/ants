#if !defined(WIN_PLATFORM_H)

typedef struct WIN_RenderBuffer{
    void *memory;
    int memory_size;
    BITMAPINFO info;

    int bytes_per_pixel;
    int width;
    int height;
    int pitch;
} WIN_RenderBuffer;

typedef struct WIN_WindowDimensions{
    int x;
    int y;
    int width;
    int height;
} WIN_WindowDimensions;

typedef struct WIN_Clock{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    uint64 cpu_start;
    uint64 cpu_end;

    float target_seconds_per_frame;
    bool sleep_granularity_set;
} WIN_Clock;

typedef struct WIN_GameCode{
    HMODULE gamecode_dll;
    MainGameLoop *main_game_loop;

    FILETIME write_time;
    bool is_valid;
} WIN_GameCode;

#define WIN_PLATFORM_H
#endif
