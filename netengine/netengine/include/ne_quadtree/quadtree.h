#ifndef QUADTREE_H_INCLUDED
#define QUADTREE_H_INCLUDED

#define IN
#define OUT

#ifdef __LINUX__
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "ne_common/list.h"

#define QUAD_SUBNODES        4

#define QBOX_OVERLAP_MAX    0.4
#define QBOX_OVERLAP_MIN    0.02

#define QTREE_DEPTH_MAX        8
#define QTREE_DEPTH_MIN        4

#define QUADRANT_BITS        3

/* a quadrant defined below:
          
        NW(1)   |    NE(0)
        -----------|-----------
        SW(2)   |    SE(3)
*/
typedef enum
{
    NE = 0,
    NW = 1,
    SW = 2,
    SE = 3
}QuadrantEnum;

/* a box defined below:
           _____max
          |__|__|
          |__|__| 
   min
*/
typedef struct _quadbox_t
{    
    NEFLOAT    _xmin, 
            _ymin, 
            _xmax, 
            _ymax;
} quadbox_t;

/* quad node */
typedef struct _quadnode_t
{
    quadbox_t             _box;                        /* node bound box */
//    list_t                *_lst;                     /* node data list*/
//	NEINT32 node_key;     /* I think we do not need it  */
	struct list_head	_lst;		                   /* node data list*/
	struct _quadnode_t    *_sub[QUAD_SUBNODES];        /* pointer to subnodes of this node */
}quadnode_t;

/* quad tree */
typedef struct _quadtree_t
{
    quadnode_t        *_root;
    NEINT32                 _depth;                   /* max depth of tree: 0-based */
    NEFLOAT             _overlap;                   /* overlapped ratio of quanbox */    
} quadtree_t;

/*=============================================================================
                        Public Functions
=============================================================================*/
/* creates a quadtree and returns a pointer to it */
extern  quadtree_t*
quadtree_create (quadbox_t    box, 
                 NEINT32        depth,  /* 4~8 */
                 NEFLOAT        overlap /* 0.02 ~ 0.4 */
                 );

/* destroys a quad tree and free all memory */
extern  void
quadtree_destroy (IN  quadtree_t        *qtree
                  );

/* inserts a node identified by node_key into a quadtree, returns the node quadtree encoding */
extern  quadnode_t *
quadtree_insert (IN  quadtree_t            *qtree, 
	IN  struct list_head *node_lst,
	IN  quadbox_t            *node_box
                 );
 
/* searches nodes inside search_box */
extern  void
quadtree_search (IN  const quadtree_t    *qtree, 
	IN  quadbox_t            *search_box, 
	OUT list_t                *results_list[],
	IN OUT NEINT32 *index,
	IN NEINT32 max_index
                 );

quadnode_t *quadtree_move(IN quadtree_t *qtree,
	IN struct list_head *node_lst,
	IN quadbox_t *node_box);

typedef void (*quad_travel_func)(struct list_head *, void *);
void quad_travel(quadnode_t *current_node, quad_travel_func f, void *param);

#endif // QUADTREE_H_INCLUDED
