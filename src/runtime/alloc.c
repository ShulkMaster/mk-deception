#include "runtime/critical_regions.h"
#include "runtime/cstring.h"
#include "runtime/cstdlib.h"

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

typedef char BlockSizeCheck[sizeof(Block) == 0x10 ? 1 : -1];
typedef char SubBlockSizeCheck[sizeof(SubBlock) == 0x10 ? 1 : -1];
typedef char FixBlockSizeCheck[sizeof(FixBlock) == 0x14 ? 1 : -1];
typedef char FixSubBlockSizeCheck[sizeof(FixSubBlock) == 0x08 ? 1 : -1];
typedef char FixStartSizeCheck[sizeof(FixStart) == 0x08 ? 1 : -1];
typedef char MemPoolObjSizeCheck[sizeof(MemPoolObj) == 0x34 ? 1 : -1];
typedef char MemPoolSizeCheck[sizeof(__mem_pool) == 0x38 ? 1 : -1];

static const unsigned long fix_pool_sizes[] = {4, 12, 20, 36, 52, 68};

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

static SubBlock* merge_previous(SubBlock*, SubBlock**);
static inline void merge_next(SubBlock*, SubBlock**);

#define subblock_size(ths)  ((ths)->size & 0xFFFFFFF8)
#define subblock_owner(ths) ((Block*)((unsigned long)((ths)->block) & ~0x1))
#define block_size(ths)     ((ths)->size & 0xFFFFFFF8)
#define set_subblock_free(ths)                                                                     \
  unsigned long this_size = subblock_size((ths));                                                \
  (ths)->size &= ~0x2;                                                                           \
  *(unsigned long*)((char*)(ths) + this_size) &= ~0x4;                                           \
  *(unsigned long*)((char*)(ths) + this_size - sizeof(unsigned long)) = this_size
#define subblock_is_free(ths) !((ths)->size & 2)
#define subblock_set_size(ths, sz)                                                                 \
  (ths)->size &= ~0xFFFFFFF8;                                                                    \
  (ths)->size |= (sz) & 0xFFFFFFF8;                                                              \
  if (subblock_is_free((ths)))                                                                   \
  *(unsigned long*)((char*)(ths) + (sz) - sizeof(unsigned long)) = (sz)
#define subblock_from_pointer(ptr)    ((SubBlock*)((char*)(ptr) - 8))
#define fixed_subblock_from_pointer(ptr) ((FixSubBlock*)((char*)(ptr) - 4))
#define fixed_client_size(ths) ((ths)->client_size)
#define fixed_subblock_size(ths)     (fixed_client_size((ths)->block))
#define classify_allocation(ptr) (*(unsigned long*)((char*)(ptr) - sizeof(unsigned long)) & 1)
#define allocation_size(ptr)                                                                        \
  (!classify_allocation(ptr) ? fixed_subblock_size(fixed_subblock_from_pointer(ptr))                              \
                  : subblock_size(subblock_from_pointer(ptr)) - 8)
#define block_start(ths) (&(*(SubBlock**)((char*)(ths) + block_size((ths)) - sizeof(unsigned long))))

static inline void construct_subblock(
        SubBlock* ths, Block* block, unsigned long size, int prev_allocated, int allocated)
{
    ths->block = (Block*)((unsigned long)block | 1);
    ths->size  = size;
    if (prev_allocated)
        ths->size |= 4;
    if (allocated) {
        ths->size |= 2;
        *(unsigned long*)((char*)ths + size) |= 4;
    } else {
        *(unsigned long*)((char*)ths + size - sizeof(unsigned long)) = size;
    }
}

static inline void split_subblock(SubBlock* ths, unsigned long size)
{
    SubBlock* next         = (SubBlock*)((char*)ths + size);
    unsigned long old_size = subblock_size(ths);
    int is_free            = subblock_is_free(ths);
    Block* block           = subblock_owner(ths);

    construct_subblock(ths, block, size, ths->size & 4, !is_free);
    construct_subblock(next, block, old_size - size, !is_free, !is_free);
    if (is_free) {
        next->next       = ths->next;
        next->next->prev = next;
        next->prev       = ths;
        ths->next        = next;
    }
}

static inline SubBlock* merge_previous(SubBlock* ths, SubBlock** start)
{
    unsigned long prevsz;
    SubBlock* p;

    if (!(ths->size & 0x04)) {
        prevsz = *(unsigned long*)((char*)ths - sizeof(unsigned long));
        if (prevsz & 0x2)
            return ths;
        p = (SubBlock*)((char*)ths - prevsz);
        subblock_set_size(p, prevsz + subblock_size(ths));

        if (*start == ths)
            *start = (*start)->next;
        ths->next->prev       = ths->prev;
        ths->next->prev->next = ths->next;
        return p;
    }
    return ths;
}

static inline void Block_link(Block* ths, SubBlock* sb)
{
    SubBlock** st;
    set_subblock_free(sb);
    st = block_start(ths);

    if (*st != 0) {
        sb->prev       = (*st)->prev;
        sb->prev->next = sb;
        sb->next       = *st;
        (*st)->prev    = sb;
        *st            = sb;
        *st            = merge_previous(*st, st);
        merge_next(*st, st);
    } else {
        *st      = sb;
        sb->prev = sb;
        sb->next = sb;
    }
    if (ths->max_size < subblock_size(*st))
        ths->max_size = subblock_size(*st);
}

static inline void merge_next(SubBlock* pBlock, SubBlock** pStart)
{
    SubBlock* next_sub_block;
    unsigned long this_cur_size;

    next_sub_block = (SubBlock*)((char*)pBlock + (pBlock->size & 0xFFFFFFF8));

    if (!(next_sub_block->size & 2)) {
        this_cur_size = (pBlock->size & 0xFFFFFFF8) + (next_sub_block->size & 0xFFFFFFF8);

        pBlock->size &= ~0xFFFFFFF8;
        pBlock->size |= this_cur_size & 0xFFFFFFF8;

        if (!(pBlock->size & 2)) {
            *(unsigned long*)((char*)(pBlock) + (this_cur_size)-4) = (this_cur_size);
        }

        if (!(pBlock->size & 2)) {
            *(unsigned long*)((char*)pBlock + this_cur_size) &= ~4;
        } else {
            *(unsigned long*)((char*)pBlock + this_cur_size) |= 4;
        }

        if (*pStart == next_sub_block) {
            *pStart = (*pStart)->next;
        }

        if (*pStart == next_sub_block) {
            *pStart = 0;
        }

        next_sub_block->next->prev = next_sub_block->prev;
        next_sub_block->prev->next = next_sub_block->next;
    }
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
        *start = subblock->next;
    }
    if (*start == subblock) {
        *start = NULL;
        block->max_size = 0;
    } else {
        subblock->next->prev = subblock->prev;
        subblock->prev->next = subblock->next;
    }
}

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

void* __pool_realloc(__mem_pool* pool, void* ptr, unsigned long size)
{
    unsigned long current_size;
    void* newptr;

    if (ptr == 0)
        return __pool_alloc(pool, size);

    if (size == 0) {
        __pool_free(pool, ptr);
        return 0;
    }

    current_size = allocation_size(ptr);

    if (size > current_size) {
        if (classify_allocation(ptr)) {
            unsigned long sz;
            SubBlock* sb;
            Block* bp;

            if (size > 0xFFFFFFCF)
                return 0;

            sz = (size + 15) & ~7;
            if (sz < 80)
                sz = 80;

            sb = subblock_from_pointer(ptr);
            bp = subblock_owner(sb);
            merge_next(sb, block_start(bp));

            if (subblock_size(sb) >= sz) {
                if (subblock_size(sb) - sz >= 80) {
                    split_subblock(sb, sz);
                    Block_link(subblock_owner(sb), (SubBlock*)((char*)sb + sz));
                }
                return ptr;
            }
        }

        newptr = __pool_alloc(pool, size);
        if (newptr == 0)
            return 0;

        memcpy(newptr, ptr, current_size);
        __pool_free(pool, ptr);
        return newptr;
    }

    if (classify_allocation(ptr)) {
        SubBlock* sb;

        size = (size + 15) & ~7;
        if (size < 80)
            size = 80;

        sb = subblock_from_pointer(ptr);
        if (subblock_size(sb) - size >= 80) {
            split_subblock(sb, size);
            Block_link(subblock_owner(sb), (SubBlock*)((char*)sb + size));
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

static inline void construct_fixed_subblock(
    FixSubBlock* subblock, FixBlock* block, FixSubBlock* next)
{
    subblock->block = block;
    subblock->next = next;
}

static inline void initialize_fixed_pool_block(
    FixStart* start, FixBlock* block, unsigned long index,
    unsigned long available, FixSubBlock* buffer)
{
    FixBlock* head;
    FixBlock* tail;
    unsigned long stride;
    unsigned long count;
    unsigned long i;
    unsigned char* cursor;

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
    stride = fix_pool_sizes[index] + sizeof(FixBlock*);
    count = (available - sizeof(FixBlock)) / stride;
    cursor = (unsigned char*)buffer;
    for (i = 0; i < count - 1; i++) {
        unsigned char* next = cursor + stride;
        construct_fixed_subblock((FixSubBlock*)cursor, block,
                                (FixSubBlock*)next);
        cursor = next;
    }
    construct_fixed_subblock((FixSubBlock*)cursor, block, NULL);
    block->free = buffer;
    block->allocated = 0;
    start->head = block;
}

void* allocate_from_fixed_pools(MemPoolObj* pool, unsigned long size) {
    unsigned long index = 0;
    unsigned long count;
    unsigned long maximum_count;
    unsigned long largest;
    unsigned long available;
    FixStart* start;
    FixBlock* block;
    FixSubBlock* subblock;
    void* memory;

    while (size > fix_pool_sizes[index]) {
        index++;
    }
    start = &pool->fixed[index];
    block = start->head;
    if (block == NULL || block->free == NULL) {
        count = 0xFEC / (fix_pool_sizes[index] + sizeof(FixBlock*));
        if (count > 0x100) {
            count = 0x100;
        }
        maximum_count = count;
        while (count >= 10) {
            memory = soft_allocate_from_var_pools(
                pool, count * (fix_pool_sizes[index] + sizeof(FixBlock*)) +
                          sizeof(FixBlock),
                &largest);
            if (memory != NULL) {
                break;
            }
            if (largest > sizeof(FixBlock)) {
                count = (largest - sizeof(FixBlock)) /
                        (fix_pool_sizes[index] + sizeof(FixBlock*));
            } else {
                count = 0;
            }
        }
        if (memory == NULL && count < maximum_count) {
            memory = allocate_from_var_pools(
                pool, maximum_count *
                          (fix_pool_sizes[index] + sizeof(FixBlock*)) +
                          sizeof(FixBlock));
            if (memory == NULL) {
                return NULL;
            }
        }

        available = allocation_size(memory);
        block = memory;
        initialize_fixed_pool_block(start, block, index, available,
            (FixSubBlock*)(block + 1));
    }

    subblock = start->head->free;
    start->head->free = subblock->next;
    start->head->allocated++;
    if (start->head->free == NULL) {
        start->head = start->head->next;
        start->tail = start->tail->next;
    }
    return (unsigned char*)subblock + sizeof(FixBlock*);
}

static inline int block_is_empty(const Block* block) {
    const SubBlock* first = (const SubBlock*)(block + 1);
    unsigned long tag = first->size;
    int empty = 0;

    if (!(tag & 2) && (tag & ~7UL) == block_size(block) - 24) {
        empty = 1;
    }
    return empty;
}

static void deallocate_from_var_pools(MemPoolObj* pool, void* ptr) {
    SubBlock* subblock = (SubBlock*)((unsigned char*)ptr - 8);
    Block* block = (Block*)((unsigned long)subblock->block & ~1UL);

    Block_link(block, subblock);
    if (block_is_empty(block)) {
        unlink_block(pool, block);
        __sys_free(block);
    }
}

static void* soft_allocate_from_var_pools(
    MemPoolObj* pool, unsigned long size, unsigned long* largest) {
    Block* block;
    SubBlock* subblock;

    size = (size + 15) & ~7UL;

    if (size < 0x50) {
        size = 0x50;
    }
    *largest = 0;
    block = pool->start;
    if (block == NULL) {
        return NULL;
    }
    for (;;) {
        if (size <= block->max_size) {
            subblock = Block_subBlock(block, size);
            if (subblock != NULL) {
                pool->start = block;
                break;
            }
        }
        if (block->max_size > 8 && *largest < block->max_size - 8) {
            *largest = block->max_size - 8;
        }
        block = block->next;
        if (block == pool->start) {
            return NULL;
        }
    }
    return &subblock->prev;
}

static void* allocate_from_var_pools(MemPoolObj* pool, unsigned long size) {
    Block* block;
    SubBlock* subblock;
    unsigned long needed;

    needed = (size + 15) & ~7UL;

    if (needed < 0x50) {
        needed = 0x50;
    }
    block = pool->start != NULL ? pool->start : link_new_block(pool, needed);
    if (block == NULL) {
        return NULL;
    }
    while (1) {
        if (needed <= block->max_size) {
            subblock = Block_subBlock(block, needed);
            if (subblock != NULL) {
                pool->start = block;
                break;
            }
        }
        block = block->next;
        if (block == pool->start) {
            block = link_new_block(pool, needed);
            if (block == NULL) {
                return NULL;
            }
            subblock = Block_subBlock(block, needed);
            break;
        }
    }
    return &subblock->prev;
}

static Block* link_new_block(MemPoolObj* pool, unsigned long size) {
    Block* block;
    unsigned long block_size = (size + 31) & ~7UL;

    if (block_size < 0x10000) {
        block_size = 0x10000;
    }
    block = __sys_alloc(block_size);
    if (block == NULL) {
        return NULL;
    }
    Block_construct(block, block_size);
    if (pool->start != NULL) {
        block->prev = pool->start->prev;
        block->prev->next = block;
        block->next = pool->start;
        pool->start->prev = block;
        pool->start = block;
    } else {
        pool->start = block;
        block->prev = block;
        block->next = block;
    }
    return block;
}

static SubBlock* Block_subBlock(Block* block, unsigned long size) {
    SubBlock* start = *block_start(block);
    SubBlock* subblock;
    unsigned long max_size;
    unsigned long current_size;

    if (start == NULL) {
        block->max_size = 0;
        return NULL;
    }
    subblock = start;
    current_size = subblock_size(subblock);
    max_size = current_size;
    while (current_size < size) {
        subblock = subblock->next;
        current_size = subblock_size(subblock);
        if (max_size < current_size) {
            max_size = current_size;
        }
        if (subblock == start) {
            block->max_size = max_size;
            return NULL;
        }
    }

    if (current_size - size >= 0x50) {
        split_subblock(subblock, size);
    }

    unlink_subblock(block, subblock);
    return subblock;
}

static void Block_construct(Block* block, unsigned long size) {
    SubBlock* subblock = (SubBlock*)(block + 1);

    block->size = size | 3;
    *(unsigned long*)((unsigned char*)block + size - 2 * sizeof(unsigned long)) = block->size;
    construct_subblock(subblock, block,
        size - sizeof(Block) - 2 * sizeof(unsigned long), 0, 0);
    block->max_size = size - sizeof(Block) - 2 * sizeof(unsigned long);
    *block_start(block) = NULL;
    Block_link(block, subblock);
}
