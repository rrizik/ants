#if !defined(GAME_H)

//#include <stdbool.h> //FUTURE: Use this later once your more comfortable for bool
#include <stdint.h>

//#define Assert(Expression) if(!(Expression)) {volatie *(int *)0 = 0;}
#define ArrayCount(value) (sizeof(value)/sizeof(value[0]))

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define local_static static
#define global static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32_t bool;
enum{false, true};

typedef struct RenderBuffer{
    void *memory;
    int memory_size;

    int bytes_per_pixel;
    int width;
    int height;
    int pitch;
} RenderBuffer;

typedef enum{PAD_NONE, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_BACK} EventPad;
typedef enum{KEY_NONE, KEY_W, KEY_A, KEY_S, KEY_D, KEY_ESCAPE} EventKey;
typedef enum{EVENT_NONE, EVENT_KEYDOWN, EVENT_KEYUP, EVENT_TEXT, EVENT_PADDOWN, EVENT_PADUP} EventType;
typedef struct Event{
    EventType type;
    EventKey key;
    EventPad pad;
    // QUESTION: ask about this
    uint16 text;
} Event;

typedef struct FileData{
    uint32 size;
    void* content;
} FileData;

typedef struct GameState{
    int xoffset;
    bool move;
} GameState;

#define READ_ENTIRE_FILE(name) FileData name(char *filename)
typedef READ_ENTIRE_FILE(ReadEntireFile);

#define WRITE_ENTIRE_FILE(name) bool name(char *filename, void *memory, uint32 memory_size)
typedef WRITE_ENTIRE_FILE(WriteEntireFile);

#define FREE_FILE_MEMORY(name) void name(void *memory)
typedef FREE_FILE_MEMORY(FreeFileMemory);

typedef struct GameMemory{
	bool running;
    bool initialized;

    uint64 total_size;
    uint64 permanent_storage_size;
    uint64 temporary_storage_size;

    void *total_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup
    void *temporary_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup
    void *permanent_storage; // IMPORTANT: REQUIRED to be cleared to zero at startup

    ReadEntireFile *read_entire_file;
    WriteEntireFile *write_entire_file;
    FreeFileMemory *free_file_memory;
} GameMemory;

#define MAIN_GAME_LOOP(name) void name(int *running, GameMemory *memory, RenderBuffer *render_buffer, Event events[], uint32 event_i)
typedef MAIN_GAME_LOOP(MainGameLoop);

#define GAME_H
#endif
