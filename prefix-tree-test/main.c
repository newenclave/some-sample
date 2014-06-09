#include <stdio.h>
#include <string.h>

#include "prefix-tree.h"
#include "mm-block.h"

static const char * cp_black           = "\x1b[30;1m";
static const char * cp_red             = "\x1b[31;1m";
static const char * cp_green           = "\x1b[32;1m";
static const char * cp_yellow          = "\x1b[33;1m";
static const char * cp_orange          = "\x1b[33;1m";
static const char * cp_blue            = "\x1b[34;1m";
static const char * cp_purple          = "\x1b[35;1m";
static const char * cp_lightblue       = "\x1b[36;1m";
static const char * cp_white           = "\x1b[37;1m";

static const char * cp_none            = "\x1b[0m";

static const char * cn_black           = "black";
static const char * cn_red             = "red";
static const char * cn_green           = "green";
static const char * cn_yellow          = "yellow";
static const char * cn_orange          = "orange";
static const char * cn_blue            = "blue";
static const char * cn_purple          = "purple";
static const char * cn_lightblue       = "lightblue";
static const char * cn_white           = "white";

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
    struct color_pair {
        const char *name_;
        const char *value_;
    } const colors[ ] = {
         { cn_black,     cp_black     }
        ,{ cn_red,       cp_red       }
        ,{ cn_green,     cp_green     }
        ,{ cn_yellow,    cp_yellow    }
        ,{ cn_orange,    cp_orange    }
        ,{ cn_blue,      cp_blue      }
        ,{ cn_purple,    cp_purple    }
        ,{ cn_lightblue, cp_lightblue }
        ,{ cn_white,     cp_white     }
    };

    const size_t csize = sizeof( colors ) / sizeof(colors[0]);
    size_t i;
    int result = 1;

    for( i=0; i<csize && result; ++i ) {
        struct value_data *value = create_data( colors[i].value_ );
        if( value ) {
            prefix_tree_insert_string( trie, colors[i].name_, value );
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

