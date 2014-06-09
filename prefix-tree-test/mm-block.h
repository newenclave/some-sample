#if !defined(MM_BLOCK_20131026_H)
#define MM_BLOCK_20131026_H

#include <stdlib.h>

struct mm_block;

struct mm_block *mm_block_new( );
struct mm_block *mm_block_new2( size_t init_size );
struct mm_block *mm_block_new_reserved( size_t reserve_size );
struct mm_block *mm_block_new_copy( const struct mm_block *other );
void   mm_block_free(struct mm_block *mb);

size_t mm_block_size      (const struct mm_block *mb);
size_t mm_block_capacity  (const struct mm_block *mb);
size_t mm_block_available (const struct mm_block *mb);

void  *mm_block_begin     (struct mm_block *mb);
void  *mm_block_end       (struct mm_block *mb);
void  *mm_block_at        (struct mm_block *mb, size_t position);

const void  *mm_block_const_begin (const struct mm_block *mb);
const void  *mm_block_const_end   (const struct mm_block *mb);
const void  *mm_block_const_at    (const struct mm_block *mb, size_t position);

int  mm_block_resize  (struct mm_block *mb, size_t new_size);
int  mm_block_resize2 (struct mm_block *mb, size_t new_size, int c);
int  mm_block_reserve (struct mm_block *mb, size_t new_size);
void mm_block_clear   (struct mm_block *mb);
void mm_block_swap    (struct mm_block *lmb, struct mm_block *rmb);
void mm_block_zero    (struct mm_block *mb);

int mm_block_append   (struct mm_block *mb, const void *data, size_t len);
int mm_block_append2  (struct mm_block *mb, const struct mm_block *cmb);

int    mm_block_push_back( struct mm_block *mb, char c);
void  *mm_block_create_back( struct mm_block *mb, size_t count );
void  *mm_block_create_front( struct mm_block *mb, size_t count );
void  *mm_block_create_insertion( struct mm_block *mb,
                                  size_t position, size_t count );

void *mm_block_delete( struct mm_block *mb, size_t position, size_t count );
void  mm_block_reduce( struct mm_block *mb, size_t count );
int   mm_block_extend( struct mm_block *mb, size_t count );

#endif // MM_BLOCK_20131026_H
