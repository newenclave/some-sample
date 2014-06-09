#ifndef PREFIXTREE_20131109_H
#define PREFIXTREE_20131109_H

#include <stdlib.h>

struct prefix_tree;

typedef void (* prefix_tree_data_free)( void *data );

struct prefix_tree *prefix_tree_new( );
struct prefix_tree *prefix_tree_new2( prefix_tree_data_free free_call );

void   prefix_tree_free( struct prefix_tree *pt );
void   prefix_tree_free2( struct prefix_tree *pt,
                          prefix_tree_data_free free_call);

void   prefix_tree_set_free( struct prefix_tree *pt,
                             prefix_tree_data_free free_call);

int   prefix_tree_insert( struct prefix_tree *pt,
                          const void *key, size_t length, void *data );

int   prefix_tree_insert_string( struct prefix_tree *pt,
                          const char *key_string, void *data );

void *prefix_tree_get_next( const struct prefix_tree *pt,
                            const void **stream, size_t *length );

#endif // PREFIXTREE_20131109_H
