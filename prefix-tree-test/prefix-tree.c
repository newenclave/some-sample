#include "prefix-tree.h"
#include "mm-array.h"

enum prefix_tree_flags {
    PT_FLAG_FINAL = 1
};

union pt_key_type {
    char  k8_;
//    short k16_;
};

struct pt_key_info {
    union pt_key_type   key_;
    unsigned short      flags_;
    struct prefix_tree *parent_;
    void               *data_;
    struct mm_array    *next_keys_;
};

typedef int    (*pt_key_comparator)( const void *lptr, const void *rptr,
                                     size_t element_size );

typedef size_t (*pt_key_length)( const void *key );
typedef void   (*pt_key_setter)( struct pt_key_info *e, const void *k);

struct pt_key_tools_type {
    size_t             element_size_;
    pt_key_comparator  cmp_;
    pt_key_setter      set_;
    pt_key_length      len_;
};

static int pt_key_compare_8( const void *lptr, const void *rptr, size_t size );
static void pt_key_set_8( struct pt_key_info *e, const void *k );
static size_t pt_key_lenght_8( const void *k );

static const struct pt_key_tools_type s_pt_key_tools[ ] = {
    { 1, pt_key_compare_8, pt_key_set_8, pt_key_lenght_8 }
};

struct prefix_tree {
    struct mm_array                 *root_keys_;
    prefix_tree_data_free            free_;
    const struct pt_key_tools_type  *ktools_;
};

static int pt_key_compare_8( const void *lptr, const void *rptr, size_t size)
{
    (void)(size);
    const struct pt_key_info *l = (const struct pt_key_info *)lptr;
    const struct pt_key_info *r = (const struct pt_key_info *)rptr;

    return (l->key_.k8_ < r->key_.k8_) ? -1 : (r->key_.k8_ < l->key_.k8_);
}

static void pt_key_set_8( struct pt_key_info *e, const void *k )
{
    e->key_.k8_ = *((const char *)k);
}

static size_t pt_key_lenght_8( const void *k )
{
    const char *p = (const char *)k;
    size_t len = 0;
    while( *p++ ) len++;
    return len;
}

static struct pt_key_info *pt_key_get( const struct pt_key_tools_type *tools,
                                       struct mm_array *key_map,
                                       const void *k )
{
    struct pt_key_info tmp;
    tools->set_( &tmp, k );
    struct pt_key_info *res =
            (struct pt_key_info *)mm_array_bin_find(key_map, &tmp, tools->cmp_);
    return res;
}

static void pt_free_keys(struct mm_array *keys, prefix_tree_data_free free_call)
{
    if( free_call ) {
        size_t count = mm_array_size( keys );
        struct pt_key_info *next;
        while( count-- ) {
            next = (struct pt_key_info *)mm_array_at(keys, count);
            if( next->data_ )
                free_call( next->data_ );
        }
    }
}

static void pt_free_key_pair( void *ptr )
{
    struct pt_key_info *kp = (struct pt_key_info *)(ptr);
    if( kp->next_keys_ ) {
        pt_free_keys( kp->next_keys_, kp->parent_->free_ );
        mm_array_free(kp->next_keys_);
    }
}

static struct mm_array *pt_new_keys(  )
{
    struct mm_array *new_keys =
            mm_array_new_reserved(sizeof(struct pt_key_info), 1);
    if( new_keys ) {
        mm_array_set_free( new_keys, pt_free_key_pair );
    }
    return new_keys;
}


struct prefix_tree *prefix_tree_new2( prefix_tree_data_free free_call )
{
    struct prefix_tree *new_tree =
            (struct prefix_tree *)malloc(sizeof(*new_tree));
    if( new_tree ) {
        if( (new_tree->root_keys_ = pt_new_keys( )) ) {
            new_tree->ktools_     = &s_pt_key_tools[0];
            new_tree->free_       = free_call;
        } else {
            free( new_tree );
            new_tree = NULL;
        }
    }
    return new_tree;
}

struct prefix_tree *prefix_tree_new( )
{
    return prefix_tree_new2( NULL );
}

void prefix_tree_set_free( struct prefix_tree *pt,
                           prefix_tree_data_free free_call)
{
    pt->free_ = free_call;
}


void prefix_tree_free( struct prefix_tree *pt )
{
    if( pt ) {
        pt_free_keys( pt->root_keys_, pt->free_ );
        mm_array_free( pt->root_keys_ );
        free(pt);
    }
}

void prefix_tree_free2( struct prefix_tree *pt,
                        prefix_tree_data_free free_call)
{
    if( pt ) {
        prefix_tree_set_free( pt, free_call );
        prefix_tree_free( pt );
    }
}

static struct pt_key_info *prefix_tree_next( const struct prefix_tree *pt,
                                             const void **stream,
                                             size_t *available,
                                             int greedy )

{
    const char *p      = *((const char **)stream);
    const char *p_last = p;
    size_t len         = *available;
    size_t len_last    = len;

    struct mm_array    *key_map  = pt->root_keys_;
    struct pt_key_info *element  = NULL;
    struct pt_key_info *result   = NULL;

    const size_t esize = pt->ktools_->element_size_;

    while( key_map && len-- ) {

        element = pt_key_get( pt->ktools_, key_map, p );
        p += esize;

        if( element && (element->flags_ & PT_FLAG_FINAL) ) {
            result   = element;
            len_last = len;
            p_last   = p;
            if( !greedy ) break;
        }

        key_map = element ? element->next_keys_ : NULL;
    }

    if( result ) {
        *stream = p_last;
        *available = len_last;
    }

    return result;
}

void *prefix_tree_get_next( const struct prefix_tree *pt,
                            const void **stream, size_t *available )
{
    struct pt_key_info * res = prefix_tree_next( pt, stream, available, 1 );
    return res ? res->data_ : NULL;
}

int prefix_tree_insert( struct prefix_tree *pt,
                        const void *key, size_t length, void *data )
{
    struct mm_array *key_map = pt->root_keys_;
    int result = 1;
    const struct pt_key_tools_type *tools = pt->ktools_;

    const size_t esize = tools->element_size_;
    const char *next = (const char *)key;
    struct pt_key_info tmp;

    while( length-- && result ) {

        tools->set_( &tmp, next );

        struct pt_key_info *element =
            (struct pt_key_info *)mm_array_bin_find(key_map,
                                                    &tmp, tools->cmp_);
        if( !element ) {
            tmp.flags_      = length ? 0 : PT_FLAG_FINAL;
            tmp.data_       = length ? NULL : data;
            tmp.next_keys_  = NULL;
            tmp.parent_     = pt;
            element = mm_array_bin_insert( key_map, &tmp, tools->cmp_ );
            result = (element != NULL);
        } else {
            if( 0 == length ) {
                if(  element->data_
                     && (element->flags_ & PT_FLAG_FINAL)
                     && pt->free_ )
                {
                    pt->free_( element->data_ );
                }

                element->data_   = data;
                element->flags_ |= PT_FLAG_FINAL;
                result = 1;
            }
        }
        if( element && length ) {
            if( !element->next_keys_ ) {
                element->next_keys_ = pt_new_keys( );
            }
            key_map = element->next_keys_;
            result = (key_map != NULL);
        }
        next += esize;
    }

    return result;
}

int prefix_tree_insert_string( struct prefix_tree *pt,
                          const char *key_string, void *data )
{
    return prefix_tree_insert( pt,
                key_string, pt->ktools_->len_(key_string), data);
}

