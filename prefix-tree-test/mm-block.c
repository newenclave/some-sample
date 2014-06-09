
#include <memory.h>
#include <stdio.h>

#include "mm-block.h"

struct mm_block {

    union block_data {
        void *ptr_;
        char *str_;
    }        data_;

    size_t   capacity_;
    size_t   used_;
};

typedef struct mm_block mm_block_data_type;

static const size_t void_ptr_size      =   sizeof(void *);
static const size_t void_ptr_size_mask =   sizeof(void *) - 1;

#define MM_BLOCK_AT(ptr, pos) (((char *)ptr) + (pos))
#define MM_BLOCK_DEF_INC(size) (size + (size >> 1))

#define MM_BLOCK_AVAILABLE_LOCAL( mb ) ((mb)->capacity_ - (mb)->used_)

#define MM_BLOCK_FIX_SIZE( new_size )                               \
      ((new_size + void_ptr_size_mask) & (~(void_ptr_size_mask)))

#define MM_BLOCK_FIX_SIZE0( new_size )                              \
      ( new_size <= void_ptr_size)                                  \
      ? void_ptr_size                                               \
      : MM_BLOCK_FIX_SIZE( new_size )


static void *mm_block_memset (void *data, int c, size_t len )
{
    return memset( data, c, len );
}

static void *mm_block_realloc(void *ptr, size_t size)
{
//    static size_t i = 0;
//    printf( "%u realloc %p %u\n", i++, ptr, size );
    return realloc(ptr, size);
}

static void *mm_block_malloc(size_t size)
{
    return malloc(size);
}

static void mm_block_free_ptr(void *ptr)
{
    //printf( "free %p\n", ptr );
    free(ptr);
}

static void *mm_block_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy( dest, src, n );
}

static void *mm_block_memmove(void *dest, const void *src, size_t n)
{
    return memmove( dest, src, n );
}

static size_t mm_block_calc_prefer_size( size_t old_capa, size_t desired_size )
{
    size_t new_capa = MM_BLOCK_FIX_SIZE(MM_BLOCK_DEF_INC(old_capa));
    desired_size = MM_BLOCK_FIX_SIZE(desired_size);

    return ( new_capa > desired_size ) ? new_capa : desired_size;
}

struct mm_block *mm_block_new_reserved( size_t reserve_size )
{
    mm_block_data_type *new_block =
            (mm_block_data_type *)mm_block_malloc(sizeof(*new_block));

    size_t new_size = MM_BLOCK_FIX_SIZE0(reserve_size);

    if( new_block ) {

        new_block->data_.ptr_ = (char *)mm_block_malloc(new_size);

        if( new_block->data_.ptr_ ) {
            new_block->capacity_ = new_size;
            new_block->used_     = 0;
        } else {
            mm_block_free_ptr(new_block);
            new_block = NULL;
        }
    }

    return new_block;
}

/// mm_block_new
/// creating new memory block
mm_block_data_type *mm_block_new2( size_t init_size )
{
    mm_block_data_type *new_block = mm_block_new_reserved( init_size );

    if( new_block ) {
        new_block->used_ = init_size;
    }

    return new_block;
}

mm_block_data_type *mm_block_new( )
{
    return mm_block_new2( 0 );
}


struct mm_block *mm_block_new_copy( const struct mm_block *other )
{
    size_t new_size = other->used_;
    struct mm_block *new_block = mm_block_new2(new_size);
    if( new_block && (new_size > 0)) {
        mm_block_memcpy( new_block->data_.ptr_, other->data_.ptr_, new_size );
    }
    return new_block;
}

/// mm_block_free
/// free memory
void mm_block_free(mm_block_data_type *mb)
{
    if( mb ) {
        mm_block_free_ptr(mb->data_.ptr_);
        mm_block_free_ptr(mb);
    }
}

int mm_block_reserve(struct mm_block *mb, size_t new_size)
{
    char *new_data = mb->data_.ptr_;

    if( new_size > mb->capacity_ ) {

        new_size = mm_block_calc_prefer_size( mb->capacity_, new_size );
        new_data = (char *)mm_block_realloc( new_data, new_size );

        if( new_data ) {
            mb->data_.ptr_ = new_data;
            mb->capacity_  = new_size;
        }
    }

    return (new_data != NULL);
}

int mm_block_resize(struct mm_block *mb, size_t new_size)
{
    int result = mm_block_reserve( mb, new_size );
    if( result ) mb->used_ = new_size;
    return result;
}


int mm_block_resize2(struct mm_block *mb, size_t new_size, int c)
{
    size_t old_used = mb->used_;

    int result = mm_block_resize(mb, new_size);

    if( result && (old_used < new_size) )
        mm_block_memset(MM_BLOCK_AT(mb->data_.ptr_, old_used),
                                        c, new_size - old_used);

    return result;
}

size_t mm_block_size( const mm_block_data_type *mb )
{
    return mb->used_;
}

size_t mm_block_capacity( const struct mm_block *mb )
{
    return mb->capacity_;
}

size_t mm_block_available( const struct mm_block *mb )
{
    return MM_BLOCK_AVAILABLE_LOCAL( mb );
}

void *mm_block_begin( struct mm_block *mb )
{
    return mb->data_.ptr_;
}

void *mm_block_end( struct mm_block *mb )
{
    return MM_BLOCK_AT(mb->data_.ptr_, mb->used_);
}

void *mm_block_at( struct mm_block *mb, size_t position )
{
    return MM_BLOCK_AT(mb->data_.ptr_, position);
}

const void  *mm_block_const_begin (const struct mm_block *mb)
{
    return mb->data_.ptr_;
}

const void  *mm_block_const_end(const struct mm_block *mb)
{
    return MM_BLOCK_AT(mb->data_.ptr_, mb->used_);
}

const void  *mm_block_const_at(const struct mm_block *mb, size_t posision)
{
    return MM_BLOCK_AT(mb->data_.ptr_, posision);
}


void mm_block_clear(struct mm_block *mb)
{
    mb->used_ = 0;
}

void mm_block_swap(struct mm_block *lmb, struct mm_block *rmb)
{
    char   *tmp_data = lmb->data_.ptr_;
    size_t  tmp_used = lmb->used_;
    size_t  tmp_capa = lmb->capacity_;

    lmb->data_.ptr_  = rmb->data_.ptr_ ;
    lmb->used_       = rmb->used_ ;
    lmb->capacity_   = rmb->capacity_;

    rmb->data_.ptr_  = tmp_data;
    rmb->used_       = tmp_used;
    rmb->capacity_   = tmp_capa;
}

void mm_block_zero(struct mm_block *mb)
{
    mm_block_memset( mb->data_.ptr_, 0, mb->used_ );
}

int mm_block_concat(struct mm_block *lmb, const void *data, size_t len)
{
    size_t old_used = lmb->used_;
    if( 0 == mm_block_resize2(lmb, old_used + len, 0) ) return 0;
    mm_block_memcpy( MM_BLOCK_AT(lmb->data_.ptr_, old_used), data, len );
    return 1;
}

int mm_block_concat2(struct mm_block *lmb, const struct mm_block *rmb)
{
    return mm_block_concat(lmb, rmb->data_.ptr_, rmb->used_);
}

int mm_block_push_back(struct mm_block *mb, char c)
{
    size_t old_capa = mb->capacity_;
    if( mb->used_ >= old_capa ) {
        size_t new_capa = MM_BLOCK_DEF_INC( old_capa );
        if( !mm_block_reserve( mb, new_capa ) )
            return 0;
    }
    *MM_BLOCK_AT(mb->data_.ptr_, mb->used_++) = c;
    return 1;
}

void *mm_block_create_back( struct mm_block *mb, size_t count )
{
    size_t old_size = mb->used_;
    void *tail = NULL;
    int res = mm_block_resize( mb, old_size + count );
    if( 0 != res ) {
        tail = MM_BLOCK_AT(mb->data_.ptr_, old_size);
    }
    return tail;
}

void *mm_block_create_insertion( struct mm_block *mb,
                                 size_t position, size_t count )
{
    void *block = NULL;

    if( position == mb->used_ ) {

        block = mm_block_create_back( mb, count );

    } else if( MM_BLOCK_AVAILABLE_LOCAL( mb ) < count ) {

        size_t new_size
                = mm_block_calc_prefer_size( mb->capacity_, mb->used_ + count );

        struct mm_block *new_block = mm_block_new_reserved( new_size );

        if( new_block ) {
            new_block->used_ = mb->used_+count;
            size_t new_tail_shift  = position + count;
            if( position ) {
                mm_block_memcpy( new_block->data_.ptr_, mb->data_.ptr_,
                                                        position );
            }
            mm_block_memcpy( MM_BLOCK_AT(new_block->data_.ptr_, new_tail_shift),
                             MM_BLOCK_AT(mb->data_.ptr_, position),
                             mb->used_ - position);
            mm_block_swap( mb, new_block );
            mm_block_free( new_block );
            block = MM_BLOCK_AT(mb->data_.ptr_, position);
        }
    } else {
        void *from = MM_BLOCK_AT(mb->data_.ptr_ , position);
        mm_block_memmove( MM_BLOCK_AT(from, count), from, mb->used_ - position);
        mb->used_ += count;
        block = from;
    }

    return block;
}

void *mm_block_create_front( struct mm_block *mb, size_t count )
{
    return mm_block_create_insertion( mb, 0, count );
}

void *mm_block_delete( struct mm_block *mb, size_t position, size_t count )
{
    void  *tail_begin = MM_BLOCK_AT(mb->data_.ptr_,  position);
    size_t tail_len  = mb->used_ - (position + count);
    void *res = mm_block_memmove( tail_begin,
                                  MM_BLOCK_AT(tail_begin, count), tail_len );
    mb->used_ -= count;
    return res;
}

void mm_block_reduce( struct mm_block *mb, size_t count )
{
    mb->used_ -= count;
}

int mm_block_extend( struct mm_block *mb, size_t count )
{
    return mm_block_resize( mb, mb->used_ + count );
}
