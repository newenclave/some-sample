#include <stdio.h>
#include <string.h>

#include "prefix-tree.h"
#include "mm-block.h"

#define COLOR_PAIR( Name, Value ) { Name, Value }

static struct {

    const char *name_;
    const char *value_;

} const global_colors[ ] = {

      COLOR_PAIR( "black",      "\x1b[30;1m" )
     ,COLOR_PAIR( "red",        "\x1b[31;1m" )
     ,COLOR_PAIR( "green",      "\x1b[32;1m" )
     ,COLOR_PAIR( "yellow",     "\x1b[1;33m" )
     ,COLOR_PAIR( "orange",     "\x1b[33;1m" )
     ,COLOR_PAIR( "brown",      "\x1b[0;33m" )
     ,COLOR_PAIR( "blue",       "\x1b[34;1m" )
     ,COLOR_PAIR( "purple",     "\x1b[35;1m" )
     ,COLOR_PAIR( "lightblue",  "\x1b[36;1m" )
     ,COLOR_PAIR( "white",      "\x1b[37;1m" )

     ,COLOR_PAIR( "Black",      "\x1b[30;1m" )
     ,COLOR_PAIR( "Red",        "\x1b[31;1m" )
     ,COLOR_PAIR( "Green",      "\x1b[32;1m" )
     ,COLOR_PAIR( "Yellow",     "\x1b[1;33m" )
     ,COLOR_PAIR( "Orange",     "\x1b[33;1m" )
     ,COLOR_PAIR( "Brown",      "\x1b[0;33m" )
     ,COLOR_PAIR( "Blue",       "\x1b[34;1m" )
     ,COLOR_PAIR( "Purple",     "\x1b[35;1m" )
     ,COLOR_PAIR( "Lightblue",  "\x1b[36;1m" )
     ,COLOR_PAIR( "White",      "\x1b[37;1m" )

};

static const size_t global_count =
             sizeof(global_colors) / sizeof(global_colors[0]);

static const char * cp_none            = "\x1b[0m";

struct value_data {
    const char *color_ptr_;
    size_t length_;
};

struct value_data *create_data( const char *color )
{
    struct value_data *value = (struct value_data *)malloc( sizeof(*value) );
    if( value ) {
        value->color_ptr_ = color;
        value->length_    = strlen(color);
    }
    return value;
}

int fill_trie( struct prefix_tree *trie )
{

    size_t i;
    int result = 1;

    for( i=0; i<global_count && result; ++i ) {
        struct value_data *value = create_data( global_colors[i].value_ );
        if( value ) {
            prefix_tree_insert_string( trie, global_colors[i].name_, value );
        } else {
            result = 0;
        }
    }
    return result;
}

int print_colored_line( const struct prefix_tree *trie, const char *line )
{
    struct mm_block *tmp_str = mm_block_new_reserved( 1024 );
    int result = 1;

    const size_t none_length = strlen(cp_none);

    if( !tmp_str ) {
        fprintf( stderr, "Create mm_block failed\n" );
        return 0;
    }

    const char *p = line;
    size_t len = strlen( p );

    while ( len != 0 ) {

        const char *old_p = p;

        struct value_data *value = (struct value_data *)
                prefix_tree_get_next( trie, (const void **)&p, &len );

        if( value ) {
            mm_block_concat( tmp_str, value->color_ptr_, value->length_ );
            mm_block_concat( tmp_str, old_p, p - old_p );
            mm_block_concat( tmp_str, cp_none, none_length );
       } else {
            mm_block_push_back( tmp_str, *p++ );
            --len;
        }
    }

    mm_block_push_back( tmp_str, '\0' );

    fputs( (const char *)mm_block_const_begin( tmp_str ), stdout );

    mm_block_free( tmp_str );

    return result;
}

int main( void )
{
    struct prefix_tree *trie = prefix_tree_new2( free );
    int res;

    if( !trie ) {
        fprintf( stderr, "Create trie failed\n" );
        return 1;
    }

    res = fill_trie( trie );

    if( res ) {

        char block[4096 + 1];

        while( fgets( block, 4096, stdin ) ) {
            print_colored_line( trie, &block[0] );
        }

    } else {
        fprintf( stderr, "Fill trie failed\n" );
    }

    prefix_tree_free( trie );
    return res;
}

