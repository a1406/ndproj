/* �����������ʹ�ö�̬�����ڴ�.
 * ʹ�þ�̬����.�����ö�����֮ǰ
 * ����Ҫ�������ɶ������ýڵ������
 * �����������ݽṹ��ֱ�Ӱ���һ��ne_binnode.
 */
#ifndef _NDBINTREE_H_
#define _NDBINTREE_H_


typedef size_t TTREE_DATA ;
/* A threaded binary tree node. */
struct tbin_node {
	struct tbin_node *left;
	struct tbin_node *right;
	TTREE_DATA data;
	unsigned l_thread:1;
	unsigned r_thread:1;
};

typedef struct tbin_node* (*alloc_tbin_node)() ;
typedef void (*free_tbin_node)(struct tbin_node* ) ;
typedef void (*walk_func)(const struct tbin_node*) ;

/* A binary tree. */
struct tbin_tree {
	struct tbin_node *root;
	NEINT32 count;

	alloc_tbin_node alloc_node ;
	free_tbin_node free_node;
};

/* Iinit threaded tree*/
NE_COMMON_API struct tbin_tree *tbin_init(struct tbin_tree *tree,	alloc_tbin_node fa,	free_tbin_node ff) ;

/* Searches TREE for matching ITEM.  Returns 1 if found, 0
   otherwise. */
NE_COMMON_API struct tbin_node* tbin_search(const struct tbin_tree *tree, TTREE_DATA item);

/* �����в���һ���ڵ㣬����Ҫ����ռ�
 */
NE_COMMON_API struct tbin_node* tbin_insert_node(struct tbin_tree *tree, struct tbin_node *node);

/* Inserts ITEM into TREE.  Returns 1 if the item was inserted, 2 if
   an identical item already existed in TREE, or 0 if a memory
   allocation error occurred. */
NE_COMMON_API struct tbin_node* tbin_insert(struct tbin_tree *tree, TTREE_DATA item);

/* Deletes any item matching ITEM from TREE.  Returns 1 if such an
   item was deleted, 0 if none was found. */
NE_COMMON_API NEINT32 tbin_delete(struct tbin_tree *tree, TTREE_DATA item);


NE_COMMON_API NEINT32 tbin_delete_node(struct tbin_tree *tree, struct tbin_node *node);

/* Prints all the data items in TREE in in-order. 
   �ݹ�����������нڵ� */
NE_COMMON_API void tbin_walk(const struct tbin_tree *tree,walk_func cb_func);

/* Prints all the data items in TREE in in-order, using an iterative algorithm. 
   ���������������нڵ�*/
NE_COMMON_API void tbin_traverse(const struct tbin_tree *tree,walk_func cb_func);

/* Destroys TREE. */
NE_COMMON_API void tbin_destroy(struct tbin_tree *tree);

/* Returns the number of data items in TREE. */
NE_COMMON_API NEINT32 tbin_count(const struct tbin_tree *tree);

/* Returns the next node in in-order in the tree after X, or NULL if X
   is the greatest node in the tree. */
/*���ر�x�����һ���ڵ�*/
NE_COMMON_API struct tbin_node *tbin_successor(struct tbin_node *x);	

/* Return the node with the least value in the tree rooted at X. */
/*������С�ڵ�Ľڵ�*/
NE_COMMON_API struct tbin_node *tbin_minimum(struct tbin_node *x);	

/* Returns the previous node in in-order in the tree after X, or NULL if X
   is the greatest node in the tree. */
/* ���ر�xС�Ľڵ�*/
NE_COMMON_API struct tbin_node *tbin_predecessor(struct tbin_node *x);

/* Return the node with the greatest value in the tree rooted at X. */
/* �������ڵ�*/
NE_COMMON_API struct tbin_node *tbin_maximum(struct tbin_node *x);

/* ��������������нڵ� */
#define tbin_for_each(tree, node) \
    for ((node)=tbin_minimum((tree)->root); (node) != NULL; (node)=tbin_successor(node)) {

/* ��������������нڵ� */
#define tbin_for_each_desc(tree, node) \
	for ((node) = tbin_maximum((tree)->root); (node)!=NULL; (node) = tbin_predecessor(node))

#endif 
