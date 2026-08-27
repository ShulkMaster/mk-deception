#define NULL ((void*)0)

typedef struct Block Block;
typedef struct SubBlock SubBlock;
typedef struct FixBlock FixBlock;
typedef struct FixSubBlock FixSubBlock;

struct Block {
    Block* prev;
    Block* next;
    unsigned long max_size;
    unsigned long size;
};

struct SubBlock {
    unsigned long size;
    Block* block;
    SubBlock* prev;
    SubBlock* next;
};

struct FixBlock {
    FixBlock* prev;
    FixBlock* next;
    unsigned long client_size;
    FixSubBlock* free;
    unsigned long allocated;
};

struct FixSubBlock {
    FixBlock* block;
    FixSubBlock* next;
};

typedef struct FixStart {
    FixBlock* tail;
    FixBlock* head;
} FixStart;

typedef struct MemPoolObj {
    Block* start;
    FixStart fixed[6];
} MemPoolObj;

typedef struct __mem_pool {
    void* reserved[14];
} __mem_pool;

static const unsigned long fix_pool_sizes[] = {4, 12, 20, 36, 52, 68};

void* __sys_alloc(unsigned long size);
void __sys_free(void* ptr);
void* memset(void* dst, int value, unsigned long size);
void* memcpy(void* dst, const void* src, unsigned long size);
void __begin_critical_region(int region);
void __end_critical_region(int region);

void* __pool_alloc(__mem_pool* pool, unsigned long size);
void* __pool_realloc(__mem_pool* pool, void* ptr, unsigned long size);
void __pool_free(__mem_pool* pool, void* ptr);
void* __pool_alloc_clear(__mem_pool* pool, unsigned long size);
void deallocate_from_fixed_pools(MemPoolObj* pool, void* ptr, unsigned long size);
void* allocate_from_fixed_pools(MemPoolObj* pool, unsigned long size);
static void deallocate_from_var_pools(MemPoolObj* pool, void* ptr);
static void* soft_allocate_from_var_pools(
    MemPoolObj* pool, unsigned long size, unsigned long* largest);
static void* allocate_from_var_pools(MemPoolObj* pool, unsigned long size);
static Block* link_new_block(MemPoolObj* pool, unsigned long size);
static SubBlock* Block_subBlock(Block* block, unsigned long size);
static void Block_construct(Block* block, unsigned long size);

static inline unsigned long subblock_size(const SubBlock* block) {
    return block->size & ~7UL;
}

static inline unsigned long allocation_size(void* ptr) {
    unsigned long tag = *(unsigned long*)((unsigned char*)ptr - 4);

    if ((tag & 1) == 0) {
        FixSubBlock* subblock = (FixSubBlock*)((unsigned char*)ptr - 4);
        return subblock->block->client_size;
    }
    return (*(unsigned long*)((unsigned char*)ptr - 8) & ~7UL) - 8;
}

static inline Block* unlink_block(MemPoolObj* pool, Block* block) {
    Block* next = block->next;

    if (next == block) {
        next = NULL;
    }
    if (pool->start == block) {
        pool->start = next;
    }
    if (next != NULL) {
        next->prev = block->prev;
        next->prev->next = next;
    }
    block->next = NULL;
    block->prev = NULL;
    return next;
}

static inline unsigned long block_size(const Block* block) {
    return block->size & ~7UL;
}

static inline SubBlock** block_start(Block* block) {
    return (SubBlock**)((unsigned char*)block + block_size(block) - 4);
}

static inline int subblock_is_free(const SubBlock* subblock) {
    return !(subblock->size & 2);
}

static inline void subblock_set_size(SubBlock* subblock, unsigned long size) {
    subblock->size &= 7;
    subblock->size |= size & ~7UL;
    if (!(subblock->size & 2)) {
        *(unsigned long*)((unsigned char*)subblock + size - 4) = size;
    }
}

static inline SubBlock* merge_previous(SubBlock* subblock, SubBlock** start) {
    unsigned long previous_size;
    SubBlock* previous;

    if (!(subblock->size & 4)) {
        previous_size = *(unsigned long*)((unsigned char*)subblock - 4);
        if (previous_size & 2) {
            return subblock;
        }
        previous = (SubBlock*)((unsigned char*)subblock - previous_size);
        subblock_set_size(previous, previous_size + subblock_size(subblock));
        if (*start == subblock) {
            *start = (*start)->next;
        }
        subblock->next->prev = subblock->prev;
        subblock->next->prev->next = subblock->next;
        return previous;
    }
    return subblock;
}

static inline void merge_next(SubBlock* subblock, SubBlock** start) {
    SubBlock* next = (SubBlock*)((unsigned char*)subblock + subblock_size(subblock));
    unsigned long size;

    if (!(next->size & 2)) {
        size = subblock_size(subblock) + subblock_size(next);
        subblock->size &= 7;
        subblock->size |= size & ~7UL;
        if (!(subblock->size & 2)) {
            *(unsigned long*)((unsigned char*)subblock + size - 4) = size;
        }
        if (!(subblock->size & 2)) {
            *(unsigned long*)((unsigned char*)subblock + size) &= ~4UL;
        } else {
            *(unsigned long*)((unsigned char*)subblock + size) |= 4;
        }
        if (*start == next) {
            *start = (*start)->next;
        }
        if (*start == next) {
            *start = NULL;
        }
        next->next->prev = next->prev;
        next->prev->next = next->next;
    }
}

static inline void construct_subblock(
    SubBlock* subblock,
    Block* owner,
    unsigned long size,
    int allocated,
    int previous_allocated) {
    subblock->block = (Block*)((unsigned long)owner | 1);
    subblock->size = size;
    if (previous_allocated) {
        subblock->size |= 4;
    }
    if (allocated) {
        subblock->size |= 2;
        *(unsigned long*)((unsigned char*)subblock + size) |= 4;
    } else {
        *(unsigned long*)((unsigned char*)subblock + size - 4) = size;
    }
}

static inline SubBlock* split_subblock(SubBlock* subblock, unsigned long size) {
    unsigned long old_size = subblock_size(subblock);
    int was_free = subblock_is_free(subblock);
    int allocated = !was_free;
    int previous_allocated = (subblock->size & 4) != 0;
    Block* owner = (Block*)((unsigned long)subblock->block & ~1UL);
    SubBlock* remainder = (SubBlock*)((unsigned char*)subblock + size);

    construct_subblock(subblock, owner, size, allocated, previous_allocated);
    construct_subblock(remainder, owner, old_size - size, allocated, allocated);
    if (was_free) {
        remainder->next = subblock->next;
        remainder->next->prev = remainder;
        remainder->prev = subblock;
        subblock->next = remainder;
    }
    return remainder;
}

static inline void unlink_subblock(Block* block, SubBlock* subblock) {
    SubBlock** start;
    unsigned long size;

    *block_start(block) = subblock->next;
    size = subblock->size;
    subblock->size = size | 2;
    size &= ~7UL;
    *(unsigned long*)((unsigned char*)subblock + size) |= 4;
    start = block_start(block);
    if (*start == subblock) {
        *start = (*start)->next;
    }
    if (*start == subblock) {
        *start = NULL;
        block->max_size = 0;
    } else {
        subblock->next->prev = subblock->prev;
        subblock->prev->next = subblock->next;
    }
}

#define LINK_FREE_SUBBLOCK(block_, subblock_)                                  \
    do {                                                                       \
        SubBlock** link_start = block_start(block_);                           \
        unsigned long link_size = subblock_size(subblock_);                    \
                                                                               \
        (subblock_)->size &= ~2UL;                                             \
        *(unsigned long*)((unsigned char*)(subblock_) + link_size) &= ~4UL;    \
        *(unsigned long*)((unsigned char*)(subblock_) + link_size - 4) =       \
            link_size;                                                         \
        if (*link_start != NULL) {                                             \
            (subblock_)->prev = (*link_start)->prev;                           \
            (subblock_)->prev->next = (subblock_);                             \
            (subblock_)->next = *link_start;                                   \
            (*link_start)->prev = (subblock_);                                 \
            *link_start = (subblock_);                                         \
            *link_start = merge_previous(*link_start, link_start);             \
            merge_next(*link_start, link_start);                               \
        } else {                                                               \
            *link_start = (subblock_);                                         \
            (subblock_)->prev = (subblock_);                                   \
            (subblock_)->next = (subblock_);                                   \
        }                                                                      \
        if ((block_)->max_size < subblock_size(*link_start)) {                 \
            (block_)->max_size = subblock_size(*link_start);                   \
        }                                                                      \
    } while (0)

static inline __mem_pool* get_malloc_pool(void) {
    static __mem_pool pool;
    static unsigned char initialized;

    if (initialized == 0) {
        memset(&pool, 0, sizeof(MemPoolObj));
        initialized = 1;
    }
    return &pool;
}

void* calloc(unsigned long count, unsigned long size) {
    void* result;

    __begin_critical_region(1);
    result = __pool_alloc_clear(get_malloc_pool(), size * count);
    __end_critical_region(1);
    return result;
}

void* realloc(void* ptr, unsigned long size) {
    void* result;

    __begin_critical_region(1);
    result = __pool_realloc(get_malloc_pool(), ptr, size);
    __end_critical_region(1);
    return result;
}

void* __pool_alloc_clear(__mem_pool* pool, unsigned long size) {
    void* ptr = __pool_alloc(pool, size);

    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void* __pool_realloc(__mem_pool* pool, void* ptr, unsigned long size) {
    unsigned long old_size;
    unsigned long needed;
    unsigned long is_variable;
    SubBlock* subblock;
    SubBlock* remainder;
    Block* block;
    SubBlock** start;
    void* result;

    if (ptr == NULL) {
        return __pool_alloc(pool, size);
    }
    if (size == 0) {
        __pool_free(pool, ptr);
        return NULL;
    }

    is_variable = (*(unsigned long*)((unsigned char*)ptr - 4) & 1) != 0;
    old_size = allocation_size(ptr);
    if (size > old_size) {
        if (is_variable) {
            if (size > ~0UL - 48) {
                return NULL;
            }
            needed = (size + 15) & ~7UL;
            if (needed < 0x50) {
                needed = 0x50;
            }
            subblock = (SubBlock*)((unsigned char*)ptr - 8);
            block = (Block*)((unsigned long)subblock->block & ~1UL);
            start = block_start(block);
            merge_next(subblock, start);
            if (subblock_size(subblock) >= needed) {
                if (subblock_size(subblock) - needed >= 0x50) {
                    remainder = split_subblock(subblock, needed);
                    LINK_FREE_SUBBLOCK(block, remainder);
                }
                return ptr;
            }
        }
        result = __pool_alloc(pool, size);
        if (result == NULL) {
            return NULL;
        }
        memcpy(result, ptr, old_size);
        __pool_free(pool, ptr);
        return result;
    }

    if (is_variable) {
        needed = (size + 15) & ~7UL;
        if (needed < 0x50) {
            needed = 0x50;
        }
        subblock = (SubBlock*)((unsigned char*)ptr - 8);
        block = (Block*)((unsigned long)subblock->block & ~1UL);
        if (subblock_size(subblock) - needed >= 0x50) {
            remainder = split_subblock(subblock, needed);
            LINK_FREE_SUBBLOCK(block, remainder);
        }
    }
    return ptr;
}

void __pool_free(__mem_pool* pool, void* ptr) {
    unsigned long size;

    if (ptr == NULL) {
        return;
    }
    size = allocation_size(ptr);
    if (size <= 68) {
        deallocate_from_fixed_pools((MemPoolObj*)pool, ptr, size);
    } else {
        deallocate_from_var_pools((MemPoolObj*)pool, ptr);
    }
}

void* __pool_alloc(__mem_pool* pool, unsigned long size) {
    if (size == 0) {
        return NULL;
    }
    if (size > ~0UL - 48) {
        return NULL;
    }
    if (size <= 68) {
        return allocate_from_fixed_pools((MemPoolObj*)pool, size);
    }
    return allocate_from_var_pools((MemPoolObj*)pool, size);
}

void deallocate_from_fixed_pools(MemPoolObj* pool, void* ptr, unsigned long size) {
    unsigned long index = 0;
    FixSubBlock* subblock;
    FixBlock* block;
    FixStart* start;

    while (size > fix_pool_sizes[index]) {
        index++;
    }
    start = &pool->fixed[index];
    subblock = (FixSubBlock*)((unsigned char*)ptr - 4);
    block = subblock->block;
    if (block->free == NULL && start->head != block) {
        if (start->tail == block) {
            start->head = start->head->prev;
            start->tail = start->tail->prev;
        } else {
            block->prev->next = block->next;
            block->next->prev = block->prev;
            block->next = start->head;
            block->prev = block->next->prev;
            block->prev->next = block;
            block->next->prev = block;
            start->head = block;
        }
    }
    subblock->next = block->free;
    block->free = subblock;
    if (--block->allocated == 0) {
        if (start->head == block) {
            start->head = block->next;
        }
        if (start->tail == block) {
            start->tail = block->prev;
        }
        block->prev->next = block->next;
        block->next->prev = block->prev;
        if (start->head == block) {
            start->head = NULL;
        }
        if (start->tail == block) {
            start->tail = NULL;
        }
        deallocate_from_var_pools(pool, block);
    }
}

void* allocate_from_fixed_pools(MemPoolObj* pool, unsigned long size) {
    unsigned long index = 0;
    unsigned long count;
    unsigned long maximum_count;
    unsigned long largest;
    unsigned long available;
    unsigned long stride;
    unsigned long i;
    FixStart* start;
    FixBlock* block;
    FixBlock* head;
    FixBlock* tail;
    FixSubBlock* subblock;
    FixSubBlock* next;
    void* memory;

    while (size > fix_pool_sizes[index]) {
        index++;
    }
    start = &pool->fixed[index];
    block = start->head;
    if (block == NULL || block->free == NULL) {
        maximum_count = 0xFEC / (fix_pool_sizes[index] + 4);
        if (maximum_count > 0x100) {
            maximum_count = 0x100;
        }
        count = maximum_count;
        while (count >= 10) {
            memory = soft_allocate_from_var_pools(
                pool, count * (fix_pool_sizes[index] + 4) + 0x14, &largest);
            if (memory != NULL) {
                break;
            }
            if (largest > 0x14) {
                count = (largest - 0x14) / (fix_pool_sizes[index] + 4);
            } else {
                count = 0;
            }
        }
        if (memory == NULL && count < maximum_count) {
            memory = allocate_from_var_pools(
                pool, maximum_count * (fix_pool_sizes[index] + 4) + 0x14);
            if (memory == NULL) {
                return NULL;
            }
        }

        available = allocation_size(memory);
        block = (FixBlock*)memory;
        if (start->head == NULL) {
            start->head = block;
            start->tail = block;
        }
        head = start->head;
        tail = start->tail;
        block->prev = tail;
        block->next = head;
        tail->next = block;
        head->prev = block;
        block->client_size = fix_pool_sizes[index];
        stride = fix_pool_sizes[index] + 4;
        count = (available - 0x14) / stride;
        subblock = (FixSubBlock*)((unsigned char*)block + 0x14);
        for (i = 0; i < count - 1; i++) {
            next = (FixSubBlock*)((unsigned char*)subblock + stride);
            subblock->block = block;
            subblock->next = next;
            subblock = next;
        }
        subblock->block = block;
        subblock->next = NULL;
        block->free = (FixSubBlock*)((unsigned char*)block + 0x14);
        block->allocated = 0;
        start->head = block;
    }

    subblock = start->head->free;
    start->head->free = subblock->next;
    start->head->allocated++;
    if (start->head->free == NULL) {
        start->head = start->head->next;
        start->tail = start->tail->next;
    }
    return (unsigned char*)subblock + 4;
}

static void deallocate_from_var_pools(MemPoolObj* pool, void* ptr) {
    SubBlock* subblock = (SubBlock*)((unsigned char*)ptr - 8);
    Block* block = (Block*)((unsigned long)subblock->block & ~1UL);
    SubBlock* first;
    SubBlock** start;
    unsigned long size;

    size = subblock_size(subblock);
    subblock->size &= ~2UL;
    *(unsigned long*)((unsigned char*)subblock + size) &= ~4UL;
    *(unsigned long*)((unsigned char*)subblock + size - 4) = size;
    start = block_start(block);
    if (*start != NULL) {
        subblock->prev = (*start)->prev;
        subblock->prev->next = subblock;
        subblock->next = *start;
        (*start)->prev = subblock;
        *start = subblock;
        *start = merge_previous(*start, start);
        merge_next(*start, start);
    } else {
        *start = subblock;
        subblock->prev = subblock;
        subblock->next = subblock;
    }
    if (block->max_size < subblock_size(*start)) {
        block->max_size = subblock_size(*start);
    }
    first = (SubBlock*)((unsigned char*)block + 16);
    if ((first->size & 2) == 0 && subblock_size(first) == (block->size & ~7UL) - 24) {
        unlink_block(pool, block);
        __sys_free(block);
    }
}

static void* soft_allocate_from_var_pools(
    MemPoolObj* pool, unsigned long size, unsigned long* largest) {
    unsigned long needed = (size + 15) & ~7UL;
    Block* block;
    SubBlock* subblock;

    if (needed < 0x50) {
        needed = 0x50;
    }
    *largest = 0;
    block = pool->start;
    if (block == NULL) {
        return NULL;
    }
    do {
        if (needed <= block->max_size) {
            subblock = Block_subBlock(block, needed);
            if (subblock != NULL) {
                pool->start = block;
                return (unsigned char*)subblock + 8;
            }
        }
        if (block->max_size > 8 && *largest < block->max_size - 8) {
            *largest = block->max_size - 8;
        }
        block = block->next;
    } while (block != pool->start);
    return NULL;
}

static void* allocate_from_var_pools(MemPoolObj* pool, unsigned long size) {
    unsigned long needed = (size + 15) & ~7UL;
    Block* block;
    SubBlock* subblock;

    if (needed < 0x50) {
        needed = 0x50;
    }
    block = pool->start;
    if (block == NULL) {
        block = link_new_block(pool, needed);
    }
    if (block == NULL) {
        return NULL;
    }
    do {
        if (needed <= block->max_size) {
            subblock = Block_subBlock(block, needed);
            if (subblock != NULL) {
                pool->start = block;
                return (unsigned char*)subblock + 8;
            }
        }
        block = block->next;
    } while (block != pool->start);
    block = link_new_block(pool, needed);
    if (block == NULL) {
        return NULL;
    }
    return (unsigned char*)Block_subBlock(block, needed) + 8;
}

static Block* link_new_block(MemPoolObj* pool, unsigned long size) {
    unsigned long block_size = (size + 31) & ~7UL;
    Block* block;
    Block* start;

    if (block_size < 0x10000) {
        block_size = 0x10000;
    }
    block = (Block*)__sys_alloc(block_size);
    if (block == NULL) {
        return NULL;
    }
    Block_construct(block, block_size);
    start = pool->start;
    if (start != NULL) {
        block->prev = start->prev;
        block->prev->next = block;
        block->next = start;
        start->prev = block;
        pool->start = block;
    } else {
        pool->start = block;
        block->prev = block;
        block->next = block;
    }
    return block;
}

static SubBlock* Block_subBlock(Block* block, unsigned long size) {
    SubBlock** rover = block_start(block);
    SubBlock* start = *rover;
    SubBlock* subblock;
    unsigned long max_size;

    if (start == NULL) {
        block->max_size = 0;
        return NULL;
    }
    subblock = start;
    max_size = subblock_size(subblock);
    while (subblock_size(subblock) < size) {
        subblock = subblock->next;
        if (max_size < subblock_size(subblock)) {
            max_size = subblock_size(subblock);
        }
        if (subblock == start) {
            block->max_size = max_size;
            return NULL;
        }
    }

    if (subblock_size(subblock) - size >= 0x50) {
        split_subblock(subblock, size);
    }

    unlink_subblock(block, subblock);
    return subblock;
}

static void Block_construct(Block* block, unsigned long size) {
    SubBlock* subblock = (SubBlock*)((unsigned char*)block + 16);
    SubBlock** start;
    unsigned long subblock_bytes = size - 24;

    block->size = size | 3;
    *(unsigned long*)((unsigned char*)block + size - 8) = block->size;
    subblock->size = subblock_bytes;
    subblock->block = (Block*)((unsigned long)block | 1);
    *(unsigned long*)((unsigned char*)subblock + subblock_bytes - 4) = subblock_bytes;
    block->max_size = subblock_bytes;
    start = block_start(block);
    *start = NULL;

    subblock->size &= ~2UL;
    *(unsigned long*)((unsigned char*)subblock + subblock_bytes) &= ~4UL;
    *(unsigned long*)((unsigned char*)subblock + subblock_bytes - 4) = subblock_bytes;
    if (*start != NULL) {
        subblock->prev = (*start)->prev;
        subblock->prev->next = subblock;
        subblock->next = *start;
        (*start)->prev = subblock;
        *start = subblock;
        *start = merge_previous(*start, start);
        merge_next(*start, start);
    } else {
        *start = subblock;
        subblock->prev = subblock;
        subblock->next = subblock;
    }
    if (block->max_size < subblock_size(*start)) {
        block->max_size = subblock_size(*start);
    }
}
