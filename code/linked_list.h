typedef struct 
LinkedList{
    struct LinkedList *next;
    struct LinkedList *prev;
    void* data;
    u32 count;
} LinkedList;

static void
LL_push(LinkedList *sentinel, LinkedList *node){
    node->next = sentinel->next;
    node->prev = sentinel;
    node->next->prev = node;
    node->prev->next = node;
    sentinel->count++;
}

static void
LL_append(LinkedList *sentinel, LinkedList *node){
    node->next = sentinel;
    node->prev = sentinel->prev;
    node->next->prev = node;
    node->prev->next = node; 
    sentinel->count++;
}

static void
LL_remove(LinkedList *sentinel, LinkedList *node){
    node->next->prev = node->prev;
    node->prev->next = node->next;
    sentinel->count--;
}
