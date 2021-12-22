#if !defined(WIN_PLATFORM_H)

typedef struct WIN_RenderBuffer{
    void *memory;
    i32 memory_size;
    BITMAPINFO info;

    i32 bytes_per_pixel;
    i32 width;
    i32 height;
    i32 pitch;
} WIN_RenderBuffer;

typedef struct WIN_WindowDimensions{
    i32 x;
    i32 y;
    i32 width;
    i32 height;
} WIN_WindowDimensions;

typedef struct WIN_Clock{
    i64 start;
    i64 end;
    i64 frequency;
    u64 cpu_start;
    u64 cpu_end;

    f32 target_seconds_per_frame;
    bool sleep_granularity_set;
} WIN_Clock;

typedef struct WIN_GameCode{
    char root[MAX_PATH];

    HMODULE gamecode_dll;
    MainGameLoop *main_game_loop;

    FILETIME write_time;
    bool is_valid;
} WIN_GameCode;

typedef struct WIN_ReplayBuffer{
    void *memory;
    char file_name[MAX_PATH];
    HANDLE file_handle;
    HANDLE file_memory_mapping;
} WIN_ReplayBuffer;

typedef struct WIN_State{
    char root_dir[256];
    u64 root_dir_length;

    void *replay_buffers[4];
    // CONSIDER: maybe do this in the future if it ends up slowing down WIN_ReplayBuffer replay_buffers[4];

    i32 recording_index;
    HANDLE recording_handle;

    HANDLE playback_handle;
    i32 playback_index;
} WIN_State;

#define WIN_PLATFORM_H
#endif
