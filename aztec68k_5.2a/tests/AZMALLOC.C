/*
 * Simple 4-byte-aligned allocator for Aztec C 68K.
 *
 * Uses:
 *     _mcur  current unallocated heap position
 *     _mtop  first byte beyond the heap
 *
 * Free blocks are kept in address order and coalesced.
 *
 * Replace Aztec's malloc/free/calloc/realloc together; do not mix allocators.
 * Aztec 68K 5.2a has a bug in lib\stdlib\amifree.c that corrupts the _Free (allocated) list, so don't use it.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct block {
    struct block *next;
    unsigned long size;          /* usable bytes following this header */
};

extern unsigned long _mcur;
extern unsigned long _mtop;

static struct block *free_list;

#define ALIGNMENT       4UL
#define HEADER_SIZE     ((unsigned long)sizeof(struct block))
#define MIN_SPLIT       (HEADER_SIZE + ALIGNMENT)

static unsigned long
align4(unsigned long n)
{
    return (n + (ALIGNMENT - 1UL)) & ~(ALIGNMENT - 1UL);
}

static void
insert_free(struct block *blk)
{
    struct block *prev;
    struct block *cur;
    unsigned long blk_end;

    prev = 0;
    cur = free_list;

    while (cur != 0 && cur < blk) {
        prev = cur;
        cur = cur->next;
    }

    blk->next = cur;

    /*
     * Merge with following block first.
     */
    if (cur != 0) {
        blk_end = (unsigned long)(blk + 1) + blk->size;
        if (blk_end == (unsigned long)cur) {
            blk->size += HEADER_SIZE + cur->size;
            blk->next = cur->next;
        }
    }

    /*
     * Then merge with preceding block.
     */
    if (prev != 0) {
        blk_end = (unsigned long)(prev + 1) + prev->size;
        if (blk_end == (unsigned long)blk) {
            prev->size += HEADER_SIZE + blk->size;
            prev->next = blk->next;
            return;
        }

        prev->next = blk;
    } else {
        free_list = blk;
    }
}

void *
malloc(size_t requested)
{
    struct block *prev;
    struct block *cur;
    struct block *remainder;
    unsigned long size;
    unsigned long total;
    unsigned long start;
    unsigned long remain;

    if (requested == 0)
        return 0;

    size = align4((unsigned long)requested);

    /*
     * Detect rounding or header-addition overflow.
     */
    if (size < (unsigned long)requested)
        return 0;
    if (size > ULONG_MAX - HEADER_SIZE)
        return 0;

    total = HEADER_SIZE + size;

    /*
     * First fit from the free list.
     */
    prev = 0;
    cur = free_list;

    while (cur != 0) {
        if (cur->size >= size) {
            remain = cur->size - size;

            if (remain >= MIN_SPLIT) {
                /*
                 * Keep the front portion allocated and place the remainder
                 * back in the free list.
                 */
                remainder = (struct block *)
                    ((char *)(cur + 1) + size);
                remainder->size = remain - HEADER_SIZE;
                remainder->next = cur->next;

                if (prev != 0)
                    prev->next = remainder;
                else
                    free_list = remainder;

                cur->size = size;
            } else {
                /*
                 * The leftover space is too small to be useful.
                 */
                if (prev != 0)
                    prev->next = cur->next;
                else
                    free_list = cur->next;
            }

            cur->next = 0;
            return (void *)(cur + 1);
        }

        prev = cur;
        cur = cur->next;
    }

    /*
     * Nothing reusable was large enough. Extend from _mcur.
     */
    start = align4(_mcur);

    if (start < _mcur)
        return 0;
    if (start > _mtop)
        return 0;
    if (total > _mtop - start)
        return 0;

    cur = (struct block *)start;
    cur->next = 0;
    cur->size = size;

    _mcur = start + total;

    return (void *)(cur + 1);
}

void
free(void *ptr)
{
    struct block *blk;

    if (ptr == 0)
        return;

    blk = ((struct block *)ptr) - 1;
    insert_free(blk);
}

void *
calloc(size_t count, size_t item_size)
{
    size_t total;
    void *ptr;

    if (count != 0 && item_size > (size_t)(ULONG_MAX / count))
        return 0;

    total = count * item_size;
    ptr = malloc(total);

    if (ptr != 0)
        memset(ptr, 0, total);

    return ptr;
}

void *
realloc(void *ptr, size_t requested)
{
    struct block *blk;
    struct block *tail;
    void *new_ptr;
    unsigned long old_size;
    unsigned long new_size;
    unsigned long remain;
    size_t copy_size;

    if (ptr == 0)
        return malloc(requested);

    if (requested == 0) {
        free(ptr);
        return 0;
    }

    new_size = align4((unsigned long)requested);
    if (new_size < (unsigned long)requested)
        return 0;

    blk = ((struct block *)ptr) - 1;
    old_size = blk->size;

    /*
     * Shrink in place, optionally returning the tail to the free list.
     */
    if (new_size <= old_size) {
        remain = old_size - new_size;

        if (remain >= MIN_SPLIT) {
            blk->size = new_size;

            tail = (struct block *)((char *)(blk + 1) + new_size);
            tail->size = remain - HEADER_SIZE;
            tail->next = 0;
            insert_free(tail);
        }

        return ptr;
    }

    /*
     * Simple implementation: grow by allocate/copy/free.
     */
    new_ptr = malloc(requested);
    if (new_ptr == 0)
        return 0;

    copy_size = (size_t)old_size;
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);

    return new_ptr;
}
