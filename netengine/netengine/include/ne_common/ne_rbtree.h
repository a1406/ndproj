#ifndef _NDRBTREE_H_
#define _NDRBTREE_H_

#include <stddef.h>

#define container_of(ptr, type, member) ({             \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);     \
         (type *)( (char *)__mptr - offsetof(type,member) );})

typedef NEINT32 key_t;
typedef void *data_t;

typedef enum color_t
{
    RED = 0,
    BLACK = 1
}color_t;

typedef struct rb_node_t
{
    struct rb_node_t *left, *right, *parent;
    key_t key;
    data_t data;
    color_t color;
}rb_node_t;

typedef void (*rb_travel_func)(data_t, void *);
void rb_travel(rb_node_t *root, rb_travel_func f, void *param);

/* forward declaration */
rb_node_t* rb_insert(key_t key, data_t data, rb_node_t* root);
rb_node_t* rb_search(key_t key, rb_node_t* root);
rb_node_t* rb_erase(key_t key, rb_node_t* root);
NEUINT32 calc_hashnr(const NEINT8 *key, NEUINT32 length);

//key is length, data is mem, use mem[length] to compare
rb_node_t* rb_insert_v2(key_t len, data_t data, rb_node_t* root);
rb_node_t* rb_search_v2(data_t key, rb_node_t* root);
rb_node_t* rb_erase_v2(data_t data, rb_node_t *root);
/*
NEINT32 main()
{
    NEINT32 i, count = 900000;
    key_t key;
    rb_node_t* root = NULL, *node = NULL;

    srand(time(NULL));
    for (i = 1; i < count; ++i)
    {
        key = rand() % count;
        if ((root = rb_insert(key, i, root)))
        {
            printf("[i = %d] insert key %d success!\n", i, key);
        }
        else
        {
            printf("[i = %d] insert key %d error!\n", i, key);
            exit(-1);
        }

        if ((node = rb_search(key, root)))
        {
            printf("[i = %d] search key %d success!\n", i, key);
        }
        else
        {
            printf("[i = %d] search key %d error!\n", i, key);
            exit(-1);
        }

        if (!(i % 10))
        {
            if ((root = rb_erase(key, root)))
            {
                printf("[i = %d] erase key %d success\n", i, key);
            }
            else
            {
                printf("[i = %d] erase key %d error\n", i, key);
            }
        }
    }
    return 0;
}
*/

#endif

