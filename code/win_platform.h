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
    uint64 root_dir_length;

    WIN_ReplayBuffer replay_buffers[4];
    //void *replay_buffers[4];
    //HANDLE file_handles[4];

    int recording_index;
    HANDLE recording_handle;

    HANDLE playback_handle;
    int playback_index;
} WIN_State;

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
typedef struct win32_replay_buffer{
    HANDLE FileHandle;
    HANDLE MemoryMap;
    char FileName[WIN32_STATE_FILE_NAME_COUNT];
    void *MemoryBlock;
} win32_replay_buffer;

typedef struct win32_state{
    char root_dir[256];
    uint64 root_dir_length;
    uint64 TotalSize;
    void *GameMemoryBlock;
    win32_replay_buffer ReplayBuffers[4];

    HANDLE RecordingHandle;
    int InputRecordingIndex;

    HANDLE PlayBackHandle;
    int InputPlayingIndex;


    char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
    char *OnePastLastEXEFileNameSlash;
} win32_state;
#define WIN_PLATFORM_H
#endif
