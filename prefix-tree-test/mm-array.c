
#include <string.h>
#include "mm-array.h"
#include "mm-block.h"

struct mm_array {
    struct mm_block       *mmblock_;
    mm_array_element_free  free_;
    size_t                 element_size_;
};

#define MM_ARRAY_AT_LOCAL( arr, index )                 \
    ((char *)mm_block_at((arr)->mmblock_, (index)*(arr)->element_size_))

#define MM_ELEMENTS_SIZE( arr, count )                  \
        ((arr)->element_size_ * (count))

#define MM_ARRAY_AT_LOCAL_CONST( arr, index )           \
    ((char *)mm_block_const_at((arr)->mmblock_, MM_ELEMENTS_SIZE(arr, index)))

#define MM_ELEMENT_SHIFT( ptr, element_size, count )    \
    (((char *)ptr) + ((element_size) * (count)))

#define MM_ELEMENT_NEXT( ptr, element_size )    \
    (((char *)ptr) + (element_size))

#define MM_ELEMENT_PREV( ptr, element_size )    \
    (((char *)ptr) - (element_size))


struct mm_array *mm_array_new_reserved2( size_t element_size, size_t count,
                                         mm_array_element_free free_call)
{
    struct mm_array *new_data =
        (struct mm_array *)malloc( sizeof *new_data );

    if( new_data ) {
        new_data->mmblock_ = mm_block_new_reserved( count * element_size );
        if( new_data->mmblock_ ) {
            new_data->element_size_ = element_size;
            new_data->free_         = free_call;
        } else {
            free( new_data );
            new_data = NULL;
        }
    }
    return new_data;

}

struct mm_array *mm_array_new_reserved( size_t element_size, size_t count )
{
    return mm_array_new_reserved2( element_size, count, NULL );
}

struct mm_array *mm_array_new3( size_t element_size, size_t count,
                                     mm_array_element_free free_call)
{
    struct mm_array *new_data =
        mm_array_new_reserved2( element_size, count, free_call );
    if( new_data ) {
        mm_block_resize( new_data->mmblock_, element_size * count );
    }
    return new_data;
}


struct mm_array *mm_array_new2( size_t element_size, size_t count )
{
    return mm_array_new3( element_size, count, NULL );
}

struct mm_array *mm_array_new( size_t element_size )
{
    return mm_array_new3( element_size, 0, NULL );
}

void mm_array_swap( struct mm_array *mar, struct mm_array *other )
{
    mm_array_element_free tfree = mar->free_;
    size_t                esize = mar->element_size_;
    struct mm_block      *block = mar->mmblock_;

    mar->free_           = other->free_;
    mar->element_size_   = other->element_size_;
    mar->mmblock_        = other->mmblock_;

    other->free_         = tfree;
    other->element_size_ = esize;
    other->mmblock_      = block;

}

void mm_array_free_interval( struct mm_array *mar,
                             size_t position, size_t count,
                             mm_array_element_free free_call )
{
    if( free_call ) {
        size_t size = mar->element_size_;
        void *begin = MM_ARRAY_AT_LOCAL( mar, position );
        void *end   = MM_ARRAY_AT_LOCAL( mar, position + count );
        for( ; begin != end; begin = MM_ELEMENT_NEXT( begin, size ) ) {
            free_call( begin );
        }
    }
}

void mm_array_free2( struct mm_array *mar,
                     mm_array_element_free free_call )
{
    if( mar ) {
        mm_array_free_interval( mar, 0, mm_array_size(mar), free_call );
        mm_block_free( mar->mmblock_ );
        free( mar );
    }
}

void mm_array_free( struct mm_array *mar )
{
    if( mar ) mm_array_free2(mar, mar->free_);
}

void mm_array_set_free( struct mm_array *mar,
                        mm_array_element_free free_call )
{
    mar->free_ = free_call;
}

void *mm_array_begin( struct mm_array *mar )
{
    return mm_block_begin( mar->mmblock_ );
}

void  *mm_array_end( struct mm_array *mar )
{
    return mm_block_end( mar->mmblock_ );
}

void  *mm_array_at( struct mm_array *mar, size_t element_index )
{
    return MM_ARRAY_AT_LOCAL( mar, element_index );
}

const void *mm_array_const_begin( const struct mm_array *mar )
{
    return mm_block_const_begin( mar->mmblock_ );
}

const void *mm_array_const_end( const struct mm_array *mar )
{
    return mm_block_const_end( mar->mmblock_ );
}

const void *mm_array_const_at(const struct mm_array *mar, size_t element_index)
{
    return MM_ARRAY_AT_LOCAL_CONST( mar, element_index );
}

size_t mm_array_size( const struct mm_array *mar )
{
    return (mm_block_size( mar->mmblock_ ) / mar->element_size_);
}

size_t mm_array_element_size( const struct mm_array *mar )
{
    return mar->element_size_;
}

void *mm_array_create_back( struct mm_array *mar, size_t count )
{
    void *tail = mm_block_create_back( mar->mmblock_,
                                       MM_ELEMENTS_SIZE(mar, count));
    return tail;
}

void mm_array_copy_elements( void *dst, void *src,
                             size_t element_size, size_t count,
                             mm_array_element_copy copy_call)
{
    if( dst && copy_call ) {
        while ( count-- ) {
            copy_call( dst, src, element_size );
            dst = MM_ELEMENT_NEXT( dst, element_size );
            src = MM_ELEMENT_NEXT( src, element_size );
        }
    }
}

int mm_array_push_back2( struct mm_array *mar, void *element, size_t count)
{
    int res = mm_block_concat(mar->mmblock_, element,
                              MM_ELEMENTS_SIZE(mar, count));
    return res;
}

int mm_array_push_back( struct mm_array *mar, void *element )
{
    return mm_array_push_back2( mar, element, 1 );
}

int mm_array_push_back3( struct mm_array *mar,
                         void *element, size_t count,
                         mm_array_element_copy copy_call)
{
    void *tail = mm_block_create_back( mar->mmblock_,
                                       MM_ELEMENTS_SIZE(mar, count));
    mm_array_copy_elements(tail, element, mar->element_size_, count, copy_call);
    return (tail != NULL);
}

void *mm_array_create_front( struct mm_array *mar, size_t count )
{
    void *head = mm_block_create_front(mar->mmblock_,
                                       MM_ELEMENTS_SIZE(mar, count));
    return head;
}

int mm_array_push_front2 ( struct mm_array *mar, void *element,
                           size_t count )
{
    void *head = mm_array_create_front( mar, count );
    if( head ) {
        memcpy( head, element, MM_ELEMENTS_SIZE( mar, count) );
    }
    return (head != NULL);
}

int mm_array_push_front  ( struct mm_array *mar, void *element )
{
    return mm_array_push_front2( mar, element, 1 );
}

int mm_array_push_front3 ( struct mm_array *mar,
                              void *element, size_t count,
                              mm_array_element_copy copy_call)
{
    void *head = mm_array_create_front( mar, count );
    mm_array_copy_elements(head, element, mar->element_size_, count, copy_call);
    return (head != NULL);
}

int mm_array_resize2( struct mm_array *mar, size_t new_count,
                      mm_array_element_free free_call)
{
    size_t count = mm_array_size( mar );
    int res = 0;
    if( new_count < count ) {
        mm_array_free_interval( mar, new_count, count - new_count, free_call );
    }
    res = mm_block_resize( mar->mmblock_, MM_ELEMENTS_SIZE(mar, new_count));
    return res;
}

int mm_array_resize( struct mm_array *mar, size_t new_count )
{
    return mm_array_resize2( mar, new_count, mar->free_ );
}

int mm_array_reserve( struct mm_array *mar, size_t count )
{
    return mm_block_reserve( mar->mmblock_, MM_ELEMENTS_SIZE(mar, count) );
}

size_t mm_array_available ( const struct mm_array *mar)
{
    return (mm_block_available( mar->mmblock_ ) / mar->element_size_);
}

static void *mm_array_memcopy( void *dst, const void *src, size_t size )
{
    return memcpy( dst, src, size );
}

void  *mm_array_create_insertion( struct mm_array *mar,
                                  size_t pos, size_t count )
{
    void *insertion =
        mm_block_create_insertion( mar->mmblock_,
               MM_ELEMENTS_SIZE( mar, pos ),
               MM_ELEMENTS_SIZE( mar, count ));
    return insertion;
}

int mm_array_insert2 ( struct mm_array *mar, void *element,
                              size_t pos, size_t count )
{
    void * insertion = mm_array_create_insertion( mar, pos, count );
    if( insertion ) {
        mm_array_memcopy( insertion, element, MM_ELEMENTS_SIZE(mar, count));
    }
    return (insertion != NULL);
}

int mm_array_insert ( struct mm_array *mar,
                       void *element, size_t pos )
{
    (void)(pos);
    return mm_array_insert2( mar, element, pos, 1 );
}

int mm_array_insert3 ( struct mm_array *mar,
                       void *element, size_t pos, size_t count,
                       mm_array_element_copy copy_call)
{
    void * insertion = mm_array_create_insertion( mar, pos, count );
    if( insertion ) {
        mm_array_copy_elements( insertion, element,
                                mar->element_size_, count, copy_call );
    }
    return (insertion != NULL);
}

static int mm_array_bin_bound( const struct mm_array *mar,
                               const void  *element,
                               mm_array_compare cmp_call,
                               size_t  *position )
{
    size_t right  =  mm_array_size( mar );
    size_t left   =  0;
    size_t middle =  0;
    int cmp       = -1;
    while( (right != left) && (cmp != 0) ) {

        middle = left + ((right - left) >> 1);

        cmp = cmp_call( element, mm_array_const_at( mar, middle ),
                                                    mar->element_size_ );

        if( cmp != 0 ) {
            if( cmp < 0 ) {
                right = middle;
            } else  {
                middle++;
                left  = middle;
            }
        }
    }
    *position = middle;
    return (cmp == 0);
}


size_t mm_array_bin_lower_bound( const struct mm_array *mar,
                                 const void  *element,
                                 mm_array_compare cmp_call)
{
    size_t right  =  mm_array_size( mar );
    size_t left   =  0;
    size_t middle =  0;
    int cmp       = -1;

    while( right != left ) {

        middle = left + ((right - left) >> 1);

        cmp = cmp_call( element, mm_array_const_at( mar, middle ),
                                                    mar->element_size_ );

        if( cmp <= 0 ) {
            right =   middle;
        } else  {
            left  = ++middle;
        }
    }
    return middle;
}

size_t mm_array_bin_upper_bound( const struct mm_array *mar,
                                 const void  *element,
                                 mm_array_compare cmp_call)
{
    size_t right  =  mm_array_size( mar );
    size_t left   =  0;
    size_t middle =  0;
    int cmp       = -1;

    while( right != left ) {

        middle = left + ((right - left) >> 1);

        cmp = cmp_call( element, mm_array_const_at( mar, middle ),
                                                        mar->element_size_ );

        if( cmp < 0 ) {
            right =   middle;
        } else  {
            left  = ++middle;
        }
    }
    return middle;
}


void  *mm_array_bin_find( struct mm_array *mar, void *element,
                          mm_array_compare cmp_call)
{
    size_t pos = 0;
    int res = mm_array_bin_bound( mar, element, cmp_call, &pos );
    return res ? MM_ARRAY_AT_LOCAL( mar, pos ) : NULL;
}

const void *mm_array_const_bin_find( const struct mm_array *mar, void *element,
                                     mm_array_compare cmp_call)
{
    size_t pos = 0;
    int res = mm_array_bin_bound( mar, element, cmp_call, &pos );
    return res ? MM_ARRAY_AT_LOCAL_CONST( mar, pos ) : NULL;
}

void *mm_array_bin_insert2( struct mm_array *mar, void *element,
                             mm_array_compare cmp_call,
                             mm_array_element_copy copy_call)
{
    size_t   pos = 0;
    void *result = NULL;
    mm_array_bin_bound( mar, element, cmp_call, &pos );
    if( mm_array_insert3(mar, element, pos, 1, copy_call) )
        result = MM_ARRAY_AT_LOCAL( mar, pos );

    return result;
}


void  *mm_array_bin_insert( struct mm_array *mar, void *element,
                         mm_array_compare cmp_call)
{
    return mm_array_bin_insert2( mar, element, cmp_call, mm_array_memcopy );
}


void *mm_array_erase2( struct mm_array *mar, size_t position, size_t count,
                         mm_array_element_free free_call)
{
    mm_array_free_interval( mar, position, count, free_call );
    return mm_block_delete( mar->mmblock_,
                            MM_ELEMENTS_SIZE(mar, position),
                            MM_ELEMENTS_SIZE(mar, count));
}

void *mm_array_erase( struct mm_array *mar, size_t position, size_t count )
{
    return mm_array_erase2( mar, position, count, mar->free_ );
}

void mm_array_reduce2( struct mm_array *mar, size_t count,
                         mm_array_element_free free_call)
{
    mm_array_free_interval( mar, mm_array_size( mar )-count, count, free_call );
    mm_block_reduce( mar->mmblock_, MM_ELEMENTS_SIZE( mar, count ) );
}

void mm_array_reduce( struct mm_array *mar, size_t count )
{
    mm_array_reduce2( mar, count, mar->free_ );
}

void mm_array_reduce_from2( struct mm_array *mar, size_t position,
                              mm_array_element_free free_call)
{
    mm_array_reduce2( mar, mm_array_size( mar )-position, free_call );
}

void mm_array_reduce_from ( struct mm_array *mar, size_t position )
{
    mm_array_reduce_from2( mar, position, mar->free_ );
}

int mm_array_extend( struct mm_array *mar, size_t count )
{
    return mm_array_resize( mar, mm_array_size( mar ) + count );
    //return mm_block_extend( mar->mmblock_, MM_ELEMENTS_SIZE( mar, count ) );
}
